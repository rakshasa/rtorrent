#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <torrent/torrent.h>
#include <torrent/http.h>
#include <sigc++/bind.h>

#include "display/canvas.h"
#include "display/manager.h"
#include "display/window_downloads.h"

#include "engine/poll.h"
#include "engine/curl_stack.h"
#include "engine/curl_get.h"
#include "engine/downloads.h"

#include "input/bindings.h"
#include "input/manager.h"

int main(int argc, char** argv) {
  try {

  engine::Poll poll;
  engine::Downloads downloads;
  engine::CurlStack curlStack;

  display::Canvas::init();
  display::Manager display;

  input::Manager inputManager;
  inputManager.push_back(new input::Bindings);

  poll.slot_read_stdin(sigc::mem_fun(inputManager, &input::Manager::pressed));

  display.push_back(new display::WindowDownloads(&downloads));

  torrent::initialize();
  torrent::listen_open(6880, 6999);

  torrent::Http::set_factory(sigc::bind(sigc::ptr_fun(&engine::CurlGet::new_object), &curlStack));

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
