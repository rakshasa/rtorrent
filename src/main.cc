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
    optionParser.insert_option('p', sigc::bind<0>(sigc::ptr_fun(&rpc::call_command_set_string), "port_range.set"));
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

      rpc::commands.create_redirect("system.method.insert", "method.insert", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("system.method.set", "method.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("system.method.set_key", "method.set_key", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_handshake_log", "log.handshake", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_handshake_log", "log.handshake.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_log.tracker", "log.tracker", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_log.tracker", "log.tracker.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_name", "system.session_name", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_name", "system.session_name.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("system.file_allocate", "system.file.allocate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("system.file_allocate.set", "system.file.allocate.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_preload_type", "pieces.preload.type", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_preload_min_size", "pieces.preload.min_size", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_preload_required_rate", "pieces.preload.min_rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_preload_type", "pieces.preload.type.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_preload_min_size", "pieces.preload.min_size.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_preload_required_rate", "pieces.preload.min_rate.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_stats_preloaded", "pieces.stats_preloaded", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_stats_not_preloaded", "pieces.stats_not_preloaded", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_memory_usage", "pieces.memory.current", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_max_memory_usage", "pieces.memory.max", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_max_memory_usage", "pieces.memory.max.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_send_buffer_size", "network.send_buffer.size", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_send_buffer_size", "network.send_buffer.size.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_receive_buffer_size", "network.receive_buffer.size", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_receive_buffer_size", "network.receive_buffer.size.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_up_rate", "throttle.global_up.rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_up_total", "throttle.global_up.total", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_upload_rate", "throttle.global_up.max_rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_upload_rate", "throttle.global_up.max_rate.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_down_rate", "throttle.global_down.rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_down_total", "throttle.global_down.total", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_download_rate", "throttle.global_down.max_rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_download_rate", "throttle.global_down.max_rate.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("bind", "network.bind_address.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_bind", "network.bind_address.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_bind", "network.bind_address", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("ip", "network.local_address.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_ip", "network.local_address.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_ip", "network.local_address", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("proxy_address", "network.proxy_address.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_proxy_address", "network.proxy_address.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_proxy_address", "network.proxy_address", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("scgi_port", "network.scgi.open_port", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("scgi_local", "network.scgi.open_local", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("scgi_dont_route", "network.scgi.dont_route.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_scgi_dont_route", "network.scgi.dont_route.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_scgi_dont_route", "network.scgi.dont_route", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("xmlrpc_dialect", "network.xmlrpc.size_limit.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("get_connection_leech", "connection_leech", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_connection_leech", "connection_leech.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_connection_seed", "connection_seed", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_connection_seed", "connection_seed.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("dht", "dht.mode.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("dht_add_node", "dht.add_node", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("dht_statistics", "dht.statistics", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_dht_port", "dht.port", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_dht_port", "dht.port", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_dht_throttle", "dht.throttle.name", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_dht_throttle", "dht.throttle.name.set", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("directory", "directory.default.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("get_directory", "directory.default", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("set_directory", "directory.default.set", rpc::CommandMap::flag_public_xmlrpc);

      //
      // Download:
      //

      rpc::commands.create_redirect("d.get_hash", "d.hash", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_local_id", "d.local_id", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_local_id_html", "d.local_id_html", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_bitfield", "d.bitfield", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_base_path", "d.base_path", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("d.get_name", "d.name", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_creation_date", "d.creation_date", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("d.get_peer_exchange", "d.peer_exchange", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("d.get_up_rate", "d.up.rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_up_total", "d.up.total", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_down_rate", "d.down.rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_down_total", "d.down.total", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_skip_rate", "d.skip.rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_skip_total", "d.skip.total", rpc::CommandMap::flag_public_xmlrpc);

      //
      rpc::commands.create_redirect("d.get_bytes_done", "d.bytes_done", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_chunk_size", "d.chunk_size", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_chunks_hashed", "d.chunks_hashed", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_complete", "d.complete", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_completed_bytes", "d.completed_bytes", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_completed_chunks", "d.completed_chunks", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_connection_current", "d.connection_current", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_connection_leech", "d.connection_leech", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_connection_seed", "d.connection_seed", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom", "d.custom", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom1", "d.custom1", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom2", "d.custom2", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom3", "d.custom3", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom4", "d.custom4", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom5", "d.custom5", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_custom_throw", "d.custom_throw", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_directory", "d.directory", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_directory_base", "d.directory_base", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_free_diskspace", "d.free_diskspace", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_hashing", "d.hashing", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_hashing_failed", "d.hashing_failed", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_ignore_commands", "d.ignore_commands", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_left_bytes", "d.left_bytes", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_loaded_file", "d.loaded_file", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_max_file_size", "d.max_file_size", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_max_size_pex", "d.max_size_pex", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_message", "d.message", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_mode", "d.mode", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_peers_accounted", "d.peers_accounted", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_peers_complete", "d.peers_complete", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_peers_connected", "d.peers_connected", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_peers_max", "d.peers_max", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_peers_min", "d.peers_min", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_peers_not_connected", "d.peers_not_connected", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_priority", "d.priority", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_priority_str", "d.priority_str", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_ratio", "d.ratio", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_size_bytes", "d.size_bytes", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_size_chunks", "d.size_chunks", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_size_files", "d.size_files", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_size_pex", "d.size_pex", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_state", "d.state", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_state_changed", "d.state_changed", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_state_counter", "d.state_counter", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_throttle_name", "d.throttle_name", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_tied_to_file", "d.tied_to_file", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_tracker_focus", "d.tracker_focus", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_tracker_numwant", "d.tracker_numwant", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_tracker_size", "d.tracker_size", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.get_uploads_max", "d.uploads_max", rpc::CommandMap::flag_public_xmlrpc);

      rpc::commands.create_redirect("d.set_connection_current", "d.connection_current.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_custom", "d.custom.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_custom1", "d.custom1.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_custom2", "d.custom2.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_custom3", "d.custom3.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_custom4", "d.custom4.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_custom5", "d.custom5.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_directory", "d.directory.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_directory_base", "d.directory_base.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_hashing_failed", "d.hashing_failed.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_ignore_commands", "d.ignore_commands.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_max_file_size", "d.max_file_size.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_message", "d.message.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_peers_max", "d.peers_max.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_peers_min", "d.peers_min.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_priority", "d.priority.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_throttle_name", "d.throttle_name.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_tied_to_file", "d.tied_to_file.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_tracker_numwant", "d.tracker_numwant.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("d.set_uploads_max", "d.uploads_max.set", rpc::CommandMap::flag_public_xmlrpc);

      //
      // Tracker:
      //

      rpc::commands.create_redirect("t.get_group", "t.group", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_id", "t.id", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_min_interval", "t.min_interval", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_normal_interval", "t.normal_interval", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_scrape_complete", "t.scrape_complete", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_scrape_downloaded", "t.scrape_downloaded", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_scrape_incomplete", "t.scrape_incomplete", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_scrape_time_last", "t.scrape_time_last", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_type", "t.type", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("t.get_url", "t.url", rpc::CommandMap::flag_public_xmlrpc);

      //
      // Tracker:
      //

      rpc::commands.create_redirect("f.get_completed_chunks", "f.completed_chunks", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_frozen_path", "f.frozen_path", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_last_touched", "f.last_touched", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_match_depth_next", "f.match_depth_next", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_match_depth_prev", "f.match_depth_prev", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_offset", "f.offset", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_path", "f.path", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_path_components", "f.path_components", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_path_depth", "f.path_depth", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_priority", "f.priority", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_range_first", "f.range_first", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_range_second", "f.range_second", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_size_bytes", "f.size_bytes", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.get_size_chunks", "f.size_chunks", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("f.set_priority", "f.priority.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("fi.get_filename_last", "fi.filename_last", rpc::CommandMap::flag_public_xmlrpc);

      //
      // Peer:
      //

      rpc::commands.create_redirect("p.get_address", "p.address", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_client_version", "p.client_version", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_completed_percent", "p.completed_percent", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_down_rate", "p.down_rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_down_total", "p.down_total", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_id", "p.id", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_id_html", "p.id_html", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_options_str", "p.options_str", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_peer_rate", "p.peer_rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_peer_total", "p.peer_total", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_port", "p.port", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_up_rate", "p.up_rate", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("p.get_up_total", "p.up_total", rpc::CommandMap::flag_public_xmlrpc);

      //
      // View:
      //

      rpc::commands.create_redirect("view_add", "view.add", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_filter", "view.filter", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_filter_on", "view.filter_on", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_list", "view.list", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_set", "view.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_sort", "view.sort", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_sort_current", "view.sort_current", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("view_sort_new", "view.sort_new", rpc::CommandMap::flag_public_xmlrpc);

      // Functions that might not get depracted as they are nice for
      // configuration files, and thus might do with just some
      // cleanup.
      rpc::commands.create_redirect("upload_rate", "throttle.global_up.max_rate.set", rpc::CommandMap::flag_public_xmlrpc);
      rpc::commands.create_redirect("download_rate", "throttle.global_down.max_rate.set", rpc::CommandMap::flag_public_xmlrpc);
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
