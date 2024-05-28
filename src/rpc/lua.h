#ifndef RTORRENT_LUA_H
#define RTORRENT_LUA_H

#include <memory>
#include <torrent/object.h>
#include "rpc/command.h"

#ifdef HAVE_LUA
#include "lua.h"
#include <cstdint>
#include <lua.hpp>
#endif

namespace rpc {

class LuaEngine {
public:
  static const int flag_string           = 0x1;
  static const int flag_autocall_upvalue = 0x2;
  static const std::string module_name;
  LuaEngine();
  ~LuaEngine();
#ifdef HAVE_LUA
  // lua_CFunctions
  static int lua_init_module(lua_State *L);
  static int lua_rtorrent_call(lua_State *L);
  void set_package_preload();
  lua_State* state() {return m_LuaState;}
private:
  static std::string search_lua_path(lua_State *L);
  lua_State* m_LuaState;
#endif
};
torrent::Object execute_lua(LuaEngine* engine, rpc::target_type target, const torrent::Object& rawArgs, int flags);
#ifdef HAVE_LUA
int rtorrent_call(lua_State* L);
void init_rtorrent_module(lua_State* L);
void object_to_lua(lua_State* L, torrent::Object const& object);
void check_lua_status(lua_State* L, int status);
torrent::Object lua_to_object(lua_State* L, int index);
torrent::Object lua_callstack_to_object(lua_State* L, int commandFlags, rpc::target_type* target);
#endif

} // namespace rpc


#endif
