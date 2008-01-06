// rTorrent - BitTorrent client
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

#include "config.h"

#include <locale>
#include <rak/path.h>
#include <torrent/exceptions.h>

#include "parse.h"

namespace rpc {

const char*
parse_skip_wspace(const char* first, const char* last) {
  while (first != last && parse_is_space(*first))
    first++;

  return first;
}

const char*
parse_skip_wspace(const char* first) {
  while (parse_is_space(*first))
    first++;

  return first;
}

const char*
parse_string(const char* first, const char* last, std::string* dest, bool (*delim)(const char)) {
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
      if (delim(*first))
        return first;
    }
        
    if (parse_is_escape(*first) && ++first == last)
      throw torrent::input_error("Escape character at end of input.");

    dest->push_back(*first);
    first++;
  }
  
  if (quoted)
    throw torrent::input_error("Missing closing quote.");

  return first;
}

void
parse_whole_string(const char* first, const char* last, std::string* dest) {
  first = parse_skip_wspace(first, last);
  first = parse_string(first, last, dest);
  first = parse_skip_wspace(first, last);
   
  if (first != last)
    throw torrent::input_error("Junk at end of input.");
}

const char*
parse_value(const char* src, int64_t* value, int base, int unit) {
  const char* last = parse_value_nothrow(src, value, base, unit);

  if (last == src)
    throw torrent::input_error("Could not convert string to value.");

  return last;
}

void
parse_whole_value(const char* src, int64_t* value, int base, int unit) {
  const char* last = parse_value_nothrow(src, value, base, unit);

  if (last == src || *parse_skip_wspace(last) != '\0')
    throw torrent::input_error("Could not convert string to value.");
}

bool
parse_whole_value_nothrow(const char* src, int64_t* value, int base, int unit) {
  const char* last = parse_value_nothrow(src, value, base, unit);

  if (last == src || *parse_skip_wspace(last) != '\0')
    return false;

  return true;
}

const char*
parse_value_nothrow(const char* src, int64_t* value, int base, int unit) {
  if (unit <= 0)
    throw torrent::input_error("Command::string_to_value_unit(...) received unit <= 0.");

  char* last;
  *value = strtoll(src, &last, base);

  if (last == src) {
    if (strcasecmp(src, "no") == 0) { *value = 0; return src + strlen("no"); }
    if (strcasecmp(src, "yes") == 0) { *value = 1; return src + strlen("yes"); }
    if (strcasecmp(src, "true") == 0) { *value = 1; return src + strlen("true"); }
    if (strcasecmp(src, "false") == 0) { *value = 0; return src + strlen("false"); }

    return src;
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

// Somewhat ugly...
const char*
parse_object(const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char)) {
  if (*first == '{') {
    *dest = torrent::Object::create_list();
    first = parse_list(first + 1, last, dest, &parse_is_delim_list);
    first = parse_skip_wspace(first, last);
    
    if (first == last || *first != '}')
      throw torrent::input_error("Could not find closing '}'.");

    return ++first;

  } else {
    *dest = std::string();

    return parse_string(first, last, &dest->as_string(), delim);
  }
}

const char*
parse_list(const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char)) {
  if (!dest->is_list())
    throw torrent::internal_error("parse_list(...) !dest->is_list().");

  while (true) {
    torrent::Object tmp;

    first = parse_skip_wspace(first, last);
    first = parse_object(first, last, &tmp, delim);
    first = parse_skip_wspace(first, last);

    dest->as_list().push_back(tmp);
    
    if (first == last || !parse_is_seperator(*first))
      break;

    first++;
  }

  return first;
}

const char*
parse_whole_list(const char* first, const char* last, torrent::Object* dest) {
  first = parse_skip_wspace(first, last);
  first = parse_object(first, last, dest);
  first = parse_skip_wspace(first, last);

  if (first != last && parse_is_seperator(*first)) {
    torrent::Object tmp = torrent::Object::create_list();
    tmp.swap(*dest);

    dest->as_list().push_back(tmp);
    first = parse_list(++first, last, dest);
  }

  return first;
}

std::string
convert_list_to_string(const torrent::Object& src) {
  if (!src.is_list())
    throw torrent::internal_error("convert_list_to_string(...) !src->is_list().");

  return convert_list_to_string(src.as_list().begin(), src.as_list().end());
}

std::string
convert_list_to_string(torrent::Object::list_const_iterator first,
                       torrent::Object::list_const_iterator last) {
  std::string dest;

  while (first != last) {
    if (!first->is_string())
      throw torrent::input_error("Could not convert non-string list element to string.");

    // Meh.
    if (!dest.empty())
      dest += ",\"";
    else
      dest += '"';

    std::string::size_type quoteItr = dest.size();
    dest += first->as_string();

    // Finding a quote inside the string should be relatively rare, so
    // use something that is fast in the general case and ignore the
    // cost of the unusual one.
    while (quoteItr != dest.size()) {
      if (dest[quoteItr] == '"' || dest[quoteItr] == '\\')
        dest.insert(quoteItr++, 1, '\\');

      quoteItr++;
    }

    dest += '"';
    first++;
  }

  return dest;
}

std::string
convert_list_to_command(torrent::Object::list_const_iterator first,
                        torrent::Object::list_const_iterator last) {
  if (first == last)
    throw torrent::input_error("Too few arguments.");

  std::string dest = (first++)->as_string();
  std::string::size_type quoteItr = dest.find('=');
  
  if (quoteItr == std::string::npos)
    throw torrent::input_error("Could not find '=' in command.");

  // We should only escape backslash, not quote here as the string
  // will start with the command name which isn't quoted.
  while ((quoteItr = dest.find('\\', quoteItr + 1)) != std::string::npos)
    dest.insert(quoteItr++, 1, '\\');

  while (first != last) {
    if (!first->is_string())
      throw torrent::input_error("Could not convert non-string list element to string.");

    dest += ",\"";

    std::string::size_type quoteItr = dest.size();
    dest += first->as_string();

    // Finding a quote inside the string should be relatively rare, so
    // use something that is fast in the general case and ignore the
    // cost of the unusual one.
    while (quoteItr != dest.size()) {
      if (dest[quoteItr] == '"' || dest[quoteItr] == '\\')
        dest.insert(quoteItr++, 1, '\\');

      quoteItr++;
    }

    dest += '"';
    first++;
  }

  return dest;
}

int64_t
convert_to_value(const torrent::Object& src, int base, int unit) {
  int64_t value;

  if (!convert_to_value_nothrow(src, &value, base, unit))
    throw torrent::input_error("Not convertible to a value.");

  return value;
}

bool
convert_to_value_nothrow(const torrent::Object& src, int64_t* value, int base, int unit) {
  const torrent::Object& unpacked = (src.is_list() && src.as_list().size() == 1) ? src.as_list().front() : src;

  switch (unpacked.type()) {
  case torrent::Object::TYPE_VALUE:
    *value = unpacked.as_value();
    break;

  case torrent::Object::TYPE_STRING:
    if (parse_skip_wspace(parse_value(unpacked.as_string().c_str(), value, base, unit),
                          unpacked.as_string().c_str() + unpacked.as_string().size()) != unpacked.as_string().c_str() + unpacked.as_string().size())
      return false;

    break;

  case torrent::Object::TYPE_NONE:
    *value = 0;
    break;

  default:
    return false;
  }
  
  return true;
}

char*
print_object(char* first, char* last, const torrent::Object* src, int flags) {
  switch (src->type()) {
  case torrent::Object::TYPE_STRING:
  {
    const std::string& str = src->as_string();

    if ((flags & print_expand_tilde) && *str.c_str() == '~') {
      return rak::path_expand(str.c_str(), first, last);

    } else {
      if (first == last)
        return first;

      size_t n = std::min<size_t>(str.size(), std::distance(first, last) - 1);

      std::memcpy(first, str.c_str(), n);
      *(first += n) = '\0';

      return first;
    }
  }
  case torrent::Object::TYPE_VALUE:
    return std::max(first + snprintf(first, std::distance(first, last), "%lli", src->as_value()), last);

  case torrent::Object::TYPE_LIST:
    for (torrent::Object::list_const_iterator itr = src->as_list().begin(), itrEnd = src->as_list().end(); itr != itrEnd; itr++) {
      first = print_object(first, last, &*itr, flags);

      // Don't expand tilde after the first element in the list.
      flags &= ~print_expand_tilde;
    }

    return first;

  default:
    throw torrent::input_error("Invalid type.");
  }
}

void
print_object_std(std::string* dest, const torrent::Object* src, int flags) {
  switch (src->type()) {
  case torrent::Object::TYPE_STRING:
  {
    const std::string& str = src->as_string();

    if ((flags & print_expand_tilde) && *str.c_str() == '~')
      *dest += rak::path_expand(str);
    else
      *dest += str;

    return;
  }
  case torrent::Object::TYPE_VALUE:
  {
    char buffer[64];
    snprintf(buffer, 64, "%lli", src->as_value());

    *dest += buffer;
    return;
  }
  case torrent::Object::TYPE_LIST:
    for (torrent::Object::list_const_iterator itr = src->as_list().begin(), itrEnd = src->as_list().end(); itr != itrEnd; itr++) {
      print_object_std(dest, &*itr, flags);

      // Don't expand tilde after the first element in the list.
      flags &= ~print_expand_tilde;
    }

    return;

  default:
    throw torrent::input_error("Invalid type.");
  }
}

}
