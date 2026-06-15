#include "config.h"

#include <cerrno>
#include <fcntl.h>
#include <functional>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <torrent/torrent.h>
#include <torrent/chunk_manager.h>
#include <torrent/data/file_manager.h>
#include <torrent/data/chunk_utils.h>
#include <torrent/runtime/runtime.h>
#include <torrent/runtime/socket_manager.h>
#include <torrent/utils/chrono.h>
#include <torrent/utils/option_strings.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "rpc/parse_commands.h"
#include "rpc/scgi.h"
#include "session/session_manager.h"
#include "utils/file_status_cache.h"

#include "globals.h"
#include "rpc/lua.h"
#include "control.h"
#include "command_helpers.h"

typedef torrent::ChunkManager CM_t;
typedef torrent::FileManager  FM_t;

torrent::Object
apply_pieces_stats_total_size() {
  uint64_t size = 0;

  for (const auto& d : *control->core()->download_list())
    if (d->is_active())
      size += d->file_list()->size_bytes();

  return size;
}

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

inline torrent::Object::list_const_iterator
post_increment(torrent::Object::list_const_iterator& itr, const torrent::Object::list_const_iterator& last) {
  if (itr == last)
    throw torrent::input_error("Invalid number of arguments.");

  return itr++;
}

inline const std::string&
check_name(const std::string& str) {
  auto itr = std::find_if(str.begin(), str.end(), [](char c) {
      return !std::isalnum(c, std::locale::classic()) && c != '_';
    });

  if (itr != str.end())
    throw torrent::input_error("Invalid characters found in name.");

  return str;
}

torrent::Object
group_insert(const torrent::Object::list_type& args) {
  torrent::Object::list_const_iterator itr = args.begin();
  torrent::Object::list_const_iterator last = args.end();

  const std::string& name = check_name(post_increment(itr, last)->as_string());
  const std::string& view = check_name(post_increment(itr, last)->as_string());

  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.enable", "simple",
                                                              "schedule=group." + name + ".ratio,5,60,on_ratio=" + name));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.disable", "simple",
                                                              "schedule_remove=group." + name + ".ratio"));
  rpc::commands.call("method.insert", rpc::create_object_list("group."  + name + ".ratio.command", "simple",
                                                              "d.try_close= ;d.ignore_commands.set=1"));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".view", "string", view));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.min", "value", (int64_t)200));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.max", "value", (int64_t)300));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.upload", "value", (int64_t)20 << 20));

  rpc::rpc.mark_safe("group." + name + ".view");
  rpc::rpc.mark_safe("group." + name + ".view.set");
  rpc::rpc.mark_safe("group." + name + ".ratio.min");
  rpc::rpc.mark_safe("group." + name + ".ratio.min.set");
  rpc::rpc.mark_safe("group." + name + ".ratio.max");
  rpc::rpc.mark_safe("group." + name + ".ratio.max.set");
  rpc::rpc.mark_safe("group." + name + ".ratio.upload");
  rpc::rpc.mark_safe("group." + name + ".ratio.upload.set");

  return name;
}

static const int file_print_use_space = 0x1;
static const int file_print_delim_space = 0x2;

void
file_print_list(torrent::Object::list_const_iterator first, torrent::Object::list_const_iterator last, FILE* output, int flags) {
  while (first != last) {
    switch (first->type()) {
    case torrent::Object::TYPE_STRING:
      fprintf(output, (const char*)" %s" + !(flags & file_print_use_space), first->as_string().c_str());
      break;
    case torrent::Object::TYPE_VALUE:
      fprintf(output, (const char*)" %" PRIi64 + !(flags & file_print_use_space), first->as_value());
      break;
    case torrent::Object::TYPE_LIST:
      file_print_list(first->as_list().begin(), first->as_list().end(), output, 0);
      break;
    case torrent::Object::TYPE_NONE:
      break;
    default:
      throw torrent::input_error("Invalid type.");
    }

    flags |= (flags & file_print_delim_space) >> 1;
    first++;
  }
}

torrent::Object
cmd_file_append(const torrent::Object::list_type& args) {
  if (args.empty())
    throw torrent::input_error("Invalid number of arguments.");

  FILE* output = fopen(args.front().as_string().c_str(), "a");

  if (output == nullptr)
    throw torrent::input_error("Could not append to file '" + args.front().as_string() + "': " + std::strerror(errno));

  try {
    file_print_list(++args.begin(), args.end(), output, file_print_delim_space);
    fprintf(output, "\n");
  } catch (...) {
    fclose(output);
    throw;
  }
  fclose(output);
  return torrent::Object();
}

void
initialize_command_local() {
  core::DownloadList*    dList = control->core()->download_list();
  torrent::ChunkManager* chunkManager = torrent::chunk_manager();
  torrent::FileManager*  fileManager = torrent::file_manager();

  if (rpc::call_command_value("method.use_deprecated") == 1) {
    CMD_ANY_LIST    ("file.append",    std::bind(&cmd_file_append, std::placeholders::_2));
  }

  CMD_ANY         ("system.hostname", std::bind(&system_hostname));
  CMD_ANY         ("system.pid",      std::bind(&getpid));

  CMD_VAR_C_STRING("system.api_version",           (int64_t)API_VERSION);
  CMD_VAR_C_STRING("system.client_version",        PACKAGE_VERSION);
  CMD_VAR_C_STRING("system.library_version",       torrent::runtime::version());

  CMD_VAR_VALUE   ("system.file.allocate",         0);
  CMD_VAR_VALUE   ("system.file.max_size",         (int64_t)512 << 30);
  CMD_VAR_VALUE   ("system.file.split_size",       -1);
  CMD_VAR_STRING  ("system.file.split_suffix",     ".part");

  CMD_ANY         ("system.file_status_cache.size",   std::bind(&utils::FileStatusCache::size,
                                                                 (utils::FileStatusCache::base_type*)control->core()->file_status_cache()));
  CMD_ANY_V       ("system.file_status_cache.prune",  std::bind(&utils::FileStatusCache::prune, control->core()->file_status_cache()));

  CMD_VAR_BOOL    ("file.prioritize_toc",          0);
  CMD_VAR_LIST    ("file.prioritize_toc.first");
  CMD_VAR_LIST    ("file.prioritize_toc.last");

  CMD_ANY         ("system.files.advise_random",             std::bind(&FM_t::advise_random, fileManager));
  CMD_ANY_VALUE_V ("system.files.advise_random.set",         std::bind(&FM_t::set_advise_random, fileManager, std::placeholders::_2));
  CMD_ANY         ("system.files.advise_random.hashing",     std::bind(&FM_t::advise_random_hashing, fileManager));
  CMD_ANY_VALUE_V ("system.files.advise_random.hashing.set", std::bind(&FM_t::set_advise_random_hashing, fileManager, std::placeholders::_2));
  CMD_ANY         ("system.files.session.fdatasync",         [](auto, auto)        { return session_thread::manager()->use_fsyncdisk(); });
  CMD_ANY_VALUE_V ("system.files.session.fdatasync.set",     [](auto, auto& value) { return session_thread::manager()->set_use_fsyncdisk(value); });

  CMD_ANY         ("system.files.opened_counter",     std::bind(&FM_t::files_opened_counter, fileManager));
  CMD_ANY         ("system.files.closed_counter",     std::bind(&FM_t::files_closed_counter, fileManager));
  CMD_ANY         ("system.files.failed_counter",     std::bind(&FM_t::files_failed_counter, fileManager));

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
  CMD_ANY_VALUE_V ("system.sockets.max_size.set",     [](auto, auto& value) { return torrent::runtime::socket_manager()->set_max_size_and_adjust(value); });

  CMD_ANY         ("system.sockets.generic.size",     [](auto, auto)        { return torrent::runtime::socket_manager()->category_managed_size(torrent::runtime::category_generic); });
  CMD_ANY         ("system.sockets.generic.max_size", [](auto, auto)        { return torrent::runtime::socket_manager()->category_max_size(torrent::runtime::category_generic); });
  CMD_ANY         ("system.sockets.http.size",        [](auto, auto)        { return torrent::runtime::socket_manager()->category_managed_size(torrent::runtime::category_http); });
  CMD_ANY         ("system.sockets.http.max_size",    [](auto, auto)        { return torrent::runtime::socket_manager()->category_max_size(torrent::runtime::category_http); });
  CMD_ANY         ("system.sockets.internal.size",    [](auto, auto)        { return torrent::runtime::socket_manager()->category_managed_size(torrent::runtime::category_internal); });
  CMD_ANY         ("system.sockets.internal.max_size",[](auto, auto)        { return torrent::runtime::socket_manager()->category_max_size(torrent::runtime::category_internal); });
  CMD_ANY         ("system.sockets.scgi.size",        [](auto, auto)        { return torrent::runtime::socket_manager()->category_managed_size(torrent::runtime::category_scgi); });
  CMD_ANY         ("system.sockets.scgi.max_size",    [](auto, auto)        { return torrent::runtime::socket_manager()->category_max_size(torrent::runtime::category_scgi); });
  CMD_ANY         ("system.sockets.files.size",       [](auto, auto)        { return torrent::runtime::socket_manager()->category_managed_size(torrent::runtime::category_files); });
  CMD_ANY         ("system.sockets.files.max_size",   [](auto, auto)        { return torrent::runtime::socket_manager()->category_max_size(torrent::runtime::category_files); });

  CMD_ANY         ("pieces.sync.always_safe",         std::bind(&CM_t::safe_sync, chunkManager));
  CMD_ANY_VALUE_V ("pieces.sync.always_safe.set",     std::bind(&CM_t::set_safe_sync, chunkManager, std::placeholders::_2));
  CMD_ANY         ("pieces.sync.safe_free_diskspace", std::bind(&CM_t::safe_free_diskspace, chunkManager));
  CMD_ANY         ("pieces.sync.timeout",             std::bind(&CM_t::timeout_sync, chunkManager));
  CMD_ANY_VALUE_V ("pieces.sync.timeout.set",         std::bind(&CM_t::set_timeout_sync, chunkManager, std::placeholders::_2));
  CMD_ANY         ("pieces.sync.timeout_safe",        std::bind(&CM_t::timeout_safe_sync, chunkManager));
  CMD_ANY_VALUE_V ("pieces.sync.timeout_safe.set",    std::bind(&CM_t::set_timeout_safe_sync, chunkManager, std::placeholders::_2));
  CMD_ANY         ("pieces.sync.queue_size",          std::bind(&CM_t::sync_queue_size, chunkManager));

  CMD_ANY         ("pieces.preload.type",             std::bind(&CM_t::preload_type, chunkManager));
  CMD_ANY_VALUE_V ("pieces.preload.type.set",         std::bind(&CM_t::set_preload_type, chunkManager, std::placeholders::_2));
  CMD_ANY         ("pieces.preload.min_size",         std::bind(&CM_t::preload_min_size, chunkManager));
  CMD_ANY_VALUE_V ("pieces.preload.min_size.set",     std::bind(&CM_t::set_preload_min_size, chunkManager, std::placeholders::_2));
  CMD_ANY         ("pieces.preload.min_rate",         std::bind(&CM_t::preload_required_rate, chunkManager));
  CMD_ANY_VALUE_V ("pieces.preload.min_rate.set",     std::bind(&CM_t::set_preload_required_rate, chunkManager, std::placeholders::_2));

  CMD_ANY         ("pieces.memory.current",           std::bind(&CM_t::memory_usage, chunkManager));
  CMD_ANY         ("pieces.memory.sync_queue",        std::bind(&CM_t::sync_queue_memory_usage, chunkManager));
  CMD_ANY         ("pieces.memory.block_count",       std::bind(&CM_t::memory_block_count, chunkManager));
  CMD_ANY         ("pieces.memory.max",               std::bind(&CM_t::max_memory_usage, chunkManager));
  CMD_ANY_VALUE_V ("pieces.memory.max.set",           std::bind(&CM_t::set_max_memory_usage, chunkManager, std::placeholders::_2));
  CMD_ANY         ("pieces.stats_preloaded",          std::bind(&CM_t::stats_preloaded, chunkManager));
  CMD_ANY         ("pieces.stats_not_preloaded",      std::bind(&CM_t::stats_not_preloaded, chunkManager));

  CMD_ANY         ("pieces.stats.total_size",         std::bind(&apply_pieces_stats_total_size));

  CMD_ANY         ("pieces.hash.queue_size",          std::bind(&torrent::main_thread::hash_queue_size));
  CMD_VAR_BOOL    ("pieces.hash.on_completion",       true);

  CMD_VAR_STRING  ("directory.default",               "./");

  CMD_VAR_STRING  ("session.name",                    "");
  CMD_ANY         ("session.path",                    [](auto, auto)        { return session_thread::manager()->path(); });
  CMD_ANY_STRING_V("session.path.set",                [](auto, auto& str)   { return session_thread::manager()->set_path(str); });
  CMD_ANY         ("session.use_lock",                [](auto, auto)        { return session_thread::manager()->use_lock(); });
  CMD_ANY_VALUE_V ("session.use_lock.set",            [](auto, auto& value) { return session_thread::manager()->set_use_lock(value); });
  CMD_VAR_BOOL    ("session.on_completion",           true);

  CMD_ANY_V       ("session.save",                    [dList](auto, auto)   { return dList->session_save(); });

  CMD_ANY         ("magnet.path",                     [](auto, auto)        { return control->core()->magnet_path(); });
  CMD_ANY_STRING_V("magnet.path.set",                 [](auto, auto& str)   { return control->core()->set_magnet_path(str); });

#ifdef HAVE_LUA
  rpc::LuaEngine* lua_engine = control->lua_engine();

  CMD_ANY         ("lua.execute",                    std::bind(&rpc::execute_lua, lua_engine, std::placeholders::_1, std::placeholders::_2, 0));
  CMD_ANY         ("lua.execute.str",                std::bind(&rpc::execute_lua, lua_engine, std::placeholders::_1, std::placeholders::_2, rpc::LuaEngine::flag_string));
#endif

#define CMD_EXECUTE(key, flags)                                        \
  CMD_ANY(key, std::bind(&rpc::ExecFile::execute_object, &rpc::execFile, std::placeholders::_2, flags));

  CMD_EXECUTE     ("execute",                 rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_throw);
  CMD_EXECUTE     ("execute.throw",           rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_throw);
  CMD_EXECUTE     ("execute.throw.bg",        rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_throw | rpc::ExecFile::flag_background);
  CMD_EXECUTE     ("execute.nothrow",         rpc::ExecFile::flag_expand_tilde);
  CMD_EXECUTE     ("execute.nothrow.bg",      rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_background);
  CMD_EXECUTE     ("execute.raw",             rpc::ExecFile::flag_throw);
  CMD_EXECUTE     ("execute.raw.bg",          rpc::ExecFile::flag_throw | rpc::ExecFile::flag_background);
  CMD_EXECUTE     ("execute.raw_nothrow",     0);
  CMD_EXECUTE     ("execute.raw_nothrow.bg",  rpc::ExecFile::flag_background);
  CMD_EXECUTE     ("execute.capture",         rpc::ExecFile::flag_throw | rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_capture);
  CMD_EXECUTE     ("execute.capture_nothrow", rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_capture);

  // TODO: Convert to new command types:
  *rpc::command_base::argument(0) = "placeholder.0";
  *rpc::command_base::argument(1) = "placeholder.1";
  *rpc::command_base::argument(2) = "placeholder.2";
  *rpc::command_base::argument(3) = "placeholder.3";
  CMD_ANY_P("argument.0", std::bind(&rpc::command_base::argument_ref, 0));
  CMD_ANY_P("argument.1", std::bind(&rpc::command_base::argument_ref, 1));
  CMD_ANY_P("argument.2", std::bind(&rpc::command_base::argument_ref, 2));
  CMD_ANY_P("argument.3", std::bind(&rpc::command_base::argument_ref, 3));

  CMD_ANY_LIST  ("group.insert", std::bind(&group_insert, std::placeholders::_2));

  rpc::rpc.mark_safe("system.api_version");
  rpc::rpc.mark_safe("system.client_version");
  rpc::rpc.mark_safe("system.library_version");
  rpc::rpc.mark_safe("system.file.max_size");
  rpc::rpc.mark_safe("system.file.split_size");
  rpc::rpc.mark_safe("system.file.split_suffix");

  rpc::rpc.mark_safe("system.sockets.size");
  rpc::rpc.mark_safe("system.sockets.max_size");
  rpc::rpc.mark_safe("system.sockets.generic.size");
  rpc::rpc.mark_safe("system.sockets.generic.max_size");
  rpc::rpc.mark_safe("system.sockets.http.size");
  rpc::rpc.mark_safe("system.sockets.http.max_size");
  rpc::rpc.mark_safe("system.sockets.internal.size");
  rpc::rpc.mark_safe("system.sockets.internal.max_size");
  rpc::rpc.mark_safe("system.sockets.scgi.size");
  rpc::rpc.mark_safe("system.sockets.scgi.max_size");
  rpc::rpc.mark_safe("system.sockets.files.size");
  rpc::rpc.mark_safe("system.sockets.files.max_size");

  rpc::rpc.mark_safe("directory.default");
  rpc::rpc.mark_safe("session.path");
  rpc::rpc.mark_safe("session.use_lock");
  rpc::rpc.mark_safe("session.on_completion");

  rpc::rpc.mark_safe("pieces.sync.always_safe");
  rpc::rpc.mark_safe("pieces.sync.timeout");
  rpc::rpc.mark_safe("pieces.sync.timeout_safe");
  rpc::rpc.mark_safe("pieces.preload.type");
  rpc::rpc.mark_safe("pieces.preload.min_size");
  rpc::rpc.mark_safe("pieces.preload.min_rate");
  rpc::rpc.mark_safe("pieces.memory.max");
  rpc::rpc.mark_safe("pieces.hash.on_completion");
}
