-- The "rtorrent" table is passed in by the C++ code, modify and
-- return it for loading.
local args = {...}
local rtorrent = args[1]
local math = require('math')

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
-- @param method_args number|string[]|number[]
--  rtorrent's arguments like `(d.name)` to be passed to lua-function when it be
--  called by rtorrent:
--
--  1) non-negative integer - number of first arguments to be passed
--
--     example:
--       rtorrent.insert_lua_method('d.watch_handler', 'watch_handler', 1)
--
--     gives following rtorrent event handler:
--       d.watch_handler=$argument.0
--
--  2) table (array) of strings - concrete rtorrent strings, passed in specified
--     order
--
--     example:
--       rtorrent.insert_lua_method('d.watch_handler', 'watch_handler', { '$argument.0=' })
--
--     gives following rtorrent event handler:
--       d.watch_handler=$argument.0
--
--  3) table (array) of non-negative integer - indexes, converted to
--     $argument.0, $argument.1 ..., passed in specified order
--
--     example:
--       rtorrent.insert_lua_method('d.watch_handler', 'watch_handler', { 1, 0 })
--     or:
--       rtorrent.insert_lua_method('d.watch_handler', 'watch_handler', { [1]=1, [2]=0 })
--
--     gives following rtorrent event handler:
--       d.watch_handler=$argument.1,$argument.0
--
--  NOTE: first argement of func_name is always target, may be empty string
rtorrent["insert_lua_method"] = function (name, func_name, method_args)
   local args = {}

   if method_args == nil then method_args = 0 end

   if math.type(method_args) == 'integer' and method_args >= 0 then
      for i = 1, method_args do
         args[i] = "$argument." .. i-1 .. "="
      end
   elseif type(method_args) == 'table' then
      for i, v in ipairs(method_args) do
         if type(v) == 'string' then
            args[i] = v
         elseif math.type(v) == 'integer' and v >= 0 then
            args[i] = "$argument." .. v .. "="
         else
            error("Incorrect arguments")
         end
      end
   else
      error("Incorrect arguments")
   end

   local arg_str = table.concat(args, ",")
   if #arg_str > 0 then arg_str = ","..arg_str end
   rtorrent.autocall.method.insert(name, "simple", 'lua.execute.str="return '..func_name..'(...)"'..arg_str)
end

return rtorrent
