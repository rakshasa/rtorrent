#include "config.h"

#include "manager.h"
#include "canvas.h"

#include <functional>
#include <algorithm>

namespace display {

void
Manager::do_update() {
  std::for_each(rbegin(), rend(), std::mem_fun(&Canvas::refresh));

  Canvas::do_update();
}

}
