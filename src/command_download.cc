// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

#include <functional>
#include <rak/file_stat.h>
#include <rak/error_number.h>
#include <rak/path.h>
#include <torrent/rate.h>
#include <torrent/data/file.h>
#include <torrent/data/file_list.h>

#include "core/download.h"
#include "core/manager.h"
#include "rpc/command_slot.h"
#include "rpc/command_variable.h"
#include "rpc/command_download_slot.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

std::string
retrieve_d_base_path(core::Download* download) {
  if (download->file_list()->is_multi_file())
    return download->file_list()->root_dir();
  else
    return download->file_list()->at(0)->frozen_path();
}

std::string
retrieve_d_base_filename(core::Download* download) {
  std::string base;

  if (download->file_list()->is_multi_file())
    base = download->file_list()->root_dir();
  else
    base = download->file_list()->at(0)->frozen_path();

  std::string::size_type split = base.rfind('/');

  if (split == std::string::npos)
    return base;
  else
    return base.substr(split + 1);
}

torrent::Object
apply_d_create_link(core::Download* download, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() != 3)
    throw torrent::input_error("Wrong argument count.");

  torrent::Object::list_type::const_iterator itr = args.begin();

  const std::string& type    = (itr++)->as_string();
  const std::string& prefix  = (itr++)->as_string();
  const std::string& postfix = (itr++)->as_string();
  
  if (type.empty())
    throw torrent::input_error("Invalid arguments.");

  std::string target;
  std::string link;

  if (type == "base_path") {
    target = download->get_string("base_path");
    link = rak::path_expand(prefix + download->get_string("base_path") + postfix);

  } else if (type == "base_filename") {
    target = download->get_string("base_path");
    link = rak::path_expand(prefix + download->get_string("base_filename") + postfix);

  } else if (type == "tied") {
    link = rak::path_expand(download->get_string("get_tied_to_file"));

    if (link.empty())
      return torrent::Object();

    link = rak::path_expand(prefix + link + postfix);
    target = download->get_string("base_path");

  } else {
    throw torrent::input_error("Unknown type argument.");
  }

  if (symlink(target.c_str(), link.c_str()) == -1)
//     control->core()->push_log("create_link failed: " + std::string(rak::error_number::current().c_str()));
//     control->core()->push_log("create_link failed: " + std::string(rak::error_number::current().c_str()) + " to " + target);
    ; // Disabled.

  return torrent::Object();
}

torrent::Object
apply_d_delete_link(core::Download* download, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() != 3)
    throw torrent::input_error("Wrong argument count.");

  torrent::Object::list_type::const_iterator itr = args.begin();

  const std::string& type    = (itr++)->as_string();
  const std::string& prefix  = (itr++)->as_string();
  const std::string& postfix = (itr++)->as_string();
  
  if (type.empty())
    throw torrent::input_error("Invalid arguments.");

  std::string link;

  if (type == "base_path") {
    link = rak::path_expand(prefix + download->get_string("base_path") + postfix);

  } else if (type == "base_filename") {
    link = rak::path_expand(prefix + download->get_string("base_filename") + postfix);

  } else if (type == "tied") {
    link = rak::path_expand(download->get_string("get_tied_to_file"));

    if (link.empty())
      return torrent::Object();

    link = rak::path_expand(prefix + link + postfix);

  } else {
    throw torrent::input_error("Unknown type argument.");
  }

  rak::file_stat fileStat;
  rak::error_number::clear_global();

  if (!fileStat.update_link(link) || !fileStat.is_link() ||
      unlink(link.c_str()) == -1)
    ; //     control->core()->push_log("delete_link failed: " + std::string(rak::error_number::current().c_str()));

  return torrent::Object();
}

#define ADD_COMMAND_DOWNLOAD_SLOT(key, function, slot, parm, doc)    \
  commandDownloadSlotsItr->set_slot(slot); \
  variables->insert(key, commandDownloadSlotsItr++, NULL, &utils::CommandDownloadSlot::function, utils::CommandMap::flag_dont_delete | utils::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_COMMAND_DOWNLOAD_VOID(key, slot) \
  ADD_COMMAND_DOWNLOAD_SLOT(key, call_unknown, utils::object_d_fn(slot), "i:", "")

#define ADD_COMMAND_DOWNLOAD_LIST(key, slot) \
  ADD_COMMAND_DOWNLOAD_SLOT(key, call_list, slot, "i:", "")

#define ADD_COMMAND_DOWNLOAD_VARIABLE_VALUE(key, firstKey, secondKey) \
  ADD_COMMAND_DOWNLOAD_SLOT("get_" key, call_unknown, utils::get_variable_d_fn(firstKey, secondKey), "i:", ""); \
  ADD_COMMAND_DOWNLOAD_SLOT("set_" key, call_value,   utils::set_variable_d_fn(firstKey, secondKey), "i:i", "");

#define ADD_COMMAND_DOWNLOAD_VARIABLE_STRING(key, firstKey, secondKey) \
  ADD_COMMAND_DOWNLOAD_SLOT("get_" key, call_unknown, utils::get_variable_d_fn(firstKey, secondKey), "i:", ""); \
  ADD_COMMAND_DOWNLOAD_SLOT("set_" key, call_string,  utils::set_variable_d_fn(firstKey, secondKey), "i:s", "");

#define ADD_COMMAND_DOWNLOAD_VALUE_BI(key, set, get) \
  ADD_COMMAND_DOWNLOAD_SLOT("set_" key, call_value, utils::object_value_d_fn(set), "i:i", "") \
  ADD_COMMAND_DOWNLOAD_SLOT("get_" key, call_unknown, utils::object_void_d_fn(get), "i:", "")

#define ADD_COMMAND_DOWNLOAD_VALUE_MEM_BI(key, target, set, get) \
  ADD_COMMAND_DOWNLOAD_VALUE_BI(key, rak::on2(std::mem_fun(target), std::mem_fun(set)), rak::on(std::mem_fun(target), std::mem_fun(get)));

#define ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI(key, target, get) \
  ADD_COMMAND_DOWNLOAD_SLOT("get_" key, call_unknown, utils::object_void_d_fn(rak::on(rak::on(std::mem_fun(&core::Download::download), std::mem_fun(target)), std::mem_fun(get))), "i:", "");

#define ADD_COMMAND_DOWNLOAD_STRING_BI(key, set, get) \
  ADD_COMMAND_DOWNLOAD_SLOT("set_" key, call_string, utils::object_string_d_fn(set), "i:s", "") \
  ADD_COMMAND_DOWNLOAD_SLOT("get_" key, call_unknown, utils::object_void_d_fn(get), "s:", "")

void
add_copy_to_download(const char* key) {
  utils::CommandMap::iterator itr = control->variable()->find(key);

  if (itr == control->variable()->end())
    throw torrent::internal_error("add_copy_to_download(...) key not found.");

  control->download_variables()->insert(key, itr->second);
}

void
initialize_command_download() {
  utils::CommandMap* variables = control->download_variables();

  ADD_COMMAND_DOWNLOAD_VOID("base_path", &retrieve_d_base_path);
  ADD_COMMAND_DOWNLOAD_VOID("base_filename", &retrieve_d_base_filename);

  ADD_COMMAND_DOWNLOAD_LIST("create_link", rak::ptr_fn(&apply_d_create_link));
  ADD_COMMAND_DOWNLOAD_LIST("delete_link", rak::ptr_fn(&apply_d_delete_link));

  add_copy_to_download("print");

  // 0 - stopped
  // 1 - started
  ADD_COMMAND_DOWNLOAD_VARIABLE_VALUE("state", "rtorrent", "state");
  ADD_COMMAND_DOWNLOAD_VARIABLE_VALUE("complete", "rtorrent", "complete");

  // 0 - Not hashing
  // 1 - Normal hashing
  // 2 - Download finished, hashing
  ADD_COMMAND_DOWNLOAD_VARIABLE_VALUE("hashing", "rtorrent", "hashing");
  ADD_COMMAND_DOWNLOAD_VARIABLE_STRING("tied_to_file", "rtorrent", "tied_to_file");

  // The "state_changed" variable is required to be a valid unix time
  // value, it indicates the last time the torrent changed its state,
  // resume/pause.
  ADD_COMMAND_DOWNLOAD_VARIABLE_VALUE("state_changed", "rtorrent", "state_changed");
  ADD_COMMAND_DOWNLOAD_VARIABLE_VALUE("ignore_commands", "rtorrent", "ignore_commands");

  ADD_COMMAND_DOWNLOAD_STRING_BI("connection_current", std::mem_fun(&core::Download::set_connection_current), std::mem_fun(&core::Download::connection_current));

  add_copy_to_download("get_connection_leech");
  add_copy_to_download("set_connection_leech");
  add_copy_to_download("get_connection_seed");
  add_copy_to_download("set_connection_seed");

  ADD_COMMAND_DOWNLOAD_VALUE_MEM_BI("max_file_size", &core::Download::file_list, &torrent::FileList::set_max_file_size, &torrent::FileList::max_file_size);

  ADD_COMMAND_DOWNLOAD_VALUE_MEM_BI("min_peers",     &core::Download::download, &torrent::Download::set_peers_min, &torrent::Download::peers_min);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_BI("max_peers",     &core::Download::download, &torrent::Download::set_peers_max, &torrent::Download::peers_max);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_BI("max_uploads",   &core::Download::download, &torrent::Download::set_uploads_max, &torrent::Download::uploads_max);

  ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI("up_rate",    &torrent::Download::mutable_up_rate, &torrent::Rate::rate);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI("up_total",   &torrent::Download::mutable_up_rate, &torrent::Rate::total);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI("down_rate",  &torrent::Download::mutable_down_rate, &torrent::Rate::rate);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI("down_total", &torrent::Download::mutable_down_rate, &torrent::Rate::total);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI("skip_rate",  &torrent::Download::mutable_skip_rate, &torrent::Rate::rate);
  ADD_COMMAND_DOWNLOAD_VALUE_MEM_UNI("skip_total", &torrent::Download::mutable_skip_rate, &torrent::Rate::total);

  //   variables->insert("split_file_size",    new utils::VariableValueSlot(rak::mem_fn(file_list(), &torrent::FileList::split_file_size),
  //                                                                         rak::mem_fn(file_list(), &torrent::FileList::set_split_file_size)));
  //   variables->insert("split_suffix",       new utils::VariableStringSlot(rak::mem_fn(file_list(), &torrent::FileList::split_suffix),
  //                                                                          rak::mem_fn(file_list(), &torrent::FileList::set_split_suffix)));

  ADD_COMMAND_DOWNLOAD_VALUE_MEM_BI("tracker_numwant", &core::Download::tracker_list, &torrent::TrackerList::set_numwant, &torrent::TrackerList::numwant);

  ADD_COMMAND_DOWNLOAD_STRING_BI("directory", std::mem_fun(&core::Download::set_root_directory), rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::root_dir)));
  ADD_COMMAND_DOWNLOAD_VALUE_BI("priority", std::mem_fun(&core::Download::set_priority), std::mem_fun(&core::Download::priority));
}
