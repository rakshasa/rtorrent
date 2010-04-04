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

#ifndef RTORRENT_RPC_COMMAND_IMPL_H
#define RTORRENT_RPC_COMMAND_IMPL_H

namespace rpc {

//template <> struct target_type_id<Command::generic_slot>       { static const int value = Command::target_generic; };
template <> struct target_type_id<Command::cleaned_slot>       { static const int value = Command::target_generic; };
template <> struct target_type_id<Command::any_slot>           { static const int value = Command::target_any; };
template <> struct target_type_id<Command::download_slot>      { static const int value = Command::target_download; };
template <> struct target_type_id<Command::peer_slot>          { static const int value = Command::target_peer; };
template <> struct target_type_id<Command::tracker_slot>       { static const int value = Command::target_tracker; };
template <> struct target_type_id<Command::file_slot>          { static const int value = Command::target_file; };
template <> struct target_type_id<Command::file_itr_slot>      { static const int value = Command::target_file_itr; };

template <> struct target_type_id<Command::download_pair_slot> { static const int value = Command::target_download_pair; };

template <> struct target_type_id<>                            { static const int value = Command::target_generic; };
template <> struct target_type_id<target_type>                 { static const int value = Command::target_any;      static const int proper_type = 1; };
template <> struct target_type_id<core::Download*>             { static const int value = Command::target_download; static const int proper_type = 1; };
template <> struct target_type_id<torrent::Peer*>              { static const int value = Command::target_peer;     static const int proper_type = 1; };
template <> struct target_type_id<torrent::Tracker*>           { static const int value = Command::target_tracker;  static const int proper_type = 1; };
template <> struct target_type_id<torrent::File*>              { static const int value = Command::target_file;     static const int proper_type = 1; };
template <> struct target_type_id<torrent::FileListIterator*>  { static const int value = Command::target_file_itr; static const int proper_type = 1; };

template <> struct target_type_id<core::Download*, core::Download*> { static const int value = Command::target_download_pair; };

template <> inline bool
is_target_compatible<target_type>(const target_type& target) { return true; }
template <> inline bool
is_target_compatible<torrent::File*>(const target_type& target) { return target.first == Command::target_file || Command::target_file_itr; }

template <> inline target_type
get_target_cast<target_type>(target_type target, int type) { return target; }

template <> inline torrent::File*
get_target_cast<torrent::File*>(target_type target, int type) {
  if (target.first == Command::target_file_itr)
    return static_cast<torrent::FileListIterator*>(target.second)->file();
  else
    return static_cast<torrent::File*>(target.second);
}

inline torrent::Object*
Command::push_stack(const torrent::Object::list_type& args, torrent::Object* tmp_stack) {
  unsigned int idx = 0;

  for (torrent::Object::list_const_iterator itr = args.begin(), last = args.end();
       itr != last && idx < Command::max_arguments; itr++, idx++) {
    new (tmp_stack + idx) torrent::Object(*itr);
    tmp_stack[idx].swap(*Command::argument(idx));
  }

  return tmp_stack + idx;
}

inline void
Command::pop_stack(torrent::Object* first_stack, torrent::Object* last_stack) {
  while (last_stack-- != first_stack) {
    last_stack->swap(*Command::argument(std::distance(first_stack, last_stack)));
    last_stack->~Object();
  }
}

}

#endif
