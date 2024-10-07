-- "rtorrent" is a global variable set by the module loading call
-- Autocall
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

-- autocall-config
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
rtorrent["autocall_config"] = setmetatable({}, mt)

return rtorrent
