-- The "rtorrent" table is passed in by the C++ code, modify and
-- return it for loading.
local args = {...}
local rtorrent = args[1]

-- Autocall
-- Passes an empty first argument implicitly, as "".
-- Allows syntax like:
-- - `rtorrent.autocall.system.hostname()` or
-- - `rtorrent.autocall.session.directory.set("/tmp/")`
-- Has assignment operator, syntax sugar for `.set()` with single argument
-- - `rtorrent.autocall.session.directory = "/tmp/"`
-- Autocall-chains can be reused as aliases:
-- ```lua
-- a = rtorrent.autocall.system
-- a.hostname() -- same as `rtorrent.autocall.system.hostname()`
-- a.pid()      -- same as `rtorrent.autocall.system.pid()`
local mt = {}
function mt.__call (t, ...)
   name = table.concat(rawget(t, "__namestack"), ".")
   tg = rawget(t, "__target") or ""
   success, ret = pcall(rtorrent.call, name, tg, ...)
   if not success then error(name..": "..ret, 2) end
   return ret
end
function mt.__index (t, key)
   ns = {table.unpack(rawget(t, "__namestack") or {})}
   tg = rawget(t, "__target") or nil
   table.insert(ns, key)
   return setmetatable({__namestack=ns, __target=tg}, mt)
end
function mt.__newindex (t, key, value)
   t[key].set(value)
end
rtorrent["autocall"] = setmetatable({}, mt)

-- Target-object
-- Sets first argment for Autocall, for commands that require target.
-- Second argument allows to add prefix to command.
-- Allows syntax like:
-- - `rtorrent.Target("some_infohash").d.name()` or
-- - `rtorrent.Target:new("some_infohash").d.name()`
-- Also can be stored as variable
local Target = setmetatable({
   new = function (self, target, prefix)
      return self(target, prefix)
   end;
}, {
   __call = function (self, target, prefix)
      return setmetatable(
         {__namestack={prefix}, __target=target}, mt)
   end
})
rtorrent["Target"] = Target

-- Some aliases for Target-commands
for i, p in ipairs({ "d", "f", "p", "t", "load" }) do
   rtorrent[p] = function (target) return Target(target, p) end
end

-- Insert Lua method
-- Allow insert global lua-finction `func_name` at rtorrent slot `name`
-- @param name string rtorrent's slot
-- @param func_name string name of global lua-function
-- @param ... string list of rtorrent's arguments like `(d.name)` to be
--                   passed to lua-function when it be called by rtorrent
rtorrent["insert_lua_method"] = function (name, func_name, ...)
   local args = {}
   for i, v in ipairs({...}) do
      args[i] = v or ("$argument." .. i-1 .. "=")
   end

   local arg_str = table.concat(args, ",")
   if #arg_str > 0 then arg_str = ","..arg_str end
   rtorrent.autocall.method.insert(name, "simple", 'lua.execute.str="return '..func_name..'(...)"'..arg_str)
end

return rtorrent
