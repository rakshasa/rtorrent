#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <torrent/torrent.h>
#include <sigc++/bind.h>

#include "display/canvas.h"
#include "display/manager.h"
#include "display/window_title.h"
#include "display/window_download_list.h"

#include "core/poll.h"
#include "core/download_list.h"

#include "input/bindings.h"
#include "input/manager.h"
#include "ui/control.h"
#include "ui/download_list.h"

#include "signal_handler.h"

bool start_shutdown = false;
bool is_shutting_down = false;

core::Poll poll;
core::DownloadList downloads;

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

int
main(int argc, char** argv) {
  try {

  display::Canvas::init();

  ui::Control uiControl;
  ui::DownloadList uiDownloadList(&downloads, &uiControl);

  uiDownloadList.activate();

  poll.slot_read_stdin(sigc::mem_fun(uiControl.get_input(), &input::Manager::pressed));
  poll.register_http();

  torrent::initialize();
  torrent::listen_open(6880, 6999);

  for (int i = 1; i < argc; ++i) {
    std::fstream f(argv[i], std::ios::in);

    core::DownloadList::iterator itr = downloads.create(f);

    itr->open();
    itr->hash_check();
  }

  SignalHandler::set_handler(SIGINT, sigc::ptr_fun(&set_shutdown));

  while (!is_shutting_down || !torrent::get(torrent::SHUTDOWN_DONE)) {
    if (start_shutdown && !is_shutting_down)
      do_shutdown();

    uiControl.get_display().adjust_layout();
    uiControl.get_display().do_update();

    poll.poll();
    poll.work();
  }

  display::Canvas::cleanup();

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception: \"" << e.what() << '"' << std::endl;
  }

  torrent::cleanup();

  return 0;
}
