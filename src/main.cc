#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <torrent/torrent.h>

#include "display/canvas.h"
#include "display/manager.h"
#include "display/window_downloads.h"
#include "input/bindings.h"
#include "input/manager.h"

#include "poll.h"
#include "downloads.h"

int main(int argc, char** argv) {
  try {

  Poll poll;
  Downloads downloads;

  display::Canvas::init();
  display::Manager display;

  input::Manager inputManager;
  inputManager.push_back(new input::Bindings);

  poll.slot_read_stdin(sigc::mem_fun(inputManager, &input::Manager::pressed));

  display.push_back(new display::WindowDownloads(&downloads));

  torrent::initialize();
  torrent::listen_open(6880, 6999);

  for (int i = 1; i < argc; ++i) {
    std::fstream f(argv[i], std::ios::in);

    downloads.create(f);
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
