#include "config.h"

#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "rpc/lua.h"
#ifdef HAVE_LUA
#include <lua.hpp>
#endif

#include <rak/error_number.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/object.h>

#include "core/download.h"
#include "rpc/command.h"
#include "rpc/command_map.h"
#include "rpc/parse_commands.h"
#include "rpc/xmlrpc.h"

namespace rpc {

#ifdef HAVE_LUA

const int         LuaEngine::flag_string;
const std::string LuaEngine::module_name = "rtorrent";
const std::string LuaEngine::local_path = LUA_DATADIR "/?.lua;" LUA_DATADIR "/?/init.lua";

LuaEngine::LuaEngine() {
  m_luaState = luaL_newstate();
  luaL_openlibs(m_luaState);
  set_package_preload();
  override_package_path();
}

LuaEngine::~LuaEngine() { lua_close(m_luaState); }

void
LuaEngine::set_package_preload() {
  auto l_state = m_luaState;
  lua_getglobal(l_state, "package");
  lua_getfield(l_state, -1, "preload");
  lua_pushcfunction(l_state, LuaEngine::lua_init_module);
  lua_setfield(l_state, -2, LuaEngine::module_name.c_str());
  lua_pop(l_state, 2);
}

void
LuaEngine::override_package_path() {
  auto l_state = m_luaState;
  lua_getglobal(l_state, "package");
  lua_getfield(l_state, -1, "path");

  std::string current_path(lua_tostring(l_state, -1));
  std::string new_path = local_path + ';' + current_path;

  lua_pushstring(l_state, new_path.c_str());
  lua_setfield(l_state, -3, "path");
  lua_pop(l_state, 2);
}

void
check_lua_status(lua_State* l_state, int status) {
  if (status != LUA_OK) {
    throw torrent::input_error(lua_tostring(l_state, -1));
  }
}

std::string
LuaEngine::search_lua_path(lua_State* l_state) {
  // Get package.path
  lua_getglobal(l_state, "package");
  lua_getfield(l_state, -1, "path");
  std::string lua_path(lua_tostring(l_state, -1));
  lua_pop(l_state, 2);

  // Use lambda here to ensure file closes as soon as it goes out of scope
  auto file_exists = [](const std::string& path) -> bool {
    std::ifstream file(path);
    return file.is_open();
  };

  // Tokenize path using ';' as delimiter
  for (size_t pos = 0, end = 0; end != std::string::npos; pos = end + 1) {
    end = lua_path.find(';', pos);

    std::string file_path((end == std::string::npos)
      ? lua_path.substr(pos)
      : lua_path.substr(pos, end - pos));

    if (!file_path.empty()) {
      size_t ppos = 0;
      while ((ppos = file_path.find('?')) != std::string::npos) {
        file_path.replace(ppos, 1, LuaEngine::module_name);
      }
      if (file_exists(file_path)) {
        return file_path;
      }
    }
  }
  return "";
}

int
LuaEngine::lua_rtorrent_call(lua_State* l_state) {
  auto method = lua_tostring(l_state, 1);
  lua_remove(l_state, 1);

  rpc::CommandMap::iterator itr = rpc::commands.find(std::string(method).c_str());

  if (itr == rpc::commands.end()) {
    throw torrent::input_error("method not found: " + std::string(method));
  }

  auto target  = rpc::make_target();
  auto deleter = std::function<void()>();

  auto object = lua_callstack_to_object(l_state, itr->second.m_flags, &target, &deleter);

  try {
    const auto& result = rpc::commands.call_command(itr, object, target);

    object_to_lua(l_state, result);
    if (deleter) deleter();

    return 1;

  } catch (torrent::base_error& e) {
    if (deleter) deleter();
    throw luaL_error(l_state, e.what());
  }
}

int
LuaEngine::lua_init_module(lua_State* l_state) {
  // Should this throw if it fails to find the file?
  auto lua_file = search_lua_path(l_state);

  if (!lua_file.empty()) {
    check_lua_status(l_state, luaL_loadfile(l_state, lua_file.c_str()));
  }

  lua_createtable(l_state, 0, 1);
  // Assign functions
  lua_pushliteral(l_state, "call");
  lua_pushcfunction(l_state, LuaEngine::lua_rtorrent_call);
  lua_settable(l_state, -3);

  if (!lua_file.empty()) {
    check_lua_status(l_state, lua_pcall(l_state, 1, 1, 0));
  }

  return 1;
}

void
object_to_target(const torrent::Object& obj, int call_flags, rpc::target_type* target, std::function<void()>* deleter) {
  if (!obj.is_string()) {
    throw torrent::input_error("invalid parameters: target must be a string");
  }

  std::string target_string = obj.as_string();
  bool        require_index = (call_flags & (CommandMap::flag_tracker_target | CommandMap::flag_file_target));

  if (target_string.empty() && !require_index) {
    return;
  }

  // Length of SHA1 hash is 40
  if (target_string.size() < 40) {
    throw torrent::input_error("invalid parameters: invalid target");
  }

  char        type = 'd';
  std::string hash;
  std::string index;
  const auto& delim_pos = target_string.find_first_of(':', 40);

  if (delim_pos == target_string.npos || delim_pos + 2 >= target_string.size()) {
    if (require_index) {
      throw torrent::input_error("invalid parameters: no index");
    }

    hash = target_string;

  } else {
    hash  = target_string.substr(0, delim_pos);
    type  = target_string[delim_pos + 1];
    index = target_string.substr(delim_pos + 2);
  }

  core::Download* download = rpc.slot_find_download()(hash.c_str());

  if (download == nullptr)
    throw torrent::input_error("invalid parameters: info-hash not found");

  try {
    switch (type) {
    case 'd':
      *target = rpc::make_target(download);
      break;

    case 'f':
      *target = rpc::make_target(command_base::target_file, rpc.slot_find_file()(download, std::stoi(std::string(index))));
      break;

    case 't':
      {
        auto tracker = new torrent::tracker::Tracker(rpc.slot_find_tracker()(download, std::stoi(std::string(index))));

        *deleter = [tracker]() { delete tracker; };
        *target = rpc::make_target(command_base::target_tracker, target);
      }
      break;

    case 'p':
      {
        if (index.size() < 40) {
          throw torrent::input_error("Not a hash string.");
        }

        torrent::HashString hash;
        torrent::hash_string_from_hex_c_str(index.c_str(), hash);
        *target = rpc::make_target(command_base::target_peer, rpc.slot_find_peer()(download, hash));
      }
      break;

    default:
      throw torrent::input_error("invalid parameters: unexpected target type");
    }
  } catch (const std::logic_error&) {
    throw torrent::input_error("invalid parameters: invalid index");
  }
}

void
object_to_lua(lua_State* l_state, torrent::Object const& object) {
  // Converts an object to a single Lua stack object
  switch (object.type()) {
  case torrent::Object::TYPE_VALUE:
    lua_pushinteger(l_state, static_cast<int64_t>(object.as_value()));
    break;
  case torrent::Object::TYPE_NONE:
    lua_pushnil(l_state);
    break;
  case torrent::Object::TYPE_STRING:
    lua_pushstring(l_state, object.as_string().c_str());
    break;
  case torrent::Object::TYPE_LIST: {
    int  index       = 1;
    auto object_list = object.as_list();

    lua_createtable(l_state, static_cast<int>(object_list.size()), 0);
    int table_idx = lua_gettop(l_state);
    for (const auto& itr : object_list) {
      object_to_lua(l_state, itr);
      lua_rawseti(l_state, table_idx, lua_Integer(index++));
    }
    break;
  }
  case torrent::Object::TYPE_MAP: {
    auto object_map = object.as_map();
    lua_createtable(l_state, 0, static_cast<int>(object_map.size()));
    int table_idx = lua_gettop(l_state);
    for (const auto& itr : object_map) {
      object_to_lua(l_state, itr.second);
      lua_pushlstring(l_state, itr.first.c_str(), itr.first.size());
      lua_rawset(l_state, table_idx);
    }
    break;
  }
  default:
    lua_pushnumber(l_state, lua_Number(object.type()));
    break;
  }
}

torrent::Object
lua_to_object(lua_State* l_state) {
  switch (lua_type(l_state, -1)) {
  case LUA_TNUMBER:
    return torrent::Object(lua_tonumber(l_state, -1));
  case LUA_TSTRING:
    return torrent::Object(lua_tostring(l_state, -1));
  case LUA_TBOOLEAN:
    return torrent::Object(lua_toboolean(l_state, -1));
  case LUA_TNIL:
    return torrent::Object(torrent::Object());
  case LUA_TTABLE: {
    lua_pushnil(l_state);
    int status = lua_next(l_state, -2);
    if (status == 0)
      return torrent::Object::create_map();
    // If the table starts at 1, assume it's an array
    if (lua_isnumber(l_state, -2) && (lua_tonumber(l_state, -2) == 1)) {
      torrent::Object list = torrent::Object::create_list();
      list.insert_back(lua_to_object(l_state));
      lua_pop(l_state, 1);
      while (lua_next(l_state, -2) != 0) {
        list.insert_back(lua_to_object(l_state));
        lua_pop(l_state, 1);
      }
      return list;
    } else {
      torrent::Object map = torrent::Object::create_map();
      // Reset the stack and start from the first element
      lua_pop(l_state, 2);
      lua_pushnil(l_state);
      while (lua_next(l_state, -2) != 0) {
        std::string key;
        switch (lua_type(l_state, -2)) {
        case LUA_TSTRING:
          key = lua_tostring(l_state, -2);
          break;
        default:
          // Bencode dictionaries require string keys, but naively
          // calling lua_tostring will auto-convert any other object
          // (particularly numbers) to strings in-place, which would
          // throw off lua_next, so we copy the value and only then
          // get the string.
          // https://www.lua.org/manual/5.3/manual.html#lua_tolstring
          lua_pushvalue(l_state, -2);
          key = lua_tostring(l_state, -1);
          lua_pop(l_state, 1);
          break;
        }
        map.insert_key(key, lua_to_object(l_state));
        lua_pop(l_state, 1);
      }
      return map;
    }
    return torrent::Object(lua_tostring(l_state, -1));
  }
  default:
    std::string result = lua_tostring(l_state, -1);
    lua_pop(l_state, 1);
    return torrent::Object(result);
  }
}

torrent::Object
lua_callstack_to_object(lua_State* l_state, int command_flags, rpc::target_type* target, std::function<void()>* deleter) {
  if (lua_gettop(l_state) == 0) {
    return torrent::Object();
  }

  if (!lua_isstring(l_state, 1)) {
    throw torrent::input_error("invalid parameters: target must be a string");
  }

  object_to_target(torrent::Object(lua_tostring(l_state, 1)), command_flags, target, deleter);

  // start from the second argument since the first is the target
  lua_remove(l_state, 1);

  if (lua_gettop(l_state) == 0)
    return torrent::Object();

  torrent::Object             result   = torrent::Object::create_list();
  torrent::Object::list_type& list_ref = result.as_list();

  while (lua_gettop(l_state) != 0) {
    list_ref.insert(list_ref.begin(), lua_to_object(l_state));
    lua_remove(l_state, -1);
  }

  return result;
}

torrent::Object
execute_lua(LuaEngine* engine, rpc::target_type target_type, torrent::Object const& raw_args, int flags) {
  size_t      lua_argc = 1; // Target is always present, even if empty
  lua_State*  l_state  = engine->state();
  std::string target_string;

  switch (target_type.first) {
  case (command_base::target_download):
    core::Download*     dl_target = (core::Download*)target_type.second;
    torrent::HashString infohash  = dl_target->info()->hash();
    target_string                 = rak::transform_hex_str(infohash);
    break;
  }

  switch (raw_args.type()) {
  case torrent::Object::TYPE_LIST: {
    const torrent::Object::list_type& args = raw_args.as_list();
    if (flags & LuaEngine::flag_string) {
      check_lua_status(l_state, luaL_loadstring(l_state, args.begin()->as_string().c_str()));
    } else {
      check_lua_status(l_state, luaL_loadfile(l_state, args.begin()->as_string().c_str()));
    }
    object_to_lua(l_state, target_string);
    for (torrent::Object::list_const_iterator itr = std::next(args.begin()), last = args.end(); itr != last; itr++) {
      object_to_lua(l_state, *itr);
    }
    lua_argc += args.size() - 1;
    break;
  }
  case torrent::Object::TYPE_STRING: {
    const torrent::Object::string_type& target = raw_args.as_string();
    if (flags & LuaEngine::flag_string) {
      check_lua_status(l_state, luaL_loadstring(l_state, target.c_str()));
    } else {
      check_lua_status(l_state, luaL_loadfile(l_state, target.c_str()));
    }
    object_to_lua(l_state, target_string);
    break;
  }
  default:
    throw torrent::input_error("execute_lua(...): cannot call lua with arg type " + std::to_string(raw_args.type()));
  }
  check_lua_status(l_state, lua_pcall(l_state, lua_argc, LUA_MULTRET, 0));
  return lua_to_object(l_state);
}

#else
torrent::Object
execute_lua(LuaEngine* engine, torrent::Object const& rawArgs, int flags) {
  throw torrent::input_error("Lua support not enabled");
  return torrent::Object();
}
LuaEngine::LuaEngine() {}
LuaEngine::~LuaEngine() {}
#endif
} // namespace rpc
