#include "config.h"

#include <algorithm>
#include <functional>

#include "manager.h"
#include "bindings.h"

namespace input {

void
Manager::pressed(int key) {
  std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Bindings::pressed), key));
}

}
