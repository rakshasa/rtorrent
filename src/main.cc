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

#include "engine/poll.h"
#include "engine/download_list.h"

#include "input/bindings.h"
#include "input/manager.h"

#include "signal_handler.h"

bool start_shutdown = false;
bool is_shutting_down = false;

engine::Poll poll;
engine::DownloadList downloads;

display::Manager displayManager;
input::Manager inputManager;

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

  std::for_each(downloads.begin(), downloads.end(), std::mem_fun_ref(&engine::Download::stop));
}

int
main(int argc, char** argv) {
  try {

  display::Canvas::init();

  inputManager.push_back(new input::Bindings);

  poll.slot_read_stdin(sigc::mem_fun(inputManager, &input::Manager::pressed));
  poll.register_http();

  displayManager.push_back(new display::WindowTitle);
  displayManager.push_back(new display::WindowDownloadList(&downloads));

  torrent::initialize();
  torrent::listen_open(6880, 6999);

  for (int i = 1; i < argc; ++i) {
    std::fstream f(argv[i], std::ios::in);

    engine::DownloadList::iterator itr = downloads.create(f);

    itr->open();
    itr->hash_check();
  }

  SignalHandler::set_handler(SIGINT, sigc::ptr_fun(&set_shutdown));

  while (!is_shutting_down || !torrent::get(torrent::SHUTDOWN_DONE)) {
    if (start_shutdown && !is_shutting_down)
      do_shutdown();

    displayManager.adjust_layout();
    displayManager.do_update();

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
