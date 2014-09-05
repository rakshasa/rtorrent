// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#define __STDC_FORMAT_MACROS

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <inttypes.h>
#include <unistd.h>
#include <torrent/http.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>
#include <torrent/poll.h>
#include <torrent/data/chunk_utils.h>
#include <torrent/utils/log.h>
#include <rak/functional.h>
#include <rak/error_number.h>

#ifdef USE_EXECINFO
#include <execinfo.h>
#endif

#include "core/dht_manager.h"
#include "core/download.h"
#include "core/download_factory.h"
#include "core/download_store.h"
#include "core/manager.h"
#include "display/canvas.h"
#include "display/window.h"
#include "display/manager.h"
#include "input/bindings.h"

#include "rpc/command_scheduler.h"
#include "rpc/command_scheduler_item.h"
#include "rpc/parse_commands.h"
#include "utils/directory.h"

#include "control.h"
#include "command_helpers.h"
#include "globals.h"
#include "signal_handler.h"
#include "option_parser.h"

#include "thread_worker.h"

void handle_sigbus(int signum, siginfo_t* sa, void* ptr);
void do_panic(int signum);
void print_help();
void initialize_commands();

void do_nothing() {}
void do_nothing_str(const std::string&) {}

int
parse_options(Control* c, int argc, char** argv) {
  try {
    OptionParser optionParser;

    // Converted.
    optionParser.insert_flag('h', std::bind(&print_help));
    optionParser.insert_flag('n', std::bind(&do_nothing_str, std::placeholders::_1));
    optionParser.insert_flag('D', std::bind(&do_nothing_str, std::placeholders::_1));
    optionParser.insert_flag('I', std::bind(&do_nothing_str, std::placeholders::_1));
    optionParser.insert_flag('K', std::bind(&do_nothing_str, std::placeholders::_1));

    optionParser.insert_option('b', std::bind(&rpc::call_command_set_string, "network.bind_address.set", std::placeholders::_1));
    optionParser.insert_option('d', std::bind(&rpc::call_command_set_string, "directory.default.set", std::placeholders::_1));
    optionParser.insert_option('i', std::bind(&rpc::call_command_set_string, "ip", std::placeholders::_1));
    optionParser.insert_option('p', std::bind(&rpc::call_command_set_string, "network.port_range.set", std::placeholders::_1));
    optionParser.insert_option('s', std::bind(&rpc::call_command_set_string, "session", std::placeholders::_1));

    optionParser.insert_option('O',      std::bind(&rpc::parse_command_single_std, std::placeholders::_1));
    optionParser.insert_option_list('o', std::bind(&rpc::call_command_set_std_string, std::placeholders::_1, std::placeholders::_2));

    return optionParser.process(argc, argv);

  } catch (torrent::input_error& e) {
    throw torrent::input_error("Failed to parse command line option: " + std::string(e.what()));
  }
}

void
load_session_torrents(Control* c) {
  utils::Directory entries = c->core()->download_store()->get_formated_entries();

  for (utils::Directory::const_iterator first = entries.begin(), last = entries.end(); first != last; ++first) {
    // We don't really support session torrents that are links. These
    // would be overwritten anyway on exit, and thus not really be
    // useful.
    if (!first->is_file())
      continue;

    core::DownloadFactory* f = new core::DownloadFactory(c->core());

    // Replace with session torrent flag.
    f->set_session(true);
    f->slot_finished(std::bind(&rak::call_delete_func<core::DownloadFactory>, f));
    f->load(entries.path() + first->d_name);
    f->commit();
  }
}

void
load_arg_torrents(Control* c, char** first, char** last) {
  //std::for_each(begin, end, std::bind1st(std::mem_fun(&core::Manager::insert), &c->get_core()));
  for (; first != last; ++first) {
    core::DownloadFactory* f = new core::DownloadFactory(c->core());

    // Replace with session torrent flag.
    f->set_start(true);
    f->slot_finished(std::bind(&rak::call_delete_func<core::DownloadFactory>, f));
    f->load(*first);
    f->commit();
  }
}

static uint64_t
client_next_timeout(Control* c) {
  if (taskScheduler.empty())
    return (c->is_shutdown_started() ? rak::timer::from_milliseconds(100) : rak::timer::from_seconds(60)).usec();
  else if (taskScheduler.top()->time() <= cachedTime)
    return 0;
  else
    return (taskScheduler.top()->time() - cachedTime).usec();
}

static void
client_perform() {
  // Use throw exclusively.
  if (control->is_shutdown_completed())
    throw torrent::shutdown_exception();

  if (control->is_shutdown_received())
    control->handle_shutdown();

  control->inc_tick();

  cachedTime = rak::timer::current();
  rak::priority_queue_perform(&taskScheduler, cachedTime);
}

int
main(int argc, char** argv) {
  try {

    // Temporary.
    setlocale(LC_ALL, "");

    cachedTime = rak::timer::current();

    // Initialize logging:
    torrent::log_initialize();

    control = new Control;
    
    srandom(cachedTime.usec() ^ (getpid() << 16) ^ getppid());
    srand48(cachedTime.usec() ^ (getpid() << 16) ^ getppid());

    SignalHandler::set_ignore(SIGPIPE);
    SignalHandler::set_handler(SIGINT,   std::bind(&Control::receive_normal_shutdown, control));
    SignalHandler::set_handler(SIGTERM,  std::bind(&Control::receive_quick_shutdown, control));
    SignalHandler::set_handler(SIGWINCH, std::bind(&display::Manager::force_redraw, control->display()));
    SignalHandler::set_handler(SIGSEGV,  std::bind(&do_panic, SIGSEGV));
    SignalHandler::set_handler(SIGILL,   std::bind(&do_panic, SIGILL));
    SignalHandler::set_handler(SIGFPE,   std::bind(&do_panic, SIGFPE));

    SignalHandler::set_sigaction_handler(SIGBUS, &handle_sigbus);

    // SIGUSR1 is used for interrupting polling, forcing the target
    // thread to process new non-socket events.
    //
    // LibTorrent uses sockets for this purpose on Solaris and other
    // platforms that do not properly pass signals to the target
    // threads. Use '--enable-interrupt-socket' when configuring
    // LibTorrent to enable this workaround.
    if (torrent::thread_base::should_handle_sigusr1())
      SignalHandler::set_handler(SIGUSR1, std::bind(&do_nothing));

    torrent::log_add_group_output(torrent::LOG_NOTICE, "important");
    torrent::log_add_group_output(torrent::LOG_INFO, "complete");

    torrent::Poll::slot_create_poll() = std::bind(&core::create_poll);

    torrent::initialize();
    torrent::main_thread()->slot_do_work() = std::bind(&client_perform);
    torrent::main_thread()->slot_next_timeout() = std::bind(&client_next_timeout, control);

    worker_thread = new ThreadWorker();
    worker_thread->init_thread();

    // Initialize option handlers after libtorrent to ensure
    // torrent::ConnectionManager* are valid etc.
    initialize_commands();

    if (OptionParser::has_flag('D', argc, argv)) {
      rpc::call_command_set_value("method.use_deprecated.set", true);
      lt_log_print(torrent::LOG_WARN, "Disabled deprecated commands.");
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
       "method.set_key = event.download.inserted_new,     1_prepare, {(branch,((d.state)),((view.set_visible,started)),((view.set_visible,stopped)) ),(d.save_full_session)}\n"
       "method.set_key = event.download.inserted_session, 1_prepare, {(branch,((d.state)),((view.set_visible,started)),((view.set_visible,stopped)) )}\n"

       "method.set_key = event.download.inserted, 1_prioritize_toc, \"branch=file.prioritize_toc=,{\\\"f.multicall=(file.prioritize_toc.first),f.prioritize_first.enable=\\\",\\\"f.multicall=(file.prioritize_toc.last),f.prioritize_last.enable=\\\",d.update_priorities=}\"\n"

       "method.set_key = event.download.erased, !_download_list, ui.unfocus_download=\n"
       "method.set_key = event.download.erased, ~_delete_tied, d.delete_tied=\n"

       "method.set_key = event.download.resumed,   !_timestamp, ((d.timestamp.started.set_if_z, ((system.time)) ))\n"
       "method.set_key = event.download.finished,  !_timestamp, ((d.timestamp.finished.set_if_z, ((system.time)) ))\n"

       "method.insert.c_simple = group.insert_persistent_view,"
       "((view.add,((argument.0)))),((view.persistent,((argument.0)))),((group.insert,((argument.0)),((argument.0))))\n"

       "file.prioritize_toc.first.set = {*.avi,*.mp4,*.mkv,*.gz}\n"
       "file.prioritize_toc.last.set  = {*.zip}\n"

       // Allow setting 'group2.view' as constant, so that we can't
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

       "schedule2 = view.main,10,10,((view.sort,main,20))\n"
       "schedule2 = view.name,10,10,((view.sort,name,20))\n"

       "schedule2 = session_save,1200,1200,((session.save))\n"
       "schedule2 = low_diskspace,5,60,((close_low_diskspace,500M))\n"
       "schedule2 = prune_file_status,3600,86400,((system.file_status_cache.prune))\n"

       "protocol.encryption.set=allow_incoming,prefer_plaintext,enable_retry\n"
    );

    // Functions that might not get depracted as they are nice for
    // configuration files, and thus might do with just some
    // cleanup.
    CMD2_REDIRECT_GENERIC("upload_rate",   "throttle.global_up.max_rate.set_kb");
    CMD2_REDIRECT_GENERIC("download_rate", "throttle.global_down.max_rate.set_kb");

    CMD2_REDIRECT_GENERIC("ratio.enable",     "group.seeding.ratio.enable");
    CMD2_REDIRECT_GENERIC("ratio.disable",    "group.seeding.ratio.disable");
    CMD2_REDIRECT_GENERIC("ratio.min",        "group2.seeding.ratio.min");
    CMD2_REDIRECT_GENERIC("ratio.max",        "group2.seeding.ratio.max");
    CMD2_REDIRECT_GENERIC("ratio.upload",     "group2.seeding.ratio.upload");
    CMD2_REDIRECT_GENERIC("ratio.min.set",    "group2.seeding.ratio.min.set");
    CMD2_REDIRECT_GENERIC("ratio.max.set",    "group2.seeding.ratio.max.set");
    CMD2_REDIRECT_GENERIC("ratio.upload.set", "group2.seeding.ratio.upload.set");

    CMD2_REDIRECT_GENERIC("encryption", "protocol.encryption.set");
    CMD2_REDIRECT_GENERIC("encoding_list", "encoding.add");

    CMD2_REDIRECT_GENERIC("connection_leech", "protocol.connection.leech.set");
    CMD2_REDIRECT_GENERIC("connection_seed", "protocol.connection.seed.set");

    CMD2_REDIRECT        ("min_peers", "throttle.min_peers.normal.set");
    CMD2_REDIRECT        ("max_peers", "throttle.max_peers.normal.set");
    CMD2_REDIRECT        ("min_peers_seed", "throttle.min_peers.seed.set");
    CMD2_REDIRECT        ("max_peers_seed", "throttle.max_peers.seed.set");

    CMD2_REDIRECT        ("min_uploads",   "throttle.min_uploads.set");
    CMD2_REDIRECT        ("max_uploads",   "throttle.max_uploads.set");
    CMD2_REDIRECT        ("min_downloads", "throttle.min_downloads.set");
    CMD2_REDIRECT        ("max_downloads", "throttle.max_downloads.set");

    CMD2_REDIRECT        ("max_uploads_div",      "throttle.max_uploads.div.set");
    CMD2_REDIRECT        ("max_uploads_global",   "throttle.max_uploads.global.set");
    CMD2_REDIRECT        ("max_downloads_div",    "throttle.max_downloads.div.set");
    CMD2_REDIRECT        ("max_downloads_global", "throttle.max_downloads.global.set");

    CMD2_REDIRECT_GENERIC("max_memory_usage", "pieces.memory.max.set");

    CMD2_REDIRECT        ("bind",       "network.bind_address.set");
    CMD2_REDIRECT        ("ip",         "network.local_address.set");
    CMD2_REDIRECT        ("port_range", "network.port_range.set");

    CMD2_REDIRECT_GENERIC("dht",      "dht.mode.set");
    CMD2_REDIRECT_GENERIC("dht_port", "dht.port.set");

    CMD2_REDIRECT        ("port_random", "network.port_random.set");
    CMD2_REDIRECT        ("proxy_address", "network.proxy_address.set");

    CMD2_REDIRECT        ("scgi_port", "network.scgi.open_port");
    CMD2_REDIRECT        ("scgi_local", "network.scgi.open_local");

    CMD2_REDIRECT_GENERIC("directory", "directory.default.set");
    CMD2_REDIRECT_GENERIC("session",   "session.path.set");

    CMD2_REDIRECT        ("check_hash", "pieces.hash.on_completion.set");

    CMD2_REDIRECT        ("key_layout", "keys.layout.set");

    CMD2_REDIRECT_GENERIC("to_gm_time", "convert.gm_time");
    CMD2_REDIRECT_GENERIC("to_gm_date", "convert.gm_date");
    CMD2_REDIRECT_GENERIC("to_time", "convert.time");
    CMD2_REDIRECT_GENERIC("to_date", "convert.date");
    CMD2_REDIRECT_GENERIC("to_elapsed_time", "convert.elapsed_time");
    CMD2_REDIRECT_GENERIC("to_kb", "convert.kb");
    CMD2_REDIRECT_GENERIC("to_mb", "convert.mb");
    CMD2_REDIRECT_GENERIC("to_xb", "convert.xb");
    CMD2_REDIRECT_GENERIC("to_throttle", "convert.throttle");

    // Deprecated commands. Don't use these anymore.

    if (rpc::call_command_value("method.use_intermediate") == 1) {
      CMD2_REDIRECT_GENERIC("execute", "execute2");

      CMD2_REDIRECT_GENERIC("schedule", "schedule2");
      CMD2_REDIRECT_GENERIC("schedule_remove", "schedule_remove2");

    } else if (rpc::call_command_value("method.use_intermediate") == 2) {
      // Allow for use in config files, etc, just don't export it.
      CMD2_REDIRECT_GENERIC_NO_EXPORT("execute", "execute2");

      CMD2_REDIRECT_GENERIC_NO_EXPORT("schedule", "schedule2");
      CMD2_REDIRECT_GENERIC_NO_EXPORT("schedule_remove", "schedule_remove2");
    }

#if LT_SLIM_VERSION != 1
    if (rpc::call_command_value("method.use_deprecated")) {
      // Deprecated in 0.7.0:

      CMD2_REDIRECT_GENERIC("system.method.insert", "method.insert");
      CMD2_REDIRECT_GENERIC("system.method.erase", "method.erase");
      CMD2_REDIRECT_GENERIC("system.method.get", "method.get");
      CMD2_REDIRECT_GENERIC("system.method.set", "method.set");
      CMD2_REDIRECT_GENERIC("system.method.list_keys", "method.list_keys");
      CMD2_REDIRECT_GENERIC("system.method.has_key", "method.has_key");
      CMD2_REDIRECT_GENERIC("system.method.set_key", "method.set_key");

      CMD2_REDIRECT        ("get_directory", "directory.default");
      CMD2_REDIRECT_GENERIC("set_directory", "directory.default.set");

      CMD2_REDIRECT        ("get_session", "session.path");
      CMD2_REDIRECT_GENERIC("set_session", "session.path.set");

      CMD2_REDIRECT        ("session_save", "session.save");

      CMD2_REDIRECT        ("get_name", "session.name");
      CMD2_REDIRECT_GENERIC("set_name", "session.name.set");

      CMD2_REDIRECT        ("system.file_allocate", "system.file.allocate");
      CMD2_REDIRECT        ("system.file_allocate.set", "system.file.allocate.set");
      CMD2_REDIRECT        ("get_max_file_size", "system.file.max_size");
      CMD2_REDIRECT_GENERIC("set_max_file_size", "system.file.max_size.set");
      CMD2_REDIRECT        ("get_split_file_size", "system.file.split_size");
      CMD2_REDIRECT_GENERIC("set_split_file_size", "system.file.split_size.set");
      CMD2_REDIRECT        ("get_split_suffix", "system.file.split_suffix");
      CMD2_REDIRECT_GENERIC("set_split_suffix", "system.file.split_suffix.set");

      CMD2_REDIRECT        ("get_timeout_sync", "pieces.sync.timeout");
      CMD2_REDIRECT_GENERIC("set_timeout_sync", "pieces.sync.timeout.set");
      CMD2_REDIRECT        ("get_timeout_safe_sync", "pieces.sync.timeout_safe");
      CMD2_REDIRECT_GENERIC("set_timeout_safe_sync", "pieces.sync.timeout_safe.set");

      CMD2_REDIRECT_GENERIC("get_preload_type", "pieces.preload.type");
      CMD2_REDIRECT_GENERIC("get_preload_min_size", "pieces.preload.min_size");
      CMD2_REDIRECT_GENERIC("get_preload_required_rate", "pieces.preload.min_rate");
      CMD2_REDIRECT_GENERIC("set_preload_type", "pieces.preload.type.set");
      CMD2_REDIRECT_GENERIC("set_preload_min_size", "pieces.preload.min_size.set");
      CMD2_REDIRECT_GENERIC("set_preload_required_rate", "pieces.preload.min_rate.set");
      CMD2_REDIRECT_GENERIC("get_stats_preloaded", "pieces.stats_preloaded");
      CMD2_REDIRECT_GENERIC("get_stats_not_preloaded", "pieces.stats_not_preloaded");

      CMD2_REDIRECT_GENERIC("get_safe_sync", "pieces.sync.always_safe");
      CMD2_REDIRECT_GENERIC("set_safe_sync", "pieces.sync.always_safe.set");

      CMD2_REDIRECT        ("get_memory_usage",     "pieces.memory.current");
      CMD2_REDIRECT_GENERIC("get_max_memory_usage", "pieces.memory.max");
      CMD2_REDIRECT_GENERIC("set_max_memory_usage", "pieces.memory.max.set");

      CMD2_REDIRECT_GENERIC("load", "load.normal");
      CMD2_REDIRECT_GENERIC("load_verbose", "load.verbose");
      CMD2_REDIRECT_GENERIC("load_start", "load.start");
      CMD2_REDIRECT_GENERIC("load_start_verbose", "load.start_verbose");

      CMD2_REDIRECT_GENERIC("load_raw", "load.raw");
      CMD2_REDIRECT_GENERIC("load_raw_start", "load.raw_start");
      CMD2_REDIRECT_GENERIC("load_raw_verbose", "load.raw_verbose");

      CMD2_REDIRECT_GENERIC("get_connection_leech", "protocol.connection.leech");
      CMD2_REDIRECT_GENERIC("get_connection_seed",  "protocol.connection.seed");
      CMD2_REDIRECT_GENERIC("set_connection_leech", "protocol.connection.leech.set");
      CMD2_REDIRECT_GENERIC("set_connection_seed",  "protocol.connection.seed.set");

      //
      // Throttle:
      //

      CMD2_REDIRECT_GENERIC("throttle_up", "throttle.up");
      CMD2_REDIRECT_GENERIC("throttle_down", "throttle.down");
      CMD2_REDIRECT_GENERIC("throttle_ip", "throttle.ip");

      CMD2_REDIRECT_GENERIC("get_throttle_up_max", "throttle.up.max");
      CMD2_REDIRECT_GENERIC("get_throttle_up_rate", "throttle.up.rate");
      CMD2_REDIRECT_GENERIC("get_throttle_down_max", "throttle.down.max");
      CMD2_REDIRECT_GENERIC("get_throttle_down_rate", "throttle.down.rate");

      CMD2_REDIRECT_GENERIC("set_min_peers", "throttle.min_peers.normal.set");
      CMD2_REDIRECT_GENERIC("set_max_peers", "throttle.max_peers.normal.set");
      CMD2_REDIRECT_GENERIC("set_min_peers_seed", "throttle.min_peers.seed.set");
      CMD2_REDIRECT_GENERIC("set_max_peers_seed", "throttle.max_peers.seed.set");

      CMD2_REDIRECT_GENERIC("set_max_uploads", "throttle.max_uploads.set");

      CMD2_REDIRECT_GENERIC("set_max_uploads_div", "throttle.max_uploads.div.set");
      CMD2_REDIRECT_GENERIC("set_max_uploads_global", "throttle.max_uploads.global.set");
      CMD2_REDIRECT_GENERIC("set_max_downloads_div", "throttle.max_downloads.div.set");
      CMD2_REDIRECT_GENERIC("set_max_downloads_global", "throttle.max_downloads.global.set");

      CMD2_REDIRECT        ("get_min_peers", "throttle.min_peers.normal");
      CMD2_REDIRECT        ("get_max_peers", "throttle.max_peers.normal");
      CMD2_REDIRECT        ("get_min_peers_seed", "throttle.min_peers.seed");
      CMD2_REDIRECT        ("get_max_peers_seed", "throttle.max_peers.seed");

      CMD2_REDIRECT        ("get_max_uploads", "throttle.max_uploads");

      CMD2_REDIRECT        ("get_max_uploads_div", "throttle.max_uploads.div");
      CMD2_REDIRECT        ("get_max_uploads_global", "throttle.max_uploads.global");
      CMD2_REDIRECT        ("get_max_downloads_div", "throttle.max_downloads.div");
      CMD2_REDIRECT        ("get_max_downloads_global", "throttle.max_downloads.global");

      CMD2_REDIRECT        ("get_up_rate", "throttle.global_up.rate");
      CMD2_REDIRECT        ("get_up_total", "throttle.global_up.total");
      CMD2_REDIRECT        ("get_upload_rate", "throttle.global_up.max_rate");
      CMD2_REDIRECT_GENERIC("set_upload_rate", "throttle.global_up.max_rate.set");
      CMD2_REDIRECT        ("get_down_rate", "throttle.global_down.rate");
      CMD2_REDIRECT        ("get_down_total", "throttle.global_down.total");
      CMD2_REDIRECT        ("get_download_rate", "throttle.global_down.max_rate");
      CMD2_REDIRECT_GENERIC("set_download_rate", "throttle.global_down.max_rate.set");

      //
      // Network:
      //

      CMD2_REDIRECT        ("get_send_buffer_size", "network.send_buffer.size");
      CMD2_REDIRECT_GENERIC("set_send_buffer_size", "network.send_buffer.size.set");
      CMD2_REDIRECT        ("get_receive_buffer_size", "network.receive_buffer.size");
      CMD2_REDIRECT_GENERIC("set_receive_buffer_size", "network.receive_buffer.size.set");

      CMD2_REDIRECT_GENERIC("set_bind", "network.bind_address.set");
      CMD2_REDIRECT        ("get_bind", "network.bind_address");

      CMD2_REDIRECT_GENERIC("set_ip", "network.local_address.set");
      CMD2_REDIRECT        ("get_ip", "network.local_address");

      CMD2_REDIRECT        ("get_port_range", "network.port_range");
      CMD2_REDIRECT_GENERIC("set_port_range", "network.port_range.set");

      CMD2_REDIRECT        ("get_port_random", "network.port_random");
      CMD2_REDIRECT_GENERIC("set_port_random", "network.port_random.set");

      CMD2_REDIRECT        ("port_open", "network.port_open.set");
      CMD2_REDIRECT        ("get_port_open", "network.port_open");
      CMD2_REDIRECT_GENERIC("set_port_open", "network.port_open.set");

      CMD2_REDIRECT_GENERIC("set_proxy_address", "network.proxy_address.set");
      CMD2_REDIRECT        ("get_proxy_address", "network.proxy_address");

      CMD2_REDIRECT_GENERIC("set_scgi_dont_route", "network.scgi.dont_route.set");
      CMD2_REDIRECT        ("get_scgi_dont_route", "network.scgi.dont_route");

      CMD2_REDIRECT        ("get_max_open_sockets", "network.max_open_sockets");

      CMD2_REDIRECT        ("get_max_open_files", "network.max_open_files");
      CMD2_REDIRECT_GENERIC("set_max_open_files", "network.max_open_files.set");

      //
      // XMLRPC stuff:
      //

      CMD2_REDIRECT_GENERIC("xmlrpc_dialect", "network.xmlrpc.dialect.set");
      CMD2_REDIRECT_GENERIC("set_xmlrpc_dialect", "network.xmlrpc.dialect.set");

      CMD2_REDIRECT_GENERIC("xmlrpc_size_limit", "network.xmlrpc.size_limit.set");
      CMD2_REDIRECT        ("get_xmlrpc_size_limit", "network.xmlrpc.size_limit");
      CMD2_REDIRECT_GENERIC("set_xmlrpc_size_limit", "network.xmlrpc.size_limit.set");

      //
      // HTTP stuff:
      //

      CMD2_REDIRECT_GENERIC("http_capath", "network.http.capath.set");
      CMD2_REDIRECT        ("get_http_capath", "network.http.capath");
      CMD2_REDIRECT_GENERIC("set_http_capath", "network.http.capath.set");

      CMD2_REDIRECT_GENERIC("http_cacert", "network.http.cacert.set");
      CMD2_REDIRECT        ("get_http_cacert", "network.http.cacert");
      CMD2_REDIRECT_GENERIC("set_http_cacert", "network.http.cacert.set");

      CMD2_REDIRECT        ("get_max_open_http", "network.http.max_open");
      CMD2_REDIRECT_GENERIC("set_max_open_http", "network.http.max_open.set");

      CMD2_REDIRECT_GENERIC("http_proxy", "network.http.proxy_address.set");
      CMD2_REDIRECT        ("get_http_proxy", "network.http.proxy_address");
      CMD2_REDIRECT_GENERIC("set_http_proxy", "network.http.proxy_address.set");

      CMD2_REDIRECT        ("peer_exchange", "protocol.pex.set");
      CMD2_REDIRECT        ("get_peer_exchange", "protocol.pex");
      CMD2_REDIRECT_GENERIC("set_peer_exchange", "protocol.pex.set");

      //
      // Trackers:
      //

      CMD2_REDIRECT        ("tracker_numwant", "trackers.numwant.set");
      CMD2_REDIRECT        ("get_tracker_numwant", "trackers.numwant");
      CMD2_REDIRECT_GENERIC("set_tracker_numwant", "trackers.numwant.set");
      
      CMD2_REDIRECT        ("use_udp_trackers", "trackers.use_udp.set");
      CMD2_REDIRECT        ("get_use_udp_trackers", "trackers.use_udp");
      CMD2_REDIRECT_GENERIC("set_use_udp_trackers", "trackers.use_udp.set");

      //
      // DHT stuff
      //

      CMD2_REDIRECT        ("dht_add_node", "dht.add_node");
      CMD2_REDIRECT        ("dht_statistics", "dht.statistics");
      CMD2_REDIRECT        ("get_dht_port", "dht.port");
      CMD2_REDIRECT_GENERIC("set_dht_port", "dht.port.set");
      CMD2_REDIRECT        ("get_dht_throttle", "dht.throttle.name");
      CMD2_REDIRECT_GENERIC("set_dht_throttle", "dht.throttle.name.set");

      //
      // Various system stuff:
      //

      CMD2_REDIRECT        ("get_session_lock", "session.use_lock");
      CMD2_REDIRECT_GENERIC("set_session_lock", "session.use_lock.set");
      CMD2_REDIRECT        ("get_session_on_completion", "session.on_completion");
      CMD2_REDIRECT_GENERIC("set_session_on_completion", "session.on_completion.set");

      CMD2_REDIRECT        ("get_check_hash", "pieces.hash.on_completion");
      CMD2_REDIRECT_GENERIC("set_check_hash", "pieces.hash.on_completion.set");

      //
      // Download:
      //

      CMD2_REDIRECT        ("d.get_hash", "d.hash");
      CMD2_REDIRECT        ("d.get_local_id", "d.local_id");
      CMD2_REDIRECT        ("d.get_local_id_html", "d.local_id_html");
      CMD2_REDIRECT        ("d.get_bitfield", "d.bitfield");
      CMD2_REDIRECT        ("d.get_base_path", "d.base_path");
      CMD2_REDIRECT        ("d.get_base_filename", "d.base_filename");

      CMD2_REDIRECT        ("d.get_name", "d.name");
      CMD2_REDIRECT        ("d.get_creation_date", "d.creation_date");

      CMD2_REDIRECT        ("d.get_peer_exchange", "d.peer_exchange");

      CMD2_REDIRECT        ("d.get_up_rate", "d.up.rate");
      CMD2_REDIRECT        ("d.get_up_total", "d.up.total");
      CMD2_REDIRECT        ("d.get_down_rate", "d.down.rate");
      CMD2_REDIRECT        ("d.get_down_total", "d.down.total");
      CMD2_REDIRECT        ("d.get_skip_rate", "d.skip.rate");
      CMD2_REDIRECT        ("d.get_skip_total", "d.skip.total");

      //
      CMD2_REDIRECT        ("d.get_bytes_done", "d.bytes_done");
      CMD2_REDIRECT        ("d.get_chunk_size", "d.chunk_size");
      CMD2_REDIRECT        ("d.get_chunks_hashed", "d.chunks_hashed");
      CMD2_REDIRECT        ("d.get_complete", "d.complete");
      CMD2_REDIRECT        ("d.get_completed_bytes", "d.completed_bytes");
      CMD2_REDIRECT        ("d.get_completed_chunks", "d.completed_chunks");
      CMD2_REDIRECT        ("d.get_connection_current", "d.connection_current");
      CMD2_REDIRECT        ("d.get_connection_leech", "d.connection_leech");
      CMD2_REDIRECT        ("d.get_connection_seed", "d.connection_seed");
      CMD2_REDIRECT        ("d.get_custom", "d.custom");
      CMD2_REDIRECT        ("d.get_custom1", "d.custom1");
      CMD2_REDIRECT        ("d.get_custom2", "d.custom2");
      CMD2_REDIRECT        ("d.get_custom3", "d.custom3");
      CMD2_REDIRECT        ("d.get_custom4", "d.custom4");
      CMD2_REDIRECT        ("d.get_custom5", "d.custom5");
      CMD2_REDIRECT        ("d.get_custom_throw", "d.custom_throw");
      CMD2_REDIRECT        ("d.get_directory", "d.directory");
      CMD2_REDIRECT        ("d.get_directory_base", "d.directory_base");
      CMD2_REDIRECT        ("d.get_free_diskspace", "d.free_diskspace");
      CMD2_REDIRECT        ("d.get_hashing", "d.hashing");
      CMD2_REDIRECT        ("d.get_hashing_failed", "d.hashing_failed");
      CMD2_REDIRECT        ("d.get_ignore_commands", "d.ignore_commands");
      CMD2_REDIRECT        ("d.get_left_bytes", "d.left_bytes");
      CMD2_REDIRECT        ("d.get_loaded_file", "d.loaded_file");
      CMD2_REDIRECT        ("d.get_max_file_size", "d.max_file_size");
      CMD2_REDIRECT        ("d.get_max_size_pex", "d.max_size_pex");
      CMD2_REDIRECT        ("d.get_message", "d.message");
      CMD2_REDIRECT        ("d.get_mode", "d.mode");
      CMD2_REDIRECT        ("d.get_peers_accounted", "d.peers_accounted");
      CMD2_REDIRECT        ("d.get_peers_complete", "d.peers_complete");
      CMD2_REDIRECT        ("d.get_peers_connected", "d.peers_connected");
      CMD2_REDIRECT        ("d.get_peers_max", "d.peers_max");
      CMD2_REDIRECT        ("d.get_peers_min", "d.peers_min");
      CMD2_REDIRECT        ("d.get_peers_not_connected", "d.peers_not_connected");
      CMD2_REDIRECT        ("d.get_priority", "d.priority");
      CMD2_REDIRECT        ("d.get_priority_str", "d.priority_str");
      CMD2_REDIRECT        ("d.get_ratio", "d.ratio");
      CMD2_REDIRECT        ("d.get_size_bytes", "d.size_bytes");
      CMD2_REDIRECT        ("d.get_size_chunks", "d.size_chunks");
      CMD2_REDIRECT        ("d.get_size_files", "d.size_files");
      CMD2_REDIRECT        ("d.get_size_pex", "d.size_pex");
      CMD2_REDIRECT        ("d.get_state", "d.state");
      CMD2_REDIRECT        ("d.get_state_changed", "d.state_changed");
      CMD2_REDIRECT        ("d.get_state_counter", "d.state_counter");
      CMD2_REDIRECT        ("d.get_throttle_name", "d.throttle_name");
      CMD2_REDIRECT        ("d.get_tied_to_file", "d.tied_to_file");
      CMD2_REDIRECT        ("d.get_tracker_focus", "d.tracker_focus");
      CMD2_REDIRECT        ("d.get_tracker_numwant", "d.tracker_numwant");
      CMD2_REDIRECT        ("d.get_tracker_size", "d.tracker_size");
      CMD2_REDIRECT        ("d.get_uploads_max", "d.uploads_max");

      CMD2_REDIRECT        ("d.set_connection_current", "d.connection_current.set");
      CMD2_REDIRECT        ("d.set_custom", "d.custom.set");
      CMD2_REDIRECT        ("d.set_custom1", "d.custom1.set");
      CMD2_REDIRECT        ("d.set_custom2", "d.custom2.set");
      CMD2_REDIRECT        ("d.set_custom3", "d.custom3.set");
      CMD2_REDIRECT        ("d.set_custom4", "d.custom4.set");
      CMD2_REDIRECT        ("d.set_custom5", "d.custom5.set");
      CMD2_REDIRECT        ("d.set_directory", "d.directory.set");
      CMD2_REDIRECT        ("d.set_directory_base", "d.directory_base.set");
      CMD2_REDIRECT        ("d.set_hashing_failed", "d.hashing_failed.set");
      CMD2_REDIRECT        ("d.set_ignore_commands", "d.ignore_commands.set");
      CMD2_REDIRECT        ("d.set_max_file_size", "d.max_file_size.set");
      CMD2_REDIRECT        ("d.set_message", "d.message.set");
      CMD2_REDIRECT        ("d.set_peers_max", "d.peers_max.set");
      CMD2_REDIRECT        ("d.set_peers_min", "d.peers_min.set");
      CMD2_REDIRECT        ("d.set_priority", "d.priority.set");
      CMD2_REDIRECT        ("d.set_throttle_name", "d.throttle_name.set");
      CMD2_REDIRECT        ("d.set_tied_to_file", "d.tied_to_file.set");
      CMD2_REDIRECT        ("d.set_tracker_numwant", "d.tracker_numwant.set");
      CMD2_REDIRECT        ("d.set_uploads_max", "d.uploads_max.set");

      CMD2_REDIRECT        ("create_link", "d.create_link");
      CMD2_REDIRECT        ("delete_link", "d.delete_link");
      CMD2_REDIRECT        ("delete_tied", "d.delete_tied");

      //
      // Tracker:
      //

      CMD2_REDIRECT_TRACKER("t.get_group", "t.group");
      CMD2_REDIRECT_TRACKER("t.get_id", "t.id");
      CMD2_REDIRECT_TRACKER("t.get_min_interval", "t.min_interval");
      CMD2_REDIRECT_TRACKER("t.get_normal_interval", "t.normal_interval");
      CMD2_REDIRECT_TRACKER("t.get_scrape_complete", "t.scrape_complete");
      CMD2_REDIRECT_TRACKER("t.get_scrape_downloaded", "t.scrape_downloaded");
      CMD2_REDIRECT_TRACKER("t.get_scrape_incomplete", "t.scrape_incomplete");
      CMD2_REDIRECT_TRACKER("t.get_scrape_time_last", "t.scrape_time_last");
      CMD2_REDIRECT_TRACKER("t.get_type", "t.type");
      CMD2_REDIRECT_TRACKER("t.get_url", "t.url");
      CMD2_REDIRECT_TRACKER("t.set_enabled", "t.is_enabled.set");

      //
      // File:
      //

      CMD2_REDIRECT_FILE   ("f.get_completed_chunks", "f.completed_chunks");
      CMD2_REDIRECT_FILE   ("f.get_frozen_path", "f.frozen_path");
      CMD2_REDIRECT_FILE   ("f.get_last_touched", "f.last_touched");
      CMD2_REDIRECT_FILE   ("f.get_match_depth_next", "f.match_depth_next");
      CMD2_REDIRECT_FILE   ("f.get_match_depth_prev", "f.match_depth_prev");
      CMD2_REDIRECT_FILE   ("f.get_offset", "f.offset");
      CMD2_REDIRECT_FILE   ("f.get_path", "f.path");
      CMD2_REDIRECT_FILE   ("f.get_path_components", "f.path_components");
      CMD2_REDIRECT_FILE   ("f.get_path_depth", "f.path_depth");
      CMD2_REDIRECT_FILE   ("f.get_priority", "f.priority");
      CMD2_REDIRECT_FILE   ("f.get_range_first", "f.range_first");
      CMD2_REDIRECT_FILE   ("f.get_range_second", "f.range_second");
      CMD2_REDIRECT_FILE   ("f.get_size_bytes", "f.size_bytes");
      CMD2_REDIRECT_FILE   ("f.get_size_chunks", "f.size_chunks");
      CMD2_REDIRECT_FILE   ("f.set_priority", "f.priority.set");
      CMD2_REDIRECT_FILE   ("fi.get_filename_last", "fi.filename_last");

      //
      // Peer:
      //

      CMD2_REDIRECT        ("p.get_address", "p.address");
      CMD2_REDIRECT        ("p.get_client_version", "p.client_version");
      CMD2_REDIRECT        ("p.get_completed_percent", "p.completed_percent");
      CMD2_REDIRECT        ("p.get_down_rate", "p.down_rate");
      CMD2_REDIRECT        ("p.get_down_total", "p.down_total");
      CMD2_REDIRECT        ("p.get_id", "p.id");
      CMD2_REDIRECT        ("p.get_id_html", "p.id_html");
      CMD2_REDIRECT        ("p.get_options_str", "p.options_str");
      CMD2_REDIRECT        ("p.get_peer_rate", "p.peer_rate");
      CMD2_REDIRECT        ("p.get_peer_total", "p.peer_total");
      CMD2_REDIRECT        ("p.get_port", "p.port");
      CMD2_REDIRECT        ("p.get_up_rate", "p.up_rate");
      CMD2_REDIRECT        ("p.get_up_total", "p.up_total");

      //
      // View:
      //

      CMD2_REDIRECT_GENERIC("view_add", "view.add");
      CMD2_REDIRECT_GENERIC("view_filter", "view.filter");
      CMD2_REDIRECT_GENERIC("view_filter_on", "view.filter_on");
      CMD2_REDIRECT_GENERIC("view_list", "view.list");
      CMD2_REDIRECT_GENERIC("view_set", "view.set");
      CMD2_REDIRECT_GENERIC("view_sort", "view.sort");
      CMD2_REDIRECT_GENERIC("view_sort_current", "view.sort_current");
      CMD2_REDIRECT_GENERIC("view_sort_new", "view.sort_new");

      // Rename these to avoid conflicts with old style.
      CMD2_REDIRECT_GENERIC("d.multicall", "d.multicall2");

      CMD2_REDIRECT_GENERIC("execute_throw", "execute.throw");
      CMD2_REDIRECT_GENERIC("execute_nothrow", "execute.nothrow");
      CMD2_REDIRECT_GENERIC("execute_raw", "execute.raw");
      CMD2_REDIRECT_GENERIC("execute_raw_nothrow", "execute.raw_nothrow");
      CMD2_REDIRECT_GENERIC("execute_capture", "execute.capture");
      CMD2_REDIRECT_GENERIC("execute_capture_nothrow", "execute.capture_nothrow");
    }
#endif

    int firstArg = parse_options(control, argc, argv);

    if (OptionParser::has_flag('n', argc, argv)) {
      lt_log_print(torrent::LOG_WARN, "Ignoring ~/.rtorrent.rc.");
    } else {
      rpc::parse_command_single(rpc::make_target(), "try_import = ~/.rtorrent.rc");
    }

    control->initialize();

    // Load session torrents and perform scheduled tasks to ensure
    // session torrents are loaded before arg torrents.
    control->dht_manager()->load_dht_cache();
    load_session_torrents(control);
    rak::priority_queue_perform(&taskScheduler, cachedTime);

    load_arg_torrents(control, argv + firstArg, argv + argc);

    // Make sure we update the display before any scheduled tasks can
    // run, so that loading of torrents doesn't look like it hangs on
    // startup.
    control->display()->adjust_layout();
    control->display()->receive_update();

    worker_thread->start_thread();

    torrent::thread_base::event_loop(torrent::main_thread());

    control->core()->download_list()->session_save();
    control->cleanup();

  } catch (torrent::internal_error& e) {
    control->cleanup_exception();

    std::cout << "Caught internal_error: " << e.what() << std::endl
              << e.backtrace();
    lt_log_print_dump(torrent::LOG_CRITICAL, e.backtrace().c_str(), e.backtrace().size(),
                      "Caught internal_error: '%s'.", e.what());
    return -1;

  } catch (std::exception& e) {
    control->cleanup_exception();

    std::cout << "rtorrent: " << e.what() << std::endl;
    lt_log_print(torrent::LOG_CRITICAL, "Caught exception: '%s'.", e.what());
    return -1;
  }

  torrent::log_cleanup();

  delete control;
  delete worker_thread;

  return 0;
}

void
handle_sigbus(int signum, siginfo_t* sa, void* ptr) {
  if (signum != SIGBUS)
    do_panic(signum);

  SignalHandler::set_default(signum);
  display::Canvas::cleanup();

  std::stringstream output;
  output << "Caught SIGBUS, dumping stack:" << std::endl;

#ifdef USE_EXECINFO
  void* stackPtrs[20];

  // Print the stack and exit.
  int stackSize = backtrace(stackPtrs, 20);
  char** stackStrings = backtrace_symbols(stackPtrs, stackSize);

  for (int i = 0; i < stackSize; ++i)
    output << stackStrings[i] << std::endl;

#else
  output << "Stack dump not enabled." << std::endl;
#endif
  output << std::endl << "Error: " << rak::error_number(sa->si_errno).c_str() << std::endl;

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
  display::Canvas::cleanup();

  std::stringstream output;

  output << "Caught " << SignalHandler::as_string(signum) << ", dumping stack:" << std::endl;
  
#ifdef USE_EXECINFO
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
  std::cout << "Rakshasa's BitTorrent client version " VERSION "." << std::endl;
  std::cout << std::endl;
  std::cout << "All value pairs (f.ex rate and queue size) will be in the UP/DOWN" << std::endl;
  std::cout << "order. Use the up/down/left/right arrow keys to move between screens." << std::endl;
  std::cout << std::endl;
  std::cout << "Usage: rtorrent [OPTIONS]... [FILE]... [URL]..." << std::endl;
  std::cout << "  -D                Enable deprecated commands" << std::endl;
  std::cout << "  -h                Display this very helpful text" << std::endl;
  std::cout << "  -n                Don't try to load ~/.rtorrent.rc on startup" << std::endl;
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

  std::cout << "Report bugs to <sundell.software@gmail.com>." << std::endl;

  exit(0);
}
