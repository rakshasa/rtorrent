#include "config.h"

#include <algorithm>
#include <functional>
#include <torrent/exceptions.h>

#include "manager.h"
#include "bindings.h"
#include "text_input.h"

namespace input {

void
Manager::erase(Bindings* b) {
  iterator itr = std::find(begin(), end(), b);

  if (itr == end())
    return;

  Base::erase(itr);

  if (std::find(begin(), end(), b) != end())
    throw torrent::internal_error("Manager::erase(...) found duplicate bindings.");
}

void
Manager::pressed(int key) {
  if (m_textInput != NULL)
    m_textInput->pressed(key);
  else
    [[maybe_unused]] auto result = std::find_if(rbegin(), rend(), [&key](Bindings* bind) { return bind->pressed(key); });
}

}
