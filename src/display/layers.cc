#include "config.h"

#include "layers.h"
#include "canvas.h"

#include <functional>
#include <algorithm>

namespace display {

void
Layers::do_update() {
  std::for_each(rbegin(), rend(), std::mem_fun(&Canvas::refresh));

  Canvas::do_update();
}

}
