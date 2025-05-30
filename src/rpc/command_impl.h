#ifndef RTORRENT_RPC_COMMAND_IMPL_H
#define RTORRENT_RPC_COMMAND_IMPL_H

#include "rpc/command.h"

namespace rpc {

//template <> struct target_type_id<command_base::generic_slot>       { static const int value = command_base::target_generic; };
template <> struct target_type_id<command_base::cleaned_slot>       { static const int value = command_base::target_generic; };
template <> struct target_type_id<command_base::any_slot>           { static const int value = command_base::target_any; };
template <> struct target_type_id<command_base::download_slot>      { static const int value = command_base::target_download; };
template <> struct target_type_id<command_base::peer_slot>          { static const int value = command_base::target_peer; };
template <> struct target_type_id<command_base::tracker_slot>       { static const int value = command_base::target_tracker; };
template <> struct target_type_id<command_base::file_slot>          { static const int value = command_base::target_file; };
template <> struct target_type_id<command_base::file_itr_slot>      { static const int value = command_base::target_file_itr; };

template <> struct target_type_id<command_base::download_pair_slot> { static const int value = command_base::target_download_pair; };

template <> struct target_type_id<>                            { static const int value = command_base::target_generic; };
template <> struct target_type_id<target_type>                 { static const int value = command_base::target_any;      static const int proper_type = 1; };
template <> struct target_type_id<core::Download*>             { static const int value = command_base::target_download; static const int proper_type = 1; };
template <> struct target_type_id<torrent::Peer*>              { static const int value = command_base::target_peer;     static const int proper_type = 1; };
template <> struct target_type_id<torrent::tracker::Tracker*>  { static const int value = command_base::target_tracker;  static const int proper_type = 1; };
template <> struct target_type_id<torrent::File*>              { static const int value = command_base::target_file;     static const int proper_type = 1; };
template <> struct target_type_id<torrent::FileListIterator*>  { static const int value = command_base::target_file_itr; static const int proper_type = 1; };

template <> struct target_type_id<core::Download*, core::Download*> { static const int value = command_base::target_download_pair; };

template <> inline bool
is_target_compatible<target_type>(const target_type& target) { return true; }
template <> inline bool
is_target_compatible<torrent::File*>(const target_type& target) { return target.first == command_base::target_file || target.first == command_base::target_file_itr; }

template <> inline target_type
get_target_cast<target_type>(target_type target, int type) { return target; }

template <> inline torrent::File*
get_target_cast<torrent::File*>(target_type target, int type) {
  if (target.first == command_base::target_file_itr)
    return static_cast<torrent::FileListIterator*>(target.second)->file();
  else
    return static_cast<torrent::File*>(target.second);
}

inline torrent::Object*
command_base::push_stack(const torrent::Object* first_arg, const torrent::Object* last_arg, stack_type* stack) {
  unsigned int idx = 0;

  while (first_arg != last_arg && idx < command_base::max_arguments) {
    new (&(*stack)[idx]) torrent::Object(*first_arg++);
    (*stack)[idx].swap(*command_base::argument(idx));

    idx++;
  }

  return stack->begin() + idx;
}

inline torrent::Object*
command_base::push_stack(const torrent::Object::list_type& args, stack_type* stack) {
  return push_stack(args.data(), args.data() + args.size(), stack);
}

inline void
command_base::pop_stack(stack_type* stack, torrent::Object* last_stack) {
  while (last_stack-- != stack->begin()) {
    last_stack->swap(*command_base::argument(std::distance(stack->begin(), last_stack)));
    last_stack->~Object();

    // To ensure we catch errors:
    std::memset((void*)last_stack, 0xAA, sizeof(torrent::Object));
  }
}

}

#endif
