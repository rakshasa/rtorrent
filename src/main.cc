#include <string>
#include <stdexcept>
#include <iostream>

#include "display/window.h"
#include "display/canvas.h"
#include "display/manager.h"
#include "input/bindings.h"
#include "input/manager.h"

class WindowTest : public display::Window {
public:
  WindowTest(const std::string& str, bool reverse = false) :
    Window(new display::Canvas, true, 3), m_str(str), m_reverse(reverse) {}
  
  virtual void redraw() {
    if (m_canvas == NULL)
      return;

    if (m_reverse)
      m_canvas->set_background(A_REVERSE);
    else
      m_canvas->set_background(0);

    m_canvas->erase();
    m_canvas->print_border('|', '|', '-', '-', '+', '+', '+', '+');
    m_canvas->print(1, 1, "%s minHeight %i", m_str.c_str(), m_minHeight);
  }

  void reverse() {
    m_reverse = !m_reverse;
  }

  void inc_min() {
    m_minHeight++;
  }

  void dec_min() {
    if (m_minHeight)
      m_minHeight--;
  }

private:
  std::string m_str;

  bool m_reverse;
};  

int main(int argc, char** argv) {
  try {

  display::Canvas::init();
  display::Manager display;
  input::Manager inputManager;

  WindowTest window1("This is window 1");
  WindowTest window2("This is window 2");

  input::Bindings bindings1;
  input::Bindings bindings2;

  inputManager.push_back(&bindings1);
  inputManager.push_back(&bindings2);

  bindings1[KEY_LEFT] = sigc::mem_fun(window1, &WindowTest::reverse);
  bindings1[KEY_UP] = sigc::mem_fun(window1, &WindowTest::inc_min);
  bindings1[KEY_DOWN] = sigc::mem_fun(window1, &WindowTest::dec_min);
  bindings1[KEY_LEFT] = sigc::mem_fun(window1, &WindowTest::reverse);
  bindings2[KEY_LEFT] = sigc::mem_fun(window2, &WindowTest::reverse);
  bindings2[KEY_RIGHT] = sigc::mem_fun(window2, &WindowTest::reverse);

  display.push_back(&window1);
  display.push_back(&window2);

  display.adjust_layout();

  while (true) {
    display.adjust_layout();
    display.do_update();
    
    inputManager.pressed(getch());

    sleep(0);
  }

  display::Canvas::cleanup();

  } catch (std::exception& e) {
    display::Canvas::cleanup();

    std::cout << "Caught exception: \"" << e.what() << '"' << std::endl;
  }

  return 0;
}
