#include "config.h"

#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <torrent/torrent.h>
#include <sigc++/bind.h>

#ifdef USE_EXECINFO
#include <execinfo.h>
#endif

#include "display/canvas.h"

#include "core/poll.h"
#include "core/curl_stack.h"
#include "core/download_list.h"

#include "ui/control.h"
#include "ui/download_list.h"

#include "input/bindings.h"

#include "timer.h"
#include "signal_handler.h"

int64_t Timer::m_cache;

bool start_shutdown = false;
bool is_shutting_down = false;

core::Poll poll;
core::DownloadList downloads;

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
do_shutdown() {
  if (is_shutting_down)
    // Be quick about it...
    return;

  is_shutting_down = true;

  torrent::listen_close();

  std::for_each(downloads.begin(), downloads.end(), std::mem_fun_ref(&core::Download::stop));
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

int
main(int argc, char** argv) {
  try {

  SignalHandler::set_handler(SIGINT, sigc::ptr_fun(&set_shutdown));
  SignalHandler::set_handler(SIGSEGV, sigc::bind(sigc::ptr_fun(&do_panic), SIGSEGV));
  SignalHandler::set_handler(SIGBUS, sigc::bind(sigc::ptr_fun(&do_panic), SIGBUS));

  display::Canvas::init();
  core::CurlStack::init();

  ui::Control uiControl;
  ui::DownloadList uiDownloadList(&downloads, &uiControl);

  uiDownloadList.activate();

  // Register main key events.
  input::Bindings inputMain;
  uiControl.get_input().push_back(&inputMain);

  inputMain[KEY_RESIZE] = sigc::mem_fun(uiControl.get_display(), &display::Manager::adjust_layout);

  poll.slot_read_stdin(sigc::mem_fun(uiControl.get_input(), &input::Manager::pressed));
  poll.register_http();

  poll.slot_select_interrupted(sigc::ptr_fun(display::Canvas::do_update));

  torrent::initialize();
  torrent::listen_open(6880, 6999);

  for (int i = 1; i < argc; ++i) {
    std::fstream f(argv[i], std::ios::in);

    core::DownloadList::iterator itr = downloads.insert(f);

    itr->open();
    itr->hash_check();
  }

  uiControl.get_display().adjust_layout();

  while (!is_shutting_down || !torrent::get(torrent::SHUTDOWN_DONE)) {
    if (start_shutdown && !is_shutting_down)
      do_shutdown();

    Timer::update();

    uiControl.get_display().do_update();

    poll.poll();
  }

  display::Canvas::cleanup();

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception: \"" << e.what() << '"' << std::endl;
  }

  torrent::cleanup();
  core::CurlStack::cleanup();

  return 0;
}
