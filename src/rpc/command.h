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

#ifndef RTORRENT_RPC_VARIABLE_H
#define RTORRENT_RPC_VARIABLE_H

#include <torrent/object.h>

namespace core {
  class Download;
}

namespace torrent {
  class File;
  class FileListIterator;
  class Peer;
  class Tracker;
}

namespace rpc {

// Since c++0x isn't out yet...
template <typename T1, typename T2, typename T3>
struct rt_triple : private std::pair<T1, T2> {
  typedef std::pair<T1, T2> base_type;
  typedef T3                third_type;

  using base_type::first;
  using base_type::second;
  using base_type::first_type;
  using base_type::second_type;

  T3 third;

  rt_triple() : base_type(), third() {}

  rt_triple(const T1& a, const T2& b) :
    base_type(a, b), third() {}

  rt_triple(const T1& a, const T2& b, const T3& c) :
    base_type(a, b), third(c) {}

  template <typename U1, typename U2>
  rt_triple(const std::pair<U1, U2>& b) : base_type(b), third() {}

  template <typename U1, typename U2, typename U3>
  rt_triple(const rt_triple& src) :
    base_type(src.first, src.second), third(src.third) {}
};

// Since it gets used so many places we might as well put it in the
// rpc namespace.
//typedef std::pair<int, void*> target_type;
typedef rt_triple<int, void*, void*> target_type;

class Command {
public:
  typedef torrent::Object::value_type  value_type;
  typedef torrent::Object::string_type string_type;
  typedef torrent::Object::list_type   list_type;
  typedef torrent::Object::map_type    map_type;
  typedef torrent::Object::key_type    key_type;

  typedef const torrent::Object (*generic_slot)  (Command*, const torrent::Object&);
  typedef const torrent::Object (*any_slot)      (Command*, target_type, const torrent::Object&);
  typedef const torrent::Object (*download_slot) (Command*, core::Download*, const torrent::Object&);
  typedef const torrent::Object (*file_slot)     (Command*, torrent::File*, const torrent::Object&);
  typedef const torrent::Object (*file_itr_slot) (Command*, torrent::FileListIterator*, const torrent::Object&);
  typedef const torrent::Object (*peer_slot)     (Command*, torrent::Peer*, const torrent::Object&);
  typedef const torrent::Object (*tracker_slot)  (Command*, torrent::Tracker*, const torrent::Object&);

  typedef const torrent::Object (*download_pair_slot) (Command*, core::Download*, core::Download*, const torrent::Object&);

  static const int target_generic  = 0;
  static const int target_any      = 1;
  static const int target_download = 2;
  static const int target_peer     = 3;
  static const int target_tracker  = 4;
  static const int target_file     = 5;
  static const int target_file_itr = 6;

  static const int target_download_pair = 7;

  Command() {}
  virtual ~Command() {}

protected:
  Command(const Command&);
  void operator = (const Command&);
};

template <typename T1 = void, typename T2 = void>
struct target_type_id {
  // Nothing here, so we cause an error.
};

template <> struct target_type_id<Command::generic_slot>       { static const int value = Command::target_generic; };
template <> struct target_type_id<Command::any_slot>           { static const int value = Command::target_any; };
template <> struct target_type_id<Command::download_slot>      { static const int value = Command::target_download; };
template <> struct target_type_id<Command::peer_slot>          { static const int value = Command::target_peer; };
template <> struct target_type_id<Command::tracker_slot>       { static const int value = Command::target_tracker; };
template <> struct target_type_id<Command::file_slot>          { static const int value = Command::target_file; };
template <> struct target_type_id<Command::file_itr_slot>      { static const int value = Command::target_file_itr; };

template <> struct target_type_id<Command::download_pair_slot> { static const int value = Command::target_download_pair; };

template <> struct target_type_id<>                            { static const int value = Command::target_generic; };
template <> struct target_type_id<target_type>                 { static const int value = Command::target_any; };
template <> struct target_type_id<core::Download*>             { static const int value = Command::target_download; };
template <> struct target_type_id<torrent::Peer*>              { static const int value = Command::target_peer; };
template <> struct target_type_id<torrent::Tracker*>           { static const int value = Command::target_tracker; };
template <> struct target_type_id<torrent::File*>              { static const int value = Command::target_file; };
template <> struct target_type_id<torrent::FileListIterator*>  { static const int value = Command::target_file_itr; };

template <> struct target_type_id<core::Download*, core::Download*> { static const int value = Command::target_download_pair; };

// Splitting pairs into separate targets.
inline bool is_target_pair(const target_type& target) { return target.first >= Command::target_download_pair; }

inline target_type get_target_left(const target_type& target)  { return target_type(target.first - 5, target.second); }
inline target_type get_target_right(const target_type& target) { return target_type(target.first - 5, target.third); }

}

#endif
