#include "config.h"

#include <algorithm>
#include <functional>
#include <stdexcept>

#include "manager.h"
#include "bindings.h"

namespace input {

void
Manager::erase(Bindings* b) {
  iterator itr = std::find(begin(), end(), b);

  if (itr == end())
    throw std::logic_error("Manager::erase(...) could not find binding");

  Base::erase(itr);
}

void
Manager::pressed(int key) {
  std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Bindings::pressed), key));
}

}
