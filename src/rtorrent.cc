#include <string>

#include "display/window.h"
#include "display/canvas.h"
#include "display/manager.h"

class WindowTest : public display::Window {
public:
  WindowTest(const std::string& str) : m_str(str) {}
  
  virtual void redraw() {
    if (m_canvas == NULL)
      return;

    m_canvas->erase();
    m_canvas->print_border('|', '|', '-', '-', '+', '+', '+', '+');
    m_canvas->print(1, 1, "%s", m_str.c_str());
  }

private:
  std::string m_str;
};  

int main(int argc, char** argv) {
  display::Canvas::init();
  display::Manager manager;

  WindowTest window1("This is window 1");
  WindowTest window2("This is window 2");

  window1.set_canvas(new display::Canvas());
  window2.set_canvas(new display::Canvas());

  manager.add(&window1);
  manager.add(&window2);

  window1.get_canvas()->set_background(A_REVERSE);

  manager.adjust_layout();
  manager.do_update();

  while (true);

  display::Canvas::cleanup();

  delete window1.get_canvas();
  delete window2.get_canvas();

  return 0;
}
