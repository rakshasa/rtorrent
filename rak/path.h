// rak - Rakshasa's toolbox
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

// Various functions for manipulating file paths. Also consider making
// a directory iterator.

#ifndef RAK_PATH_H
#define RAK_PATH_H

#include <cstdlib>
#include <string>

namespace rak {

inline std::string
path_expand(const std::string& path) {
  if (path.empty() || path[0] != '~')
    return path;

  char* home = std::getenv("HOME");

  if (home == NULL)
    return path;
  
  return home + path.substr(1);
}

// Don't inline this...
//
// Same strlcpy as found in *bsd.
inline size_t
strlcpy(char *dest, const char *src, size_t size) {
  size_t n = size;
  const char* first = src;

  if (n != 0) {
    while (--n != 0)
      if ((*dest++ = *src++) == '\0')
        break;
  }

  if (n == 0) {
    if (size != 0)
      *dest = '\0';
    
    while (*src++)
      ;
  }

  return src - first - 1;
}

inline char*
path_expand(const char* src, char* first, char* last) {
  if (*src == '~') {
    char* home = std::getenv("HOME");

    if (home == NULL)
      return first;

    first += strlcpy(first, home, std::distance(first, last));

    if (first > last)
      return last;

    src++;
  }

  return std::min(first + strlcpy(first, src, std::distance(first, last)), last);
}

}

#endif
