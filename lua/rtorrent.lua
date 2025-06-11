-- The "rtorrent" table is passed in by the C++ code, modify and
-- return it for loading.
local args = {...}
local rtorrent = args[1]

-- Autocall
-- Allows syntax like `rtorrent.autocall.system.hostname()`
local mt = {}
function mt.__call (t, ...)
   name = table.concat(rawget(t, "__namestack"), ".")
   success, ret = pcall(rtorrent.call, name, ...)
   if not success then error(name..": "..ret, 2) end
   return ret
end
function mt.__index (t, key)
   ns = rawget(t, "__namestack") or {}
   table.insert(ns, key)
   return setmetatable({__namestack=ns}, mt)
end
rtorrent["autocall"] = setmetatable({}, mt)

-- Autocall-config
-- Same as autocall, but passes an empty first target implicitly, for syntax
-- like `rtorrent.autocall_config.session.directory.set("/tmp/")` or
-- like `rtorrent.autocall_config.session.directory = "/tmp/"`
local mt = {}
function mt.__call (t, ...)
   name = table.concat(rawget(t, "__namestack"), ".")
   success, ret = pcall(rtorrent.call, name, "", ...)
   if not success then error(name..": "..ret, 2) end
   return ret
end
function mt.__index (t, key)
   ns = rawget(t, "__namestack")
   if ns == nil then
      if _G[key] ~= nil then return _G[key] end
      ns = {}
   end
   table.insert(ns, key)
   return setmetatable({__namestack=ns}, mt)
end
function mt.__newindex (t, key, value)
   t[key].set(value)
end
rtorrent["autocall_config"] = setmetatable({}, mt)

return rtorrent
