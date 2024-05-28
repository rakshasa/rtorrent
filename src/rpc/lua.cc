#include "config.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

#include "lua.h"
#ifdef HAVE_LUA
#include <lua.hpp>
#endif

#include <torrent/object.h>
#include <rak/path.h>
#include <rak/error_number.h>
#include <rak/string_manip.h>

#include "core/download.h"
#include "rpc/command_map.h"
#include "rpc/command.h"
#include "rpc/parse_commands.h"
#include "rpc/xmlrpc.h"

namespace rpc {

const int LuaEngine::flag_string;
const int LuaEngine::flag_autocall_upvalue;
const std::string LuaEngine::module_name = "rtorrent";

#ifdef HAVE_LUA

LuaEngine::LuaEngine() {
  m_LuaState = luaL_newstate();
  luaL_openlibs(m_LuaState);
  set_package_preload();
}

LuaEngine::~LuaEngine() {
  lua_close(m_LuaState);
}

void LuaEngine::set_package_preload() {
  auto L = m_LuaState;
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, LuaEngine::lua_init_module);
  lua_setfield(L, -2, LuaEngine::module_name.c_str());
  lua_pop(L, 2);
}


// Helper func to allow RAII to do it's job
bool is_file_exist(const std::string &path) {
    std::ifstream infile(path);
    return infile.good();
}

std::string LuaEngine::search_lua_path(lua_State *L) {
  // Get package.path
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  std::stringstream ss(lua_tostring(L, -1));
  lua_pop(L, 2);
  // Tokenize path using ';' as delimiter
  std::vector<std::string> paths;
  std::string token;
  while (getline(ss, token, ';')) {
      paths.push_back(std::string(token));
  }

  // Replace '?' in each path with the module name and check if file exists
  for (const std::string &templatePath : paths) {
      std::string filePath = templatePath;
      size_t pos;
      while ((pos = filePath.find('?')) != std::string::npos) {
          filePath.replace(pos, 1, LuaEngine::module_name);
      }
      if (is_file_exist(filePath)) {
          return filePath;
      }
  }
  return "";
}

int LuaEngine::lua_rtorrent_call(lua_State *L) {
  auto method = lua_tostring(L, 1);
  lua_remove(L, 1);
  torrent::Object object;
  rpc::target_type target = rpc::make_target();;
  rpc::CommandMap::iterator itr = rpc::commands.find(std::string(method).c_str());
  if (itr == rpc::commands.end()) {
    throw torrent::input_error("method not found: " + std::string(method));
  }
  object = lua_callstack_to_object(L, itr->second.m_flags, &target);
  try {
    const auto& result = rpc::commands.call_command(itr, object, target);
    object_to_lua(L, result);
    return 1;
  } catch (torrent::base_error& e) {
    throw luaL_error(L, e.what());
  }
}


int LuaEngine::lua_init_module(lua_State *L) {
  lua_createtable(L, 0, 1);
  int tableIndex = lua_gettop(L);
  lua_pushliteral(L, "call");
  lua_pushcfunction(L, LuaEngine::lua_rtorrent_call);
  lua_settable(L, tableIndex);
  auto luaFile = search_lua_path(L);
  // Should this throw on failure?
  if (!luaFile.empty()) {
    // Pop table into global var for file to manipulate
    lua_setglobal(L, "rtorrent");
    int status = luaL_loadfile(L, luaFile.c_str());
    if (status == LUA_OK) {
      status = lua_pcall(L, 0, LUA_MULTRET, 0);
    }
    check_lua_status(L, status);
    // Clean up global var
    lua_pushnil(L);
    lua_setglobal(L, "rtorrent");
    // Leave the original table in place
  }
  return 1;
}
void
object_to_lua(lua_State* L, torrent::Object const& object) {
  // Converts an object to a single Lua stack object
  switch (object.type()) {
  case torrent::Object::TYPE_VALUE:
    lua_pushnumber(L, object.as_value());
    break;
  case torrent::Object::TYPE_NONE:
    lua_pushnil(L);
    break;
  case torrent::Object::TYPE_STRING:
    lua_pushstring(L, object.as_string().c_str());
    break;
  case torrent::Object::TYPE_LIST: {
    lua_createtable(L, object.as_list().size(), 0);
    int index = 1;
    int tableIndex = lua_gettop(L);
    for (torrent::Object::list_const_iterator itr = object.as_list().begin(), last = object.as_list().end(); itr != last; itr++) {
      object_to_lua(L, *itr);
      lua_rawseti(L, tableIndex, lua_Integer(index++));
    }
    break;
  }
  case torrent::Object::TYPE_MAP: {
    lua_createtable(L, 0, object.as_map().size());
    int tableIndex = lua_gettop(L);
    for (torrent::Object::map_const_iterator itr = object.as_map().begin(), last = object.as_map().end(); itr != last; itr++) {
      object_to_lua(L, itr->second);
      lua_pushlstring(L, itr->first.c_str(), itr->first.size());
      lua_settable(L, tableIndex);
    }
    break;
  }
  default:
    lua_pushnumber(L, lua_Number(object.type()));
    break;
  }
}

torrent::Object
lua_to_object(lua_State* L, int index) {
  torrent::Object object;
  switch(lua_type(L, index)) {
  case LUA_TNUMBER:
    return torrent::Object(lua_tonumber(L, index));
  case LUA_TSTRING:
    return torrent::Object(lua_tostring(L, index));
  case LUA_TBOOLEAN:
    return torrent::Object(lua_toboolean(L, index));
  case LUA_TTABLE: {
    lua_pushnil(L);
    int status = lua_next(L, -2);
    if (status == 0)
      return torrent::Object::create_map();
    // If the table starts at 1, assume it's an array
    if (lua_isnumber(L, -2) && (lua_tonumber(L, -2) == 1)) {
      torrent::Object list = torrent::Object::create_list();
      list.insert_back(lua_to_object(L, -1));
      lua_pop(L, 1);
      while(lua_next(L, -2) != 0) {
        list.insert_back(lua_to_object(L, -1));
        lua_pop(L, 1);
      }
      return list;
    } else {
      torrent::Object map = torrent::Object::create_map();
      // Reset the stack and start from the first element
      lua_pop(L, 2);
      lua_pushnil(L);
      while(lua_next(L, -2) != 0) {
        std::string key;
        // Bencode dictionaries require string keys, but naively calling
        // lua_tostring would auto-convert any numbers to strings,
        // which would throw off lua_next
        switch(lua_type(L, -2)) {
          case LUA_TNUMBER:
            key = std::to_string(lua_tonumber(L, -2));
          case LUA_TSTRING:
            key = lua_tostring(L, -2);
        }
        map.insert_key(key, lua_to_object(L, -1));
        lua_pop(L, 1);
      }
      return map;
    }
    return torrent::Object(lua_tostring(L, index));
  }
  default:
    std::string result = luaL_tolstring(L, index, NULL);
    lua_pop(L, 1);
    return torrent::Object(result);
  }
  return object;
}

void
string_to_target(std::string targetString,
                 bool                    requireIndex,
                 rpc::target_type*       target) {
  // target_any: ''
  // target_download: <hash>
  // target_file: <hash>:f<index>
  // target_peer: <hash>:p<index>
  // target_tracker: <hash>:t<index>

  if (targetString.size() == 0 && !requireIndex) {
    return;
  }

  // Length of SHA1 hash is 40
  if (targetString.size() < 40) {
    throw torrent::input_error("invalid parameters: invalid target");
  }

  std::string hash;
  char type = 'd';
  std::string index;

  const auto& delimPos = targetString.find_first_of(':', 40);
  if (delimPos == std::string::npos ||
      delimPos + 2 >= targetString.size()) {
    if (requireIndex) {
      throw torrent::input_error("invalid parameters: no index");
    }
    hash = targetString;
  } else {
    hash  = targetString.substr(0, delimPos);
    type  = targetString[delimPos + 1];
    index = targetString.substr(delimPos + 2, std::string::npos);
  }

  core::Download* download =
    rpc::xmlrpc.slot_find_download()(std::string(hash).c_str());

  if (download == nullptr) {
    throw torrent::input_error("invalid parameters: info-hash not found");
  }

  try {
    switch (type) {
      case 'd':
        *target = rpc::make_target(download);
        break;
      case 'f':
        *target = rpc::make_target(
          command_base::target_file,
          rpc::xmlrpc.slot_find_file()(download, std::stoi(std::string(index))));
        break;
      case 't':
        *target = rpc::make_target(
          command_base::target_tracker,
          rpc::xmlrpc.slot_find_tracker()(download, std::stoi(std::string(index))));
        break;
      case 'p':
        torrent::HashString peerHash;
        torrent::hash_string_from_hex_c_str(index.c_str(), peerHash);
        *target = rpc::make_target(
          command_base::target_peer,
          rpc::xmlrpc.slot_find_peer()(download, peerHash));
        break;
      default:
        throw torrent::input_error(
          "invalid parameters: unexpected target type");
    }
  } catch (const std::logic_error&) {
    throw torrent::input_error("invalid parameters: invalid index");
  }

  if (target == nullptr || target->second == nullptr) {
    throw torrent::input_error(
      "invalid parameters: unable to find requested target");
  }
}

torrent::Object
lua_callstack_to_object(lua_State* L, int commandFlags, rpc::target_type* target) {
  torrent::Object object;
  if (lua_gettop(L) == 0) {
    return torrent::Object();
  }

  if (!lua_isstring(L, 1)) {
    throw torrent::input_error("invalid parameters: target must be a string");
  }
  string_to_target(std::string(lua_tostring(L, 1)),
                   (commandFlags & (CommandMap::flag_tracker_target & CommandMap::flag_file_target)),
                   target);

  // start from the second argument since the first is the target
  lua_remove(L, 1);

  if (lua_gettop(L) == 0) {
    return torrent::Object();
  } else {
    torrent::Object             result  = torrent::Object::create_list();
    torrent::Object::list_type& listRef = result.as_list();
    while (lua_gettop(L) != 0) {
      listRef.insert(listRef.begin(), lua_to_object(L, -1));
      lua_remove(L, -1);
    }
    return result;
  }

  return object;
}


int
lua_rtorrent_call(lua_State* L) {
  auto method = lua_tostring(L, 1);
  lua_remove(L, 1);
  torrent::Object object;
  rpc::target_type target = rpc::make_target();;
  rpc::CommandMap::iterator itr = rpc::commands.find(std::string(method).c_str());
  if (itr == rpc::commands.end()) {
    throw torrent::input_error("method not found: " + std::string(method));
  }
  object = lua_callstack_to_object(L, itr->second.m_flags, &target);
  try {
    const auto& result = rpc::commands.call_command(itr, object, target);
    object_to_lua(L, result);
    return 1;
  } catch (torrent::base_error& e) {
    throw luaL_error(L, e.what());
  }
}

void
init_rtorrent_module(lua_State* L) {
  lua_createtable(L, 0, 1);
  int tableIndex = lua_gettop(L);
  lua_pushliteral(L, "call");
  lua_pushcfunction(L, lua_rtorrent_call);
  lua_settable(L, tableIndex);
  // Allows use of the dot syntax for calling RPC, through metatables
  lua_pushliteral(L, "autocall");
  luaL_dostring(L, "\
     local mt = {} \n\
     function mt.__call (t, ...) \n\
       return rtorrent.call(table.concat(rawget(t, \"__namestack\"),\".\"), ...) \n\
     end \n\
     function mt.__index (t, key) \n\
       -- Create a new sub-table, preserving the name of the key in a stack \n\
       ns = rawget(t, \"__namestack\") \n\
       if ns == nil then \n\
         ns = {} \n\
       end \n\
       table.insert(ns, key) \n\
       return setmetatable({__namestack=ns}, mt) \n\
     end \n\
     return setmetatable({}, mt) \n\
  ");
  lua_settable(L, tableIndex);
  // Variant on the above that auto-provides an empty target
  lua_pushliteral(L, "autocall_config");
  luaL_dostring(L, "\
     local mt = {} \n\
     function mt.__call (t, ...) \n\
       return rtorrent.call(table.concat(rawget(t, \"__namestack\"), \".\"), \"\", ...) \n\
     end \n\
     function mt.__index (t, key) \n\
       -- Create a new sub-table, preserving the name of the key in a stack \n\
       ns = rawget(t, \"__namestack\") \n\
       if ns == nil then \n\
         -- Allow loading top-level global names \n\
         if _G[key] ~= nil then \n\
           return _G[key] \n\
         end \n\
         ns = {} \n\
       end \n\
       table.insert(ns, key) \n\
       return setmetatable({__namestack=ns}, mt) \n\
     end \n\
     return setmetatable({}, mt) \n\
  ");
  lua_settable(L, tableIndex);
  lua_setglobal(L, "rtorrent");
}
    
void
check_lua_status(lua_State* L, int status) {
  if (status != LUA_OK) {
    std::string str = lua_tostring(L, -1);
    throw torrent::input_error(str);
  }
}

torrent::Object
execute_lua(LuaEngine* engine, rpc::target_type target, torrent::Object const& rawArgs, int flags) {
  int lua_argc = 1; // Target is always present, even if empty
  lua_State* L = engine->state();
  std::string target_string = "";
  switch (target.first) {
  case (command_base::target_download):
    core::Download* dl_target = (core::Download*)target.second;
    torrent::HashString infohash = dl_target->info()->hash();
    target_string = rak::transform_hex_str(infohash);
    break;
  }
  if (rawArgs.is_list()) {
    const torrent::Object::list_type& args = rawArgs.as_list();
    if (flags & LuaEngine::flag_string) {
      check_lua_status(L, luaL_loadstring(L, args.begin()->as_string().c_str()));
    } else {
      check_lua_status(L, luaL_loadfile(L, args.begin()->as_string().c_str()));
    }
    object_to_lua(L, target_string);
    for (torrent::Object::list_const_iterator itr = std::next(args.begin()), last = args.end(); itr != last; itr++) {
      object_to_lua(L, *itr);
    }
    lua_argc += args.size()-1;
  } else {
    const torrent::Object::string_type& target = rawArgs.as_string();
    if (flags & LuaEngine::flag_string) {
      check_lua_status(L, luaL_loadstring(L, target.c_str()));
    } else {
      check_lua_status(L, luaL_loadfile(L, target.c_str()));
    }
    object_to_lua(L, target_string);
  }
  if (flags & LuaEngine::flag_autocall_upvalue) {
    lua_getglobal(L, "rtorrent");
    lua_getfield(L, -1, "autocall_config");
    lua_remove(L, -2);
    lua_setupvalue(L, -2, 1);
  }
  check_lua_status(L, lua_pcall(L, lua_argc, LUA_MULTRET, 0));
  return lua_to_object(L, -1);
}

#else
torrent::Object execute_lua(LuaEngine* engine, torrent::Object const& rawArgs, int flags) { throw torrent::input_error("Lua support not enabled"); return torrent::Object(); }
void string_to_target(std::string targetString, bool requireIndex, rpc::target_type* target) {}
LuaEngine::LuaEngine() {}
LuaEngine::~LuaEngine() {}
#endif
}
