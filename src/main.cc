#include "config.h"

#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <torrent/http.h>
#include <torrent/torrent.h>
#include <sigc++/bind.h>
#include <sigc++/retype_return.h>

#ifdef USE_EXECINFO
#include <execinfo.h>
#endif

#include "core/download.h"
#include "display/canvas.h"
#include "ui/control.h"
#include "ui/download_list.h"
#include "input/bindings.h"
#include "utils/timer.h"
#include "utils/directory.h"

#include "signal_handler.h"
#include "option_parser.h"

int64_t utils::Timer::m_cache;

bool start_shutdown = false;
bool is_shutting_down = false;

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

void
set_shutdown() {
  start_shutdown = true;
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
		  std::mem_fun(&core::Download::close));

  }

  start_shutdown = false;
}

void
test_dir(const std::string& s) {
  utils::Directory d(s);

  d.update();

  std::cout << "Listing dir \"" << s << '"' << std::endl;

  for (utils::Directory::iterator itr = d.begin(); itr != d.end(); ++itr)
    std::cout << '"' << *itr << '"' << std::endl;

  exit(0);
}

int
main(int argc, char** argv) {
  ui::Control uiControl;

  try {

  SignalHandler::set_handler(SIGINT, sigc::ptr_fun(&set_shutdown));
  SignalHandler::set_handler(SIGSEGV, sigc::bind(sigc::ptr_fun(&do_panic), SIGSEGV));
  SignalHandler::set_handler(SIGBUS, sigc::bind(sigc::ptr_fun(&do_panic), SIGBUS));

  OptionParser optionParser;
  optionParser.insert_flag('h', sigc::ptr_fun(&print_help));
  optionParser.insert_option('p', sigc::bind(sigc::ptr_fun(OptionParser::call_int_pair),
					     sigc::mem_fun(uiControl.get_core(), &core::Manager::set_port_range)));
  optionParser.insert_option('s', sigc::mem_fun(uiControl.get_core().get_download_store(), &core::DownloadStore::activate));

  int firstArg = optionParser.process(argc, argv);

  display::Canvas::init();

  ui::DownloadList uiDownloadList(&uiControl);

  uiDownloadList.activate();
  uiDownloadList.slot_open_uri(sigc::mem_fun(uiControl.get_core(), &core::Manager::insert));

  // Register main key events.
  input::Bindings inputMain;
  uiControl.get_input().push_back(&inputMain);

  inputMain[KEY_RESIZE] = sigc::mem_fun(uiControl.get_display(), &display::Manager::adjust_layout);
  inputMain['\x11'] = sigc::ptr_fun(&set_shutdown);

  uiControl.get_core().get_poll().slot_read_stdin(sigc::mem_fun(uiControl.get_input(), &input::Manager::pressed));
  uiControl.get_core().get_poll().slot_select_interrupted(sigc::ptr_fun(display::Canvas::do_update));

  uiControl.get_core().initialize();

  while (firstArg < argc)
    uiControl.get_core().insert(argv[firstArg++]);

  uiControl.get_display().adjust_layout();

  while (!is_shutting_down || !torrent::get(torrent::SHUTDOWN_DONE)) {
    if (start_shutdown)
      do_shutdown(&uiControl);

    utils::Timer::update();

    uiControl.get_display().do_update();

    uiControl.get_core().get_poll().poll();
  }

  display::Canvas::cleanup();

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception: \"" << e.what() << '"' << std::endl;
    return -1;
  }

  uiControl.get_core().cleanup();

  return 0;
}

void
do_panic(int signum) {
  display::Canvas::cleanup();

  std::cout << "Signal " << (signum == SIGSEGV ? "SIGSEGV" : "SIGBUS") << " recived, dumping stack:" << std::endl;
  
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
print_help() {
  std::cout << "Rakshasa's Torrent client. Neko-Mimi Mode-o!" << std::endl;
  std::cout << "Usage: rtorrent [OPTIONS]... [FILE]... [URL]..." << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                Display this very helpfull text" << std::endl;
  std::cout << "  -p <int>-<int>    Set port range for incoming connections" << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <jaris@ifi.uio.no>." << std::endl;

  exit(0);
}
