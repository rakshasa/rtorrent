#include "config.h"

#include <sstream>

#include "utils/parse.h"

namespace utils {

std::string
escape_string(const std::string& src) {
  std::stringstream stream;

  // TODO: Correct would be to save the state.
  stream << std::hex << std::uppercase;

  for (std::string::const_iterator itr = src.begin(); itr != src.end(); ++itr)
    if ((*itr >= 'A' && *itr <= 'Z') ||
	(*itr >= 'a' && *itr <= 'z') ||
	(*itr >= '0' && *itr <= '9') ||
	*itr == '-')
      stream << *itr;
    else
      stream << '%' << ((unsigned char)*itr >> 4) << ((unsigned char)*itr & 0xf);

  return stream.str();
}

}
