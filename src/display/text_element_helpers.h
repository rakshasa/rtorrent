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

#ifndef RTORRENT_DISPLAY_TEXT_ELEMENT_HELPERS_H
#define RTORRENT_DISPLAY_TEXT_ELEMENT_HELPERS_H

#include <rak/functional.h>
#include <rak/socket_address.h>
#include <torrent/bitfield.h>
#include <torrent/torrent.h>
#include <torrent/data/file_list_iterator.h>

#include "core/download.h"
#include "rpc/parse_commands.h"

#include "text_element_lambda.h"
#include "text_element_string.h"
#include "text_element_value.h"

namespace display { namespace helpers {

// All we're ever going to need:

inline TextElementCommand*
te_command(const char* command, int flags = TextElementCommand::flag_normal, int attributes = Attributes::a_invalid) {
  return new TextElementCommand(command, flags, attributes, TextElementCommand::extent_full);
}

// End All we're ever going to need.

typedef TextElementStringBase string_base;
typedef TextElementValueBase  value_base;

// String stuff:

inline TextElementStringBase*
te_string(const char* str) {
  return new TextElementCString(str);
}

// template <typename Object, typename Return>
// inline TextElementStringBase*
// te_string(Return (Object::*fptr)() const, const Object* object, int attributes = Attributes::a_invalid) {
//   return text_element_string_void(rak::make_mem_fun(object, fptr), attributes);
// }

template <typename Return, typename Arg1>
inline TextElementStringBase*
te_string(Return (*fptr)(Arg1), int flags = TextElementStringBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_string_slot(std::ptr_fun(fptr), flags, attributes);
}

template <typename Return>
inline TextElementStringBase*
te_string(Return (core::Download::*fptr)() const, int flags = TextElementStringBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_string_slot(std::mem_fun(fptr), flags, attributes);
}

template <typename Return>
inline TextElementStringBase*
te_string(Return (torrent::Download::*fptr)() const, int flags = TextElementStringBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_string_slot(rak::on(std::mem_fun(&core::Download::c_download), std::mem_fun(fptr)), flags, attributes);
}

template <typename Return>
inline TextElementStringBase*
te_string(Return (torrent::FileList::*fptr)() const, int flags = TextElementStringBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_string_slot(rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(fptr)), flags, attributes);
}

// TMP HACK
inline std::string  te_call_command_d_string(const char* key, core::Download* download) { return rpc::commands.call_command(key, torrent::Object(), rpc::make_target(download)).as_string(); }
inline int64_t      te_call_command_d_value(const char* key, core::Download* download) { return rpc::commands.call_command(key, torrent::Object(), rpc::make_target(download)).as_value(); }

inline TextElementStringBase*
te_variable_string(const char* variable, int flags = TextElementStringBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_string_slot(rak::bind1st(std::ptr_fun(&te_call_command_d_string), variable), flags, attributes);
}

// Value stuff:

template <typename Return>
inline TextElementValueBase*
te_value(Return (torrent::Download::*fptr)() const, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_value_slot(rak::on(std::mem_fun(&core::Download::c_download), std::mem_fun(fptr)), flags, attributes);
}

template <typename Return>
inline TextElementValueBase*
te_value(Return (torrent::FileList::*fptr)() const, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_value_slot(rak::on(std::mem_fun(&core::Download::c_file_list), std::mem_fun(fptr)), flags, attributes);
}

template <typename Return>
inline TextElementValueBase*
te_value(Return (torrent::File::*fptr)() const, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_value_slot(rak::on(std::mem_fun(&torrent::FileListIterator::file), std::mem_fun(fptr)), flags, attributes);
}

inline TextElementValueBase*
te_variable_value(const char* variable, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_value_slot(rak::bind1st(std::ptr_fun(&te_call_command_d_value), variable), flags, attributes);
}

template <typename Return>
inline TextElementValueBase*
te_value(Return (torrent::ChunkManager::*fptr)() const, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_value_void(rak::make_mem_fun(torrent::chunk_manager(), fptr), flags, attributes);
}

template <typename Return>
inline TextElementValueBase*
te_value(Return (torrent::ConnectionManager::*fptr)() const, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return text_element_value_void(rak::make_mem_fun(torrent::connection_manager(), fptr), flags, attributes);
}

// Various:

inline unsigned int
te_bitfield_percentage(const torrent::Bitfield* bitfield) {
  return bitfield != NULL ? (100 * bitfield->size_set()) / bitfield->size_bits() : 0;
}

inline std::string
te_address(const ::sockaddr* address) {
  return rak::socket_address::cast_from(address)->address_str();
}

inline unsigned int
te_port(const ::sockaddr* address) {
  return rak::socket_address::cast_from(address)->port();
}

// Lambda

template <typename Return>
inline TextElement*
te_branch(Return (*fptr)(), TextElement* branch1, TextElement* branch2) {
  return text_element_branch_void(std::mem_fun(fptr), branch1, branch2);
}

template <typename Return, typename Arg1>
inline TextElement*
te_branch(Return (*fptr)(Arg1), TextElement* branch1, TextElement* branch2) {
  return text_element_branch(std::mem_fun(fptr), branch1, branch2);
}

template <typename Return, typename Object>
inline TextElement*
te_branch(Return (Object::*fptr)() const, TextElement* branch1, TextElement* branch2) {
  return text_element_branch(std::mem_fun(fptr), branch1, branch2);
}

template <typename Return, typename Object>
inline TextElement*
te_branch(Return (Object::*fptr)() const, const Object* object, TextElement* branch1, TextElement* branch2) {
  return text_element_branch_void(rak::make_mem_fun(object, fptr), branch1, branch2);
}

template <typename Return1, typename Return2, typename Object1, typename Object2>
inline TextElement*
te_branch(Return1 (Object1::*fptr1)() const, Return2 (Object2::*fptr2)() const, TextElement* branch1, TextElement* branch2) {
  return text_element_branch(rak::on(std::mem_fun(fptr1), std::mem_fun(fptr2)), branch1, branch2);
}

} }

#endif
