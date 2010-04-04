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

#ifndef RTORRENT_RPC_COMMAND_H
#define RTORRENT_RPC_COMMAND_H

#include <torrent/object.h>
#include <torrent/data/file_list_iterator.h>

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

template <typename Target>
struct target_wrapper {
  typedef Target      target_type; 
  typedef Target      cleaned_type;
};

template <>
struct target_wrapper<void> {
  struct no_type {};

  typedef void        target_type; 
  typedef no_type*    cleaned_type;
};

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

  rt_triple(const base_type& b) : base_type(b), third() {}

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
  typedef const torrent::Object (*cleaned_slot)  (Command*, target_wrapper<void>::cleaned_type, const torrent::Object&);
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

  static const unsigned int max_arguments = 10;

  struct stack_type {
    torrent::Object* begin() { return reinterpret_cast<torrent::Object*>(buffer); }
    torrent::Object* end()   { return reinterpret_cast<torrent::Object*>(buffer) + max_arguments; }
    
    static stack_type* from_data(char* data) { return reinterpret_cast<stack_type*>(data); }

    char buffer[sizeof(torrent::Object) * max_arguments];
  };

  Command() {}
  virtual ~Command() {}

  static torrent::Object* argument(unsigned int index) { return m_arguments.begin() + index; }
  static torrent::Object& argument_ref(unsigned int index) { return *(m_arguments.begin() + index); }

  static stack_type m_arguments;

  static torrent::Object* stack_begin() { return m_arguments.begin(); }
  static torrent::Object* stack_end()   { return m_arguments.end(); }

  static torrent::Object* push_stack(const torrent::Object::list_type& args, torrent::Object* tmp_stack);
  static void             pop_stack(torrent::Object* first_stack, torrent::Object* last_stack);

protected:
  Command(const Command&);
  void operator = (const Command&);

  // For use by functions that need to use placeholders to arguments
  // within commands. E.d. callable command strings where one of the
  // arguments within the command needs to be supplied by the caller.
};

template <typename T1 = void, typename T2 = void>
struct target_type_id {
  // Nothing here, so we cause an error.
};

template <typename T> inline bool
is_target_compatible(const target_type& target) { return target.first == target_type_id<T>::value; }

// Splitting pairs into separate targets.
inline bool is_target_pair(const target_type& target) { return target.first >= Command::target_download_pair; }

template <typename T> inline T
get_target_cast(target_type target, int type = target_type_id<T>::value) { return (T)target.second; }

inline target_type get_target_left(const target_type& target)  { return target_type(target.first - 5, target.second); }
inline target_type get_target_right(const target_type& target) { return target_type(target.first - 5, target.third); }

}

#include "command_impl.h"

#endif
