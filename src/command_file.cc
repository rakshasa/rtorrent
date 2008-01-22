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

#include <rak/error_number.h>
#include <rak/path.h>
#include <torrent/data/file.h>
#include <torrent/data/file_list.h>
#include <torrent/data/file_list_iterator.h>

#include "core/manager.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

void
apply_f_set_priority(torrent::File* file, uint32_t value) {
  if (value > torrent::PRIORITY_HIGH)
    throw torrent::input_error("Invalid value.");

  file->set_priority((torrent::priority_t)value);
}

torrent::Object
apply_f_path(torrent::File* file) {
  if (file->path()->empty())
    return std::string();

  torrent::Object resultRaw(*file->path()->begin());
  torrent::Object::string_type& result = resultRaw.as_string();

  for (torrent::Path::const_iterator itr = ++file->path()->begin(), last = file->path()->end(); itr != last; itr++)
    result += '/' + *itr;

  return resultRaw;
}

torrent::Object
apply_f_path_components(torrent::File* file) {
  torrent::Object resultRaw = torrent::Object::create_list();
  torrent::Object::list_type& result = resultRaw.as_list();

  for (torrent::Path::const_iterator itr = file->path()->begin(), last = file->path()->end(); itr != last; itr++)
    result.push_back(*itr);

  return resultRaw;
}

torrent::Object
apply_f_path_depth(torrent::File* file) {
  return (int64_t)file->path()->size();
}

torrent::Object
apply_fi_filename_last(torrent::FileListIterator* itr) {
  if (itr->file()->path()->empty())
    return "EMPTY";

  if (itr->depth() >= itr->file()->path()->size())
    return "ERROR";

  return itr->file()->path()->at(itr->depth());
}

#define ADD_CF_SLOT(key, function, slot, parm, doc)    \
  commandFileSlotsItr->set_slot(slot); \
  rpc::commands.insert_file(key, commandFileSlotsItr++, &rpc::CommandSlot<torrent::File*>::function, rpc::CommandMap::flag_dont_delete, parm, doc);

#define ADD_CF_SLOT_PUBLIC(key, function, slot, parm, doc)    \
  commandFileSlotsItr->set_slot(slot); \
  rpc::commands.insert_file(key, commandFileSlotsItr++, &rpc::CommandSlot<torrent::File*>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_CF_VOID(key, slot) \
  ADD_CF_SLOT_PUBLIC("f." key, call_unknown, rpc::object_fn(slot), "i:", "")

#define ADD_CF_VALUE(key, get) \
  ADD_CF_SLOT_PUBLIC("f." key, call_unknown, rpc::object_void_fn<torrent::File*>(get), "i:", "")

#define ADD_CF_VALUE_UNI(key, get) \
  ADD_CF_SLOT_PUBLIC("f.get_" key, call_unknown, rpc::object_void_fn<torrent::File*>(get), "i:", "")

#define ADD_CF_VALUE_BI(key, set, get) \
  ADD_CF_SLOT_PUBLIC("f.set_" key, call_value,   rpc::object_value_fn<torrent::File*>(set), "i:i", "") \
  ADD_CF_SLOT_PUBLIC("f.get_" key, call_unknown, rpc::object_void_fn<torrent::File*>(get), "i:", "")

#define ADD_CF_STRING_UNI(key, get) \
  ADD_CF_SLOT_PUBLIC("f.get_" key, call_unknown, rpc::object_void_fn<torrent::File*>(get), "s:", "")

#define ADD_CFI_SLOT_PUBLIC(key, function, slot, parm, doc)    \
  commandFileItrSlotsItr->set_slot(slot); \
  rpc::commands.insert_file_itr(key, commandFileItrSlotsItr++, &rpc::CommandSlot<torrent::FileListIterator*>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_CFI_VOID(key, slot) \
  ADD_CFI_SLOT_PUBLIC("fi." key, call_unknown, rpc::object_fn(slot), "i:", "")

#define ADD_CFI_VALUE(key, get) \
  ADD_CFI_SLOT_PUBLIC("fi." key, call_unknown, rpc::object_void_fn<torrent::FileListIterator*>(get), "i:", "")

void
initialize_command_file() {
  ADD_CF_VALUE("is_created",             std::mem_fun(&torrent::File::is_created));
  ADD_CF_VALUE("is_open",                std::mem_fun(&torrent::File::is_open));

  ADD_CF_VALUE("is_create_queued",       std::mem_fun(&torrent::File::is_create_queued));
  ADD_CF_VALUE("is_resize_queued",       std::mem_fun(&torrent::File::is_resize_queued));

  ADD_CF_VALUE("set_create_queued",      std::bind2nd(std::mem_fun(&torrent::File::set_flags),   (int)torrent::File::flag_create_queued));
  ADD_CF_VALUE("set_resize_queued",      std::bind2nd(std::mem_fun(&torrent::File::set_flags),   (int)torrent::File::flag_resize_queued));
  ADD_CF_VALUE("unset_create_queued",    std::bind2nd(std::mem_fun(&torrent::File::unset_flags), (int)torrent::File::flag_create_queued));
  ADD_CF_VALUE("unset_resize_queued",    std::bind2nd(std::mem_fun(&torrent::File::unset_flags), (int)torrent::File::flag_resize_queued));

  ADD_CF_VALUE_UNI("size_bytes",             std::mem_fun(&torrent::File::size_bytes));
  ADD_CF_VALUE_UNI("size_chunks",            std::mem_fun(&torrent::File::size_chunks));
  ADD_CF_VALUE_UNI("completed_chunks",       std::mem_fun(&torrent::File::completed_chunks));

  ADD_CF_VALUE_UNI("offset",                 std::mem_fun(&torrent::File::offset));
  ADD_CF_VALUE_UNI("range_first",            std::mem_fun(&torrent::File::range_first));
  ADD_CF_VALUE_UNI("range_second",           std::mem_fun(&torrent::File::range_second));

  ADD_CF_VALUE_BI("priority",                std::ptr_fun(&apply_f_set_priority), std::mem_fun(&torrent::File::priority));

  ADD_CF_STRING_UNI("path",                  std::ptr_fun(&apply_f_path));
  ADD_CF_STRING_UNI("path_components",       std::ptr_fun(&apply_f_path_components));
  ADD_CF_STRING_UNI("path_depth",            std::ptr_fun(&apply_f_path_depth));
  ADD_CF_STRING_UNI("frozen_path",           std::mem_fun(&torrent::File::frozen_path));

  ADD_CF_VALUE_UNI("match_depth_prev",       std::mem_fun(&torrent::File::match_depth_prev));
  ADD_CF_VALUE_UNI("match_depth_next",       std::mem_fun(&torrent::File::match_depth_next));

  ADD_CF_VALUE_UNI("last_touched",           std::mem_fun(&torrent::File::last_touched));

  ADD_CFI_VOID("get_filename_last",              &apply_fi_filename_last);

  ADD_CFI_VALUE("is_file",                   std::mem_fun(&torrent::FileListIterator::is_file));
}
