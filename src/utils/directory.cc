#include "config.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <dirent.h>

#include "directory.h"

namespace utils {

void
Directory::update() {
  if (m_path.empty())
    throw std::logic_error("Directory::update() tried to open an empty path");

  DIR* d = opendir(m_path.c_str());

  if (d == NULL)
    throw std::runtime_error("Directory::update() could not open directory");

  struct dirent* ent;

  while ((ent = readdir(d)) != NULL) {
    std::string de(ent->d_name);

    if (de != "." && de != "..")
      Base::push_back(ent->d_name);
  }

  std::sort(begin(), end(), std::less<std::string>());
}

}
