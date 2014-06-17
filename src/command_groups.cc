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

#include "config.h"

#include <torrent/download/resource_manager.h>
#include <torrent/download/choke_group.h>
#include <torrent/download/choke_queue.h>
#include <torrent/utils/option_strings.h>

#include "ui/root.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

// For cg_d_group.
#include "core/download.h"

// A hack to allow testing of the new choke_group API without the
// working parts present.
#define USE_CHOKE_GROUP 0

#if USE_CHOKE_GROUP

int64_t
cg_get_index(const torrent::Object& raw_args) {
  const torrent::Object& arg = (raw_args.is_list() && !raw_args.as_list().empty()) ? raw_args.as_list().front() : raw_args;

  int64_t index = 0;

  if (arg.is_string()) {
    if (!rpc::parse_whole_value_nothrow(arg.as_string().c_str(), &index))
      return torrent::resource_manager()->group_index_of(arg.as_string());

  } else {
    index = arg.as_value();
  }

  if (index < 0)
    index = (int64_t)torrent::resource_manager()->group_size() + index;

  return std::min<uint64_t>(index, torrent::resource_manager()->group_size());
}

torrent::choke_group*
cg_get_group(const torrent::Object& raw_args) {
  return torrent::resource_manager()->group_at(cg_get_index(raw_args));
}

int64_t
cg_d_group(core::Download* download) {
  return torrent::resource_manager()->entry_at(download->main()).group();
}

const std::string&
cg_d_group_name(core::Download* download) {
  return torrent::resource_manager()->group_at(torrent::resource_manager()->entry_at(download->main()).group())->name();
}

void
cg_d_group_set(core::Download* download, const torrent::Object& arg) {
  torrent::resource_manager()->set_group(torrent::resource_manager()->find_throw(download->main()), cg_get_index(arg));
}

torrent::Object
apply_cg_list() {
  torrent::Object::list_type result;
  
  for (torrent::ResourceManager::group_iterator
         itr = torrent::resource_manager()->group_begin(),
         last = torrent::resource_manager()->group_end(); itr != last; itr++)
    result.push_back((*itr)->name());

  return torrent::Object::from_list(result);
}

torrent::Object
apply_cg_insert(const std::string& arg) {
  int64_t dummy;

  if (rpc::parse_whole_value_nothrow(arg.c_str(), &dummy))
    throw torrent::input_error("Cannot use a value string as choke group name.");

  torrent::resource_manager()->push_group(arg);

  return torrent::Object();
}

//
// The hacked version:
//
#else

std::vector<torrent::choke_group*> cg_list_hack;

int64_t
cg_get_index(const torrent::Object& raw_args) {
  const torrent::Object& arg = (raw_args.is_list() && !raw_args.as_list().empty()) ? raw_args.as_list().front() : raw_args;

  int64_t index = 0;

  if (arg.is_string()) {
    if (!rpc::parse_whole_value_nothrow(arg.as_string().c_str(), &index)) {
      std::vector<torrent::choke_group*>::iterator itr = std::find_if(cg_list_hack.begin(), cg_list_hack.end(),
                                                                      rak::equal(arg.as_string(), std::mem_fun(&torrent::choke_group::name)));

      if (itr == cg_list_hack.end())
        throw torrent::input_error("Choke group not found.");

      return std::distance(cg_list_hack.begin(), itr);
    }

  } else {
    index = arg.as_value();
  }

  if (index < 0)
    index = (int64_t)cg_list_hack.size() + index;

  if ((size_t)index >= cg_list_hack.size())
    throw torrent::input_error("Choke group not found.");

  return index;
}

torrent::choke_group*
cg_get_group(const torrent::Object& raw_args) {
  int64_t index = cg_get_index(raw_args);

  if ((size_t)index >= cg_list_hack.size())
    throw torrent::input_error("Choke group not found.");

  return cg_list_hack.at(index);
}

int64_t cg_d_group(core::Download* download) { return download->group(); }
void    cg_d_group_set(core::Download* download, const torrent::Object& arg) { download->set_group(cg_get_index(arg)); }

torrent::Object
apply_cg_list() {
  torrent::Object::list_type result;
  
  for (std::vector<torrent::choke_group*>::iterator itr = cg_list_hack.begin(), last = cg_list_hack.end(); itr != last; itr++)
    result.push_back((*itr)->name());

  return torrent::Object::from_list(result);
}

torrent::Object
apply_cg_insert(const std::string& arg) {
  int64_t dummy;

  if (rpc::parse_whole_value_nothrow(arg.c_str(), &dummy))
    throw torrent::input_error("Cannot use a value string as choke group name.");

  if (arg.empty() ||
      std::find_if(cg_list_hack.begin(), cg_list_hack.end(),
                   rak::equal(arg, std::mem_fun(&torrent::choke_group::name))) != cg_list_hack.end())
    throw torrent::input_error("Duplicate name for choke group.");

  cg_list_hack.push_back(new torrent::choke_group());
  cg_list_hack.back()->set_name(arg);

  cg_list_hack.back()->up_queue()->set_heuristics(torrent::choke_queue::HEURISTICS_UPLOAD_LEECH);
  cg_list_hack.back()->down_queue()->set_heuristics(torrent::choke_queue::HEURISTICS_DOWNLOAD_LEECH);

  return torrent::Object();
}

torrent::Object
apply_cg_index_of(const std::string& arg) {
  std::vector<torrent::choke_group*>::iterator itr =
    std::find_if(cg_list_hack.begin(), cg_list_hack.end(), rak::equal(arg, std::mem_fun(&torrent::choke_group::name)));

  if (itr == cg_list_hack.end())
    throw torrent::input_error("Choke group not found.");

  return std::distance(cg_list_hack.begin(), itr);
}

//
// End of choke group hack.
//
#endif


torrent::Object
apply_cg_max_set(const torrent::Object::list_type& args, bool is_up) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  int64_t second_arg = 0;
  rpc::parse_whole_value(args.back().as_string().c_str(), &second_arg);

  if (is_up)
    cg_get_group(args.front())->up_queue()->set_max_unchoked(second_arg);
  else
    cg_get_group(args.front())->down_queue()->set_max_unchoked(second_arg);

  return torrent::Object();
}

torrent::Object
apply_cg_heuristics_set(const torrent::Object::list_type& args, bool is_up) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  int t = torrent::option_find_string(is_up ? torrent::OPTION_CHOKE_HEURISTICS_UPLOAD : torrent::OPTION_CHOKE_HEURISTICS_DOWNLOAD,
                                      args.back().as_string().c_str());

  if (is_up)
    cg_get_group(args.front())->up_queue()->set_heuristics((torrent::choke_queue::heuristics_enum)t);
  else
    cg_get_group(args.front())->down_queue()->set_heuristics((torrent::choke_queue::heuristics_enum)t);

  return torrent::Object();
}

torrent::Object
apply_cg_tracker_mode_set(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  int t = torrent::option_find_string(torrent::OPTION_TRACKER_MODE, args.back().as_string().c_str());

  cg_get_group(args.front())->set_tracker_mode((torrent::choke_group::tracker_mode_enum)t);

  return torrent::Object();
}

#define CG_GROUP_AT()          std::bind(&cg_get_group, std::placeholders::_2)
#define CHOKE_GROUP(direction) std::bind(direction, CG_GROUP_AT())

/*

<cg_index> -> '0'..'(choke_group.size)'
           -> '-1'..'-(choke_group.size)'
           -> '<group_name>'

(choke_group.list) -> List of group names.
(choke_group.size) -> Number of groups.

(choke_group.insert,"group_name")

Adds a new group with default settings, use index '-1' to accessing it
immediately afterwards.

(choke_group.index_of,"group_name") -> <group_index>

Throws if the group name was not found.

(choke_group.general.size,<cg_index>) -> <size>

Number of torrents in this group.

(choke_group.tracker.mode,<cg_index>) -> "tracker_mode"
(choke_group.tracker.mode.set,<cg_index>,"tracker_mode")

Decide on how aggressive a tracker should be, see
'strings.tracker_mode' for list of available options

(choke_group.up.rate,<cg_index>) -> <bytes/second>
(choke_group.down.rate,<cg_index>) -> <bytes/second>

Upload / download rate for the aggregate of all torrents in this
particular group.

(choke_group.up.max,<cg_index>) -> <max_upload_slots>
(choke_group.up.max.unlimited,<cg_index>) -> <max_upload_slots>
(choke_group.up.max.set,<cg_index>, <max_upload_slots>)
(choke_group.down.max,<cg_index>) -> <max_download_slots>
(choke_group.down.max.unlimited,<cg_index>) -> <max_download_slots>
(choke_group.down.max.set,<cg_index>, <max_download_slots)

Number of unchoked upload / download peers regulated on a group basis.

(choke_group.up.total,<cg_index>) -> <number of queued and unchoked interested peers>
(choke_group.up.queued,<cg_index>) -> <number of queued interested peers>
(choke_group.up.unchoked,<cg_index>) -> <number of unchoked uploads>
(choke_group.down.total,<cg_index>) -> <number of queued and unchoked interested peers>
(choke_group.down.queued,<cg_index>) -> <number of queued interested peers>
(choke_group.down.unchoked,<cg_index>) -> <number of unchoked uploads>

(choke_group.up.heuristics,<cg_index>) -> "heuristics"
(choke_group.up.heuristics.set,<cg_index>,"heuristics")
(choke_group.down.heuristics,<cg_index>) -> "heuristics"
(choke_group.down.heuristics.set,<cg_index>,"heuristics")

Heuristics are used for deciding what peers to choke and unchoke, see
'strings.choke_heuristics{,_download,_upload}' for a list of available
options.

(d.group) -> <choke_group_index>
(d.group.name) -> "choke_group_name"
(d.group.set,<cg_index>)

 */


void
initialize_command_groups() {
  CMD2_ANY         ("choke_group.list",                std::bind(&apply_cg_list));
  CMD2_ANY_STRING  ("choke_group.insert",              std::bind(&apply_cg_insert, std::placeholders::_2));
		
#if USE_CHOKE_GROUP
  CMD2_ANY         ("choke_group.size",                std::bind(&torrent::ResourceManager::group_size, torrent::resource_manager()));
  CMD2_ANY_STRING  ("choke_group.index_of",            std::bind(&torrent::ResourceManager::group_index_of, torrent::resource_manager(), std::placeholders::_2));
#else
  apply_cg_insert("default");

  CMD2_ANY         ("choke_group.size",                std::bind(&std::vector<torrent::choke_group*>::size, cg_list_hack));
  CMD2_ANY_STRING  ("choke_group.index_of",            std::bind(&apply_cg_index_of, std::placeholders::_2));
#endif

  // Commands specific for a group. Supports as the first argument the
  // name, the index or a negative index.
  CMD2_ANY         ("choke_group.general.size",        std::bind(&torrent::choke_group::size, CG_GROUP_AT()));

  CMD2_ANY         ("choke_group.tracker.mode",        std::bind(&torrent::option_as_string, torrent::OPTION_TRACKER_MODE,
                                                                 std::bind(&torrent::choke_group::tracker_mode, CG_GROUP_AT())));
  CMD2_ANY_LIST    ("choke_group.tracker.mode.set",    std::bind(&apply_cg_tracker_mode_set, std::placeholders::_2));

  CMD2_ANY         ("choke_group.up.rate",             std::bind(&torrent::choke_group::up_rate, CG_GROUP_AT()));
  CMD2_ANY         ("choke_group.down.rate",           std::bind(&torrent::choke_group::down_rate, CG_GROUP_AT()));

  CMD2_ANY         ("choke_group.up.max.unlimited",    std::bind(&torrent::choke_queue::is_unlimited, CHOKE_GROUP(&torrent::choke_group::up_queue)));
  CMD2_ANY         ("choke_group.up.max",              std::bind(&torrent::choke_queue::max_unchoked_signed, CHOKE_GROUP(&torrent::choke_group::up_queue)));
  CMD2_ANY_LIST    ("choke_group.up.max.set",          std::bind(&apply_cg_max_set, std::placeholders::_2, true));

  CMD2_ANY         ("choke_group.up.total",            std::bind(&torrent::choke_queue::size_total, CHOKE_GROUP(&torrent::choke_group::up_queue)));
  CMD2_ANY         ("choke_group.up.queued",           std::bind(&torrent::choke_queue::size_queued, CHOKE_GROUP(&torrent::choke_group::up_queue)));
  CMD2_ANY         ("choke_group.up.unchoked",         std::bind(&torrent::choke_queue::size_unchoked, CHOKE_GROUP(&torrent::choke_group::up_queue)));
  CMD2_ANY         ("choke_group.up.heuristics",       std::bind(&torrent::option_as_string, torrent::OPTION_CHOKE_HEURISTICS,
                                                                 std::bind(&torrent::choke_queue::heuristics, CHOKE_GROUP(&torrent::choke_group::up_queue))));
  CMD2_ANY_LIST    ("choke_group.up.heuristics.set",   std::bind(&apply_cg_heuristics_set, std::placeholders::_2, true));

  CMD2_ANY         ("choke_group.down.max.unlimited",  std::bind(&torrent::choke_queue::is_unlimited, CHOKE_GROUP(&torrent::choke_group::down_queue)));
  CMD2_ANY         ("choke_group.down.max",            std::bind(&torrent::choke_queue::max_unchoked_signed, CHOKE_GROUP(&torrent::choke_group::down_queue)));
  CMD2_ANY_LIST    ("choke_group.down.max.set",        std::bind(&apply_cg_max_set, std::placeholders::_2, false));

  CMD2_ANY         ("choke_group.down.total",          std::bind(&torrent::choke_queue::size_total, CHOKE_GROUP(&torrent::choke_group::down_queue)));
  CMD2_ANY         ("choke_group.down.queued",         std::bind(&torrent::choke_queue::size_queued, CHOKE_GROUP(&torrent::choke_group::down_queue)));
  CMD2_ANY         ("choke_group.down.unchoked",       std::bind(&torrent::choke_queue::size_unchoked, CHOKE_GROUP(&torrent::choke_group::down_queue)));
  CMD2_ANY         ("choke_group.down.heuristics",     std::bind(&torrent::option_as_string, torrent::OPTION_CHOKE_HEURISTICS,
                                                                 std::bind(&torrent::choke_queue::heuristics, CHOKE_GROUP(&torrent::choke_group::down_queue))));
  CMD2_ANY_LIST    ("choke_group.down.heuristics.set", std::bind(&apply_cg_heuristics_set, std::placeholders::_2, false));
}
