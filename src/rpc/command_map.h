#ifndef RTORRENT_RPC_COMMAND_MAP_H
#define RTORRENT_RPC_COMMAND_MAP_H

#include <map>
#include <string>
#include <cstring>
#include <torrent/object.h>

#include "command.h"

namespace rpc {

struct command_map_data_type {
  // Some commands will need to share data, like get/set a variable. So
  // instead of using a single virtual member function, each command
  // will register a member function pointer to be used instead.
  //
  // The any_slot should perhaps replace generic_slot?

  command_map_data_type(int flags, const char* parm, const char* doc) :
    m_flags(flags), m_parm(parm), m_doc(doc) {}

  command_map_data_type(const command_map_data_type& src) = default;

  command_base             m_variable;
  command_base::any_slot   m_anySlot;

  int           m_flags;

  const char*   m_parm;
  const char*   m_doc;
};

class CommandMap : public std::map<std::string, command_map_data_type> {
public:
  typedef std::map<std::string, command_map_data_type> base_type;

  typedef torrent::Object         mapped_type;
  typedef mapped_type::value_type mapped_value_type;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::key_type;
  using base_type::value_type;

  using base_type::begin;
  using base_type::end;
  using base_type::find;

  static const int flag_dont_delete   = 0x1;
  static const int flag_public_rpc    = 0x4;
  static const int flag_modifiable    = 0x10;
  static const int flag_is_redirect   = 0x20;
  static const int flag_has_redirects = 0x40;

  static const int flag_file_target    = 0x100;
  static const int flag_tracker_target = 0x200;

  CommandMap() = default;

  bool                has(const std::string& key) const { return base_type::find(key) != base_type::end(); }

  bool                is_modifiable(const_iterator itr) { return itr != end() && (itr->second.m_flags & flag_modifiable); }

  iterator            insert(const key_type& key, int flags, const char* parm, const char* doc);

  template <typename T, typename Slot>
  void
  insert_slot(const key_type& key, Slot variable, command_base::any_slot target_slot, int flags, const char* parm, const char* doc) {
    iterator itr = insert(key, flags, parm, doc);
    itr->second.m_variable.set_function<T>(variable);
    itr->second.m_anySlot = target_slot;
  }

  void                erase(iterator itr);

  void                create_redirect(const key_type& key_new, const key_type& key_dest, int flags);

  const mapped_type   call(const key_type& key, const mapped_type& args = mapped_type());
  const mapped_type   call(const key_type& key, const target_type& target, const mapped_type& args = mapped_type()) { return call_command(key, args, target); }
  const mapped_type   call_catch(const key_type& key, const target_type& target, const mapped_type& args = mapped_type(), const char* err = "Command failed: ");

  const mapped_type   call_command  (const key_type& key, const mapped_type& arg, const target_type& target = target_type((int)command_base::target_generic, NULL));
  const mapped_type   call_command  (iterator itr, const mapped_type& arg, const target_type& target = target_type((int)command_base::target_generic, NULL));

  const mapped_type   call_command_d(const key_type& key, core::Download* download, const mapped_type& arg)  { return call_command(key, arg, target_type((int)command_base::target_download, download)); }
  const mapped_type   call_command_p(const key_type& key, torrent::Peer* peer, const mapped_type& arg)       { return call_command(key, arg, target_type((int)command_base::target_peer, peer)); }
  const mapped_type   call_command_t(const key_type& key, torrent::tracker::Tracker* tracker, const mapped_type& arg) { return call_command(key, arg, target_type((int)command_base::target_tracker, tracker)); }
  const mapped_type   call_command_f(const key_type& key, torrent::File* file, const mapped_type& arg)       { return call_command(key, arg, target_type((int)command_base::target_file, file)); }

private:
  CommandMap(const CommandMap&);
  void operator = (const CommandMap&);
};

inline target_type make_target()                                  { return target_type((int)command_base::target_generic, NULL); }
inline target_type make_target(int type, void* target)            { return target_type(type, target); }
inline target_type make_target(int type, void* target1, void* target2) { return target_type(type, target1, target2); }

template <typename T>
inline target_type make_target(T target) {
  return target_type((int)target_type_id<T>::value, target);
}

template <typename T>
inline target_type make_target_pair(T target1, T target2) {
  return target_type((int)target_type_id<T, T>::value, target1, target2);
}

// TODO: Helper-functions that really should be in the
// torrent/object.h header.

inline torrent::Object
create_object_list(const torrent::Object& o1, const torrent::Object& o2) {
  torrent::Object tmp = torrent::Object::create_list();
  tmp.as_list().push_back(o1);
  tmp.as_list().push_back(o2);
  return tmp;
}

inline torrent::Object
create_object_list(const torrent::Object& o1, const torrent::Object& o2, const torrent::Object& o3) {
  torrent::Object tmp = torrent::Object::create_list();
  tmp.as_list().push_back(o1);
  tmp.as_list().push_back(o2);
  tmp.as_list().push_back(o3);
  return tmp;
}

inline const CommandMap::mapped_type
CommandMap::call(const key_type& key, const mapped_type& args) {
  return call_command(key, args, make_target());
}

}

#endif
