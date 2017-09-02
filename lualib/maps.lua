local exports = {}

local function rspamd_map_add(mname, optname, mtype, description)
  local ret = {
    get_key = function(t, k)
      if t.__data then
        return t.__data:get_key(k)
      end

      return nil
    end
  }
  local ret_mt = {
    __index = function(t, k)
      if t.__data then
        return t.get_key(k)
      end

      return nil
    end
  }
  local opt = rspamd_config:get_module_opt(mname, optname)

  if not opt then
    return nil
  end

  if type(opt) == 'string' then
    -- We have a single string, so we treat it as a map
    local map = rspamd_config:add_map{
      type = mtype,
      description = description,
      url = opt,
    }

    if map then
      ret.__data = map
      setmetatable(ret, ret_mt)
      return ret
    end
  elseif type(opt) == 'table' then
    -- it might be plain map or map of plain elements
    if opt[1] then
      if mtype == 'radix' then

        if string.find(opt[1], '^%d') then
          local map = rspamd_config:radix_from_config(mname, optname)

          if map then
            ret.__data = map
            setmetatable(ret, ret_mt)
            return ret
          end
        else
          -- Plain table
          local map = rspamd_config:add_map{
            type = mtype,
            description = description,
            url = opt,
          }
          if map then
            ret.__data = map
            setmetatable(ret, ret_mt)
            return ret
          end
        end
      elseif mtype == 'regexp' then
        -- Plain table
        local map = rspamd_config:add_map{
          type = mtype,
          description = description,
          url = opt,
        }
        if map then
          ret.__data = map
          setmetatable(ret, ret_mt)
          return ret
        end
      else
        if string.find(opt[1], '^/%a') or string.find(opt[1], '^http') then
          -- Plain table
          local map = rspamd_config:add_map{
            type = mtype,
            description = description,
            url = opt,
          }
          if map then
            ret.__data = map
            setmetatable(ret, ret_mt)
            return ret
          end
        else
          local data = {}
          local nelts = 0
          for _,elt in ipairs(opt) do
            if type(elt) == 'string' then
              data[elt] = true
              nelts = nelts + 1
            end
          end

          if nelts > 0 then
            ret.__data = data
            ret.get_key = function(t, k)
              if k ~= '__data' then
                return t.__data[k]
              end

              return nil
            end
            return ret
          end
        end
      end
    else
      local map = rspamd_config:add_map{
        type = mtype,
        description = description,
        url = opt,
      }
      if map then
        ret.__data = map
        setmetatable(ret, ret_mt)
        return ret
      end
    end
  end

  return nil
end

exports.rspamd_map_add = rspamd_map_add

-- Check `what` for being lua_map name, otherwise just compares key with what
local function rspamd_maybe_check_map(key, what)
  local fun = require "fun"

  local function starts(where,st)
    return string.sub(where,1,string.len(st))==st
  end

  if type(what) == "table" then
    return fun.any(function(elt) return rspamd_maybe_check_map(key, elt) end, what)
  end
  if type(rspamd_maps) == "table" then
    local mn
    if starts(what, "map:") then
      mn = string.sub(what, 4)
    elseif starts(what, "map://") then
      mn = string.sub(what, 6)
    end

    if mn and rspamd_maps[mn] then
      return rspamd_maps[mn]:get_key(key)
    end
  else
    return what:lower() == key
  end

  return false
end

exports.rspamd_maybe_check_map = rspamd_maybe_check_map

return exports
