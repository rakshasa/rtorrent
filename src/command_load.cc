#include "config.h"

#include "command_helpers.h"
#include "control.h"
#include "core/manager.h"

torrent::Object
apply_load_trusted(const torrent::Object::list_type& args, int flags) {
  auto argsItr = args.begin();

  if (argsItr == args.end())
    throw torrent::input_error("Too few arguments.");

  auto& filename = argsItr->as_string();

  core::Manager::command_list_type commands;

  while (++argsItr != args.end())
    commands.push_back(argsItr->as_string());

  control->core()->try_create_download_expand(filename, flags, commands);

  return torrent::Object();
}

torrent::Object
apply_load_untrusted(const torrent::Object::list_type& args, int flags) {
  auto argsItr = args.begin();

  if (argsItr == args.end())
    throw torrent::input_error("Too few arguments.");

  auto& filename = argsItr->as_string();

  core::Manager::command_list_type commands;

  while (++argsItr != args.end())
    commands.push_back(argsItr->as_string());

  control->core()->try_create_download_untrusted(filename, flags, commands);

  return torrent::Object();
}

void
initialize_command_load() {
  CMD2_ANY_LIST    ("load.normal",            [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_quiet | core::Manager::create_tied); });
  CMD2_ANY_LIST    ("load.verbose",           [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_tied); });
  CMD2_ANY_LIST    ("load.start",             [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_quiet | core::Manager::create_tied | core::Manager::create_start); });
  CMD2_ANY_LIST    ("load.start_verbose",     [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_tied | core::Manager::create_start); });
  CMD2_ANY_LIST    ("load.raw",               [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_quiet | core::Manager::create_raw_data); });
  CMD2_ANY_LIST    ("load.raw_verbose",       [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_raw_data); });
  CMD2_ANY_LIST    ("load.raw_start",         [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_quiet | core::Manager::create_start | core::Manager::create_raw_data); });
  CMD2_ANY_LIST    ("load.raw_start_verbose", [](auto, auto& args) { return apply_load_trusted(args, core::Manager::create_start | core::Manager::create_raw_data); });

  CMD2_ANY_LIST    ("load.untrusted.normal",            [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_quiet); });
  CMD2_ANY_LIST    ("load.untrusted.verbose",           [](auto, auto& args) { return apply_load_untrusted(args, 0); });
  CMD2_ANY_LIST    ("load.untrusted.start",             [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_quiet | core::Manager::create_start); });
  CMD2_ANY_LIST    ("load.untrusted.start_verbose",     [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_start); });
  CMD2_ANY_LIST    ("load.untrusted.raw",               [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_quiet | core::Manager::create_raw_data); });
  CMD2_ANY_LIST    ("load.untrusted.raw_verbose",       [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_raw_data); });
  CMD2_ANY_LIST    ("load.untrusted.raw_start",         [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_quiet | core::Manager::create_start | core::Manager::create_raw_data); });
  CMD2_ANY_LIST    ("load.untrusted.raw_start_verbose", [](auto, auto& args) { return apply_load_untrusted(args, core::Manager::create_start | core::Manager::create_raw_data); });

  // TODO: Add a command to mark load.untrusted.* as safe.
}
