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

#include <functional>
#include <unistd.h>
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
  const std::string* base;

  if (download->file_list()->is_multi_file())
    base = &download->file_list()->root_dir();
  else
    base = &download->file_list()->at(0)->frozen_path();

  std::string::size_type split = base->rfind('/');

  if (split == std::string::npos)
    return *base;
  else
    return base->substr(split + 1);
}

torrent::Object
apply_d_change_link(int changeType, core::Download* download, const torrent::Object& rawArgs) {
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
    target = rpc::call_command_d_string("get_d_base_path", download);
    link = rak::path_expand(prefix + rpc::call_command_d_string("get_d_base_path", download) + postfix);

  } else if (type == "base_filename") {
    target = rpc::call_command_d_string("get_d_base_path", download);
    link = rak::path_expand(prefix + rpc::call_command_d_string("get_d_base_filename", download) + postfix);

  } else if (type == "tied") {
    link = rak::path_expand(rpc::call_command_d_string("get_d_tied_to_file", download));

    if (link.empty())
      return torrent::Object();

    link = rak::path_expand(prefix + link + postfix);
    target = rpc::call_command_d_string("get_d_base_path", download);

  } else {
    throw torrent::input_error("Unknown type argument.");
  }

  switch (changeType) {
  case 0:
    if (symlink(target.c_str(), link.c_str()) == -1)
      //     control->core()->push_log("create_link failed: " + std::string(rak::error_number::current().c_str()));
      //     control->core()->push_log("create_link failed: " + std::string(rak::error_number::current().c_str()) + " to " + target);
      ; // Disabled.
    break;

  case 1:
  {
    rak::file_stat fileStat;
    rak::error_number::clear_global();

    if (!fileStat.update_link(link) || !fileStat.is_link() ||
        unlink(link.c_str()) == -1)
      ; //     control->core()->push_log("delete_link failed: " + std::string(rak::error_number::current().c_str()));

    break;
  }
  default:
    break;
  }

  return torrent::Object();
}

void
apply_d_delete_tied(core::Download* download) {
  const std::string& tie = rpc::call_command_d_string("get_d_tied_to_file", download);

  if (tie.empty())
    return;

  if (::unlink(rak::path_expand(tie).c_str()) == -1)
    control->core()->push_log_std("Could not unlink tied file: " + std::string(rak::error_number::current().c_str()));

  rpc::call_command_d("set_d_tied_to_file", download, std::string());
}

void
apply_d_connection_type(core::Download* download, const std::string& name) {
  torrent::Download::ConnectionType connType;

  if (name == "leech")
    connType = torrent::Download::CONNECTION_LEECH;
  else if (name == "seed")
    connType = torrent::Download::CONNECTION_SEED;
  else
    throw torrent::input_error("Unknown peer connection type selected.");

  download->download()->set_connection_type(connType);
}

const char*
retrieve_d_connection_type(core::Download* download) {
  switch (download->download()->connection_type()) {
  case torrent::Download::CONNECTION_LEECH:
    return "leech";
  case torrent::Download::CONNECTION_SEED:
    return "seed";
  default:
    return "unknown";
  }
}

const char*
retrieve_d_priority_str(core::Download* download) {
  switch (download->priority()) {
  case 0:
    return "off";
  case 1:
    return "low";
  case 2:
    return "normal";
  case 3:
    return "high";
  default:
    throw torrent::input_error("Priority out of range.");
  }
}

torrent::Object
apply_d_ratio(core::Download* download) {
  if (download->is_hash_checking())
    return int64_t();

  int64_t bytesDone = download->download()->bytes_done();
  int64_t upTotal   = download->download()->up_rate()->total();

  return bytesDone > 0 ? (1000 * upTotal) / bytesDone : 0;
}

#define ADD_CD_SLOT(key, function, slot, parm, doc)    \
  commandDownloadSlotsItr->set_slot(slot); \
  rpc::commands.insert_download(key, commandDownloadSlotsItr++, &rpc::CommandDownloadSlot::function, rpc::CommandMap::flag_dont_delete, parm, doc);

#define ADD_CD_SLOT_PUBLIC(key, function, slot, parm, doc)    \
  commandDownloadSlotsItr->set_slot(slot); \
  rpc::commands.insert_download(key, commandDownloadSlotsItr++, &rpc::CommandDownloadSlot::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_CD_VOID(key, slot) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::object_d_fn(slot), "i:", "")

#define ADD_CD_V_VOID(key, slot) \
  ADD_CD_SLOT_PUBLIC("d_" key, call_unknown, rpc::object_d_fn(slot), "i:", "")

#define ADD_CD_F_VOID(key, slot) \
  ADD_CD_SLOT_PUBLIC("d_" key, call_unknown, rpc::object_void_d_fn(slot), "i:", "")

#define ADD_CD_LIST(key, slot) \
  ADD_CD_SLOT_PUBLIC(key, call_list, slot, "i:", "")

#define ADD_CD_VARIABLE_VALUE(key, firstKey, secondKey) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::get_variable_d_fn(firstKey, secondKey), "i:", ""); \
  ADD_CD_SLOT("set_d_" key, call_value,   rpc::set_variable_d_fn(firstKey, secondKey), "i:i", "");

#define ADD_CD_VARIABLE_VALUE_PUBLIC(key, firstKey, secondKey) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::get_variable_d_fn(firstKey, secondKey), "i:", ""); \
  ADD_CD_SLOT_PUBLIC("set_d_" key, call_value,   rpc::set_variable_d_fn(firstKey, secondKey), "i:i", "");

#define ADD_CD_VARIABLE_STRING(key, firstKey, secondKey) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::get_variable_d_fn(firstKey, secondKey), "i:", ""); \
  ADD_CD_SLOT("set_d_" key, call_string,  rpc::set_variable_d_fn(firstKey, secondKey), "i:s", "");

#define ADD_CD_VALUE_UNI(key, get) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::object_void_d_fn(get), "i:", "")

#define ADD_CD_VALUE_BI(key, set, get) \
  ADD_CD_SLOT_PUBLIC("set_d_" key, call_value, rpc::object_value_d_fn(set), "i:i", "") \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::object_void_d_fn(get), "i:", "")

#define ADD_CD_VALUE_MEM_BI(key, target, set, get) \
  ADD_CD_VALUE_BI(key, rak::on2(std::mem_fun(target), std::mem_fun(set)), rak::on(std::mem_fun(target), std::mem_fun(get)));

#define ADD_CD_VALUE_MEM_UNI(key, target, get) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::object_void_d_fn(rak::on(rak::on(std::mem_fun(&core::Download::download), std::mem_fun(target)), std::mem_fun(get))), "i:", "");

#define ADD_CD_STRING_UNI(key, get) \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::object_void_d_fn(get), "s:", "")

#define ADD_CD_STRING_BI(key, set, get) \
  ADD_CD_SLOT_PUBLIC("set_d_" key, call_string, rpc::object_string_d_fn(set), "i:s", "") \
  ADD_CD_SLOT_PUBLIC("get_d_" key, call_unknown, rpc::object_void_d_fn(get), "s:", "")

void
add_copy_to_download(const char* src, const char* dest) {
  rpc::CommandMap::iterator itr = rpc::commands.find(src);

  if (itr == rpc::commands.end())
    throw torrent::internal_error("add_copy_to_download(...) key not found.");

  rpc::commands.insert(dest, itr->second);
}

void
initialize_command_download() {
  ADD_CD_VOID("base_path",     &retrieve_d_base_path);
  ADD_CD_VOID("base_filename", &retrieve_d_base_filename);
  ADD_CD_STRING_UNI("name",    rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::name)));

  ADD_CD_LIST("create_link",   rak::bind_ptr_fn(&apply_d_change_link, 0));
  ADD_CD_LIST("delete_link",   rak::bind_ptr_fn(&apply_d_change_link, 1));
  ADD_CD_V_VOID("delete_tied", &apply_d_delete_tied);

  ADD_CD_F_VOID("start",      rak::make_mem_fun(control->core()->download_list(), &core::DownloadList::start_normal));
  ADD_CD_F_VOID("stop",       rak::make_mem_fun(control->core()->download_list(), &core::DownloadList::stop_normal));
  ADD_CD_F_VOID("open",       rak::make_mem_fun(control->core()->download_list(), &core::DownloadList::open_throw));
  ADD_CD_F_VOID("close",      rak::make_mem_fun(control->core()->download_list(), &core::DownloadList::close_throw));
  ADD_CD_F_VOID("erase",      rak::make_mem_fun(control->core()->download_list(), &core::DownloadList::erase_ptr));
  ADD_CD_F_VOID("check_hash", rak::make_mem_fun(control->core()->download_list(), &core::DownloadList::check_hash));

  ADD_CD_F_VOID("update_priorities", rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::update_priorities)));

  ADD_CD_VALUE_UNI("is_open",          rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::is_open)));
  ADD_CD_VALUE_UNI("is_active",        rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::is_active)));
  ADD_CD_VALUE_UNI("is_hash_checked",  rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::is_hash_checked)));
  ADD_CD_VALUE_UNI("is_hash_checking", rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::is_hash_checking)));
  ADD_CD_VALUE_UNI("is_multi_file",    rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::is_multi_file)));

  // 0 - stopped
  // 1 - started
  ADD_CD_VARIABLE_VALUE("state", "rtorrent", "state");
  ADD_CD_VARIABLE_VALUE("complete", "rtorrent", "complete");

  // 0 off
  // 1 scheduled, being controlled by a download scheduler. Includes a priority.
  // 3 forced off
  // 2 forced on
  ADD_CD_VARIABLE_VALUE("mode", "rtorrent", "mode");

  // 0 - Not hashing
  // 1 - Normal hashing
  // 2 - Download finished, hashing
  ADD_CD_VARIABLE_VALUE("hashing", "rtorrent", "hashing");
  ADD_CD_VARIABLE_STRING("tied_to_file", "rtorrent", "tied_to_file");

  // The "state_changed" variable is required to be a valid unix time
  // value, it indicates the last time the torrent changed its state,
  // resume/pause.
  ADD_CD_VARIABLE_VALUE("state_changed", "rtorrent", "state_changed");
  ADD_CD_VARIABLE_VALUE_PUBLIC("ignore_commands", "rtorrent", "ignore_commands");

  ADD_CD_STRING_BI("connection_current", std::ptr_fun(&apply_d_connection_type), std::ptr_fun(&retrieve_d_connection_type));

  add_copy_to_download("get_connection_leech", "get_d_connection_leech");
  add_copy_to_download("set_connection_leech", "set_d_connection_leech");
  add_copy_to_download("get_connection_seed", "get_d_connection_seed");
  add_copy_to_download("set_connection_seed", "set_d_connection_seed");

  ADD_CD_VALUE_MEM_BI("max_file_size", &core::Download::file_list, &torrent::FileList::set_max_file_size, &torrent::FileList::max_file_size);

  // Deprecated.
  ADD_CD_VALUE_MEM_BI("min_peers",     &core::Download::download, &torrent::Download::set_peers_min, &torrent::Download::peers_min);
  ADD_CD_VALUE_MEM_BI("max_peers",     &core::Download::download, &torrent::Download::set_peers_max, &torrent::Download::peers_max);
  ADD_CD_VALUE_MEM_BI("max_uploads",   &core::Download::download, &torrent::Download::set_uploads_max, &torrent::Download::uploads_max);

  ADD_CD_VALUE_MEM_BI("peers_min",        &core::Download::download, &torrent::Download::set_peers_min, &torrent::Download::peers_min);
  ADD_CD_VALUE_MEM_BI("peers_max",        &core::Download::download, &torrent::Download::set_peers_max, &torrent::Download::peers_max);
  ADD_CD_VALUE_MEM_BI("uploads_max",      &core::Download::download, &torrent::Download::set_uploads_max, &torrent::Download::uploads_max);
  ADD_CD_VALUE_UNI("peers_connected",     rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::peers_connected)));
  ADD_CD_VALUE_UNI("peers_not_connected", rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::peers_not_connected)));
  ADD_CD_VALUE_UNI("peers_complete",      rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::peers_complete)));
  ADD_CD_VALUE_UNI("peers_accounted",     rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::peers_accounted)));

  ADD_CD_VALUE_MEM_UNI("up_rate",      &torrent::Download::mutable_up_rate, &torrent::Rate::rate);
  ADD_CD_VALUE_MEM_UNI("up_total",     &torrent::Download::mutable_up_rate, &torrent::Rate::total);
  ADD_CD_VALUE_MEM_UNI("down_rate",    &torrent::Download::mutable_down_rate, &torrent::Rate::rate);
  ADD_CD_VALUE_MEM_UNI("down_total",   &torrent::Download::mutable_down_rate, &torrent::Rate::total);
  ADD_CD_VALUE_MEM_UNI("skip_rate",    &torrent::Download::mutable_skip_rate, &torrent::Rate::rate);
  ADD_CD_VALUE_MEM_UNI("skip_total",   &torrent::Download::mutable_skip_rate, &torrent::Rate::total);

  ADD_CD_VALUE_UNI("bytes_done",       rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::bytes_done)));
  ADD_CD_VALUE_UNI("ratio",            std::ptr_fun(&apply_d_ratio));
  ADD_CD_VALUE_UNI("chunks_hashed",    rak::on(std::mem_fun(&core::Download::download), std::mem_fun(&torrent::Download::chunks_hashed)));
  ADD_CD_VALUE_UNI("free_diskspace",   rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::free_diskspace)));

  ADD_CD_VALUE_UNI("size_files",       rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::size_files)));
  ADD_CD_VALUE_UNI("size_bytes",       rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::size_bytes)));
  ADD_CD_VALUE_UNI("size_chunks",      rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::size_chunks)));

  ADD_CD_VALUE_UNI("completed_bytes",  rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::completed_bytes)));
  ADD_CD_VALUE_UNI("completed_chunks", rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::completed_chunks)));
  ADD_CD_VALUE_UNI("left_bytes",       rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::left_bytes)));

  ADD_CD_VALUE_UNI("chunk_size",       rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::chunk_size)));

  ADD_CD_VALUE_MEM_BI("tracker_numwant", &core::Download::tracker_list, &torrent::TrackerList::set_numwant, &torrent::TrackerList::numwant);
  ADD_CD_VALUE_UNI("tracker_focus",      rak::on(std::mem_fun(&core::Download::tracker_list), std::mem_fun(&torrent::TrackerList::focus)));
  ADD_CD_VALUE_UNI("tracker_size",       rak::on(std::mem_fun(&core::Download::tracker_list), std::mem_fun(&torrent::TrackerList::size)));

  ADD_CD_STRING_BI("directory",        std::mem_fun(&core::Download::set_root_directory), rak::on(std::mem_fun(&core::Download::file_list), std::mem_fun(&torrent::FileList::root_dir)));
  ADD_CD_VALUE_BI("priority",          std::mem_fun(&core::Download::set_priority), std::mem_fun(&core::Download::priority));
  ADD_CD_STRING_UNI("priority_str",    std::ptr_fun(&retrieve_d_priority_str));
}
