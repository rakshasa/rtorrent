// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <sigc++/bind.h>
#include <sigc++/retype_return.h>
#include <torrent/http.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>
#include <rak/functional.h>

#ifdef USE_EXECINFO
#include <execinfo.h>
#endif

#include "core/download.h"
#include "core/download_factory.h"
#include "core/manager.h"
#include "display/canvas.h"
#include "display/window.h"
#include "display/manager.h"
#include "input/bindings.h"

#include "utils/directory.h"

#include "control.h"
#include "globals.h"
#include "signal_handler.h"
#include "option_file.h"
#include "option_handler.h"
#include "option_handler_rules.h"
#include "option_parser.h"

uint32_t countTicks = 0;

void do_panic(int signum);
void print_help();

bool
is_resized() {
  static int x = 0;
  static int y = 0;
  
  bool r = display::Canvas::get_screen_width() != x || display::Canvas::get_screen_height() != y;

  x = display::Canvas::get_screen_width();
  y = display::Canvas::get_screen_height();

  return r;
}

int
parse_options(Control* c, OptionHandler* optionHandler, int argc, char** argv) {
  try {
    OptionParser optionParser;

    // Converted.
    optionParser.insert_flag('h', sigc::ptr_fun(&print_help));

    optionParser.insert_option('b', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "bind"));
    optionParser.insert_option('d', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "directory"));
    optionParser.insert_option('i', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "ip"));
    optionParser.insert_option('p', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "port_range"));
    optionParser.insert_option('s', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "session"));

    optionParser.insert_option_list('o', sigc::mem_fun(*optionHandler, &OptionHandler::process));

    return optionParser.process(argc, argv);

  } catch (torrent::input_error& e) {
    throw std::runtime_error("Failed to parse command line option: " + std::string(e.what()));
  }
}

void
initialize_option_handler(Control* c, OptionHandler* optionHandler) {
  optionHandler->insert("max_peers",           new OptionHandlerInt(c, &apply_download_max_peers));
  optionHandler->insert("min_peers",           new OptionHandlerInt(c, &apply_download_min_peers));
  optionHandler->insert("max_uploads",         new OptionHandlerInt(c, &apply_download_max_uploads));

  optionHandler->insert("download_rate",       new OptionHandlerInt(c, &apply_global_download_rate));
  optionHandler->insert("upload_rate",         new OptionHandlerInt(c, &apply_global_upload_rate));

  optionHandler->insert("bind",                new OptionHandlerString(c, &apply_bind));
  optionHandler->insert("ip",                  new OptionHandlerString(c, &apply_ip));
  optionHandler->insert("port_range",          new OptionHandlerString(c, &apply_port_range));
  optionHandler->insert("port_random",         new OptionHandlerString(c, &apply_port_random));

  optionHandler->insert("check_hash",          new OptionHandlerString(c, &apply_check_hash));
  optionHandler->insert("directory",           new OptionHandlerString(c, &apply_download_directory));

  optionHandler->insert("hash_read_ahead",     new OptionHandlerInt(c, &apply_hash_read_ahead));
  optionHandler->insert("hash_interval",       new OptionHandlerInt(c, &apply_hash_interval));
  optionHandler->insert("hash_max_tries",      new OptionHandlerInt(c, &apply_hash_max_tries));
  optionHandler->insert("max_open_files",      new OptionHandlerInt(c, &apply_max_open_files));
  optionHandler->insert("max_open_sockets",    new OptionHandlerInt(c, &apply_max_open_sockets));

  optionHandler->insert("umask",               new OptionHandlerOctal(c, &apply_umask));

  optionHandler->insert("connection_leech",    new OptionHandlerString(c, &apply_connection_leech));
  optionHandler->insert("connection_seed",     new OptionHandlerString(c, &apply_connection_seed));

  optionHandler->insert("session",             new OptionHandlerString(c, &apply_session_directory));
  optionHandler->insert("encoding_list",       new OptionHandlerString(c, &apply_encoding_list));
  optionHandler->insert("tracker_dump",        new OptionHandlerString(c, &apply_tracker_dump));
  optionHandler->insert("use_udp_trackers",    new OptionHandlerString(c, &apply_use_udp_trackers));

  optionHandler->insert("http_proxy",          new OptionHandlerString(c, &apply_http_proxy));
}

void
load_session_torrents(Control* c) {
  // Load session torrents.
  std::list<std::string> l = c->core()->get_download_store().get_formated_entries().make_list();

  for (std::list<std::string>::iterator first = l.begin(), last = l.end(); first != last; ++first) {
    core::DownloadFactory* f = new core::DownloadFactory(*first, c->core());

    // Replace with session torrent flag.
    f->set_session(true);
    f->slot_finished(sigc::bind(sigc::ptr_fun(&rak::call_delete_func<core::DownloadFactory>), f));
    f->load();
    f->commit();
  }
}

void
load_arg_torrents(Control* c, char** first, char** last) {
  //std::for_each(begin, end, std::bind1st(std::mem_fun(&core::Manager::insert), &c->get_core()));
  for (; first != last; ++first) {
    core::DownloadFactory* f = new core::DownloadFactory(*first, c->core());

    // Replace with session torrent flag.
    f->set_start(true);
    f->slot_finished(sigc::bind(sigc::ptr_fun(&rak::call_delete_func<core::DownloadFactory>), f));
    f->load();
    f->commit();
  }
}

int
main(int argc, char** argv) {
  cachedTime = rak::timer::current();

  OptionHandler optionHandler;
  Control       uiControl;

  srandom(cachedTime.usec());
  srand48(cachedTime.usec());

  initialize_option_handler(&uiControl, &optionHandler);

  OptionFile optionFile;
  optionFile.slot_option(sigc::mem_fun(optionHandler, &OptionHandler::process));

  try {

    SignalHandler::set_ignore(SIGPIPE);
    SignalHandler::set_handler(SIGINT,  sigc::mem_fun(uiControl, &Control::receive_shutdown));
    SignalHandler::set_handler(SIGSEGV, sigc::bind(sigc::ptr_fun(&do_panic), SIGSEGV));
    SignalHandler::set_handler(SIGBUS,  sigc::bind(sigc::ptr_fun(&do_panic), SIGBUS));
    SignalHandler::set_handler(SIGFPE,  sigc::bind(sigc::ptr_fun(&do_panic), SIGFPE));

    uiControl.core()->initialize_first();

    if (getenv("HOME") && !optionFile.process_file(getenv("HOME") + std::string("/.rtorrent.rc")))
      uiControl.core()->get_log_important().push_front("Could not load \"~/.rtorrent.rc\".");

    int firstArg = parse_options(&uiControl, &optionHandler, argc, argv);

    uiControl.initialize();

    load_session_torrents(&uiControl);
    load_arg_torrents(&uiControl, argv + firstArg, argv + argc);

    uiControl.display()->adjust_layout();

    while (!uiControl.is_shutdown_completed()) {
      countTicks++;

      cachedTime = rak::timer::current();

      std::list<rak::priority_item*> workQueue;

      std::copy(rak::queue_popper(taskScheduler, rak::priority_ready(cachedTime)), rak::queue_popper(), std::back_inserter(workQueue));
      std::for_each(workQueue.begin(), workQueue.end(), std::mem_fun(&rak::priority_item::clear_time));
      std::for_each(workQueue.begin(), workQueue.end(), std::mem_fun(&rak::priority_item::call));

      // This needs to be called every second or so. Currently done by
      // the throttle task in libtorrent.
      if (!displayScheduler.empty() && displayScheduler.top()->time() <= cachedTime)
	uiControl.display()->do_update();

      // Do shutdown check before poll, not after.
      uiControl.core()->get_poll_manager()->poll(!taskScheduler.empty() ? taskScheduler.top()->time() - cachedTime : 60 * 1000000);
    }

    uiControl.cleanup();

  } catch (torrent::base_error& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception from libtorrent: " << e.what() << std::endl;
    return -1;

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << e.what() << std::endl;
    return -1;
  }

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
    std::cout << "A bus error propably means you ran out of diskspace." << std::endl;

  exit(-1);
}

void
receive_tracker_dump(std::istream* s) {
  std::stringstream filename;
  filename << "./tracker_dump." << rak::timer::current().seconds();

  std::fstream out(filename.str().c_str(), std::ios::out | std::ios::trunc);

  if (!out.is_open())
    return;
  
  s->seekg(0);

  std::copy(std::istream_iterator<char>(*s), std::istream_iterator<char>(),
	    std::ostream_iterator<char>(out));
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
