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

#ifndef RTORRENT_RPC_COMMAND_H
#define RTORRENT_RPC_COMMAND_H

#include <functional>
#include <limits>
#include <inttypes.h>
#include <torrent/object.h>
#include lt_tr1_functional

#include <torrent/object.h>
#include <torrent/data/file_list_iterator.h>

// Move into config.h or something.
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
  using typename base_type::first_type;
  using typename base_type::second_type;

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

class command_base;

typedef const torrent::Object (*command_base_call_type)(command_base*, target_type, const torrent::Object&);
typedef std::function<torrent::Object (target_type, const torrent::Object&)> base_function;

template <typename tmpl> struct command_base_is_valid {};
template <command_base_call_type tmpl_func> struct command_base_is_type {};

class command_base {
public:
  typedef torrent::Object::value_type  value_type;
  typedef torrent::Object::string_type string_type;
  typedef torrent::Object::list_type   list_type;
  typedef torrent::Object::map_type    map_type;
  typedef torrent::Object::key_type    key_type;

  typedef const torrent::Object (*generic_slot)  (command_base*, const torrent::Object&);
  typedef const torrent::Object (*cleaned_slot)  (command_base*, target_wrapper<void>::cleaned_type, const torrent::Object&);
  typedef const torrent::Object (*any_slot)      (command_base*, target_type, const torrent::Object&);
  typedef const torrent::Object (*download_slot) (command_base*, core::Download*, const torrent::Object&);
  typedef const torrent::Object (*file_slot)     (command_base*, torrent::File*, const torrent::Object&);
  typedef const torrent::Object (*file_itr_slot) (command_base*, torrent::FileListIterator*, const torrent::Object&);
  typedef const torrent::Object (*peer_slot)     (command_base*, torrent::Peer*, const torrent::Object&);
  typedef const torrent::Object (*tracker_slot)  (command_base*, torrent::Tracker*, const torrent::Object&);

  typedef const torrent::Object (*download_pair_slot) (command_base*, core::Download*, core::Download*, const torrent::Object&);

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
    torrent::Object*       begin() { return reinterpret_cast<torrent::Object*>(buffer); }
    torrent::Object*       end()   { return reinterpret_cast<torrent::Object*>(buffer) + max_arguments; }
    
    const torrent::Object* begin() const { return reinterpret_cast<const torrent::Object*>(buffer); }
    const torrent::Object* end()   const { return reinterpret_cast<const torrent::Object*>(buffer) + max_arguments; }
    
    torrent::Object&       operator [] (unsigned int idx)       { return *(begin() + idx); }
    const torrent::Object& operator [] (unsigned int idx) const { return *(begin() + idx); }

    static stack_type* from_data(char* data) { return reinterpret_cast<stack_type*>(data); }

    char buffer[sizeof(torrent::Object) * max_arguments];
  };

  command_base() { new (&_pod<base_function>()) base_function(); }
  command_base(const command_base& src) { new (&_pod<base_function>()) base_function(src._pod<base_function>()); }

  ~command_base() { _pod<base_function>().~base_function(); }

  static torrent::Object* argument(unsigned int index) { return current_stack.begin() + index; }
  static torrent::Object& argument_ref(unsigned int index) { return *(current_stack.begin() + index); }

  static stack_type current_stack;

  static torrent::Object* stack_begin() { return current_stack.begin(); }
  static torrent::Object* stack_end()   { return current_stack.end(); }

  static torrent::Object* push_stack(const torrent::Object::list_type& args, stack_type* stack);
  static torrent::Object* push_stack(const torrent::Object* first_arg, const torrent::Object* last_arg, stack_type* stack);
  static void             pop_stack(stack_type* stack, torrent::Object* last_stack);

  template <typename T>
  void set_function(T s, int value = command_base_is_valid<T>::value) { _pod<T>() = s; }

  template <command_base_call_type T>
  void set_function_2(typename command_base_is_type<T>::type s, int value = command_base_is_valid<typename command_base_is_type<T>::type>::value) {
    _pod<typename command_base_is_type<T>::type>() = s;
  }

  // The std::function object in GCC is castable between types with a
  // pointer to a struct of ctor/dtor/calls for non-POD slots. As such
  // it should be safe to cast between different std::function
  // template types, yet what the C++0x standard will say about this I
  // have no idea atm.
  template <typename tmpl> tmpl& _pod() { return reinterpret_cast<tmpl&>(t_pod); }
  template <typename tmpl> const tmpl& _pod() const { return reinterpret_cast<const tmpl&>(t_pod); }

  template <typename Func, typename T, typename Args>
  static const torrent::Object _call(command_base* cmd, target_type target, Args args);

  command_base& operator = (const command_base& src) {
    _pod<base_function>() = src._pod<base_function>();
    return *this;
  }

protected:
  // For use by functions that need to use placeholders to arguments
  // within commands. E.d. callable command strings where one of the
  // arguments within the command needs to be supplied by the caller.

#ifdef HAVE_CXX11
  union {
    base_function t_pod;
    // char t_pod[sizeof(base_function)];
  };
#else
  union {
    char t_pod[sizeof(base_function)];
  };
#endif
};

template <typename T1 = void, typename T2 = void>
struct target_type_id {
  // Nothing here, so we cause an error.
};

template <typename T> inline bool
is_target_compatible(const target_type& target) { return target.first == target_type_id<T>::value; }

// Splitting pairs into separate targets.
inline bool is_target_pair(const target_type& target) { return target.first >= command_base::target_download_pair; }

template <typename T> inline T
get_target_cast(target_type target, int type = target_type_id<T>::value) { return (T)target.second; }

inline target_type get_target_left(const target_type& target)  { return target_type(target.first - 5, target.second); }
inline target_type get_target_right(const target_type& target) { return target_type(target.first - 5, target.third); }

}

#include "command_impl.h"

namespace rpc {

template <typename Func, typename T, typename Args>
inline const torrent::Object
command_base::_call(command_base* cmd, target_type target, Args args) {
  return static_cast<command_base*>(cmd)->_pod<Func>()(get_target_cast<T>(target), args);
}

#define COMMAND_BASE_TEMPLATE_TYPE(func_type, func_parm)                \
  template <typename T, int proper = target_type_id<T>::proper_type> struct func_type { typedef std::function<func_parm> type; }; \
                                                                        \
  template <> struct command_base_is_valid<func_type<target_type>::type>                { static const int value = 1; }; \
  template <> struct command_base_is_valid<func_type<core::Download*>::type>            { static const int value = 1; }; \
  template <> struct command_base_is_valid<func_type<torrent::Peer*>::type>             { static const int value = 1; }; \
  template <> struct command_base_is_valid<func_type<torrent::Tracker*>::type>          { static const int value = 1; }; \
  template <> struct command_base_is_valid<func_type<torrent::File*>::type>             { static const int value = 1; }; \
  template <> struct command_base_is_valid<func_type<torrent::FileListIterator*>::type> { static const int value = 1; };

//  template <typename Q> struct command_base_is_valid<typename func_type<Q>::type > { static const int value = 1; };

COMMAND_BASE_TEMPLATE_TYPE(command_function,        torrent::Object (T, const torrent::Object&));
COMMAND_BASE_TEMPLATE_TYPE(command_value_function,  torrent::Object (T, const torrent::Object::value_type&));
COMMAND_BASE_TEMPLATE_TYPE(command_string_function, torrent::Object (T, const std::string&));
COMMAND_BASE_TEMPLATE_TYPE(command_list_function,   torrent::Object (T, const torrent::Object::list_type&));

#define COMMAND_BASE_TEMPLATE_CALL(func_name, func_type)                \
  template <typename T> const torrent::Object func_name(command_base* rawCommand, target_type target, const torrent::Object& args); \
                                                                        \
  template <> struct command_base_is_type<func_name<target_type> >       { static const int value = 1; typedef func_type<target_type>::type type; }; \
  template <> struct command_base_is_type<func_name<core::Download*> >   { static const int value = 1; typedef func_type<core::Download*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::Peer*> >    { static const int value = 1; typedef func_type<torrent::Peer*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::Tracker*> > { static const int value = 1; typedef func_type<torrent::Tracker*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::File*> >    { static const int value = 1; typedef func_type<torrent::File*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::FileListIterator*> > { static const int value = 1; typedef func_type<torrent::FileListIterator*>::type type; };

COMMAND_BASE_TEMPLATE_CALL(command_base_call, command_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_value, command_value_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_value_kb, command_value_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_string, command_string_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_list, command_list_function);

}

#endif
