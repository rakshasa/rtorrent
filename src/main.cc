#include "config.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <inttypes.h>
#include <typeinfo>
#include <unistd.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>
#include <torrent/data/chunk_utils.h>
#include <torrent/net/fd.h>
#include <torrent/net/http_stack.h>
#include <torrent/runtime/memory_manager.h>
#include <torrent/runtime/runtime.h>
#include <torrent/utils/chrono.h>
#include <torrent/utils/log.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include "core/dht_manager.h"
#include "core/download.h"
#include "core/manager.h"
#include "display/canvas.h"
#include "display/window.h"
#include "display/manager.h"
#include "input/bindings.h"
#include "rpc/command_scheduler.h"
#include "rpc/command_scheduler_item.h"
#include "rpc/parse_commands.h"
#include "scgi/thread_scgi.h"
#include "session/session_manager.h"
#include "session/thread_session.h"
#include "ui/root.h"
#include "utils/directory.h"

#include "control.h"
#include "command_helpers.h"
#include "globals.h"
#include "setup.h"
#include "signal_handler.h"
#include "option_parser.h"

#define LT_LOG(log_fmt, ...)                                    \
  lt_log_print(torrent::LOG_SYSTEM, "system: " log_fmt, __VA_ARGS__);

void handle_sigbus(int signum, siginfo_t* sa, void* ptr);
void do_panic(int signum);
void print_help();
void initialize_commands();

void
initialize_rpc_slots() {
  rpc::rpc.slot_find_download() = [](const char* hash) {
      return control->core()->download_list()->find_hex_ptr(hash);
    };
  rpc::rpc.slot_find_file() = [](core::Download* d, uint32_t index) -> torrent::File* {
      if (index >= d->file_list()->size_files())
        throw torrent::input_error("invalid parameters: index not found");

      return (*d->file_list())[index].get();
    };
  rpc::rpc.slot_find_tracker() = [](core::Download* d, uint32_t index) -> torrent::tracker::Tracker {
      if (index >= d->tracker_controller().size())
        throw torrent::input_error("invalid parameters: index not found");

      // TODO: This should be rewritten to check if the tracker is valid and use a different
      // function.
      return d->tracker_controller().at(index);
    };
  rpc::rpc.slot_find_peer() = [](core::Download* d, const torrent::HashString& hash) -> torrent::Peer* {
      auto itr = d->connection_list()->find(hash.c_str());

      if (itr == d->connection_list()->end())
        throw torrent::input_error("invalid parameters: hash not found");

      return *itr;
    };
}

static void
client_perform() {
  // Use throw exclusively.
  if (control->is_shutdown_completed())
    throw torrent::shutdown_exception();

  if (control->is_shutdown_received())
    control->handle_shutdown();

  control->inc_tick();
}

int
main(int argc, char** argv) {
  try {

    // Temporary.
    setlocale(LC_ALL, "");

    auto random_seed = []() -> unsigned int {
        auto current_time = torrent::utils::time_since_epoch();

        return (getpid() << 16) ^ getppid() ^
          torrent::utils::cast_seconds(current_time).count() ^ current_time.count();
      }();

    srandom(random_seed);
    srand48(random_seed);

    torrent::log_initialize();

    // TODO: Create a fake thread object for initializing other processes and enabling logging.
    torrent::initialize_main_thread();

    // Block SIGCHLD until all threads are created, then unblock on main-thread, to avoid SIGCHLD
    // interrupting other threads.
    //
    // This means only main-thread can fork and wait for child processes.

    SignalHandler::set_block(SIGALRM);
    SignalHandler::set_block(SIGPIPE);
    SignalHandler::set_block(SIGCHLD);

    // All signal handlers must restore errno if they return.
    SignalHandler::set_handler(SIGSEGV,  std::bind(&do_panic, SIGSEGV));
    SignalHandler::set_handler(SIGILL,   std::bind(&do_panic, SIGILL));
    SignalHandler::set_handler(SIGFPE,   std::bind(&do_panic, SIGFPE));

    // Limited list of commands with the following format:
    //
    // cat ~/.rtorrent.rc:
    //
    // # do:log.open_file=system,/usr/rakshasa/system.log
    // # do:log.add_output=system,system
    //
    parse_config_file(argc, argv, [](auto& path) {
        if (path.empty())
          return;

        parse_config_file_comments(path);
      });

    control = new Control;

    SignalHandler::set_handler(SIGINT,   std::bind(&Control::receive_normal_shutdown, control));
    SignalHandler::set_handler(SIGHUP,   std::bind(&Control::receive_normal_shutdown, control));
    SignalHandler::set_handler(SIGTERM,  std::bind(&Control::receive_quick_shutdown, control));
    SignalHandler::set_handler(SIGWINCH, std::bind(&display::Manager::force_redraw, control->display()));

    SignalHandler::set_sigaction_handler(SIGBUS, &handle_sigbus);

    torrent::log_add_group_output(torrent::LOG_NOTICE,         "important");
    torrent::log_add_group_output(torrent::LOG_DHT_ERROR,      "important");

    torrent::log_add_group_output(torrent::LOG_INFO,           "complete");
    torrent::log_add_group_output(torrent::LOG_DHT_ERROR,      "complete");
    torrent::log_add_group_output(torrent::LOG_DHT_CONTROLLER, "complete");

    initialize_rpc_slots();

    torrent::initialize();
    torrent::main_thread::set_client_callback(&client_perform);

    scgi::ThreadScgi::create_thread();
    session::ThreadSession::create_thread();

    SignalHandler::set_unblock(SIGCHLD);

    // Initialize option handlers after libtorrent to ensure
    // torrent::ConnectionManager* are valid etc.
    initialize_commands();

    if (OptionParser::has_flag('D', argc, argv)) {
      rpc::call_command_set_value("method.use_deprecated.set", true);
      lt_log_print(torrent::LOG_WARN, "Enabled deprecated commands.");
    }

    if (OptionParser::has_flag('I', argc, argv)) {
      rpc::call_command_set_value("method.use_intermediate.set", 0);
      lt_log_print(torrent::LOG_WARN, "Disabled intermediate commands.");
    }

    if (OptionParser::has_flag('K', argc, argv)) {
      rpc::call_command_set_value("method.use_intermediate.set", 2);
      lt_log_print(torrent::LOG_WARN, "Allowing intermediate commands without xmlrpc.");
    }

    rpc::parse_command_multiple
      (rpc::make_target(),
       "method.insert = event.view.hide,multi|rlookup|static\n"
       "method.insert = event.view.show,multi|rlookup|static\n"

       "method.insert = event.system.startup_done,multi|rlookup|static\n"
       "method.insert = event.system.shutdown,multi|rlookup|static\n"

       "method.insert = event.download.inserted,multi|rlookup|static\n"
       "method.insert = event.download.inserted_new,multi|rlookup|static\n"
       "method.insert = event.download.inserted_session,multi|rlookup|static\n"
       "method.insert = event.download.erased,multi|rlookup|static\n"
       "method.insert = event.download.opened,multi|rlookup|static\n"
       "method.insert = event.download.closed,multi|rlookup|static\n"
       "method.insert = event.download.resumed,multi|rlookup|static\n"
       "method.insert = event.download.paused,multi|rlookup|static\n"

       "method.insert = event.download.finished,multi|rlookup|static\n"
       "method.insert = event.download.hash_done,multi|rlookup|static\n"
       "method.insert = event.download.hash_failed,multi|rlookup|static\n"
       "method.insert = event.download.hash_final_failed,multi|rlookup|static\n"
       "method.insert = event.download.hash_removed,multi|rlookup|static\n"
       "method.insert = event.download.hash_queued,multi|rlookup|static\n"

       "method.set_key = event.download.inserted,         1_send_scrape, ((d.tracker.send_scrape,30))\n"
       "method.set_key = event.download.inserted_new,     1_prepare,   {(branch,((d.state)),((view.set_visible,started)),((view.set_visible,stopped)) )}\n"
       // d.save_full_session runs LAST (~ prefix is ASCII 0x7E, sorts after all alphanumerics,
       // matching the existing ~_delete_tied precedent on event.download.erased) so the on-disk
       // snapshot includes custom fields written by other inserted_new handlers (addtime,
       // seedingtime, etc.). Was previously inside 1_prepare alongside view setup, which
       // snapshotted bencode before later handlers ran, so d.set_custom=addtime did not reach
       // .rtorrent until the next periodic resume save — a window where rtorrent restart lost them.
       "method.set_key = event.download.inserted_new,     ~_save_full, ((d.save_full_session))\n"
       "method.set_key = event.download.inserted_session, 1_prepare, {(branch,((d.state)),((view.set_visible,started)),((view.set_visible,stopped)) )}\n"

       "method.set_key = event.download.inserted, 1_prioritize_toc, \"branch=file.prioritize_toc=,{\\\"f.multicall=(file.prioritize_toc.first),f.prioritize_first.enable=\\\",\\\"f.multicall=(file.prioritize_toc.last),f.prioritize_last.enable=\\\",d.update_priorities=}\"\n"

       "method.set_key = event.download.erased, !_download_list, ui.unfocus_download=\n"
       "method.set_key = event.download.erased, ~_delete_tied, d.delete_tied=\n"

       "method.set_key = event.download.resumed,   !_timestamp, ((d.timestamp.started.set_if_z, ((system.time)) ))\n"
       "method.set_key = event.download.finished,  !_timestamp, ((d.timestamp.finished.set_if_z, ((system.time)) ))\n"
       "method.set_key = event.download.hash_done, !_timestamp, {(branch,((d.complete)),((d.timestamp.finished.set_if_z,(system.time))))}\n"

       "method.insert.c_simple = group.insert_persistent_view,"
       "((view.add,((argument.0)))),((view.persistent,((argument.0)))),((group.insert,((argument.0)),((argument.0))))\n"

       "file.prioritize_toc.first.set = {*.avi,*.mp4,*.mkv,*.gz}\n"
       "file.prioritize_toc.last.set  = {*.zip}\n"

       // Allow setting 'group.view' as constant, so that we can't
       // modify the value. And look into the possibility of making
       // 'const' use non-heap memory, as we know they can't be
       // erased.

       // TODO: Remember to ensure it doesn't get restarted by watch
       // dir, etc. Set ignore commands, or something.

       "group.insert = seeding,seeding\n"

       "session.name.set = (cat,(system.hostname),:,(system.pid))\n"

       // Currently not doing any sorting on main.
       "view.add = main\n"
       "view.add = default\n"

       "view.add = name\n"
       "view.sort_new     = name,((less,((d.name))))\n"
       "view.sort_current = name,((less,((d.name))))\n"

       "view.add = active\n"
       "view.filter = active,((false))\n"

       "view.add = started\n"
       "view.filter = started,((false))\n"
       "view.event_added   = started,{(view.set_not_visible,stopped),(d.state.set,1),(scheduler.simple.added)}\n"
       "view.event_removed = started,{(view.set_visible,stopped),(scheduler.simple.removed)}\n"

       "view.add = stopped\n"
       "view.filter = stopped,((false))\n"
       "view.event_added   = stopped,{(d.state.set,0),(view.set_not_visible,started)}\n"
       "view.event_removed = stopped,((view.set_visible,started))\n"

       "view.add = complete\n"
       "view.filter = complete,((d.complete))\n"
       "view.filter_on    = complete,event.download.hash_done,event.download.hash_failed,event.download.hash_final_failed,event.download.finished\n"

       "view.add = incomplete\n"
       "view.filter = incomplete,((not,((d.complete))))\n"
       "view.filter_on    = incomplete,event.download.hash_done,event.download.hash_failed,"
                                      "event.download.hash_final_failed,event.download.finished\n"

       // The hashing view does not include stopped torrents.
       "view.add = hashing\n"
       "view.filter = hashing,((d.hashing))\n"
       "view.filter_on = hashing,event.download.hash_queued,event.download.hash_removed,"
                                "event.download.hash_done,event.download.hash_failed,event.download.hash_final_failed,event.download.finished\n"

       "view.add    = seeding\n"
       "view.filter = seeding,((and,((d.state)),((d.complete))))\n"
       "view.filter_on = seeding,event.download.resumed,event.download.paused,event.download.finished\n"

       "view.add    = leeching\n"
       "view.filter = leeching,((and,((d.state)),((not,((d.complete))))))\n"
       "view.filter_on = leeching,event.download.resumed,event.download.paused,event.download.finished\n"

       "schedule = view.main,10,10,((view.sort,main,20))\n"
       "schedule = view.name,10,10,((view.sort,name,20))\n"

       "schedule = session_save,1200,1200,((session.save))\n"
       "schedule = low_diskspace,5,60,((close_low_diskspace,500M))\n"
       "schedule = prune_file_status,3600,86400,((system.file_status_cache.prune))\n"

       "ui.color.focus.set=reverse\n"
    );

    // Functions that might not get depracted as they are nice for
    // configuration files, and thus might do with just some
    // cleanup.
    CMD_REDIRECT("upload_rate",           "throttle.global_up.max_rate.set_kb");
    CMD_REDIRECT("download_rate",         "throttle.global_down.max_rate.set_kb");

    CMD_REDIRECT("ratio.enable",          "group.seeding.ratio.enable");
    CMD_REDIRECT("ratio.disable",         "group.seeding.ratio.disable");
    CMD_REDIRECT("ratio.min",             "group.seeding.ratio.min");
    CMD_REDIRECT("ratio.max",             "group.seeding.ratio.max");
    CMD_REDIRECT("ratio.upload",          "group.seeding.ratio.upload");
    CMD_REDIRECT("ratio.min.set",         "group.seeding.ratio.min.set");
    CMD_REDIRECT("ratio.max.set",         "group.seeding.ratio.max.set");
    CMD_REDIRECT("ratio.upload.set",      "group.seeding.ratio.upload.set");

    CMD_REDIRECT("encryption",            "protocol.encryption.set");

    CMD_REDIRECT("check_hash",            "pieces.hash.on_completion.set");

    CMD_REDIRECT("connection_leech",      "protocol.connection.leech.set");
    CMD_REDIRECT("connection_seed",       "protocol.connection.seed.set");

    CMD_REDIRECT("min_peers",             "throttle.min_peers.normal.set");
    CMD_REDIRECT("max_peers",             "throttle.max_peers.normal.set");
    CMD_REDIRECT("min_peers_seed",        "throttle.min_peers.seed.set");
    CMD_REDIRECT("max_peers_seed",        "throttle.max_peers.seed.set");

    CMD_REDIRECT("min_uploads",           "throttle.min_uploads.set");
    CMD_REDIRECT("max_uploads",           "throttle.max_uploads.set");
    CMD_REDIRECT("min_downloads",         "throttle.min_downloads.set");
    CMD_REDIRECT("max_downloads",         "throttle.max_downloads.set");

    CMD_REDIRECT("max_uploads_div",       "throttle.max_uploads.div.set");
    CMD_REDIRECT("max_uploads_global",    "throttle.max_uploads.global.set");
    CMD_REDIRECT("max_downloads_div",     "throttle.max_downloads.div.set");
    CMD_REDIRECT("max_downloads_global",  "throttle.max_downloads.global.set");

    CMD_REDIRECT("directory",             "directory.default.set");
    CMD_REDIRECT("session",               "session.path.set");

    CMD_REDIRECT_NO_EXPORT("port_range",  "network.listen.port.range.set");
    CMD_REDIRECT_NO_EXPORT("scgi_port",   "network.scgi.open_port");
    CMD_REDIRECT_NO_EXPORT("scgi_local",  "network.scgi.open_local");

    CMD_REDIRECT("to_gm_time",            "convert.gm_time");
    CMD_REDIRECT("to_gm_date",            "convert.gm_date");
    CMD_REDIRECT("to_time",               "convert.time");
    CMD_REDIRECT("to_date",               "convert.date");
    CMD_REDIRECT("to_elapsed_time",       "convert.elapsed_time");
    CMD_REDIRECT("to_kb",                 "convert.kb");
    CMD_REDIRECT("to_mb",                 "convert.mb");
    CMD_REDIRECT("to_xb",                 "convert.xb");
    CMD_REDIRECT("to_throttle",           "convert.throttle");

    // TODO: Deprecate these at some point a while after 1.1 release.

    CMD_REDIRECT("d.multicall2",                   "d.multicall");
    CMD_REDIRECT("network.open_sockets",           "system.sockets.size");
    CMD_REDIRECT("network.max_open_sockets",       "system.sockets.max_size");
    CMD_REDIRECT("network.max_open_sockets.set",   "system.sockets.max_size.set");
    CMD_REDIRECT("network.http.proxy_address",     "network.proxy.http");
    CMD_REDIRECT("network.http.proxy_address.set", "network.proxy.http.set");

    rpc::rpc.mark_safe("d.multicall2");
    rpc::rpc.mark_safe("network.max_open_sockets");
    rpc::rpc.mark_safe("network.http.proxy_address");

    CMD2_ANY_VALUE_V("network.http.max_total_connections.set", [](auto, auto) {
        lt_log_print(torrent::LOG_WARN, "network.http.max_total_connections.set is deprecated, use system.sockets.http.min_alloc.set instead.");
      });

    CMD2_ANY_VALUE_V("network.max_open_files.set", [](auto, auto) {
        lt_log_print(torrent::LOG_WARN, "network.max_open_files.set is deprecated, use system.sockets.files.min_alloc.set instead.");
      });

    // if (rpc::call_command_value("method.use_intermediate") == 1) {

    // } else if (rpc::call_command_value("method.use_intermediate") == 2) {
    //   Allow for use in config files, etc, just don't export it.
    // }

    if (rpc::call_command_value("method.use_deprecated") == 1) {
      CMD_REDIRECT("execute2",         "execute");
      CMD_REDIRECT("schedule2",        "schedule");
      CMD_REDIRECT("schedule_remove2", "schedule.remove");

      CMD_REDIRECT("bind",                  "network.bind_address.set");
      CMD_REDIRECT("ip",                    "network.local_address.set");
      CMD_REDIRECT("port_range",            "network.port_range.set");

      // TODO: Check if dht is on by default.
      CMD_REDIRECT("dht",                   "dht.mode.set");

      CMD_REDIRECT("port_random",           "network.port_random.set");
      CMD_REDIRECT("proxy_address",         "network.proxy_address.set");

      CMD_REDIRECT("key_layout",            "keys.layout.set");

      CMD_REDIRECT("torrent_list_layout",   "ui.torrent_list.layout.set");

      CMD2_VAR_STRING("dht.throttle.name",   "deprecated");
      rpc::rpc.mark_safe("dht.throttle.name");

      CMD_REDIRECT("network.http.max_open",     "network.http.max_total_connections");
      CMD_REDIRECT("network.http.max_open.set", "network.http.max_total_connections.set");

      // Users should check their setups to see if they need to modify their use of these options.
      CMD_REDIRECT("max_memory_usage",      "pieces.memory.max.set");

      CMD_ANY_LIST("throttle.ip", []( auto, auto) {
          lt_log_print(torrent::LOG_WARN, "The 'throttle.ip' command is deprecated and does nothing.");
          return torrent::Object();
        });

      CMD_ANY("network.port_open", [](auto, auto) {
          lt_log_print(torrent::LOG_WARN, "The 'network.port_open' command is deprecated and does nothing.");
          return torrent::Object();
        });
      CMD_ANY("network.port_open.set", [](auto, auto) {
          lt_log_print(torrent::LOG_WARN, "The 'network.port_open.set' command is deprecated and does nothing.");
          return torrent::Object();
        });

      CMD_REDIRECT("network.port_random",     "network.listen.port.random");
      CMD_REDIRECT("network.port_random.set", "network.listen.port.random.set");
      CMD_REDIRECT("network.port_range",      "network.listen.port.range");
      CMD_REDIRECT("network.port_range.set",  "network.listen.port.range.set");
    }

    {
      auto fd = torrent::fd_open_family(torrent::fd_flag_stream, AF_INET6);

      if (fd == -1) {
        if (errno == EAFNOSUPPORT) {
          lt_log_print(torrent::LOG_WARN, "disabling ipv6 support, not available on system");
          rpc::call_command_set_value("network.block.ipv6.set", true);
        }
      } else {
        torrent::fd_close(fd);
      }
    }

    int firstArg = parse_main_options(argc, argv);

    parse_config_file(argc, argv, [](auto& path) {
        if (path.empty()) {
          lt_log_print(torrent::LOG_WARN, "Ignoring rtorrent.rc.");
          return;
        }

        rpc::parse_command_single(rpc::make_target(), "try_import=" + path);
      });

    LT_LOG("seeded srandom and srand48 (seed:%u)", random_seed);
    LT_LOG("max memory usage: %" PRIu64, torrent::runtime::memory_manager()->max_memory_usage());

    control->initialize();
    control->ui()->load_input_history();

    torrent::net_thread::http_stack()->set_user_agent(USER_AGENT);
    torrent::runtime::initialize_network();

    // Load session torrents and perform scheduled tasks to ensure session torrents are loaded
    // before arg torrents.
    control->dht_manager()->set_auto_if_untouched_and_has_session();
    control->dht_manager()->load_dht_cache();

    load_session_torrents(session_thread::manager()->path());
    load_arg_torrents(argv + firstArg, argv + argc);

    // Make sure we update the display before any scheduled tasks can run, so that loading of
    // torrents doesn't look like it hangs on startup.
    control->display()->adjust_layout();
    control->display()->receive_update();

    rpc::commands.call_catch("event.system.startup_done", rpc::make_target(), "startup_done", "System startup_done event action failed: ");

    torrent::system::Thread::self()->event_loop();

    control->core()->download_list()->session_save();
    control->cleanup();

  } catch (torrent::internal_error& e) {
    control->cleanup_exception();

    std::cout << "rtorrent: caught torrent::internal_error: "
              << e.what() << std::endl
              << e.backtrace();

    lt_log_print_dump(torrent::LOG_CRITICAL, e.backtrace().c_str(), e.backtrace().size(),
                      "Caught internal_error: '%s'.", e.what());

    torrent::log_cleanup();
    return -1;

  } catch (std::exception& e) {
    control->cleanup_exception();

    std::cout << "rtorrent: caught" << typeid(e).name() << " : " << e.what() << std::endl;

    lt_log_print(torrent::LOG_CRITICAL, "Caught exception: '%s'.", e.what());

    torrent::log_cleanup();
    return -1;
  }

  delete control;
  control = nullptr;

  scgi::ThreadScgi::destroy_thread();
  session::ThreadSession::destroy_thread();

  torrent::log_cleanup();

  return 0;
}

void
handle_sigbus(int signum, siginfo_t* sa, [[maybe_unused]] void* ptr) {
  if (signum != SIGBUS)
    do_panic(signum);

  SignalHandler::set_default(signum);
  display::Canvas::cleanup();

  std::stringstream output;
  output << "Caught SIGBUS, dumping stack:" << std::endl;

#ifdef HAVE_BACKTRACE
  void* stackPtrs[20];

  // Print the stack and exit.
  int stackSize = backtrace(stackPtrs, 20);
  char** stackStrings = backtrace_symbols(stackPtrs, stackSize);

  for (int i = 0; i < stackSize; ++i)
    output << stackStrings[i] << std::endl;

#else
  output << "Stack dump not enabled." << std::endl;
#endif
  output << std::endl << "Error: " << std::strerror(sa->si_errno) << std::endl;

  const char* signal_reason;

  switch (sa->si_code) {
  case BUS_ADRALN: signal_reason = "Invalid address alignment."; break;
  case BUS_ADRERR: signal_reason = "Non-existent physical address."; break;
  case BUS_OBJERR: signal_reason = "Object specific hardware error."; break;
  default:
    if (sa->si_code <= 0)
      signal_reason = "User-generated signal.";
    else
      signal_reason = "Unknown.";

    break;
  };

  output << "Signal code '" << sa->si_code << "': " << signal_reason << std::endl;
  output << "Fault address: " << sa->si_addr << std::endl;

  // New code for finding the location of the SIGBUS signal, and using
  // that to figure out how to recover.
  torrent::chunk_info_result result = torrent::chunk_list_address_info(sa->si_addr);

  if (!result.download.is_valid()) {
    output << "The fault address is not part of any chunk." << std::endl;
    goto handle_sigbus_exit;
  }

  output << "Torrent name: " << result.download.info()->name().c_str() << std::endl;
  output << "File name:    " << result.file_path << std::endl;
  output << "File offset:  " << result.file_offset << std::endl;
  output << "Chunk index:  " << result.chunk_index << std::endl;
  output << "Chunk offset: " << result.chunk_offset << std::endl;

handle_sigbus_exit:
  std::cout << output.rdbuf();

  if (lt_log_is_valid(torrent::LOG_CRITICAL)) {
    std::string dump = output.str();
    lt_log_print_dump(torrent::LOG_CRITICAL, dump.c_str(), dump.size(), "Caught signal: '%s'.", signal_reason);
  }

  torrent::log_cleanup();
  std::abort();
}

void
do_panic(int signum) {
  // Use the default signal handler in the future to avoid infinit
  // loops.
  SignalHandler::set_default(signum);

  if (control != nullptr)
    display::Canvas::cleanup();

  std::stringstream output;

  output << "Caught " << SignalHandler::as_string(signum) << ", dumping stack:" << std::endl;

#ifdef HAVE_BACKTRACE
  void* stackPtrs[20];

  // Print the stack and exit.
  int stackSize = backtrace(stackPtrs, 20);
  char** stackStrings = backtrace_symbols(stackPtrs, stackSize);

  for (int i = 0; i < stackSize; ++i)
    output << stackStrings[i] << std::endl;

#else
  output << "Stack dump not enabled." << std::endl;
#endif

  if (signum == SIGBUS)
    output << "A bus error probably means you ran out of diskspace." << std::endl;

  std::cout << output.rdbuf();

  if (lt_log_is_valid(torrent::LOG_CRITICAL)) {
    std::string dump = output.str();
    lt_log_print_dump(torrent::LOG_CRITICAL, dump.c_str(), dump.size(), "Caught signal: '%s.", strsignal(signum));
  }

  torrent::log_cleanup();
  std::abort();
}

void
print_help() {
  std::cout << "Rakshasa's BitTorrent client version " PACKAGE_VERSION "." << std::endl;
  std::cout << std::endl;
  std::cout << "All value pairs (f.ex rate and queue size) will be in the UP/DOWN" << std::endl;
  std::cout << "order. Use the up/down/left/right arrow keys to move between screens." << std::endl;
  std::cout << std::endl;
  std::cout << "Usage: rtorrent [OPTIONS]... [FILE]... [URL]..." << std::endl;
  std::cout << "  -D                Enable deprecated commands" << std::endl;
  std::cout << "  -I                Disable intermediate commands" << std::endl;
  std::cout << "  -K                Allow intermediate commands without xmlrpc" << std::endl;
  std::cout << "  -h                Display this very helpful text" << std::endl;
  std::cout << "  -n                Don't try to load rtorrent.rc on startup" << std::endl;
  std::cout << "  -b <a.b.c.d>      Bind the listening socket to this IP" << std::endl;
  std::cout << "  -i <a.b.c.d>      Change the IP that is sent to the tracker" << std::endl;
  std::cout << "  -p <int>-<int>    Set port range for incoming connections" << std::endl;
  std::cout << "  -d <directory>    Save torrents to this directory by default" << std::endl;
  std::cout << "  -s <directory>    Set the session directory" << std::endl;
  std::cout << "  -o key=opt,...    Set options, see 'rtorrent.rc' file" << std::endl;
  std::cout << std::endl;
  std::cout << "Main view keys:" << std::endl;
  std::cout << "  backspace         Add a torrent url or path" << std::endl;
  std::cout << "  ^s                Start torrent" << std::endl;
  std::cout << "  ^d                Stop torrent or delete a stopped torrent" << std::endl;
  std::cout << "  ^r                Manually initiate hash checking" << std::endl;
  std::cout << "  ^o                Change the destination directory of the download. The torrent must be closed." << std::endl;
  std::cout << "  ^q                Initiate shutdown or skip shutdown process" << std::endl;
  std::cout << "  a,s,d,z,x,c       Adjust upload throttle" << std::endl;
  std::cout << "  A,S,D,Z,X,C       Adjust download throttle" << std::endl;
  std::cout << "  I                 Toggle whether torrent ignores ratio settings" << std::endl;
  std::cout << "  right             View torrent" << std::endl;
  std::cout << std::endl;
  std::cout << "Download view keys:" << std::endl;
  std::cout << "  spacebar          Depends on the current view" << std::endl;
  std::cout << "  1,2               Adjust max uploads" << std::endl;
  std::cout << "  3,4,5,6           Adjust min/max connected peers" << std::endl;
  std::cout << "  t/T               Query tracker for more peers / Force query" << std::endl;
  std::cout << "  *                 Snub peer" << std::endl;
  std::cout << "  right             View files" << std::endl;
  std::cout << "  p                 View peer information" << std::endl;
  std::cout << "  o                 View trackers" << std::endl;
  std::cout << std::endl;

  std::cout << "Report bugs to <github.com/rakshasa/rtorrent>." << std::endl;

  exit(0);
}
