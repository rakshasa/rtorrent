#include "config.h"

#include <algorithm>

#include "bindings.h"

namespace input {

bool
Bindings::pressed(int key) {
  if (!m_active)
    return false;

  const_iterator itr = find(key);

  if (itr != end())
    itr->second();

  return itr != end();
}

}
