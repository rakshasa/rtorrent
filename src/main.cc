#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <torrent/torrent.h>
#include <sigc++/bind.h>

#include "display/canvas.h"
#include "display/manager.h"
#include "display/window_download_list.h"

#include "engine/poll.h"
#include "engine/download_list.h"

#include "input/bindings.h"
#include "input/manager.h"

int main(int argc, char** argv) {
  try {

  display::Canvas::init();

  engine::Poll poll;
  engine::DownloadList downloads;

  display::Manager display;

  input::Manager inputManager;

  inputManager.push_back(new input::Bindings);

  poll.slot_read_stdin(sigc::mem_fun(inputManager, &input::Manager::pressed));
  poll.register_http();

  display.push_back(new display::WindowDownloadList(&downloads));

  torrent::initialize();
  torrent::listen_open(6880, 6999);

  for (int i = 1; i < argc; ++i) {
    std::fstream f(argv[i], std::ios::in);

    engine::DownloadList::iterator itr = downloads.create(f);

    itr->open();
    itr->hash_check();
  }

  while (poll.is_running()) {
    display.adjust_layout();
    display.do_update();

    poll.poll();
    poll.work();
  }

  display::Canvas::cleanup();

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception: \"" << e.what() << '"' << std::endl;
  }

  return 0;
}
