#include "config.h"

#include <algorithm>
#include <functional>

#include "manager.h"
#include "bindings.h"

namespace input {

bool
Manager::pressed(int key) {
  return std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Bindings::pressed), key)) != end();
}

}
