#ifndef RTORRENT_LUA_H
#define RTORRENT_LUA_H

#include "rpc/command.h"
#include <torrent/object.h>

#ifdef HAVE_LUA
#include <lua.hpp>
#endif

namespace rpc {

class LuaEngine {
public:
  static const int         flag_string = 0x1;
  static const std::string module_name;
  static const std::string local_path;

  LuaEngine();
  ~LuaEngine();

#ifdef HAVE_LUA
  // lua_CFunctions
  static int lua_init_module(lua_State* l_state);
  static int lua_rtorrent_call(lua_State* l_state);
  void       set_package_preload();
  void       override_package_path();
  lua_State* state() { return m_luaState; }

private:
  static std::string search_lua_path(lua_State* l_state);
  lua_State*         m_luaState;
#endif
};

torrent::Object execute_lua(LuaEngine* engine, rpc::target_type target, const torrent::Object& raw_args, int flags);

#ifdef HAVE_LUA

int             rtorrent_call(lua_State* l_state);
void            init_rtorrent_module(lua_State* l_state);
void            object_to_lua(lua_State* l_state, torrent::Object const& object);
void            check_lua_status(lua_State* l_state, int status);
torrent::Object lua_to_object(lua_State* l_state);
torrent::Object lua_callstack_to_object(lua_State* l_state, int command_flags, rpc::target_type* target, std::function<void()>* deleter);

#endif

} // namespace rpc

#endif
