// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

#include <cstdlib>
#include <iostream>
#include <string>
#include <sigc++/adaptors/bind.h>
#include <torrent/http.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>
#include <rak/functional.h>

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
#include "globals.h"
#include "signal_handler.h"
#include "option_parser.h"

#include "thread_main.h"
#include "thread_worker.h"

void do_panic(int signum);
void print_help();
void initialize_commands();

void do_nothing() {}

int
parse_options(Control* c, int argc, char** argv) {
  try {
    OptionParser optionParser;

    // Converted.
    optionParser.insert_flag('h', sigc::ptr_fun(&print_help));
    optionParser.insert_flag('n', OptionParser::Slot());
    optionParser.insert_flag('D', OptionParser::Slot());

    optionParser.insert_option('b', sigc::bind<0>(sigc::ptr_fun(&rpc::call_command_set_string), "network.bind_address.set"));
    optionParser.insert_option('d', sigc::bind<0>(sigc::ptr_fun(&rpc::call_command_set_string), "directory.default.set"));
    optionParser.insert_option('i', sigc::bind<0>(sigc::ptr_fun(&rpc::call_command_set_string), "ip"));
    optionParser.insert_option('p', sigc::bind<0>(sigc::ptr_fun(&rpc::call_command_set_string), "network.port_range.set"));
    optionParser.insert_option('s', sigc::bind<0>(sigc::ptr_fun(&rpc::call_command_set_string), "session"));

    optionParser.insert_option('O', sigc::ptr_fun(&rpc::parse_command_single_std));
    optionParser.insert_option_list('o', sigc::ptr_fun(&rpc::call_command_set_std_string));

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
    f->slot_finished(sigc::bind(sigc::ptr_fun(&rak::call_delete_func<core::DownloadFactory>), f));
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
    f->slot_finished(sigc::bind(sigc::ptr_fun(&rak::call_delete_func<core::DownloadFactory>), f));
    f->load(*first);
    f->commit();
  }
}

static inline rak::timer
client_next_timeout(Control* c) {
  if (taskScheduler.empty())
    return c->is_shutdown_started() ? rak::timer::from_milliseconds(100) : rak::timer::from_seconds(60);
  else if (taskScheduler.top()->time() <= cachedTime)
    return 0;
  else
    return taskScheduler.top()->time() - cachedTime;
}

int
main(int argc, char** argv) {
  try {

    // Temporary.
    setlocale(LC_ALL, "");

    cachedTime = rak::timer::current();

    control = new Control;
    
    main_thread = new ThreadMain();
    main_thread->init_thread();

    worker_thread = new ThreadWorker();
    worker_thread->init_thread();

    srandom(cachedTime.usec());
    srand48(cachedTime.usec());

    SignalHandler::set_ignore(SIGPIPE);
    SignalHandler::set_handler(SIGINT,   sigc::mem_fun(control, &Control::receive_normal_shutdown));
    SignalHandler::set_handler(SIGTERM,  sigc::mem_fun(control, &Control::receive_quick_shutdown));
    SignalHandler::set_handler(SIGWINCH, sigc::mem_fun(control->display(), &display::Manager::force_redraw));
    SignalHandler::set_handler(SIGSEGV,  sigc::bind(sigc::ptr_fun(&do_panic), SIGSEGV));
    SignalHandler::set_handler(SIGBUS,   sigc::bind(sigc::ptr_fun(&do_panic), SIGBUS));
    SignalHandler::set_handler(SIGFPE,   sigc::bind(sigc::ptr_fun(&do_panic), SIGFPE));

    // SIGUSR1 is used for interrupting polling, forcing that thread
    // to process new non-socket events.
    SignalHandler::set_handler(SIGUSR1,  sigc::ptr_fun(&do_nothing));

    torrent::initialize(main_thread->poll());

    // Initialize option handlers after libtorrent to ensure
    // torrent::ConnectionManager* are valid etc.
    initialize_commands();

    rpc::parse_command_multiple
      (rpc::make_target(),
//        "method.insert = test.value,value\n"
//        "method.insert = test.value2,value,6\n"

//        "method.insert = test.string,string,6\n"
//        "method.insert = test.bool,bool,true\n"

       "method.insert = test.method.simple,simple,\"print=simple_test_,$argument.0=\"\n"

       "method.insert = event.download.inserted,multi\n"
       "method.insert = event.download.inserted_new,multi\n"
       "method.insert = event.download.inserted_session,multi\n"
       "method.insert = event.download.erased,multi\n"
       "method.insert = event.download.opened,multi\n"
       "method.insert = event.download.closed,multi\n"
       "method.insert = event.download.resumed,multi\n"
       "method.insert = event.download.paused,multi\n"
       
       "method.insert = event.download.finished,multi\n"
       "method.insert = event.download.hash_done,multi\n"
       "method.insert = event.download.hash_failed,multi\n"
       "method.insert = event.download.hash_final_failed,multi\n"
       "method.insert = event.download.hash_removed,multi\n"
       "method.insert = event.download.hash_queued,multi\n"

       "method.set_key = event.download.inserted,         1_connect_logs, d.initialize_logs=\n"
       "method.set_key = event.download.inserted_new,     1_prepare, \"branch=d.state=,view.set_visible=started,view.set_visible=stopped ;d.save_full_session=\"\n"
       "method.set_key = event.download.inserted_session, 1_prepare, \"branch=d.state=,view.set_visible=started,view.set_visible=stopped\"\n"

       "method.set_key = event.download.erased, !_download_list, ui.unfocus_download=\n"
       "method.set_key = event.download.erased, ~_delete_tied, d.delete_tied=\n"

       "method.insert = ratio.enable, simple|const,group.seeding.ratio.enable=\n"
       "method.insert = ratio.disable,simple|const,group.seeding.ratio.disable=\n"
       "method.insert = ratio.min,    simple|const,group.seeding.ratio.min=\n"
       "method.insert = ratio.max,    simple|const,group.seeding.ratio.max=\n"
       "method.insert = ratio.upload, simple|const,group.seeding.ratio.upload=\n"
       "method.insert = ratio.min.set,   simple|const,group.seeding.ratio.min.set=$argument.0=\n"
       "method.insert = ratio.max.set,   simple|const,group.seeding.ratio.max.set=$argument.0=\n"
       "method.insert = ratio.upload.set,simple|const,group.seeding.ratio.upload.set=$argument.0=\n"

       "method.insert = group.insert_persistent_view,simple|const,"
       "view.add=$argument.0=,view.persistent=$argument.0=,\"group.insert=$argument.0=,$argument.0=\"\n"

       // Allow setting 'group.view' as constant, so that we can't
       // modify the value. And look into the possibility of making
       // 'const' use non-heap memory, as we know they can't be
       // erased.

       // TODO: Remember to ensure it doesn't get restarted by watch
       // dir, etc. Set ignore commands, or something.

       "group.insert = seeding,seeding\n"

       "system.session_name = \"$cat=$system.hostname=,:,$system.pid=\"\n"

       // Currently not doing any sorting on main.
       "view.add = main\n"
       "view.add = default\n"

       "view.add = name\n"
       "view.sort_new     = name,less=d.name=\n"
       "view.sort_current = name,less=d.name=\n"

       "view.add = active\n"
       "view.filter = active,false=\n"

       "view.add = started\n"
       "view.filter = started,false=\n"
       "view.event_added   = started,\"view.set_not_visible=stopped ;d.state.set=1 ;scheduler.simple.added=\"\n"
       "view.event_removed = started,\"view.set_visible=stopped ;scheduler.simple.removed=\"\n"

       "view.add = stopped\n"
       "view.filter = stopped,false=\n"
       "view.event_added   = stopped,\"view.set_not_visible=started ;d.state.set=0\"\n"
       "view.event_removed = stopped,view.set_visible=started\n"

       "view.add = complete\n"
       "view.filter = complete,d.complete=\n"
       "view.filter_on    = complete,event.download.hash_done,event.download.hash_failed,event.download.hash_final_failed,event.download.finished\n"
       "view.sort_new     = complete,less=d.state_changed=\n"
       "view.sort_current = complete,less=d.state_changed=\n"

       "view.add = incomplete\n"
       "view.filter = incomplete,not=$d.complete=\n"
       "view.filter_on    = incomplete,event.download.hash_done,event.download.hash_failed,"
       "event.download.hash_final_failed,event.download.finished\n"
       "view.sort_new     = incomplete,less=d.state_changed=\n"
       "view.sort_current = incomplete,less=d.state_changed=\n"

       // The hashing view does not include stopped torrents.
       "view.add = hashing\n"
       "view.filter = hashing,d.hashing=\n"
       "view.filter_on = hashing,event.download.hash_queued,event.download.hash_removed,"
       "event.download.hash_done,event.download.hash_failed,event.download.hash_final_failed\n"
//        "view.sort_new     = hashing,less=d.state_changed=\n"
//        "view.sort_current = hashing,less=d.state_changed=\n"

       "view.add = seeding\n"
       "view.filter = seeding,\"and=d.state=,d.complete=\"\n"
       "view.filter_on    = seeding,event.download.resumed,event.download.paused,event.download.finished\n"
       "view.sort_new     = seeding,less=d.state_changed=\n"
       "view.sort_current = seeding,less=d.state_changed=\n"

       "schedule = view.main,10,10,\"view.sort=main,20\"\n"
       "schedule = view.name,10,10,\"view.sort=name,20\"\n"

       "schedule = session_save,1200,1200,session_save=\n"
       "schedule = low_diskspace,5,60,close_low_diskspace=500M\n"
       "schedule = prune_file_status,3600,86400,system.file_status_cache.prune=\n"

       "encryption=allow_incoming,prefer_plaintext,enable_retry\n"
    );

    // Deprecated commands. Don't use these anymore.

    if (!OptionParser::has_flag('D', argc, argv)) {
      // Deprecated in 0.7.0:
      // 
      // List of cleaned up files:
      // * command_download.cc
      // * command_dynamic.cc
      // * command_events.cc
      // * command_file.cc
      // * command_helpers.cc
      // + command_local.cc
      // * command_network.cc
      // * command_object.cc
      // * command_peer.cc
      // * command_scheduler.cc
      // * command_tracker.cc
      // * command_ui.cc

#define CMD2_REDIRECT(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_public_xmlrpc);

#define CMD2_REDIRECT_GENERIC(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_public_xmlrpc | rpc::CommandMap::flag_no_target);

      CMD2_REDIRECT_GENERIC("system.method.insert", "method.insert");
      CMD2_REDIRECT_GENERIC("system.method.erase", "method.erase");
      CMD2_REDIRECT_GENERIC("system.method.get", "method.get");
      CMD2_REDIRECT_GENERIC("system.method.set", "method.set");
      CMD2_REDIRECT_GENERIC("system.method.list_keys", "method.list_keys");
      CMD2_REDIRECT_GENERIC("system.method.has_key", "method.has_key");
      CMD2_REDIRECT_GENERIC("system.method.set_key", "method.set_key");

      CMD2_REDIRECT        ("get_handshake_log", "log.handshake");
      CMD2_REDIRECT        ("set_handshake_log", "log.handshake.set");
      CMD2_REDIRECT        ("get_log.tracker", "log.tracker");
      CMD2_REDIRECT        ("set_log.tracker", "log.tracker.set");

      CMD2_REDIRECT        ("get_name", "system.session_name");
      CMD2_REDIRECT        ("set_name", "system.session_name.set");

      CMD2_REDIRECT        ("system.file_allocate", "system.file.allocate");
      CMD2_REDIRECT        ("system.file_allocate.set", "system.file.allocate.set");
      CMD2_REDIRECT        ("get_max_file_size", "system.file.max_size");
      CMD2_REDIRECT        ("set_max_file_size", "system.file.max_size.set");
      CMD2_REDIRECT        ("get_split_file_size", "system.file.split_size");
      CMD2_REDIRECT        ("set_split_file_size", "system.file.split_size.set");
      CMD2_REDIRECT        ("get_split_suffix", "system.file.split_suffix");
      CMD2_REDIRECT        ("set_split_suffix", "system.file.split_suffix.set");

      CMD2_REDIRECT        ("get_timeout_sync", "pieces.sync.timeout");
      CMD2_REDIRECT        ("set_timeout_sync", "pieces.sync.timeout.set");
      CMD2_REDIRECT        ("get_timeout_safe_sync", "pieces.sync.timeout_safe");
      CMD2_REDIRECT        ("set_timeout_safe_sync", "pieces.sync.timeout_safe.set");

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

      CMD2_REDIRECT        ("get_memory_usage", "pieces.memory.current");
      CMD2_REDIRECT_GENERIC("get_max_memory_usage", "pieces.memory.max");
      CMD2_REDIRECT_GENERIC("set_max_memory_usage", "pieces.memory.max.set");

      CMD2_REDIRECT        ("get_send_buffer_size", "network.send_buffer.size");
      CMD2_REDIRECT        ("set_send_buffer_size", "network.send_buffer.size.set");
      CMD2_REDIRECT        ("get_receive_buffer_size", "network.receive_buffer.size");
      CMD2_REDIRECT        ("set_receive_buffer_size", "network.receive_buffer.size.set");

      CMD2_REDIRECT        ("get_up_rate", "throttle.global_up.rate");
      CMD2_REDIRECT        ("get_up_total", "throttle.global_up.total");
      CMD2_REDIRECT        ("get_upload_rate", "throttle.global_up.max_rate");
      CMD2_REDIRECT        ("set_upload_rate", "throttle.global_up.max_rate.set");
      CMD2_REDIRECT        ("get_down_rate", "throttle.global_down.rate");
      CMD2_REDIRECT        ("get_down_total", "throttle.global_down.total");
      CMD2_REDIRECT        ("get_download_rate", "throttle.global_down.max_rate");
      CMD2_REDIRECT        ("set_download_rate", "throttle.global_down.max_rate.set");

      CMD2_REDIRECT        ("bind", "network.bind_address.set");
      CMD2_REDIRECT        ("set_bind", "network.bind_address.set");
      CMD2_REDIRECT        ("get_bind", "network.bind_address");

      CMD2_REDIRECT        ("ip", "network.local_address.set");
      CMD2_REDIRECT        ("set_ip", "network.local_address.set");
      CMD2_REDIRECT        ("get_ip", "network.local_address");

      CMD2_REDIRECT        ("port_range", "network.port_range.set");
      CMD2_REDIRECT        ("get_port_range", "network.port_range");
      CMD2_REDIRECT        ("set_port_range", "network.port_range.set");

      CMD2_REDIRECT        ("port_random", "network.port_random.set");
      CMD2_REDIRECT        ("get_port_random", "network.port_random");
      CMD2_REDIRECT        ("set_port_random", "network.port_random.set");

      CMD2_REDIRECT        ("port_open", "network.port_open.set");
      CMD2_REDIRECT        ("get_port_open", "network.port_open");
      CMD2_REDIRECT        ("set_port_open", "network.port_open.set");

      CMD2_REDIRECT        ("proxy_address", "network.proxy_address.set");
      CMD2_REDIRECT        ("set_proxy_address", "network.proxy_address.set");
      CMD2_REDIRECT        ("get_proxy_address", "network.proxy_address");

      CMD2_REDIRECT        ("scgi_port", "network.scgi.open_port");
      CMD2_REDIRECT        ("scgi_local", "network.scgi.open_local");

      CMD2_REDIRECT        ("scgi_dont_route", "network.scgi.dont_route.set");
      CMD2_REDIRECT        ("set_scgi_dont_route", "network.scgi.dont_route.set");
      CMD2_REDIRECT        ("get_scgi_dont_route", "network.scgi.dont_route");

      CMD2_REDIRECT        ("xmlrpc_dialect", "network.xmlrpc.size_limit.set");

      CMD2_REDIRECT        ("get_connection_leech", "connection_leech");
      CMD2_REDIRECT        ("set_connection_leech", "connection_leech.set");
      CMD2_REDIRECT        ("get_connection_seed", "connection_seed");
      CMD2_REDIRECT        ("set_connection_seed", "connection_seed.set");

      CMD2_REDIRECT        ("peer_exchange", "protocol.pex.set");
      CMD2_REDIRECT        ("get_peer_exchange", "protocol.pex");
      CMD2_REDIRECT        ("set_peer_exchange", "protocol.pex.set");

      CMD2_REDIRECT        ("dht", "dht.mode.set");
      CMD2_REDIRECT        ("dht_add_node", "dht.add_node");
      CMD2_REDIRECT        ("dht_statistics", "dht.statistics");
      CMD2_REDIRECT        ("get_dht_port", "dht.port");
      CMD2_REDIRECT        ("set_dht_port", "dht.port");
      CMD2_REDIRECT        ("get_dht_throttle", "dht.throttle.name");
      CMD2_REDIRECT        ("set_dht_throttle", "dht.throttle.name.set");

      CMD2_REDIRECT        ("directory", "directory.default.set");
      CMD2_REDIRECT        ("get_directory", "directory.default");
      CMD2_REDIRECT        ("set_directory", "directory.default.set");

      CMD2_REDIRECT        ("get_session_lock", "system.session.use_lock");
      CMD2_REDIRECT        ("set_session_lock", "system.session.use_lock.set");
      CMD2_REDIRECT        ("get_session_on_completion", "system.session.on_completion");
      CMD2_REDIRECT        ("set_session_on_completion", "system.session.on_completion.set");

      CMD2_REDIRECT        ("check_hash", "pieces.hash.on_completion.set");

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

      //
      // Tracker:
      //

      CMD2_REDIRECT        ("t.get_group", "t.group");
      CMD2_REDIRECT        ("t.get_id", "t.id");
      CMD2_REDIRECT        ("t.get_min_interval", "t.min_interval");
      CMD2_REDIRECT        ("t.get_normal_interval", "t.normal_interval");
      CMD2_REDIRECT        ("t.get_scrape_complete", "t.scrape_complete");
      CMD2_REDIRECT        ("t.get_scrape_downloaded", "t.scrape_downloaded");
      CMD2_REDIRECT        ("t.get_scrape_incomplete", "t.scrape_incomplete");
      CMD2_REDIRECT        ("t.get_scrape_time_last", "t.scrape_time_last");
      CMD2_REDIRECT        ("t.get_type", "t.type");
      CMD2_REDIRECT        ("t.get_url", "t.url");

      //
      // File:
      //

      CMD2_REDIRECT        ("f.get_completed_chunks", "f.completed_chunks");
      CMD2_REDIRECT        ("f.get_frozen_path", "f.frozen_path");
      CMD2_REDIRECT        ("f.get_last_touched", "f.last_touched");
      CMD2_REDIRECT        ("f.get_match_depth_next", "f.match_depth_next");
      CMD2_REDIRECT        ("f.get_match_depth_prev", "f.match_depth_prev");
      CMD2_REDIRECT        ("f.get_offset", "f.offset");
      CMD2_REDIRECT        ("f.get_path", "f.path");
      CMD2_REDIRECT        ("f.get_path_components", "f.path_components");
      CMD2_REDIRECT        ("f.get_path_depth", "f.path_depth");
      CMD2_REDIRECT        ("f.get_priority", "f.priority");
      CMD2_REDIRECT        ("f.get_range_first", "f.range_first");
      CMD2_REDIRECT        ("f.get_range_second", "f.range_second");
      CMD2_REDIRECT        ("f.get_size_bytes", "f.size_bytes");
      CMD2_REDIRECT        ("f.get_size_chunks", "f.size_chunks");
      CMD2_REDIRECT        ("f.set_priority", "f.priority.set");
      CMD2_REDIRECT        ("fi.get_filename_last", "fi.filename_last");

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

      CMD2_REDIRECT        ("view_add", "view.add");
      CMD2_REDIRECT        ("view_filter", "view.filter");
      CMD2_REDIRECT        ("view_filter_on", "view.filter_on");
      CMD2_REDIRECT        ("view_list", "view.list");
      CMD2_REDIRECT        ("view_set", "view.set");
      CMD2_REDIRECT        ("view_sort", "view.sort");
      CMD2_REDIRECT        ("view_sort_current", "view.sort_current");
      CMD2_REDIRECT        ("view_sort_new", "view.sort_new");

      // Rename these to avoid conflicts with old style.
      CMD2_REDIRECT_GENERIC("d.multicall", "d.multicall2");

      // Functions that might not get depracted as they are nice for
      // configuration files, and thus might do with just some
      // cleanup.
      CMD2_REDIRECT        ("upload_rate", "throttle.global_up.max_rate.set");
      CMD2_REDIRECT        ("download_rate", "throttle.global_down.max_rate.set");
    }

    if (OptionParser::has_flag('n', argc, argv))
      control->core()->push_log("Ignoring ~/.rtorrent.rc.");
    else
      rpc::parse_command_single(rpc::make_target(), "try_import = ~/.rtorrent.rc");

    int firstArg = parse_options(control, argc, argv);

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

    while (!control->is_shutdown_completed()) {
      if (control->is_shutdown_received())
        control->handle_shutdown();

      control->inc_tick();

      cachedTime = rak::timer::current();
      rak::priority_queue_perform(&taskScheduler, cachedTime);

      // Do shutdown check before poll, not after.
      main_thread->poll_manager()->poll(client_next_timeout(control));
    }

    control->core()->download_list()->session_save();
    control->cleanup();

  } catch (std::exception& e) {
    control->cleanup_exception();

    std::cout << "rtorrent: " << e.what() << std::endl;
    return -1;
  }

  delete control;
  delete worker_thread;
  delete main_thread;

  return 0;
}

void
do_panic(int signum) {
  // Use the default signal handler in the future to avoid infinit
  // loops.
  SignalHandler::set_default(signum);
  display::Canvas::cleanup();

  std::cout << "Caught " << SignalHandler::as_string(signum) << ", dumping stack:" << std::endl;
  
#ifdef USE_EXECINFO
  void* stackPtrs[20];

  // Print the stack and exit.
  int stackSize = backtrace(stackPtrs, 20);
  char** stackStrings = backtrace_symbols(stackPtrs, stackSize);

  for (int i = 0; i < stackSize; ++i)
    std::cout << i << ' ' << stackStrings[i] << std::endl;

#else
  std::cout << "Stack dump not enabled." << std::endl;
#endif
  
  if (signum == SIGBUS)
    std::cout << "A bus error probably means you ran out of diskspace." << std::endl;

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

  std::cout << "Report bugs to <jaris@ifi.uio.no>." << std::endl;

  exit(0);
}
