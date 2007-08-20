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

#ifndef RTORRENT_RPC_PARSE_H
#define RTORRENT_RPC_PARSE_H

#include <string>
#include <torrent/object.h>

namespace rpc {

// parse_* functions do the bare minimum necessary to parse what was
// asked for. If a whitespace is found, it will be treated as empty
// input rather than skipped.
//
// parse_whole_* functions allow for whitespaces and throw an
// exception if there is any garbage at the end of the input.

inline bool parse_is_quote(const char c)     { return c == '"'; }
inline bool parse_is_escape(const char c)    { return c == '\\'; }
inline bool parse_is_seperator(const char c) { return c == ','; }
inline bool parse_is_space(const char c)     { return c == ' ' || c == '\t'; }

inline bool parse_is_delim_default(const char c) { return parse_is_seperator(c) || std::isspace(c); }
inline bool parse_is_delim_list(const char c)    { return parse_is_seperator(c) || c == '}' || std::isspace(c); }

const char* parse_skip_wspace(const char* first);
const char* parse_skip_wspace(const char* first, const char* last);

const char* parse_string(const char* first, const char* last, std::string* dest, bool (*delim)(const char) = &parse_is_delim_default);
void        parse_whole_string(const char* first, const char* last, std::string* dest);

const char* parse_value(const char* src, int64_t* value, int base = 0, int unit = 1);
const char* parse_value_nothrow(const char* src, int64_t* value, int base = 0, int unit = 1);

void        parse_whole_value(const char* src, int64_t* value, int base = 0, int unit = 1);
bool        parse_whole_value_nothrow(const char* src, int64_t* value, int base = 0, int unit = 1);

const char* parse_object(const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char) = &parse_is_delim_default);
const char* parse_list(const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char) = &parse_is_delim_default);
const char* parse_whole_list(const char* first, const char* last, torrent::Object* dest);

std::string convert_list_to_string(const torrent::Object& src);
std::string convert_list_to_string(torrent::Object::list_type::const_iterator first, torrent::Object::list_type::const_iterator last);
std::string convert_list_to_command(torrent::Object::list_type::const_iterator first, torrent::Object::list_type::const_iterator last);

int64_t     convert_to_value(const torrent::Object& src, int base = 0, int unit = 1);
bool        convert_to_value_nothrow(const torrent::Object& src, int64_t* value, int base = 0, int unit = 1);

inline const torrent::Object&
convert_to_single_argument(const torrent::Object& args) {
  if (args.type() == torrent::Object::TYPE_LIST && args.as_list().size() == 1)
    return args.as_list().front();
  else
    return args;
}

static const int print_expand_tilde = 0x1;

char*       print_object(char* first, char* last, const torrent::Object* src, int flags);
void        print_object_std(std::string* dest, const torrent::Object* src, int flags);

}

#endif
