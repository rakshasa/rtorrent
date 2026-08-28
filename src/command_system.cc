#include "config.h"

#include <unistd.h>
#include <sys/stat.h>
#include <torrent/data/file_manager.h>
#include <torrent/runtime/client_config.h>
#include <torrent/runtime/memory_manager.h>
#include <torrent/runtime/socket_manager.h>
#include <torrent/runtime/runtime.h>
#include <torrent/utils/chrono.h>
#include <torrent/utils/option_strings.h>

#include "command_helpers.h"
#include "control.h"
#include "globals.h"
#include "core/manager.h"
#include "session/session_manager.h"
#include "utils/file_status_cache.h"


torrent::Object
system_env(const torrent::Object::string_type& arg) {
  if (arg.empty())
    throw torrent::input_error("system.env: Missing variable name.");

  char* val = getenv(arg.c_str());
  return std::string(val ? val : "");
}

torrent::Object
system_hostname() {
  char buffer[1024];

  if (gethostname(buffer, 1023) == -1)
    throw torrent::input_error("Unable to read hostname.");

//   if (shorten)
//     *std::find(buffer, buffer + 1023, '.') = '\0';

  return std::string(buffer);
}

torrent::Object
system_get_cwd() {
  char* buffer = getcwd(NULL, 0);

  if (buffer == NULL)
    throw torrent::input_error("Unable to read cwd.");

  torrent::Object result = torrent::Object(std::string(buffer));
  free(buffer);

  return result;
}

torrent::Object
system_set_cwd(const torrent::Object::string_type& rawArgs) {
  if (::chdir(rawArgs.c_str()) != 0)
    throw torrent::input_error("Could not change current working directory.");

  return torrent::Object();
}

uint32_t
checked_socket_value(int64_t value, const char* label) {
  if (value < 0 || value > std::numeric_limits<uint32_t>::max())
    throw torrent::input_error(std::string("Invalid ") + label + " value.");

  return static_cast<uint32_t>(value);
}

void
initialize_command_system() {
  CMD_ANY         ("system.hostname", std::bind(&system_hostname));
  CMD_ANY         ("system.pid",      std::bind(&getpid));

  CMD_VAR_C_STRING("system.api_version",           (int64_t)API_VERSION);
  CMD_VAR_C_STRING("system.client_version",        PACKAGE_VERSION);
  CMD_VAR_C_STRING("system.library_version",       torrent::runtime::version());

  CMD_ANY         ("system.torrent_name.replace_slash",     [](auto, auto)        { return torrent::runtime::client_config()->torrent_name_use_sanitized(); });
  CMD_ANY_VALUE_V ("system.torrent_name.replace_slash.set", [](auto, auto& value) { return torrent::runtime::client_config()->set_torrent_name_use_sanitized(value); });

  CMD_VAR_VALUE   ("system.file.allocate",         0);
  CMD_VAR_VALUE   ("system.file.max_size",         (int64_t)512 << 30);
  CMD_VAR_VALUE   ("system.file.split_size",       -1);
  CMD_VAR_STRING  ("system.file.split_suffix",     ".part");

  CMD_ANY         ("system.file_name.replace_slash",     [](auto, auto)      { return torrent::runtime::client_config()->file_name_replace_slash(); });
  CMD_ANY_STRING_V("system.file_name.replace_slash.set", [](auto, auto& str) { return torrent::runtime::client_config()->set_file_name_replace_slash(str); });

  CMD_ANY         ("system.file_status_cache.size",      [](auto, auto)      { return control->core()->file_status_cache()->size(); });
  CMD_ANY_V       ("system.file_status_cache.prune",     [](auto, auto)      { return control->core()->file_status_cache()->prune(); });

  CMD_VAR_BOOL    ("file.prioritize_toc",          0);
  CMD_VAR_LIST    ("file.prioritize_toc.first");
  CMD_VAR_LIST    ("file.prioritize_toc.last");

  CMD_ANY         ("system.files.advise_random",             [](auto, auto)        { return torrent::file_manager()->advise_random(); });
  CMD_ANY_VALUE_V ("system.files.advise_random.set",         [](auto, auto& value) { return torrent::file_manager()->set_advise_random(value); });
  CMD_ANY         ("system.files.advise_random.hashing",     [](auto, auto)        { return torrent::file_manager()->advise_random_hashing(); });
  CMD_ANY_VALUE_V ("system.files.advise_random.hashing.set", [](auto, auto& value) { return torrent::file_manager()->set_advise_random_hashing(value); });
  CMD_ANY         ("system.files.session.fdatasync",         [](auto, auto)        { return session_thread::manager()->use_fsyncdisk(); });
  CMD_ANY_VALUE_V ("system.files.session.fdatasync.set",     [](auto, auto& value) { return session_thread::manager()->set_use_fsyncdisk(value); });

  CMD_ANY         ("system.files.close_idle",         [](auto, auto)        { return torrent::file_manager()->close_idle_timeout().count(); });
  CMD_ANY_VALUE_V ("system.files.close_idle.set",     [](auto, auto& value) { return torrent::file_manager()->set_close_idle_timeout(value * 1s); });

  CMD_ANY         ("system.files.opened_counter",     [](auto, auto)        { return torrent::file_manager()->files_opened_counter(); });
  CMD_ANY         ("system.files.closed_counter",     [](auto, auto)        { return torrent::file_manager()->files_closed_counter(); });
  CMD_ANY         ("system.files.failed_counter",     [](auto, auto)        { return torrent::file_manager()->files_failed_counter(); });

  CMD_ANY_STRING  ("system.env",                      [](auto, auto& str)   { return system_env(str); });

  CMD_ANY         ("system.time",                     [](auto, auto)        { return torrent::this_thread::cached_seconds().count(); });
  CMD_ANY         ("system.time_seconds",             [](auto, auto)        { return torrent::utils::cast_seconds(torrent::utils::time_since_epoch()).count(); });
  CMD_ANY         ("system.time_usec",                [](auto, auto)        { return torrent::utils::time_since_epoch().count(); });

  CMD_ANY_VALUE_V ("system.umask.set",                [](auto, auto& value) { return ::umask(value); });

  CMD_VAR_BOOL    ("system.daemon",                   false);

  CMD_ANY_V       ("system.shutdown.normal",          [](auto, auto)        { control->receive_normal_shutdown(); });
  CMD_ANY_V       ("system.shutdown.quick",           [](auto, auto)        { control->receive_quick_shutdown(); });

  CMD_REDIRECT_NO_EXPORT("system.shutdown", "system.shutdown.normal");

  CMD_ANY         ("system.cwd",                      [](auto, auto)        { return system_get_cwd(); });
  CMD_ANY_STRING  ("system.cwd.set",                  [](auto, auto& str)   { return system_set_cwd(str); });

  CMD_ANY         ("system.sockets.size",             [](auto, auto)        { return torrent::runtime::socket_manager()->size(); });
  CMD_ANY         ("system.sockets.max_size",         [](auto, auto)        { return torrent::runtime::socket_manager()->max_size(); });
  CMD_ANY_VALUE_V ("system.sockets.max_size.set",     [](auto, auto& value) {
      return torrent::runtime::socket_manager()->set_max_size_and_adjust(checked_socket_value(value, "socket max size"));
    });
  CMD_ANY_V       ("system.sockets.adjust_alloc",     [](auto, auto)        { torrent::runtime::socket_manager()->adjust_allocation(); });
  CMD_ANY         ("system.sockets.reserved_alloc",   [](auto, auto)        { return torrent::runtime::socket_manager()->reserved_allocation(); });
  CMD_ANY         ("system.sockets.available_alloc",  [](auto, auto)        { return torrent::runtime::socket_manager()->available_allocation(); });

  for (uint32_t i = 0; i < torrent::runtime::SocketManager::category_count; ++i) {
    auto category      = static_cast<torrent::runtime::socket_manager_category_t>(i);
    auto category_name = "system.sockets." + torrent::option_to_str_or_throw(torrent::OPTION_SOCKET_CATEGORY, i);

    CMD_ANY        (category_name + ".size",      [category](auto, auto) { return torrent::runtime::socket_manager()->category_managed_size(category); });
    CMD_ANY        (category_name + ".max_size",  [category](auto, auto) { return torrent::runtime::socket_manager()->category_max_size(category); });

    if (i == 0) {
      CMD_ANY      (category_name + ".min_alloc", [](auto, auto) { return torrent::runtime::socket_manager()->generic_min_allocation(); });
      continue;
    }

    CMD_ANY        (category_name + ".max_alloc.limit", [category](auto, auto)        { return torrent::runtime::socket_manager()->category_alloc_limit(category); });
    CMD_ANY        (category_name + ".min_alloc.limit", [category](auto, auto)        { return torrent::runtime::socket_manager()->category_alloc_minimum(category); });
    CMD_ANY        (category_name + ".min_alloc",       [category](auto, auto)        { return torrent::runtime::socket_manager()->category_min_allocation(category); });
    CMD_ANY        (category_name + ".max_alloc",       [category](auto, auto)        { return torrent::runtime::socket_manager()->category_max_allocation(category); });
    CMD_ANY_VALUE_V(category_name + ".min_alloc.set",   [category](auto, auto& value) {
        torrent::runtime::socket_manager()->set_category_min_allocation(category, checked_socket_value(value, "socket min alloc"));
      });
    CMD_ANY_VALUE_V(category_name + ".max_alloc.set",   [category](auto, auto& value) {
        torrent::runtime::socket_manager()->set_category_max_allocation(category, checked_socket_value(value, "socket max alloc"));
      });
  }

  rpc::rpc.mark_safe("system.api_version");
  rpc::rpc.mark_safe("system.client_version");
  rpc::rpc.mark_safe("system.library_version");
  rpc::rpc.mark_safe("system.time");
  rpc::rpc.mark_safe("system.time_seconds");
  rpc::rpc.mark_safe("system.time_usec");
  rpc::rpc.mark_safe("system.file.max_size");
  rpc::rpc.mark_safe("system.file.split_size");
  rpc::rpc.mark_safe("system.file.split_suffix");

  rpc::rpc.mark_safe("system.sockets.size");
  rpc::rpc.mark_safe("system.sockets.max_size");
  rpc::rpc.mark_safe("system.sockets.reserved_alloc");
  rpc::rpc.mark_safe("system.sockets.available_alloc");

  for (uint32_t i = 0; i < torrent::runtime::SocketManager::category_count; ++i) {
    auto category_name = "system.sockets." + torrent::option_to_str_or_throw(torrent::OPTION_SOCKET_CATEGORY, i);

    rpc::rpc.mark_safe(category_name + ".size");
    rpc::rpc.mark_safe(category_name + ".max_size");

    if (i == 0) {
      rpc::rpc.mark_safe(category_name + ".min_alloc");
      continue;
    }

    rpc::rpc.mark_safe(category_name + ".max_alloc.limit");
    rpc::rpc.mark_safe(category_name + ".min_alloc.limit");
    rpc::rpc.mark_safe(category_name + ".min_alloc");
    rpc::rpc.mark_safe(category_name + ".max_alloc");
  }
}
