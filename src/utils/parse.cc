// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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

#include "config.h"

#include <locale>
#include <torrent/exceptions.h>

#include "parse.h"

namespace utils {

const char*
parse_skip_wspace(const char* first, const char* last) {
  while (first != last && std::iswspace(*first))
    first++;

  return first;
}

const char*
parse_string(const char* first, const char* last, std::string* dest) {
  if (first == last)
    return first;

  bool quoted = parse_is_quote(*first);

  if (quoted)
    first++;

  while (first != last) {
    if (quoted) {
      if (parse_is_quote(*first))
        return ++first;

    } else {
      if (parse_is_seperator(*first) || std::iswspace(*first))
        return first;
    }
        
    if (parse_is_escape(*first))
      if (++first == last)
        throw torrent::input_error("Escape character at end of input.");

    dest->push_back(*first);
    first++;
  }
  
  if (quoted)
    throw torrent::input_error("Missing closing quote.");

  return first;
}

const char*
parse_whole_string(const char* first, const char* last, std::string* dest) {
  first = parse_skip_wspace(first, last);
  first = parse_string(first, last, dest);
  first = parse_skip_wspace(first, last);
   
  if (first != last)
    throw torrent::input_error("Junk at end of input.");

  return first;
}

const char*
parse_value(const char* src, int64_t* value, int base, int unit) {
  if (unit <= 0)
    throw torrent::input_error("Variable::string_to_value_unit(...) received unit <= 0.");

  char* last;
  *value = strtoll(src, &last, base);

  if (last == src) {
    if (strcasecmp(src, "no") == 0) { *value = 0; return src + strlen("no"); }
    if (strcasecmp(src, "yes") == 0) { *value = 1; return src + strlen("yes"); }
    if (strcasecmp(src, "true") == 0) { *value = 1; return src + strlen("true"); }
    if (strcasecmp(src, "false") == 0) { *value = 0; return src + strlen("false"); }

    throw torrent::input_error("Could not convert string to value.");
  }

  switch (*last) {
  case 'b':
  case 'B': ++last; break;
  case 'k':
  case 'K': *value = *value << 10; ++last; break;
  case 'm':
  case 'M': *value = *value << 20; ++last; break;
  case 'g':
  case 'G': *value = *value << 30; ++last; break;
//   case ' ':
//   case '\0': *value = *value * unit; break;
//   default: throw torrent::input_error("Could not parse value.");
  default: *value = *value * unit; break;
  }

  return last;
}

const char*
parse_list(const char* first, const char* last, torrent::Object* dest) {
  if (!dest->is_list())
    throw torrent::internal_error("parse_list(...) !dest->is_list().");

  while (true) {
    std::string str;

    first = parse_skip_wspace(first, last);
    first = parse_string(first, last, &str);
    first = parse_skip_wspace(first, last);

    dest->as_list().push_back(str);
    
    if (first == last || !parse_is_seperator(*first))
      break;

    first++;
  }

  return first;
}

const char*
parse_whole_list(const char* first, const char* last, torrent::Object* dest) {
  std::string str;

  first = parse_skip_wspace(first, last);
  first = parse_string(first, last, &str);
  first = parse_skip_wspace(first, last);

  if (first != last && parse_is_seperator(*first)) {
    *dest = torrent::Object(torrent::Object::TYPE_LIST);

    dest->as_list().push_back(str);
    first = parse_list(++first, last, dest);

  } else {
    *dest = str;
  }

  if (first != last)
    throw torrent::input_error("Junk at end of input.");

  return first;
}

std::string
convert_list_to_string(const torrent::Object& src) {
  if (!src.is_list())
    throw torrent::internal_error("convert_list_to_string(...) !src->is_list().");

  std::string dest;

  for (torrent::Object::list_type::const_iterator itr = src.as_list().begin(), last = src.as_list().end(); itr != last; itr++) {
    if (!itr->is_string())
      throw torrent::input_error("Could not convert non-string list element to string.");

    // Meh.
    if (!dest.empty())
      dest += ",";

    dest += itr->as_string();
  }

  return dest;
}

int64_t
convert_to_value(const torrent::Object& src, int base, int unit) {
  const torrent::Object& unpacked = (src.is_list() && src.as_list().size() == 1) ? src.as_list().front() : src;

  switch (unpacked.type()) {
  case torrent::Object::TYPE_VALUE:
    return unpacked.as_value();

  case torrent::Object::TYPE_STRING:
    int64_t tmp;

    if (parse_skip_wspace(parse_value(unpacked.as_string().c_str(), &tmp, base, unit),
                          unpacked.as_string().c_str() + unpacked.as_string().size()) != unpacked.as_string().c_str() + unpacked.as_string().size())
      throw torrent::input_error("Junk at end of value.");

    return tmp;

  case torrent::Object::TYPE_NONE:
    return 0;
  default:
    throw torrent::input_error("Not convertible to a value.");
  }
}

}
