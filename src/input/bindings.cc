#include "config.h"

#include <algorithm>

#include "bindings.h"

namespace input {

bool
Bindings::pressed(int key) {
  const_iterator itr = find(key);

  if (itr != end())
    itr->second();

  return itr != end();
}

}
