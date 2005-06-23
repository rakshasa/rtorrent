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

#ifdef USE_EXECINFO
#include <execinfo.h>
#endif

#include "core/download.h"
#include "display/canvas.h"
#include "display/window.h"
#include "ui/control.h"
#include "ui/root.h"
#include "input/bindings.h"

#include "utils/task.h"
#include "utils/timer.h"
#include "utils/directory.h"

#include "signal_handler.h"
#include "option_file.h"
#include "option_handler.h"
#include "option_handler_rules.h"
#include "option_parser.h"

int64_t utils::Timer::m_cache;

bool is_shutting_down = false;

void do_panic(int signum);
void print_help();

namespace utils {
  TaskScheduler taskScheduler;
}

bool
is_resized() {
  static int x = 0;
  static int y = 0;
  
  bool r = display::Canvas::get_screen_width() != x || display::Canvas::get_screen_height() != y;

  x = display::Canvas::get_screen_width();
  y = display::Canvas::get_screen_height();

  return r;
}

void
do_shutdown(ui::Control* c) {
  if (!is_shutting_down) {
    is_shutting_down = true;

    torrent::listen_close();

    std::for_each(c->get_core().get_download_list().begin(), c->get_core().get_download_list().end(),
		  std::bind1st(std::mem_fun(&core::Manager::stop), &c->get_core()));

  } else {
    // Close all torrents, this will stop all tracker connections and cause
    // a quick shutdown.
    std::for_each(c->get_core().get_download_list().begin(), c->get_core().get_download_list().end(),
		  std::mem_fun(&core::Download::call<void, &torrent::Download::close>));
  }
}

int
parse_options(ui::Control* c, OptionHandler* optionHandler, int argc, char** argv) {
  OptionParser optionParser;

  // Converted.
  optionParser.insert_flag('h', sigc::ptr_fun(&print_help));

  optionParser.insert_option('b', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "bind"));
  optionParser.insert_option('d', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "directory"));
  optionParser.insert_option('i', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "ip"));
  optionParser.insert_option('p', sigc::bind<0>(sigc::mem_fun(*optionHandler, &OptionHandler::process), "port"));
  optionParser.insert_option('s', sigc::mem_fun(c->get_core().get_download_store(), &core::DownloadStore::activate));

  optionParser.insert_option_list('o', sigc::mem_fun(*optionHandler, &OptionHandler::process));

  return optionParser.process(argc, argv);
}

void
initialize_option_handler(ui::Control* c, OptionHandler* optionHandler) {
  optionHandler->insert("max_peers",       new OptionHandlerInt(c, &apply_download_max_peers, &validate_download_peers));
  optionHandler->insert("max_uploads",     new OptionHandlerInt(c, &apply_download_max_uploads, &validate_download_peers));
  optionHandler->insert("min_peers",       new OptionHandlerInt(c, &apply_download_min_peers, &validate_download_peers));

  optionHandler->insert("download_rate",   new OptionHandlerInt(c, &apply_global_download_rate, &validate_rate));
  optionHandler->insert("hash_read_ahead", new OptionHandlerInt(c, &apply_hash_read_ahead, &validate_read_ahead));
  optionHandler->insert("max_open_files",  new OptionHandlerInt(c, &apply_max_open_files, &validate_fd));
  optionHandler->insert("upload_rate",     new OptionHandlerInt(c, &apply_global_upload_rate, &validate_rate));

  optionHandler->insert("check_hash",      new OptionHandlerString(c, &apply_check_hash, &validate_yes_no));
  optionHandler->insert("directory",       new OptionHandlerString(c, &apply_download_directory, &validate_directory));
  optionHandler->insert("tracker_dump",    new OptionHandlerString(c, &apply_tracker_dump, &validate_yes_no));

  optionHandler->insert("bind",            new OptionHandlerString(c, &apply_bind, &validate_ip));
  optionHandler->insert("ip",              new OptionHandlerString(c, &apply_ip, &validate_ip));
  optionHandler->insert("port",            new OptionHandlerString(c, &apply_port_range, &validate_port_range));
}

void
load_option_file(const std::string& filename, OptionHandler* optionHandler, bool require = false) {
  std::fstream f(filename.c_str(), std::ios::in);

  if (!f.is_open()) {
    std::cout << "Could not open option file \"" << filename << "\"" << std::endl;
    return;
  }

  OptionFile optionFile;

  optionFile.slot_option(sigc::mem_fun(*optionHandler, &OptionHandler::process));
  optionFile.process(&f);
}

void
load_session_torrents(ui::Control* c) {
  // Load session torrents.
  std::list<std::string> l = c->get_core().get_download_store().get_formated_entries().make_list();

  std::for_each(l.begin(), l.end(), std::bind1st(std::mem_fun(&core::Manager::insert), &c->get_core()));
}

void
load_arg_torrents(ui::Control* c, char** begin, char** end) {
  std::for_each(begin, end, std::bind1st(std::mem_fun(&core::Manager::insert), &c->get_core()));
}

void
initialize_display(ui::Control* c) {
  display::Canvas::init();
  display::Window::slot_adjust(sigc::mem_fun(c->get_display(), &display::Manager::adjust_layout));
}

void
initialize_core(ui::Control* c) {
  c->get_core().get_poll().slot_read_stdin(sigc::mem_fun(c->get_input(), &input::Manager::pressed));
  c->get_core().get_poll().slot_select_interrupted(sigc::ptr_fun(display::Canvas::do_update));

  c->get_core().initialize();
}

int
main(int argc, char** argv) {
  OptionHandler optionHandler;
  ui::Control   uiControl;
  ui::Root      uiRoot(&uiControl);

  utils::Timer::update();

  srandom(utils::Timer::cache().usec());
  srand48(utils::Timer::cache().usec());

  initialize_option_handler(&uiControl, &optionHandler);

  try {

    SignalHandler::set_ignore(SIGPIPE);
    SignalHandler::set_handler(SIGINT, sigc::bind(sigc::mem_fun(uiRoot, &ui::Root::set_shutdown_received), true));
    SignalHandler::set_handler(SIGSEGV, sigc::bind(sigc::ptr_fun(&do_panic), SIGSEGV));
    SignalHandler::set_handler(SIGBUS, sigc::bind(sigc::ptr_fun(&do_panic), SIGBUS));
    SignalHandler::set_handler(SIGFPE, sigc::bind(sigc::ptr_fun(&do_panic), SIGFPE));

    // Need to initialize this before parseing options.
    torrent::initialize();

    if (getenv("HOME"))
      load_option_file(getenv("HOME") + std::string("/.rtorrent.rc"), &optionHandler);

    int firstArg = parse_options(&uiControl, &optionHandler, argc, argv);

    initialize_display(&uiControl);
    initialize_core(&uiControl);

    uiRoot.init();

    load_session_torrents(&uiControl);
    load_arg_torrents(&uiControl, argv + firstArg, argv + argc);

    uiControl.get_display().adjust_layout();

    while (!is_shutting_down || !torrent::is_inactive()) {

      if (uiRoot.get_shutdown_received()) {
	do_shutdown(&uiControl);
	uiRoot.set_shutdown_received(false);
      }

      utils::Timer::update();
      utils::taskScheduler.execute(utils::Timer::cache());
    
      // This needs to be called every second or so. Currently done by
      // the throttle task in libtorrent.
      uiControl.get_display().do_update();

      uiControl.get_core().get_poll().poll(!utils::taskScheduler.empty() ?
					   utils::taskScheduler.get_next_timeout() - utils::Timer::cache() :
					   60 * 1000000);
    }

    uiRoot.cleanup();
    uiControl.get_core().cleanup();

    display::Canvas::erase_std();
    display::Canvas::refresh_std();
    display::Canvas::do_update();
    display::Canvas::cleanup();

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception: \"" << e.what() << '"' << std::endl;
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
    std::cout << "A bus error might mean you ran out of diskspace." << std::endl;
  
  exit(-1);
}

void
receive_tracker_dump(std::istream* s) {
  std::stringstream filename;
  filename << "./tracker_dump." << utils::Timer::current().sec();

  std::fstream out(filename.str().c_str(), std::ios::out | std::ios::trunc);

  if (!out.is_open())
    return;
  
  s->seekg(0);

  std::copy(std::istream_iterator<char>(*s), std::istream_iterator<char>(),
	    std::ostream_iterator<char>(out));
}

void
print_help() {
  std::cout << "Rakshasa's BitTorrent client " VERSION "." << std::endl;
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
