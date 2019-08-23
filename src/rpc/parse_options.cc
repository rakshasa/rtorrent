// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include "parse_options.h"

#include <algorithm>
#include <locale>
#include <torrent/exceptions.h>

namespace rpc {

int
parse_option_flag(const std::string& option, parse_option_flag_type ftor) {
  auto first = option.begin();
  auto last = option.end();

  first = std::find_if(first, last, [](char c) { return !std::isspace(c, std::locale::classic()); });

  if (first == last)
    throw torrent::input_error(option);
  
  auto next = std::find_if(first, last, [](char c) { return !std::isalnum(c, std::locale::classic()) && c != '_'; });

  if (first == next)
    throw torrent::input_error(option);

  if (std::find_if(next, last, [](char c) { return !std::isspace(c, std::locale::classic()); }) != last)
    throw torrent::input_error(option);

  return ftor(std::string(first, next));
}

int
parse_option_flags(const std::string& option, parse_option_flag_type ftor, int flags) {
  auto first = option.begin();
  auto last = option.end();

  while (first != last) {
    first = std::find_if(first, last, [](char c) { return !std::isspace(c, std::locale::classic()); });

    if (first == last)
      break;

    auto next = std::find_if(first, last, [](char c) { return !std::isalnum(c, std::locale::classic()) && c != '_'; });

    if (first == next)
      throw torrent::input_error(option);

    int f = ftor(std::string(first, next));

    if (f < 0)
      flags &= f;
    else
      flags |= f;

    first = std::find_if(next, last, [](char c) { return !std::isspace(c, std::locale::classic()); });

    if (first == last)
      break;

    if (*first++ != '|' || first == last)
      throw torrent::input_error(option);
  }

  return flags;
}

void
parse_option_for_each(const std::string& option, parse_option_flag_type ftor) {
  auto first = option.begin();
  auto last = option.end();

  while (first != last) {
    first = std::find_if(first, last, [](char c) { return !std::isspace(c, std::locale::classic()); });

    if (first == last)
      break;

    auto next = std::find_if(first, last, [](char c) { return !std::isalnum(c, std::locale::classic()) && c != '_'; });

    if (first == next)
      throw torrent::input_error(option);

    ftor(std::string(first, next));

    first = std::find_if(next, last, [](char c) { return !std::isspace(c, std::locale::classic()); });

    if (first == last)
      break;

    if (*first++ != '|' || first == last)
      throw torrent::input_error(option);
  }
}

std::string
parse_option_print_vector(int flags, const std::vector<std::pair<const char*, int>>& flag_list) {
  std::string result;

  for (auto f : flag_list) {
    if (f.second < 0) {
      if ((flags & f.second) != flags)
        continue;
    } else {
      if ((flags & f.second) != f.second)
        continue;
    }

    if (!result.empty())
      result += '|';

    result += f.first;
  }

  return result;
}

std::string
parse_option_print_flags(unsigned int flags, parse_option_rflag_type ftor) {
  std::string result;

  for (int i = 1; flags != 0; i <<= 1) {
    if (!(flags & i))
      continue;

    if (!result.empty())
      result += '|';

    result += ftor(i);
    flags &= ~i;
  }

  return result;
}

}
