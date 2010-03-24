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

// The command_new_slot object aims at replacing the current crop of
// Command* objects with a new type that is safe to cast from the base
// command type and thus allows for static initialization and fixed
// sized objects.
//
// All commands changed to this new class shall be safe to call with
// the raw object types.

#ifndef RTORRENT_RPC_COMMAND_NEW_SLOT_H
#define RTORRENT_RPC_COMMAND_NEW_SLOT_H

#include <functional>
#include <limits>
#include <inttypes.h>
#include <torrent/object.h>
#include <tr1/functional>

#include "command.h"

namespace rpc {

typedef const torrent::Object (*command_base_call_type)(Command*, target_type, const torrent::Object&);
typedef std::tr1::function<torrent::Object (target_type, const torrent::Object&)> base_function;

template <typename tmpl> struct command_base_is_valid {};
template <command_base_call_type tmpl_func> struct command_base_is_type {};

class command_base : public Command {
public:
  command_base() { new (&_pod<base_function>()) base_function(); }
  ~command_base() { _pod<base_function>().~base_function(); }

  template <typename T>
  void set_function(T s, int value = command_base_is_valid<T>::value) { _pod<T>() = s; }

  // The std::function object in GCC is castable between types with a
  // pointer to a struct of ctor/dtor/calls for non-POD slots. As such
  // it should be safe to cast between different std::function
  // template types, yet what the C++0x standard will say about this I
  // have no idea atm.
  template <typename tmpl> tmpl& _pod() { return reinterpret_cast<tmpl&>(t_pod); }

  template <typename Func, typename T, typename Args>
  static const torrent::Object _call(Command* cmd, target_type target, Args args) {
    return static_cast<command_base*>(cmd)->_pod<Func>()(get_target_cast<T>(target), args);
  }

protected:

  union {
    char t_pod[sizeof(base_function)];
  };
};

#define COMMAND_BASE_TEMPLATE_TYPE(func_type, func_parm)                \
  template <typename T, int proper = target_type_id<T>::proper_type> struct func_type { typedef std::tr1::function<func_parm> type; }; \
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
  template <typename T> const torrent::Object func_name(Command* rawCommand, target_type target, const torrent::Object& args); \
                                                                        \
  template <> struct command_base_is_type<func_name<target_type> >       { static const int value = 1; typedef func_type<target_type>::type type; }; \
  template <> struct command_base_is_type<func_name<core::Download*> >   { static const int value = 1; typedef func_type<core::Download*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::Peer*> >    { static const int value = 1; typedef func_type<torrent::Peer*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::Tracker*> > { static const int value = 1; typedef func_type<torrent::Tracker*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::File*> >    { static const int value = 1; typedef func_type<torrent::File*>::type type; }; \
  template <> struct command_base_is_type<func_name<torrent::FileListIterator*> > { static const int value = 1; typedef func_type<torrent::FileListIterator*>::type type; };

COMMAND_BASE_TEMPLATE_CALL(command_base_call, command_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_value, command_value_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_string, command_string_function);
COMMAND_BASE_TEMPLATE_CALL(command_base_call_list, command_list_function);

}

#endif

  
