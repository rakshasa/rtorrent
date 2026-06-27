#include "config.h"

#include "core/download.h"
#include "parse.h"

#include "command.h"

#define COMMAND_BASE_TEMPLATE_DEFINE(func_name) \
template const torrent::Object func_name<target_type>(command_base* command_raw, target_type target, const torrent::Object& args); \
template const torrent::Object func_name<core::Download*>(command_base* command_raw, target_type target, const torrent::Object& args); \
template const torrent::Object func_name<torrent::Peer*>(command_base* command_raw, target_type target, const torrent::Object& args); \
template const torrent::Object func_name<torrent::tracker::Tracker*>(command_base* command_raw, target_type target, const torrent::Object& args); \
template const torrent::Object func_name<torrent::File*>(command_base* command_raw, target_type target, const torrent::Object& args); \
template const torrent::Object func_name<torrent::FileListIterator*>(command_base* command_raw, target_type target, const torrent::Object& args);

namespace rpc {

template <typename T> const torrent::Object
command_base_call(command_base* command_raw, target_type target, const torrent::Object& args) {
  if (!is_target_compatible<T>(target))
    throw torrent::input_error("Target of wrong type to generic command.");

  return command_base::_call<typename command_function<T>::type, T>(command_raw, target, args);
}

COMMAND_BASE_TEMPLATE_DEFINE(command_base_call);

template <typename T> const torrent::Object
command_base_call_value_base(command_base* command_raw, target_type target, const torrent::Object& args_raw, int base, int unit) {
  if (!is_target_compatible<T>(target))
    throw torrent::input_error("Target of wrong type to value command.");

  auto& arg = convert_to_single_argument(args_raw);

  if (arg.type() == torrent::Object::TYPE_STRING) {
    torrent::Object::value_type val;

    if (!parse_whole_value_nothrow(arg.as_string().c_str(), &val, base, unit))
      throw torrent::input_error("Not a value.");

    return command_base::_call<typename command_value_function<T>::type, T>(command_raw, target, val);
  }

  return command_base::_call<typename command_value_function<T>::type, T>(command_raw, target, unit * arg.as_value());
}

template <typename T> const torrent::Object
command_base_call_value(command_base* command_raw, target_type target, const torrent::Object& args_raw) {
  return command_base_call_value_base<T>(command_raw, target, args_raw, 0, 1);
}

template <typename T> const torrent::Object
command_base_call_value_kb(command_base* command_raw, target_type target, const torrent::Object& args_raw) {
  return command_base_call_value_base<T>(command_raw, target, args_raw, 0, 1024);
}

COMMAND_BASE_TEMPLATE_DEFINE(command_base_call_value);
COMMAND_BASE_TEMPLATE_DEFINE(command_base_call_value_kb);

template <typename T> const torrent::Object
command_base_call_string(command_base* command_raw, target_type target, const torrent::Object& args_raw) {
  if (!is_target_compatible<T>(target))
    throw torrent::input_error("Target of wrong type to string command.");

  auto& arg = convert_to_single_argument(args_raw);

  if (arg.type() == torrent::Object::TYPE_RAW_STRING)
    return command_base::_call<typename command_string_function<T>::type, T>(command_raw, target, arg.as_raw_string().as_string());

  return command_base::_call<typename command_string_function<T>::type, T>(command_raw, target, arg.as_string());
}

COMMAND_BASE_TEMPLATE_DEFINE(command_base_call_string);

template <typename T> const torrent::Object
command_base_call_list(command_base* command_raw, target_type target, const torrent::Object& args_raw) {
  if (!is_target_compatible<T>(target))
    throw torrent::input_error("Target of wrong type to list command.");

  if (args_raw.type() != torrent::Object::TYPE_LIST) {
    torrent::Object::list_type arg;

    if (!args_raw.is_empty())
      arg.push_back(args_raw);

    return command_base::_call<typename command_list_function<T>::type, T>(command_raw, target, arg);
  }

  return command_base::_call<typename command_list_function<T>::type, T>(command_raw, target, args_raw.as_list());
}

COMMAND_BASE_TEMPLATE_DEFINE(command_base_call_list);

}
