#!/usr/bin/env lua

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Prelude

local ______f, _____________t = function()return function()end end, {nil,
   [false]  = 'Lua 5.1',
   [true]   = 'Lua 5.2',
   [1/'-0'] = 'Lua 5.3',
   [1]      = 'LuaJIT' }
local luaVersion = _____________t[1] or _____________t[1/0] or _____________t[______f()==______f()]
--  Taken from http://lua-users.org/lists/lua-l/2016-05/msg00297.html

local arg = _G.arg

if arg ~= nil and type(arg) ~= "table" then
    error("VDLp error: 'arg' global variable, if present, must be a table containing command-line arguments as strings.")
end

if arg then
    for i = 1, #arg do
        if type(arg[i]) ~= "string" then
            error("VDLp error: 'arg' table must be nil or a table containing command-line arguments as strings; it contains a "
                .. tostring(type(args[i])) .. " at position #" .. i .. ".")
        end
    end
end

local VDLp = {
    Version = "0.0.1",

    Verbose = false,
    Debug = true,
}

do
    local a, b, c = VDLp.Version:match "(%d+)%.(%d+)%.(%d+)"
    a, b, c = tonumber(a), tonumber(b), tonumber(c)

    if not a or not b or not c then
        error("VDLp internal error: Version string malformed.")
    end

    if a > 999 or b > 999 or c > 999 then
        error("VDLp internal error: Version out of bounds; components may not exceed '999'.")
    end

    VDLp.VersionNumber = a * 1000000 + b * 1000 + c
end

if VDLp.Debug then
    VDLp.Verbose = true
end

VDLp.Description = "VDLp " .. VDLp.Version .. " [" .. VDLp.VersionNumber
.. "] (c) 2017 Alexandru-Mihai Maftei, running under " .. luaVersion

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Error handling

function MSG(...)
    if not VDLp.Verbose then
        return
    end

    local items = {...}

    for i = 1, #items do
        items[i] = tostring(items[i])
    end

    print(table.concat(items))
end

local function prettyString(val)
    if type(val) == "string" then
        return string.format("%q", val)
    else
        return tostring(val)
    end
end

local function printTable(t, append, indent, done)
    local myIndent = string.rep("\t", indent)

    for key, value in pairs(t) do
        append(myIndent)
        append(prettyString(key))

        if typeEx(value) == "table" and not done[value] then
            done[value] = true

            local ts = tostring(value)

            if ts:sub(1, 9) == "table: 0x" then
                append ": "
                append(ts)
                append "\n"
                printTable(value, append, indent + 1, done)
            else
                append " = "
                append(ts)
                append "\n"
            end
        else
            append " = "
            append(prettyString(value))
            append "\n"
        end
    end
end

function table.tostring(t, indent)
    indent = indent or 0

    local res = {}
    local function append(val)
        res[#res + 1] = val
    end

    printTable(t, append, indent, { [_G] = true, [VDLp] = true })

    return table.concat(res);
end

local function printEndData(self, indent, out)
    indent = indent or 0

    local myIndent = string.rep("\t", indent)

    local res = { myIndent, "ERROR:\t", self.Error, "\n" }
    local function append(val, blergh, ...)
        res[#res + 1] = val

        if blergh then
            append(blergh, ...)
        end
    end

    myIndent = myIndent .. "\t"

    for i = 1, #self.Stack do
        local info, appendSource = self.Stack[i]

        append(myIndent)
        append(tostring(i))
        append "\t"

        if VDLp.Verbose or VDLp.Debug then
            function appendSource()
                append "\n"
                append(myIndent)
                append "\t  location: "

                if info.what ~= "C" then
                    append(info.short_src or info.source)

                    if info.currentline then
                        append ":"
                        append(tostring(info.currentline))
                    end

                    append "\n"

                    if info.what == "Lua" and info.linedefined and (info.linedefined ~= info.currentline or info.lastlinedefined ~= info.currentline) then
                        --  This hides the definition of inline functions that will contain the erroneous line of code anyway.
                        --  Also excludes C code and main chunks, which show placeholder values there.

                        append(myIndent)
                        append "\t  definition at "

                        append(tostring(info.linedefined))

                        if info.lastlinedefined then
                            append "-"
                            append(tostring(info.lastlinedefined))
                        end

                        append "\n"
                    end
                else
                    append "C\n"
                end
            end
        else
            function appendSource()
                append "\t"

                if info.what ~= "C" then
                    append "  "
                    append(info.short_src or info.source)

                    if info.currentline then
                        append ":"
                        append(tostring(info.currentline))
                    end

                    if info.what == "Lua" and info.linedefined and (info.linedefined ~= info.currentline or info.lastlinedefined ~= info.currentline) then
                        --  This hides the definition of inline functions that will contain the erroneous line of code anyway.
                        --  Also excludes C code and main chunks, which show placeholder values there.

                        append "\t  def. at "

                        append(tostring(info.linedefined))

                        if info.lastlinedefined then
                            append "-"
                            append(tostring(info.lastlinedefined))
                        end
                    end

                    append "\n"
                else
                    append "  [C]\n"
                end
            end
        end

        if info.istailcall then
            append "tail calls..."

            if info.short_src or info.source then
                appendSource()
            else
                append "\n"
            end
        elseif info.name then
            --  This is a named function!

            append(info.namewhat)
            append "\t"
            append(info.name)
            appendSource()
        elseif info.what == "Lua" then
            --  Unnamed Lua function = anonymous

            append "anonymous function"
            appendSource()
        elseif info.what == "main" then
            --  This is the main chunk of code that is executing.

            append "main chunk"
            appendSource()
        elseif info.what == "C" then
            --  This might be what invoked the main chunk.

            append "unknown C code\n"
        end

        --  Now, upvalues, locals, parameters...

        if info.what ~= "C" and VDLp.Verbose then
            --  Both main chunk and Lua functions can haz these.

            append(myIndent)
            append "\t  "

            if info.nparams == 1 then
                append "1 parameter; "
            else
                append(tostring(info.nparams))
                append " parameters; "
            end

            if #info._locals - info.nparams == 1 then
                append "1 local; "
            else
                append(tostring(#info._locals - info.nparams))
                append " locals; "
            end

            if info.nups == 1 then
                append "1 upvalue\n"
            else
                append(tostring(info.nups))
                append " upvalues\n"
            end

            local extraIndent = myIndent .. "\t\t"
            local done = { [_G] = true, [VDLp] = true, [self] = true }

            for j = 1, #info._locals do
                local key, value = info._locals[j].name, info._locals[j].value

                append(extraIndent)
                append(prettyString(key))

                if typeEx(value) == "table" and not done[value] then
                    done[value] = true

                    local ts = tostring(value)

                    if ts:sub(1, 9) == "table: 0x" then
                        append ": "
                        append(ts)
                        append "\n"
                        printTable(value, append, indent + 4, done)
                    else
                        append " = "
                        append(ts)
                        append "\n"
                    end
                else
                    append " = "
                    append(prettyString(value))
                    append "\n"
                end
            end

            if info._upvalues then
                for j = 1, #info._upvalues do
                    local key, value = info._upvalues[j].name, info._upvalues[j].value

                    append(extraIndent)
                    append(prettyString(key))

                    if typeEx(value) == "table" and not done[value] then
                        done[value] = true

                        local ts = tostring(value)

                        if ts:sub(1, 9) == "table: 0x" then
                            append ": "
                            append(ts)
                            append "\n"
                            printTable(value, append, indent + 4, done)
                        else
                            append " = "
                            append(ts)
                            append "\n"
                        end
                    else
                        append " = "
                        append(prettyString(value))
                        append "\n"
                    end
                end
            end
        end
    end

    res = table.concat(res)

    if out then
        return out(res)
    else
        return res
    end
end

local function queuefunc(fnc, functionlist, donefunctions)
    if donefunctions[fnc] then return end

    functionlist[#functionlist+1] = fnc
    donefunctions[fnc] = true
end

function GatherEndState(err)
    local functionlist, donefunctions = {}, {}

    local stack, thesaurus = {}, {}

    local i = 2

    while true do
        local info = debug.getinfo(i)

        if not info then
            break
        end

        stack[#stack+1] = info
        info._stackpos = i

        if info.func then
            thesaurus[info.func] = info
            queuefunc(info.func, functionlist, donefunctions)
            info.func = tostring(info.func)
        end

        --if info.what == "Lua" then
        info._locals = {}
        local locals = info._locals

        local j = 1

        while true do
            local n, v = debug.getlocal(i, j)

            if not n then break end

            locals[#locals+1] = {name=n,value=v}

            if type(v) == "function" then
                queuefunc(v, functionlist, donefunctions)
                locals[#locals].value = tostring(locals[#locals].value)
            end

            j = j + 1
        end
        --end

        i = i + 1
    end

    for i = 1, #functionlist do
        local fnc = functionlist[i]

        local info

        if thesaurus[fnc] then
            info = thesaurus[fnc]
        else
            info = debug.getinfo(fnc)
            thesaurus[fnc] = info
        end

        local ups = {}
        info._upvalues = ups

        for j = 1, info.nups do
            local n, v = debug.getupvalue(fnc, j)

            ups[#ups+1] = {name=n,value=v}

            if type(v) == "function" then
                queuefunc(v, functionlist, donefunctions)
                ups[#ups].value = tostring(ups[#ups].value)
            end
        end
    end

    local newthesaurus = {}

    for k, v in pairs(thesaurus) do
        newthesaurus[tostring(k)] = v
    end

    return { Stack = stack, Thesaurus = newthesaurus, Error = err, ShortTrace = debug.traceback(nil, 2), Print = printEndData }
end

do
    --  A small workaround around old xpcall versions.

    local foo = 1

    xpcall(
        function(arg)
            if arg == "bar" then
                foo = 2
            else
                foo = 0
            end
        end,
        function(err)
            io.stderr:write("This really shouldn't have happened:\n", err, "\n")
            os.exit(-100)
        end,
        "bar")

    assert(foo ~= 1, "'foo' should have changed!")

    if foo == 0 then
        MSG("xpcall doesn't seem to support passing arguments; working around it.")

        function vcall(inner_function, ...)
            local wrapped_args = {...}

            local function wrapper_function()
                return inner_function(unpack(wrapped_args))
            end

            local ret = {xpcall(wrapper_function, GatherEndState)}

            if ret[1] then
                --  Call succeeded.

                table.remove(ret, 1)

                return { Return = ret }
            else
                --  Call failed!

                return ret[2]
            end
        end
    else
        function vcall(fnc, ...)
            local ret = {xpcall(fnc, GatherEndState, ...)}

            if ret[1] then
                --  Call succeeded.

                table.remove(ret, 1)

                return { Return = ret }
            else
                --  Call failed!

                return ret[2]
            end
        end
    end
end

function assertTypeEx(tname, valType, val, vname, errlvl)
    assert(type(valType) == "string", "VDLp internal error: Argument #2 to 'assertType' should be a string.")

    if type(tname) == "string" then
        if valType ~= tname then
            error("VDLp error: Type of " .. vname .. " should be '" .. tname .. "', not '" .. valType .. "'.", errlvl + 1)
        end
    elseif type(tname) == "table" then
        assert(#tname > 0, "VDLp internal error: Argument #1 to 'assertType' should have a length greater than zero.")

        for i = #tname, 1, -1 do
            local tname2 = tname[i]

            assert(type(tname2) == "string", "VDLp internal error: Argument #1 to 'assertType' should be an array of strings.")

            if tname2 == "integer" then
                if valType == "number" and val % 1 == 0 then
                    return valType
                end

                --  Else just move on to the next type. No biggie.
            elseif valType == tname2 then
                return valType
            end
        end

        local tlist = "'" .. tname[1] .. "'"

        for i = 2, #tname - 1 do
            tlist = tlist .. ", '" .. tname[i] .. "'"
        end

        if #tname > 1 then
            tlist = tlist .. ", or '" .. tname[#tname] .. "'"
        end

        error("VDLp error: Type of " .. vname .. " should be " .. tlist .. ", not '" .. valType .. "'.", errlvl + 1)
    else
        error("VDLp internal error: Argument #1 to 'assertType' should be a string or an array of strings, not a '" .. type(tname) .. ".")
    end

    return valType
end

function assertType(tname, val, vname, errlvl)
    return assertTypeEx(tname, typeEx(val), val, vname, errlvl and (errlvl + 1) or 1)
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Class system

do
    local metamethods = {
        "add", "sub", "mul", "div", "mod", "pow", "unm", "concat", "len",
        "eq", "lt", "le", "gc", "tostring", "call"
    }

    for i = 1, #metamethods do
        metamethods[i] = "__" .. metamethods[i]
    end

    local classesFuncs = {}

    --  Will actually store the classes.
    local classStore = {}

    --  Should be quick.
    local propfuncs = {}

    --  Will hold the class bases metatables.
    local classBases = {}

    --  Inheritance table.
    local inheritance = {}
    --  Parents table.
    local parents = {}

    --  Prevent malicious changes.
    VDLp.Classes = setmetatable({}, {
        __index = function(self, key)
            return classesFuncs[key] or classStore[key]
        end,

        __newindex = function()
            error("VDLp error: Invalid operation; cannot change the VDLp.Classes table.")
        end,

        __metatable = "Nope."
    })

    local function addInheritance(cName, cParent)
        if cParent then
            if inheritance[cParent] then
                table.insert(inheritance[cParent], cName)
            else
                inheritance[cParent] = { cName }
            end

            addInheritance(cName, classStore[cParent].__parent and classStore[cParent].__parent.__name)
        else
            if inheritance[true] then
                table.insert(inheritance[true], cName)
            else
                inheritance[true] = { cName }
            end
        end
    end

    local function addParent(cName, cParent)
        parents[cName] = cParent
    end

    local function makePropFunc(prop, name, base)
        local pget, pset, povr, pret, ptyp = prop.get, prop.set, prop.OVERRIDE, prop.RETRIEVE, prop.TYPE

        local evname = "__on_update__" .. name

        local fnc

        if povr then
            if ptyp then
                pset = function(self, val, errlvl)
                    if typeEx(val) ~= ptyp then
                        error("VDLp Classes error: Property \"" .. name .. "\" value must be of type " .. tostring(ptyp), errlvl + 1)
                    end

                    rawset(self, povr, val)
                end
            else
                pset = function(self, val)
                    rawset(self, povr, val)
                end
            end

            pget = function(self)
                return rawget(self, povr)
            end

            prop.get, prop.set = pget, pset
        elseif pret then
            pget = function(self)
                return rawget(self, pret)
            end

            prop.get = pget
        end

        if pget then
            if pset then
                if ptyp then
                    fnc = function(self, write, val, errlvl)
                        if not errlvl then errlvl = 2 else errlvl = errlvl + 1 end

                        if write then
                            if typeEx(val) ~= ptyp then
                                error("VDLp Classes: Property \"" .. name .. "\" value must be of type " .. tostring(ptyp), errlvl)
                            end

                            pset(self, val, errlvl)

                            if self[evname] then
                                self[evname](self, val, errlvl)
                            end
                        else
                            return pget(self, errlvl)
                        end
                    end
                else
                    fnc = function(self, write, val, errlvl)
                        if not errlvl then errlvl = 2 else errlvl = errlvl + 1 end

                        if write then
                            pset(self, val, errlvl)

                            if self[evname] then
                                self[evname](self, val, errlvl)
                            end
                        else
                            return pget(self, errlvl)
                        end
                    end
                end
            else
                fnc = function(self, write, errlvl)
                    if not errlvl then errlvl = 2 else errlvl = errlvl + 1 end

                    if write then
                        error("VDLp Classes: Property \"" .. name .. "\" has no setter!", errlvl)
                    else
                        return pget(self, errlvl)
                    end
                end
            end
        else
            return false
        end

        propfuncs[prop] = fnc
    end

    local function spawnClass(cName, cParentName, cBase)
        local cParent, cParentBase

        local __base_new_indexer, __base_indexer

        for k, v in pairs(cBase) do
            if type(v) == "table" then
                makePropFunc(v, k, cBase)
            end
        end

        if cParentName then
            --  By now, they MUST exist.
            cParent = classStore[cParentName]
            cParentBase = classBases[cParentName]

            __base_indexer = cParentBase.__index
            __base_new_indexer = cParentBase.__newindex
        end

        local __new_indexer, __indexer = cBase.__newindex or rawset, cBase.__index or __base_indexer

        local _base_0 = {
            __metatable = "What do you want to do with this class instance's metatable?",

            __index = function(self, key, errlvl)
                if key == "__class" then
                    return classBases[cName].__class
                end

                local res = rawget(cBase, key)
                --  Will look for a property.

                if not errlvl then errlvl = 1 end

                if propfuncs[res] then
                    return propfuncs[res](self, false, nil, errlvl + 1)
                end

                if res == nil and __indexer then
                    return __indexer(self, key, errlvl + 1)
                end

                return res
            end,

            __newindex = function(self, key, val, errlvl)
                local res = rawget(cBase, key)
                --  Will look for a property.

                if not errlvl then errlvl = 1 end

                if res ~= nil and propfuncs[res] then
                    return propfuncs[res](self, true, val, errlvl + 1)
                elseif res == nil and __base_new_indexer then
                    --  If nil, keep looking up the hierarchy for a valid property.
                    --  If none is found, it will eventually just set the value on the table.
                    return __base_new_indexer(self, key, val, errlvl + 1)
                else
                    return __new_indexer(self, key, val, errlvl + 1)
                end
            end
        }

        for i = #metamethods, 1, -1 do
            local mm = metamethods[i]

            if cBase[mm] then
                _base_0[mm] = cBase[mm]
            elseif cParentBase and cParentBase[mm] then
                _base_0[mm] = cParentBase[mm]
            end
        end

        local __init = cBase.__init

        local function __init2(...)
            if cParent then
                cParent.__inheritedInitializer(...)
            end

            __init(...)
        end

        local actualInitializer = (cBase.__init_inherit ~= false) and __init2 or __init

        local _class_0 = setmetatable({
            __init = __init,    --  Poor attempt to prevent retarded usage by changing that function...
            __base = cBase,
            __name = cName,
            __parent = cParent,


            __inheritedInitializer = actualInitializer,
        }, {
            __metatable = "Why do you need to work with this class's metatable?",

            __index = function (cls, name)
                local val = rawget(cBase, name)

                if val == nil and cParent then
                    return cParent[name]
                else
                    return val
                end
            end,

            __call = function (cls, ...)
                local new = setmetatable({ }, _base_0)

                cls.__inheritedInitializer(new, ...)

                return new
            end,
        })

        _base_0.__class = _class_0

        classBases[cName] = _base_0

        addInheritance(cName, cParentName)

        return _class_0
    end

    --  Formal class creation function.
    function classesFuncs.Create(cName, cParentName, cBase)
        if type(cName) == "table" and cParentName == nil and cBase == nil then
            return VDLp.CreateClass(cName.Name, cName.ParentName, cName.Base)
        end

        if type(cName) ~= "string" then
            error("Class name must be a string, not a " .. type(cName) .. ".")
        else
            if classStore[cName] then
                error("Class named \"" .. tostring(cName) .. "\" already defined!!")
            end
        end

        if cParentName ~= nil and type(cParentName) ~= "string" then
            error("Parent class name must be nil or a string, not a " .. type(cParentName) .. ".")
        elseif cParentName and not classStore[cParentName] then
            error("Specified parent class does not exist (yet?)!")
        end

        if type(cBase) == "function" then
            cBase = cBase()
            --  "Unpack" the function which is supposed to return a table.

            if type(cBase) ~= "table" then
                error("Class base unpacker must return a table, not a " .. type(cBase) .. ".")
            end
        elseif type(cBase) ~= "table" then
            error("Class base must be a table or a function that returns a table (referred to as \"unpacker\"), not a " .. type(cBase) .. ".")
        end

        local class

        class = spawnClass(cName, cParentName, cBase)

        classStore[cName] = class
        classStore[class] = cName

        return class
    end

    --  Returns the name of each class that inherits from the given class.
    --  True = all classes.
    function classesFuncs.GetChildrenOf(cls)
        if type(cls) == "string" then
            if classStore[cls] then
                return inheritance[cls] or {}
            else
                error("No class named \"" .. cls .. "\" declared (yet?).")
            end
        elseif cls == true then
            return inheritance[true] or {}
        else
            error("Invalid argument! Must be a valid class name or true for listing defined classes!")
        end
    end

    --  Returns the name of the parent of the given class.
    function classesFuncs.GetParentOf(cls)
        if type(cls) == "string" then
            if classStore[cls] then
                return parents[cls]
            else
                error("No class named \"" .. cls .. "\" declared (yet?).")
            end
        else
            error("Invalid argument! Must be a valid class name.")
        end
    end
end

function typeEx(val)
    local t = type(val)

    if t ~= "table" then
        return t
    else
        local class = val.__class

        if class then
            return VDLp.Classes[class] or t
            --  This will not work for classes that are not actually declared.
        else
            return t
        end
    end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Paths

function string.trim(s)
    return s:match("^%s*(.-)%s*$")
end

function string.split(s, pattern, plain, n)
    --  Borrowed from https://github.com/stevedonovan/Penlight/blob/master/lua/pl/utils.lua#L172

    local i1, res = 1, {}

    if not pattern then pattern = plain and ' ' or "%s+" end
    if pattern == '' then return {s} end

    while true do
        local i2, i3 = s:find(pattern, i1, plain)

        if not i2 then
            local last = s:sub(i1)

            if last ~= '' then
                table.insert(res, last)
            end

            if #res == 1 and res[1] == '' then
                return {}
            else
                return res
            end
        else
            table.insert(res, s:sub(i1, i2 - 1))

            if n and #res == n then
                res[#res] = s:sub(i1)

                return res
            end

            i1 = i3 + 1
        end
    end
end

function string.iteratesplit(s, pattern, plain, n)
    local cnt, i1, i2, i3, res = 0, 1

    if not pattern then pattern = plain and ' ' or "%s+" end
    if pattern == '' then return {s} end

    return function()
        if not i1 then return end
        --  Means the end was reached.

        i2, i3 = s:find(pattern, i1, plain)

        if not i2 then
            local last = s:sub(i1)
            i1 = nil

            return last
        else
            cnt = cnt + 1

            if n and cnt == n then
                return s:sub(i1)
            end

            local res = s:sub(i1, i2 - 1)
            i1 = i3 + 1

            return res, s:sub(i2, i3)
        end
    end
end

do
    local typesEqualities = {
        f = {
            f = true,
            d = false,
            U = true,
        },
        d = {
            f = false,
            d = true,
            U = true,
        },
        U = {
            f = true,
            d = true,
            U = true,
        },
    }

    -- local function trimStartCurrentDirectory(str)
    --     if str:sub(1, 2) == "./" then
    --         return str:sub(3)
    --     else
    --         return str
    --     end
    -- end

    local function trimStartDirectory(str)
        return str:match("^/*[^/]+/(.-)$")
    end

    local function trimEndDirectory(str)
        return str:match("^(.-)/[^/]+/*$")
    end

    local function getFileName(str)
        return str:match("^.-/([^/]+)/*$")
    end

    local function canonizePath(str)
        local i1, res, len, frag = 1, {}, 0

        if #str < 2 then
            return str
        end

        while true do
            local i2, i3 = str:find("/+", i1, false)

            frag = str:sub(i1, i2 and (i2 - 1) or nil)

            if frag == ".." and len > 0 and res[len] ~= ".." then
                if res[len] ~= "" and res[len] ~= "." then
                    res[len] = nil

                    len = len - 1
                end

                --  Else do nothing.
            elseif frag ~= '.' then
                len = len + 1

                res[len] = frag
            end

            if i2 then
                i1 = i3 + 1
            else
                if len >= 2 and #(res[len]) == 0 and #(res[len - 1]) > 0 then
                    res[len] = nil

                    len = len - 1
                end

                if len == 0 or (len == 1 and res[1] == "") then
                    return str:sub(1, 1) == '/' and '/' or '.'
                else
                    return table.concat(res, '/')
                end
            end
        end
    end

    local function isPathAbsolute(str)
        return str[1] == '/'
    end

    VDLp.Classes.Create("Path", nil, {
        __init = function(self, val, type, mtime)
            self._key_path_valu = canonizePath(val)
            self._key_path_abso = isPathAbsolute(self._key_path_valu)
            self._key_path_leng = #self._key_path_valu
            self._key_path_type = type or 'U'
            -- self._key_path_mtim = mtime or false
            -- self._key_path_exst = mtime and true or nil
        end,

        __tostring = function(self)
            return self._key_path_valu
        end,

        ToString = function(self)
            return self._key_path_valu
        end,

        Equals = function(self, othr)
            if othr == nil then
                return false
            end

            local othrType = typeEx(othr)

            if othrType == "string" then
                if self._key_path_type == 'f' then
                    --  If the other path ends with a slash, it's definitely not a file.
                    return othr:sub(-1) ~= "/" and self._key_path_valu == canonizePath(othr)
                else
                    return self._key_path_valu == canonizePath(othr)
                end
            elseif othrType == "Path" then
                local compat = typesEqualities[self._key_path_type][othr._key_path_type]

                if compat then
                    return self._key_path_valu == othr._key_path_valu
                else
                    if VDLp.Debug and self._key_path_valu == othr._key_path_valu then
                        error("VDLp error: Paths have incompatible types but identical value: " .. self._key_path_valu, 2)
                    else
                        return false
                    end
                end
            end

            error("VDLp error: Nonsensical comparison between a Path and a '" .. othrType .. "'.", 2)
        end,

        __eq = function(self, othr)
            if not rawget(othr, _key_path_type) then
                if VDLp.Debug then
                    error("VDLp error: Nonsensical comparison between a Path and a '" .. typeEx(othr) .. "'.", 2)
                else
                    return false
                end
            end

            local compat = typesEqualities[self._key_path_type][othr._key_path_type]

            if compat then
                return self._key_path_valu == othr._key_path_valu
            else
                if VDLp.Debug and self._key_path_valu == othr._key_path_valu then
                    error("VDLp error: Paths have incompatible types but identical value: " .. self._key_path_valu, 2)
                else
                    return false
                end
            end
        end,

        __add = function(self, othr)
            if self._key_path_type == "f" then
                error("VDLp error: You cannot append a path to a file.", 2)
            end

            local otherPathType = 'U'
            local othrType = assertType({"string", "Path"}, othr, "other path", 2)

            if othrType == "Path" then
                otherPathType = othr._key_path_type
                othr = othr._key_path_valu
            elseif #othr < 1 then
                error("VDLp error: Path to append must be non-empty.", 2)
            end

            if isPathAbsolute(othr) then
                error("VDLp error: Cannot append an absolute path to any other path.", 2)
            end

            return VDLp.Classes.Path(self._key_path_valu .. '/' .. othr, otherPathType)
        end,

        __concat = function(left, right)
            local leftType = assertType({"string", "number", "Path"}, left, "left operand", 2)

            if leftType == "Path" then
                local rightType = assertType({"string", "number"}, right, "right operand", 2)

                if rightType == "number" then
                    right = tostring(right)
                end

                return VDLp.Classes.Path(left._key_path_valu .. right, 'U')
            else
                --  Right operand has to be the path.

                if leftType == "number" then
                    left = tostring(left)
                end

                return left .. right._key_path_valu
            end
        end,

        Absolute         = { RETRIEVE = "_key_path_abso" },
        Length           = { RETRIEVE = "_key_path_leng" },
        Type             = { RETRIEVE = "_key_path_type" },
        -- ModificationTime = { RETRIEVE = "_key_path_mtim" },
        -- Exists           = { RETRIEVE = "_key_path_exst" },

        IsFile      = { get = function(self) return self._key_path_type == 'f' end },
        IsDirectory = { get = function(self) return self._key_path_type == 'd' end },
        IsUnknown   = { get = function(self) return self._key_path_type == 'U' end },

        IsCurrentDirectory = { get = function(self) return self._key_path_valu == '.' end },

        GetParent = function(self)
            local val = self._key_path_valu

            if val == "." or val == ".." or val == "/" then
                return nil
            end

            val = trimEndDirectory(val)

            if not val then
                return nil
            end

            local res = VDLp.Classes.Path(val, "d")

            -- if self._key_path_exst then
            --     assurePathInfo(res)
            -- end

            return res
        end,

        GetName = function(self)
            local val = self._key_path_valu

            if val == "." or val == ".." or val == "/" then
                return self
            end

            return VDLp.Classes.Path(getFileName(val), "U")
        end,

        StartsWith = function(self, other)
            local otherType = assertType({"string", "Path"}, other, "start path", 2)

            if otherType == "string" then
                other = canonizePath(other)
            elseif otherType == "Path" then
                other = other._key_path_valu
            end

            if #other > self._key_path_leng then
                return false
            end

            local i1, i2 = self._key_path_valu:find(other, 1, true)

            if not i1 or i1 ~= 1 then
                return false
            end

            if i2 == self._key_path_leng then
                return true
            elseif self._key_path_valu:sub(i2 + 1, i2 + 1) == '/' then
                --  Okay, this is correct.

                return true
            else
                return false
            end
        end,

        EndsWith = function(self, ...)
            local vals = {...}

            if #vals < 1 then
                error("VDLp error: 'EndsWith' should receive at least one argument, besides the path itself.", 2)
            end

            for i = 1, #vals do
                local val = vals[i]
                local valType = assertType({"string", "Path"}, val, "end string #" .. i, 2)

                if valType == "string" then
                    if #val < 1 then
                        error("VDLp error: Argument #" .. i .. " to 'EndsWith' must be a non-empty string or a path.", 2)
                    end

                    if val == self._key_path_valu:sub(-#val) then
                        return true
                    end
                else
                    --  It's a path, so it must be contained fully within a directory
                    --  in this path.

                    local len = val.Length
                    local len2 = self._key_path_leng - len

                    if self._key_path_valu:sub(len2, len2) == '/'
                        and val._key_path_valu == self._key_path_valu:sub(-len) then
                        return true
                    end
                end
            end

            return false
        end,

        TrimEnd = function(self, cnt)
            assertType("number", cnt, "count", 2)

            if cnt < 1 or cnt % 1 ~= 0 then
                error("VDLp error: Argument #1 to 'TrimEnd' should be a positive integer.", 2)
            end

            local str = self._key_path_valu

            if cnt >= self._key_path_leng then
                error("VDLp error: Argument #1 to 'TrimEnd' is too large, it leaves nothing in the path \""
                    .. str .. "\".", 2)
            end

            return VDLp.Classes.Path(str:sub(1, self._key_path_leng - cnt), 'U')
            --  The type cannot be preserved here.
        end,

        RemoveExtension = function(self)
            local new = self._key_path_valu:match("^(.*)%.[^%.]-$")

            return new and VDLp.Classes.Path(new, 'U') or nil
        end,

        ChangeExtension = function(self, ext)
            assertType("string", ext, "extension", 2)

            local new = self._key_path_valu:match("^(.*)%.[^%.]-$")

            return new and VDLp.Classes.Path(new .. "." .. ext, 'U') or nil
        end,

        GetExtension = function(self)
            return self._key_path_valu:match("^.*%.([^%.]-)$")
        end,

        CheckExtension = function(self, ...)
            local vals = {...}

            if #vals < 1 then
                error("VDLp error: 'CheckExtension' should receive at least one argument, besides the path itself.", 2)
            end

            local ext = self._key_path_valu:match("^.*%.([^%.]-)$")

            if not ext then
                --  No extension? Meh.

                return false
            end

            for i = #vals, 1, -1 do
                local val = vals[i]
                local valType = assertType({"string", "List"}, val, "extension #" .. i, 2)

                if valType == "string" then
                    if ext == val then
                        return ext
                    end
                else
                    if val:Any(function(val) return ext == val end) then
                        return true
                    end
                end
            end

            return false
        end,

        SkipDirs = function(self, cnt)
            assertType("number", cnt, "count", 2)

            if cnt < 1 or cnt % 1 ~= 0 then
                error("VDLp error: Argument #1 to 'SkipDirs' should be a positive integer.", 2)
            end

            local str = self._key_path_valu

            for i = 1, cnt do
                str = trimStartDirectory(str)

                if #str < 1 then
                    error("VDLp error: Path does not contain " .. tostring(cnt) .. " directories.", 2)
                end
            end

            return VDLp.Classes.Path(str, self._key_path_type)
            --  No harm in preserving the type, I suppose. :S
        end,

        Skip = function(self, other)
            local otherType = assertType({"string", "Path"}, other, "path to skip", 2)

            if otherType == "string" then
                other = canonizePath(other)
            elseif otherType == "Path" then
                other = other._key_path_valu
            end

            if #other > self._key_path_leng then
                error("VDLp error: Path \"" .. tostring(self)
                    .. "\" cannot begin with \"" .. other
                    .. "\" because it is too short.", 2)
            end

            local i1, i2 = self._key_path_valu:find(other, 1, true)

            if not i1 or i1 ~= 1 then
                error("VDLp error: Path \"" .. tostring(self)
                    .. "\" does not begin with \"" .. other .. "\".", 2)
            end

            if i2 == self._key_path_leng then
                return VDLp.Classes.Path(".", self._key_path_type)
            elseif self._key_path_valu:sub(i2 + 1, i2 + 1) == '/' then
                --  Okay, this is correct.

                return VDLp.Classes.Path(self._key_path_valu:sub(i2 + 2), self._key_path_type)
            else
                error("VDLp error: Path \"" .. tostring(self)
                    .. "\" does not begin with \"" .. other .. "\".", 2)
            end
        end,

        Progress = function(self)
            local str, i = self._key_path_valu, 2

            local function iterator()
                if not i then
                    return  --  Nothing, zee end.
                end

                local i1 = str:find("/", i, true)

                if i1 then
                    local res = str:sub(1, i1 - 1)

                    i = i1 + 1

                    return res
                else
                    i = nil

                    return str
                end
            end

            return iterator
        end,
    })

    -- function assurePathInfo(path, force)
    --     if path._key_path_exst and not force then
    --         return path
    --     end

    --     local oldCodeLoc = codeLoc; codeLoc = 0

    --     local type, mtime = fs.GetInfo(path)

    --     codeLoc = oldCodeLoc

    --     if type then
    --         path._key_path_type = type
    --         path._key_path_mtim = mtime

    --         path._key_path_exst = true
    --     else
    --         path._key_path_exst = false
    --     end

    --     return path
    -- end
end

do
    local validFileTypes = { f = true, d = true, U = true }

    function Path(val, type)
        assertType("string", val, "path", 2)
        assertType({"nil", "string"}, type, "path type", 2)

        if #val < 1 then
            error("VDLp error: Path string must be non-empty.", 2)
        end

        if type and not validFileTypes[type] then
            error("VDLp error: Path type \"" .. type .. "\" is invalid.", 2)
        end

        return VDLp.Classes.Path(val, type or 'U')
    end

    function FilePath(val)
        assertType("string", val, "path", 2)

        if #val < 1 then
            error("VDLp error: Path string must be non-empty.", 2)
        end

        return VDLp.Classes.Path(val, "f")
    end

    function DirPath(val)
        assertType("string", val, "path", 2)

        if #val < 1 then
            error("VDLp error: Path string must be non-empty.", 2)
        end

        return VDLp.Classes.Path(val, "d")
    end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Lists

function table.shallowcopy(tab)
    local res = {}

    for k, v in pairs(tab) do
        res[k] = v
    end

    return res
end

function table.copyarray(tab)
    local res, len = {}, #tab

    for i = 1, len do
        res[i] = tab[i]
    end

    return res
end

function table.isarray(tab)
    local len = #tab

    for k, v in pairs(tab) do
        if type(k) ~= "number" or k < 1 or k > len or k % 1 ~= 0 then
            return false
        end
    end

    return true
end

VDLp.Classes.Create("List", nil, {
    __init = function(self, sealed, template)
        if (sealed or template ~= nil) and type(template) ~= "table" then
            error("VDLp error: List constructor's argument #2 must be a table, unless non-sealed.", 4)
        end

        rawset(self, "_key_list_cont", template and table.copyarray(template) or {})
        rawset(self, "_key_list_seld", sealed or false)
    end,

    __tostring = function(self)
        return "[List " .. tostring(self._key_list_cont) .. "]"
    end,

    Print = function(self, sep, conv)
        assertType({"nil", "string"}, sep, "separator", 2)
        assertType({"nil", "function"}, conv, "converter", 2)

        if not conv then conv = tostring end

        local stringz = sep and {} or {"[List Print", tostring(self._key_list_cont)}
        local stringzLen = #stringz

        for i = 1, #self._key_list_cont do
            stringz[i + stringzLen] = conv(self._key_list_cont[i])
        end

        if not sep then
            stringz[#stringz + 1] = "]"
        end

        return table.concat(stringz, sep or ' ')
    end,

    __index = function(self, key, errlvl)
        if type(key) == "number" then
            local cont = self._key_list_cont

            if key < 1 or key > #cont then
                error("VDLp error: Given numeric key to List is out of range.", errlvl + 1)
            end

            if key % 1 ~= 0 then
                error("VDLp error: Given numeric key to List is not an integer.", errlvl + 1)
            end

            return cont[key]
        end

        --  Whatever type of key this is, it is unacceptable.

        error("VDLp error: Given key to a List is of invalid type '" .. type(key) .. "'.", errlvl + 1)
    end,

    __newindex = function(self, key, val, errlvl)
        local keyType = type(key)

        if keyType == "string" and key ~= "__class" then
            --  Yes, random string keys are allowed to annotate lists.

            return rawset(self, key, val)
        elseif keyType == "number" then
            if self._key_list_seld then
                error("VDLp error: List is sealed.", errlvl + 1)
            end

            local cont = self._key_list_cont

            if key < 1 or key > #cont then
                error("VDLp error: Given numeric key to List is out of range.", errlvl + 1)
            end

            if key % 1 ~= 0 then
                error("VDLp error: Given numeric key to List is not an integer.", errlvl + 1)
            end

            cont[key] = val
            return
        end

        error("VDLp error: Given key to a List is of invalid type '" .. keyType .. "'.", errlvl + 1)
    end,

    Sealed = { RETRIEVE = _key_list_seld },

    Seal = function(self, tolerant)
        if not tolerant and self._key_list_seld then
            error("VDLp error: List is already sealed.", 2)
        end

        self._key_list_seld = true

        return self
    end,

    __len = function(self)
        return #self._key_list_cont
    end,

    Length = {
        get = function(self, errlvl)
            return #self._key_list_cont
        end,
    },

    __add = function(self, othr)
        local othrType = assertType({"table", "List"}, othr, "other list", 2)

        if othrType == "List" then
            othr = othr._key_list_cont
        elseif not table.isarray(other) then
            error("VDLp error: Table to concatenate with must be a pure array table.", 2)
        end

        local new = VDLp.Classes.List(false, self._key_list_cont)
        local arr, lenN, lenO = new._key_list_cont, #new._key_list_cont, #othr

        for i = 1, lenO do
            arr[lenN + i] = othr[i]
        end

        return new
    end,

    AppendMany = function(self, othr)
        local othrType = assertType({"table", "List"}, othr, "other list", 2)

        if othrType == "List" then
            othr = othr._key_list_cont
        elseif not table.isarray(othr) then
            error("VDLp error: Table to concatenate with must be a pure array table.", 2)
        end

        if self._key_list_seld then
            error("VDLp error: List is sealed.", 2)
        end

        local arr, lenS, lenO = self._key_list_cont, #self._key_list_cont, #othr

        for i = 1, lenO do
            arr[lenS + i] = othr[i]
        end

        return self
    end,

    Append = function(self, item)
        if self._key_list_seld then
            error("VDLp error: List is sealed.", 2)
        end

        self._key_list_cont[#self._key_list_cont + 1] = item

        return self
    end,

    AppendUnique = function(self, item)
        if self._key_list_seld then
            error("VDLp error: List is sealed.", 2)
        end

        local lenS, this = #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            if this[i] == item then
                return false
            end
        end

        this[lenS + 1] = item

        return self
    end,

    Where = function(self, pred)
        assertType("function", pred, "predicate", 2)

        local new = VDLp.Classes.List()
        local othr, lenS, this = new._key_list_cont, #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            local val = this[i]

            if pred(val) then
                othr[#othr + 1] = val
            end
        end

        return new
    end,

    Any = function(self, pred)
        assertType({"nil", "function"}, pred, "predicate", 2)

        if pred == nil then
            --  Simple.

            return #self._key_list_cont > 0
        end

        local lenS, this = #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            if pred(this[i]) then
                return true
            end
        end

        return false
    end,

    First = function(self, pred)
        assertType({"nil", "function"}, pred, "predicate", 2)

        if pred == nil then
            --  Simple.

            return self._key_list_cont[1]
        end

        local lenS, this = #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            if pred(this[i]) then
                return this[i]
            end
        end

        return nil
    end,

    Last = function(self, pred)
        assertType({"nil", "function"}, pred, "predicate", 2)

        local lenS, this = #self._key_list_cont, self._key_list_cont

        if pred == nil then
            --  Simple.

            return this[lenS]
        end

        for i = lenS, 1, -1 do
            if pred(this[i]) then
                return this[i]
            end
        end

        return nil
    end,

    Contains = function(self, val)
        local lenS, this = #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            if this[i] == val then
                return true
            end
        end

        return false
    end,

    Copy = function(self)
        local new = VDLp.Classes.List()
        local othr, lenS, this = new._key_list_cont, #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            othr[i] = this[i]
        end

        return new
    end,

    Subset = function(self, i, j)
        assertType({"integer"}, i, "start index", 1)
        assertType({"nil", "integer"}, j, "end index", 1)

        if j then
            assert(i <= j)

            if j > #self._key_list_cont then j = #self._key_list_cont end
        else
            j = #self._key_list_cont
        end

        if i < 1 then i = 0 else i = i - 1 end

        local new = VDLp.Classes.List()
        local othr, lenS, this = new._key_list_cont, #self._key_list_cont, self._key_list_cont

        for k = 1, j - i do
            othr[k] = this[i + k]
        end

        return new
    end,

    Select = function(self, trans)
        assertType("function", trans, "transformer", 2)

        local new = VDLp.Classes.List()
        local othr, lenS, this = new._key_list_cont, #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            othr[i] = trans(this[i])
        end

        return new
    end,

    SelectUnique = function(self, trans)
        assertType("function", trans, "transformer", 2)

        local new = VDLp.Classes.List()
        local othr, lenS, this = new._key_list_cont, #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            local item, add = trans(this[i]), true

            for j = 1, #othr do
                if othr[j] == item then
                    add = false
                    break
                end
            end

            if add then
                othr[#othr + 1] = item
            end
        end

        return new
    end,

    ForEach = function(self, action, extreme)
        assertType("function", action, "action", 2)

        local lenS, this, skip = #self._key_list_cont, self._key_list_cont

        for i = 1, lenS do
            extreme, skip = action(this[i], i, extreme)

            if skip then break end
        end

        return self, extreme
    end,

    Unpack = function(self)
        return unpack(self._key_list_cont)
    end,
})

local function parseListElement(line, linelen, i) end
--  Forward declaration

do
    local bracketPairs = { ["["] = "]", ["]"] = "["
                        ,  ["("] = ")", [")"] = "("
                        ,  ["{"] = "}", ["}"] = "{" }

    function parseListElement(line, linelen, i)
        local res = {}

        local inEscape, inSQ, inDQ, afterSpecial = false, false, false, false
        local parenStack, quoteStart = {}

        local firstChar = line:sub(i, i)
        local firstCharSpecial = string.find("!$", firstChar, 1, true)
        -- local firstCharSpecial = string.find("!@$", firstChar, 1, true)

        if not firstCharSpecial then
            firstChar = false
        end

        while i <= linelen do
            local c = line:sub(i, i)

            if not c then break end

            if afterSpecial then
                --  After a special character, only an opening square bracket or an identifier may follow.

                if c == "[" then
                    if afterSpecial ~= "!" then
                        parenStack[#parenStack + 1] = c
                    else
                        return table.concat(res), i, firstChar, string.format("Exclamation mark at character #%d must be followed directly by an identifier, not opening square brackets", i - 1)
                    end
                elseif c:match("([_a-zA-Z])") then
                    --  Okay, this is an identifier. Means a period must be added before it.

                    if afterSpecial ~= "!" then
                        res[#res + 1] = "."
                    end
                else
                    --  This ought to be wrong.

                    if afterSpecial == "!" then
                        return table.concat(res), i, firstChar, string.format("Special character '%s' at #%d must be followed directly by an identifier", afterSpecial, i - 1)
                    else
                        return table.concat(res), i, firstChar, string.format("Special character '%s' at #%d must be followed directly by an identifier or opening square brackets", afterSpecial, i - 1)
                    end
                end

                res[#res + 1] = c

                afterSpecial = false
            elseif inEscape then
                if c == 'n' then
                    res[#res + 1] = '\n'
                elseif c == 't' then
                    res[#res + 1] = '\t'
                elseif c == 'b' then
                    res[#res + 1] = '\b'
                else
                    res[#res + 1] = c
                end

                inEscape = false
            elseif quoteStart then
                if c == '\\' then
                    inEscape = true
                elseif inSQ and c == "'" then
                    quoteStart = nil
                    inSQ = false
                elseif inDQ and c == '"' then
                    quoteStart = nil
                    inDQ = false
                else
                    res[#res + 1] = c
                end
            else
                if c == '\\' then
                    if firstCharSpecial then
                        --  Escape sequences can only show up inside quotes if the first character is special.

                        return table.concat(res), i, firstChar, string.format("Escape sequences in special elements can only appear within quotes (strings); backslash at character #%d is invalid", i)
                    else
                        inEscape = true
                    end
                elseif c == '"' then
                    quoteStart = i
                    inDQ = true
                elseif c == "'" then
                    quoteStart = i
                    inSQ = true
                elseif string.find(" \t\n", c, 1, true) and #parenStack == 0 then
                    --  This is clearly an element separator.

                    i = i - 1

                    break
                elseif firstCharSpecial then
                    if string.find("([{", c, 1, true) then
                        parenStack[#parenStack + 1] = c
                        res[#res + 1] = c
                    elseif string.find(")]}", c, 1, true) then
                        if parenStack[#parenStack] == bracketPairs[c] then
                            parenStack[#parenStack] = nil
                            res[#res + 1] = c
                        elseif #parenStack == 0 then
                            --  Stray bracket.

                            return table.concat(res), i, firstChar, string.format("Stray closing bracket '%s' at character #%d", c, i)
                        else
                            --  Unmatching bracket.

                            return table.concat(res), i, firstChar, string.format("Unexpected closing bracket '%s' at character #%d; expected '%s'", c, i, parenStack[#parenStack])
                        end
                    -- elseif c == "@" then
                    --     res[#res + 1] = " _"
                    --     afterSpecial = c
                    elseif c == "$" then
                        res[#res + 1] = " env"
                        afterSpecial = c
                    elseif c == "!" then
                        res[#res + 1] = " "
                        afterSpecial = c
                    else
                        res[#res + 1] = c
                    end
                else
                    res[#res + 1] = c
                end
            end

            i = i + 1
        end

        res = table.concat(res)

        if inEscape then
            return res, i, firstChar, "Unfinished escape sequence at character #" .. (i - 1)
        end

        if inSQ then
            return res, i, firstChar, "Unpaired single quotes at character #" .. quoteStart
        end

        if inDQ then
            return res, i, firstChar, "Unpaired double quotes at character #" .. quoteStart
        end

        if #parenStack > 0 then
            return res, i, firstChar, "Missing closing brackets for the following opened brackets: " .. table.concat(parenStack)
        end

        return res, i, firstChar
    end
end

local function parseListInner(line, linelen)
    local res, i, special, err = nil, 1
    local arr = {}

    while i <= linelen do
        local c, oldPos = line:sub(i, i), i

        if not string.find(" \t\n", c, 1, true) then
            --  Whitespaces are completely ignored between elements.

            res, i, special, err = parseListElement(line, linelen, i)

            if res ~= nil then
                arr[#arr + 1] = { Text = res, Special = special, Position = oldPos }
            end

            if err then
                return arr, err, i
            end
        end

        i = i + 1
    end

    return arr
end

local function parseList(str)
    return parseListInner(str, #str)
end

do
    local listCache = {}
    --  Note, this caches the list creator chunks, not the lists themselves.

    function List(tab)
        local tabType = assertType({"string", "table"}, tab, "list array", 2)

        if listCache[tab] then
            return listCache[tab]()
        end

        if tabType == "table" then
            if not table.isarray(tab) then
                error("VDLp error: List must be initialized with a pure array table.", 2)
            end

            local res = VDLp.Classes.List(false, tab)

            listCache[tab] = res

            return res
        else
            local lst, err = parseList(tab)

            if err then
                error("VDLp error: List syntax error: " .. err, 2)
            end

            local template, values = { "return List {\n" }, { }

            for i = 1, #lst do
                values[i] = lst[i].Text

                if lst[i].Special then
                    -- if lst[i].Special == "@" then
                    --     error("VDLp error: List semantics error: Cannot use special character '@' in this context.", 2)
                    -- end

                    if i == 1 then
                        template[2] = "%s\n"
                    else
                        template[i + 1] = ", %s\n"
                    end
                else
                    if i == 1 then
                        template[2] = "%q\n"
                    else
                        template[i + 1] = ", %q\n"
                    end
                end
            end

            template[#template + 1] = "}\n"

            template = table.concat(template):format(unpack(values))

            --  Okay, that code now needs to be executed.

            local res, err = loadstring(template, "temporary list creator")

            if res then
                listCache[tab] = res

                return res()
            end

            local endState = GatherEndState(err)

            io.stderr:write("Failed to parse temporary list creator:\n"
                , template
                , endState:Print())

            os.exit(4)
        end
    end
end

local function cartesianProduct(a, b, first)
    local res = List { }

    if first then
        a:ForEach(function(x)
            res:AppendMany(b:Select(function(y)
                return List { x, y }
            end))
        end)
    else
        a:ForEach(function(x)
            res:AppendMany(b:Select(function(y)
                return x:Copy():Append(y)
            end))
        end)
    end

    return res
end

function CartesianProduct(...)
    local lists = {...}

    if #lists < 2 then
        error("VDLp error: Cartesian product needs at least two lists.")
    end

    for i = 1, #lists do
        assertType("List", lists[i], "cartesian product operand #" .. i, 2)
    end

    local res = cartesianProduct(lists[1], lists[2], true)

    for i = 3, #lists do
        res = cartesianProduct(res, lists[i], false)
    end

    return res
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Command-line argument handling

--  Command-line options defined.
local cmdlopts = {}

local function incrementCmdoCount(cmdo)
    cmdo._key_cmdo_cnts = cmdo._key_cmdo_cnts + 1
end

VDLp.Classes.Create("CmdOpt", nil, {
    __init = function(self, name)
        self._key_cmdo_name = name
        self._key_cmdo_desc = false
        self._key_cmdo_hndr = false
        self._key_cmdo_shnm = false
        self._key_cmdo_cnts = 0
        self._key_cmdo_type = false
        self._key_cmdo_disp = false
    end,

    __tostring = function(self)
        local shortname = self._key_cmdo_shnm

        if shortname then
            return "[CmdOpt " .. self._key_cmdo_name .. " | " .. shortname .. "]"
        else
            return "[CmdOpt " .. self._key_cmdo_name .. "]"
        end
    end,

    Name        = { RETRIEVE = "_key_cmdo_name" },
    Description = { OVERRIDE = "_key_cmdo_desc", TYPE = "string"  },
    Count       = { RETRIEVE = "_key_cmdo_cnts" },

    Many        = { OVERRIDE = "_key_cmdo_many", TYPE = "boolean" },
    Mandatory   = { OVERRIDE = "_key_cmdo_mand", TYPE = "boolean" },
    ShortPrefix = { OVERRIDE = "_key_cmdo_shpf", TYPE = "boolean" },

    Type = {
        TYPE = "string",

        get = function(self, errlvl)
            return self._key_cmdo_type
        end,

        set = function(self, val, errlvl)
            if self._key_cmdo_type then
                error("VDLp error: " .. tostring(self) .. " has already defined its type.", errlvl)
            end

            self._key_cmdo_type = val
        end,
    },

    Handler = {
        TYPE = "function",

        get = function(self, errlvl)
            return self._key_cmdo_hndr
        end,

        set = function(self, val, errlvl)
            if self._key_cmdo_hndr then
                error("VDLp error: " .. tostring(self) .. " has already defined its handler.", errlvl)
            end

            self._key_cmdo_hndr = val
        end,
    },

    ExecuteHandler = function(self, val)
        local han = self._key_cmdo_hndr

        if not han then
            error("VDLp error: " .. tostring(self) .. " does not define a handler.", 2)
        end

        --han = withEnvironment(han, getEnvironment(self))

        --local oldCodeLoc = codeLoc; codeLoc = LOC_CMDO_HAN

        local res = han(val)

        --codeLoc = oldCodeLoc
    end,

    ShortName = {
        TYPE = "string",

        get = function(self, errlvl)
            return self._key_cmdo_shnm
        end,

        set = function(self, val, errlvl)
            if self._key_cmdo_shnm then
                error("VDLp error: " .. tostring(self) .. " has already defined its short name.", errlvl)
            end

            if #val ~= 1 then
                error("VDLp error: Command-line option short name must be exactly one character long.", errlvl)
            end

            if val:find("%A") then
                error("VDLp error: Command-line option short name must be a letter (lowercase or uppercase).", errlvl)
            end

            if cmdlopts[val] then
                error("VDLp error: Cannot assign short name to "
                    .. tostring(self)
                    .. " because it is already assigned to "
                    .. tostring(cmdlopts[val]) .. ".", errlvl)
            end

            self._key_cmdo_shnm = val
            cmdlopts[val] = self
        end,
    },

    Display = {
        TYPE = "string",

        get = function(self, errlvl)
            return self._key_cmdo_disp
        end,

        set = function(self, val, errlvl)
            if self._key_cmdo_disp then
                error("VDLp error: " .. tostring(self) .. " has already defined its display value.", errlvl)
            end

            if #val < 1 then
                error("VDLp error: Command-line option display value must be non-empty.", errlvl)
            end

            self._key_cmdo_disp = val
        end,
    },
})

function CmdOpt(name)
    assertType("string", name, "command-line option long name", 2)

    if #name < 2 then
        error("VDLp error: Command-line option long name must be at least two characters long.", 2)
    end

    if cmdlopts[name] then
        error("VDLp error: Command-line option long name \"" .. name .. "\" conflicts with another.", 2)
    end

    local res = VDLp.Classes.CmdOpt(name)

    cmdlopts[#cmdlopts + 1] = res
    cmdlopts[name] = res

    local func

    func = function(val)
        local valType = assertType({"string", "table"}, val, "command-line option table / short name", 2)

        if valType == "table" then
            for k, v in pairs(val) do
                assertType("string", k, "table key", 2)

                res[k] = v
            end

            return res
        else
            res.ShortName = val

            return func
        end
    end

    return func
end

CmdOpt "help" "h" {
    Description = "Displays available command-line options and exita.",

    Handler = function()
        local parts = {}

        local function printDesc(val)
            if val.Description then
                for line in val.Description:iteratesplit("\n", true) do
                    parts[#parts + 1] = "        "
                    parts[#parts + 1] = line
                    parts[#parts + 1] = "\n"
                end
            end
        end

        if arg and arg[0] then
            parts[#parts + 1] = "Usage: "

            if arg[-1] then
                parts[#parts + 1] = arg[-1]
                parts[#parts + 1] = " "
            end

            parts[#parts + 1] = arg[0]
            parts[#parts + 1] = " [options] [--] [inputs]\n\n"
        end

        parts[#parts + 1] = --  Next lien :B
[[Compiled result goes to standard output by default.
Multiple inputs may be compiled at once into the same output.

]]

        for i = 1, #cmdlopts do
            local opt = cmdlopts[i]

            parts[#parts + 1] = "    --"
            parts[#parts + 1] = opt.Name

            if opt.Type then
                parts[#parts + 1] = " <"
                parts[#parts + 1] = opt.Display or opt.Type
                parts[#parts + 1] = ">"

                parts[#parts + 1] = "  --"
                parts[#parts + 1] = opt.Name
                parts[#parts + 1] = "=<"
                parts[#parts + 1] = opt.Display or opt.Type
                parts[#parts + 1] = ">"
            end

            if opt.ShortName then
                parts[#parts + 1] = "  -"
                parts[#parts + 1] = opt.ShortName

                if opt.Type then
                    parts[#parts + 1] = " <"
                    parts[#parts + 1] = opt.Display or opt.Type
                    parts[#parts + 1] = ">"

                    if opt.ShortPrefix then
                        parts[#parts + 1] = "  -"
                        parts[#parts + 1] = opt.ShortName
                        parts[#parts + 1] = "<"
                        parts[#parts + 1] = opt.Display or opt.Type
                        parts[#parts + 1] = ">"
                    end
                end
            end

            parts[#parts + 1] = "\n"

            printDesc(opt)

            parts[#parts + 1] = "\n"
        end

        parts[#parts + 1] = VDLp.Description

        print(table.concat(parts))
        os.exit(0)
    end,
}

CmdOpt "version" "v" {
    Description = "Displays brief information about VDLp and exits.",

    Handler = function()
        print(VDLp.Description)
        os.exit(0)
    end,
}

--  Inputs
local inputs = List { }

function VDLp.CheckArguments()
    if not arg then return end

    -- VDLp.TransferredArguments = List { }
    -- function baseEnvironment.TransferArgument(val)
    --     assertType("string", val, "transferred argument", 2)

    --     if val:sub(1, 2) ~= "--" then
    --         error("VDLp error: Transferred arguments must be in the form of long command-line option names.", 2)
    --     end

    --     local eqPos = val:find('=', 3, true)
    --     local optName = eqPos and val:sub(3, eqPos - 1) or val:sub(3)

    --     if not cmdlopts[optName] then
    --         error("VDLp error: Cannot transfer unregistered option \"" .. optName .. "\".", 2)
    --     end

    --     VDLp.TransferredArguments:Append(val)
    -- end

    local i, n, acceptOptions = 1, #arg, true

    while i <= n do
        local cur, usedNextArg, prefixed = arg[i], false, false

        if cur == "--" then
            acceptOptions = false
        elseif #cur > 0 then
            local handled = false

            local function doOption(long, optName, val, suffix)
                local opt = cmdlopts[optName]

                if not opt then
                    error("VDLp error: Unknown command-line option \"" .. optName .. "\".")
                end

                incrementCmdoCount(opt)

                if not opt.Many and opt.Count > 1 then
                    error("VDLp error: Multiple instances of command-line option \"" .. optName .. "\" found.")
                end

                local optType = opt.Type

                if optType then
                    if not val then
                        if opt.ShortPrefix and suffix then
                            val = suffix

                            prefixed = true
                        else
                            val = arg[i + 1]

                            if not val then
                                error("VDLp error: Command-line option \"" .. optName .. "\" lacks a value.")
                            end

                            usedNextArg = true
                        end
                    end

                    if optType == "boolean" then
                        local bVal = toboolean(val)

                        if bVal ~= nil then
                            val = bVal
                        else
                            error("VDLp error: Command-line argument value \""
                                .. val
                                .. "\" does not represent a valid boolean value, as required by option \""
                                .. optName .. "\".")
                        end
                    elseif optType == "number" then
                        local nVal = tonumber(val)

                        if nVal then
                            val = nVal
                        else
                            error("VDLp error: Command-line argument value \""
                                .. val
                                .. "\" does not represent a valid number, as required by option \""
                                .. optName .. "\".")
                        end
                    elseif optType == "integer" then
                        local nVal = tonumber(val)

                        if nVal and nVal % 1 == 0 then
                            val = nVal
                        else
                            error("VDLp error: Command-line argument value \""
                                .. val
                                .. "\" does not represent a valid integer, as required by option \""
                                .. optName .. "\".")
                        end
                    end
                end

                opt:ExecuteHandler(val)

                handled = true
            end

            if acceptOptions then
                if cur:sub(1, 2) == "--" then
                    --  So this is a long option name.

                    local eqPos = cur:find('=', 3, true)

                    local optName = eqPos and cur:sub(3, eqPos - 1) or cur:sub(3)

                    if #optName < 2 then
                        error("VDLp error: Command-line argument #" .. i .. " contains a long option name, but it is too short; should be at least two characters long.")
                    end

                    doOption(true,
                        optName,
                        eqPos and cur:sub(eqPos + 1) or nil,
                        nil)
                elseif cur:sub(1, 1) == '-' then
                    --  So, short option name.

                    for j = 2, #cur do
                        doOption(false, cur:sub(j, j), nil, cur:sub(j + 1))

                        if prefixed then
                            if j == 2 then break end

                            --  Otherwise, this is not valid.
                            error("VDLp error: Command-line argument #" .. i .. " contains a prefixed short name at character #" .. j .. ", however an argument may only contain one prefixed short option, if any, and nothing else.")
                        end
                    end
                end
            end

            if not handled then
                --  So, this ought to be an input.

                inputs:Append(Path(cur))
            end
        end

        i = i + (usedNextArg and 2 or 1)
    end

    --  Done.
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Lexer

VDLp.Classes.Create("LineOfCode", nil, {
    __init = function(self, file, line, start, prev)
        self._key_lico_file = file
        self._key_lico_line = line
        self._key_lico_strt = start
        self._key_lico_prev = prev
        self._key_lico_next = false
    end,

    File  = { RETRIEVE = "_key_lico_file" },
    Line  = { RETRIEVE = "_key_lico_line" },
    Start = { RETRIEVE = "_key_lico_strt" },

    PreviousLine = { RETRIEVE = "_key_lico_prev" },
    NextLine = { OVERRIDE = "_key_lico_next", TYPE = "LineOfCode" },

    __tostring = function(self)
        return string.format("%s:%d", self._key_lico_file, self._key_lico_line)
    end,
})

VDLp.Classes.Create("CodeReference", nil, {
    __init = function(self, loc, pos)
        self._key_core_lico = loc
        self._key_core_char = pos - loc.Start
    end,

    LineOfCode = { RETRIEVE = "_key_core_lico" },
    Char = { RETRIEVE = "_key_core_char" },

    File  = { get = function(self) return self._key_core_lico.File  end },
    Line  = { get = function(self) return self._key_core_lico.Line  end },

    __tostring = function(self)
        return string.format("%s:%d", self._key_core_lico, self._key_core_char)
    end,
})

local TT = { }

(List [[
EOF
IDENTIFIER INTEGER FLOAT
BRACKET_OPEN BRACKET_CLOSE
COMMA SEMICOLON
OPERATOR MEMBER_ACCESS
DOLLAR POUND
]]):ForEach(function(val)
    TT[#TT + 1] = val
    TT[val] = #TT
end)

VDLp.Classes.Create("Token", nil, {
    __init = function(self, loc, pos, tt, val)
        self._key_toke_core = VDLp.Classes.CodeReference(loc, pos)
        self._key_toke_type = tt
        self._key_toke_valu = val
    end,

    Type = { RETRIEVE = "_key_toke_type" },
    Value = { RETRIEVE = "_key_toke_valu" },
    Location = { RETRIEVE = "_key_toke_core" },

    File = { get = function(self) return self._key_toke_core.File  end },
    Line = { get = function(self) return self._key_toke_core.Line  end },
    Char = { get = function(self) return self._key_toke_core.Char  end },

    __tostring = function(self)
        return string.format("[Token @ %s - %s '%s']", self._key_toke_core, TT[self._key_toke_type], self._key_toke_valu)
    end,
})

do
    --  Operator lexer state graph.

    local operators = {
        "+", "-", "*", "/", "%",
        "|", "&", "^", "<<", ">>", "~",
        "!", "||", "&&", "==", "!=", "<", "<=", ">", ">=",
        "?", ":", "=",
    }

    local operatorStateTree = { }

    local function buildState(str, tab)
        local c = str:sub(1, 1)

        if not tab[c] then tab[c] = { } end

        if #str == 1 then
            tab[c].finish = true
        else
            buildState(str:sub(2), tab[c])
        end
    end

    for i = 1, #operators do
        buildState(operators[i], operatorStateTree)
    end

    --  Number lexer state graph.

    local floatEndState = { finish = true, float = true }

    local numberPostSeparatorState = { f = floatEndState, d = floatEndState, finish = true, float = true }
    for i = 0, 9 do numberPostSeparatorState[tostring(i)] = numberPostSeparatorState end

    local numberPreSeparatorState = { ['.'] = { error = "Expected decimals after decimal separator" }, f = floatEndState, d = floatEndState }
    for i = 0, 9 do numberPreSeparatorState[tostring(i)] = numberPreSeparatorState; numberPreSeparatorState['.'][tostring(i)] = numberPostSeparatorState end

    local integerBinStartState, integerBinState = { error = "Expected binary digit" }, { }
    for i = 0, 1 do integerBinState[tostring(i)] = integerBinState; integerBinStartState[tostring(i)] = integerBinState end

    local integerOctStartState, integerOctState = { error = "Expected octal digit" }, { }
    for i = 0, 7 do integerOctState[tostring(i)] = integerOctState; integerOctStartState[tostring(i)] = integerOctState end

    local integerDecStartState, integerDecState = { error = "Expected decimal digit" }, { }
    for i = 0, 9 do integerDecState[tostring(i)] = integerDecState; integerDecStartState[tostring(i)] = integerDecState end

    local integerHexStartState, integerHexState = { error = "Expected hexadecimal digit" }, { }
    for i = 0, 9 do integerHexState[tostring(i)] = integerHexState; integerHexStartState[tostring(i)] = integerHexState end
    (List "a b c d e f A B C D E F"):ForEach(function(i) integerHexState[i] = integerHexState; integerHexStartState[i] = integerHexState end)

    do
        local integerSuffixTemplate = { finish = true, U = { finish = true, L = { finish = true, L = { finish = true } } }, L = { finish = true, L = { finish = true } } }
        for k, v in pairs(integerSuffixTemplate) do integerBinState[k] = v; integerOctState[k] = v; integerDecState[k] = v; integerHexState[k] = v; numberPreSeparatorState[k] = v end
    end

    local numberInitState = {
        ['0'] = {
            error = "Expected base prefix after 0",

            x = integerHexStartState,
            d = integerDexStartState,
            o = integerOctStartState,
            b = integerBinStartState,
        },
    }
    for i = 1, 9 do numberInitState[tostring(i)] = numberPreSeparatorState end

    --  Speshul symbols, brackets...

    local speshulSymbol = {
        [','] = TT.COMMA,
        [';'] = TT.SEMICOLON,
        ['.'] = TT.MEMBER_ACCESS,
        ['$'] = TT.DOLLAR,
        ['#'] = TT.POUND,
        ["["] = TT.BRACKET_OPEN,
        ["("] = TT.BRACKET_OPEN,
        ["{"] = TT.BRACKET_OPEN,
        ["]"] = TT.BRACKET_CLOSE,
        [")"] = TT.BRACKET_CLOSE,
        ["}"] = TT.BRACKET_CLOSE,
    }

    local identifierStarters = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

    local function tokenizerInner(file, str)
        local line, i = VDLp.Classes.LineOfCode(file, 1, 0, nil), 1

        function nextLine(start)
            line = VDLp.Classes.LineOfCode(file, line.Line + 1, start, line)
        end

        coroutine.yield()

        while i <= #str do
            local c = str:sub(i, i)

            if identifierStarters:find(c, 1, true) then
                --  'Tis an identifier.

                local finish, identifier = str:find("[^_%w]", i + 1)

                if not finish then
                    identifier = c
                else
                    identifier = str:sub(i, finish - 1)
                end

                coroutine.yield(VDLp.Classes.Token(line, i, TT.IDENTIFIER, identifier))

                if finish then
                    i = finish - 1
                end
            elseif numberInitState[c] then
                local state, start = numberInitState, i

                repeat
                    state = state[c]

                    c = str:sub(i + 1, i + 1)

                    if state[c] then
                        i = i + 1
                    elseif state.finish then
                        break
                    else
                        return VDLp.Classes.CodeReference(line, start), state.error or "Incomplete number"
                    end
                until false

                coroutine.yield(VDLp.Classes.Token(line, start, state.float and TT.FLOAT or TT.INTEGER, str:sub(start, i)))
            elseif speshulSymbol[c] then
                coroutine.yield(VDLp.Classes.Token(line, i, speshulSymbol[c], c))
            elseif c == '/' and str:sub(i + 1, i + 1) == '/' then
                --  Single-line comment.

                i = str:find('\n', i + 2, true)

                if not i then
                    break   --  End of file.
                else
                    nextLine(i)
                end
            elseif c == '/' and str:sub(i + 1, i + 1) == '*' then
                --  Block comment.

                local lastWasAsterisk, start = false, i
                i = i + 1

                while i <= #str do
                    i = i + 1
                    c = str:sub(i, i)

                    if c == "/" and lastWasAsterisk then
                        break   --  End of block comment.
                    else
                        lastWasAsterisk = c == '*'

                        if c == '\n' then
                            nextLine(i)
                        end
                    end
                end

                if c ~= '/' or not lastWasAsterisk then
                    return VDLp.Classes.CodeReference(line, start), "Incomplete block comment."
                end
            elseif operatorStateTree[c] then
                local state, start = operatorStateTree, i

                repeat
                    state = state[c]

                    c = str:sub(i + 1, i + 1)

                    if state[c] then
                        i = i + 1
                    elseif state.finish then
                        break
                    else
                        return VDLp.Classes.CodeReference(line, start), "Incomplete operator."
                    end
                until false

                coroutine.yield(VDLp.Classes.Token(line, start, TT.OPERATOR, str:sub(start, i)))
            elseif c == '\n' then
                nextLine(i)
            elseif c ~= ' ' and c ~= '\t' and c ~= '\r' then
                return VDLp.Classes.CodeReference(line, i), "Unexpected character: " .. c
            end

            i = i + 1
        end

        return VDLp.Classes.Token(line, i, TT.EOF, nil)
    end

    function VDLp.GenerateTokenizer(file, str)
        local res = coroutine.wrap(tokenizerInner)
        res(file, str)
        return res
    end
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Parsing

function VDLp.Parse(file)
    local fIn, fErr = io.open(tostring(file), "r")

    if not fIn then
        error("Failed to open input file '" .. file .. "': " .. fErr)
    end

    local tokenizer, tk, err = VDLp.GenerateTokenizer(file, fIn:read "*a")

    local tks = List { }

    repeat
        tk, err = tokenizer()

        if err then
            io.stderr:write(string.format("Error - %s - %s\n", tk, err))

            break
        end

        print(tostring(tk))

        tks:Append(tk)
        tk.Index = tks.Length
    until not tk or (typeEx(tk) == "Token" and tk.Type == TT.EOF)

    fIn:close()

    if err then return false end

    if not VDLp.PairBrackets(tks) then return false end

    local decls, err = VDLp.SplitDeclarations(tks, "main")

    if VDLp.Debug then
        print(decls:Print " ")
    end

    if err then
        return false
    end

    return true
end

do  --  Bracket-matching
    local bracketPairs = { ["["] = "]", ["]"] = "["
                        ,  ["("] = ")", [")"] = "("
                        ,  ["{"] = "}", ["}"] = "{" }

    local openingBrackets = { ["["] = "]", ["("] = ")", ["{"] = "}" }
    local closingBrackets = { ["]"] = "[", [")"] = "(", ["}"] = "{" }

    function VDLp.PairBrackets(tks)
        local stack, skipped = {}

        tks:ForEach(function(tk)
            if tk.Type == TT.BRACKET_OPEN then
                stack[#stack + 1] = tk
            elseif tk.Type == TT.BRACKET_CLOSE then
                if #stack > 0 and stack[#stack].Value == closingBrackets[tk.Value] then
                    --  All gud.

                    stack[#stack].Pair = tk
                    tk.Pair = stack[#stack]

                    stack[#stack] = nil
                else
                    --  Mismatch.

                    io.stderr:write(string.format("Mismatched bracket %s;\n", tk))

                    if #stack > 0 then
                        io.stderr:write(string.format("Expected to close %s first.\n", stack[#stack]))
                    else
                        io.stderr:write("No matching opening bracket found.\n")
                    end

                    skipped = true

                    return nil, true
                end
            end
        end)

        return not skipped
    end
end

VDLp.Classes.Create("DeclarationRange", nil, {
    __init = function(self, lst, scope, typ)
        self._key_decr_list = lst
        self._key_decr_scop = scope
        self._key_decr_type = typ
    end,

    List   = { RETRIEVE = "_key_decr_list" },
    Scope  = { RETRIEVE = "_key_decr_scop" },
    Type   = { RETRIEVE = "_key_decr_type" },

    __tostring = function(self)
        return string.format("(%s -> %s (%s))", self._key_decr_scop, self._key_decr_type
            , self._key_decr_list:Print(" ", function(elem)
                local eType = typeEx(elem)

                if eType == "Token" then
                    return string.format("(%s '%s')", TT[elem._key_toke_type], elem._key_toke_valu)
                else
                    return tostring(elem)
                end
            end))
    end,
})

function VDLp.SplitDeclarations(tks, scope)
    --  Just a slight separation of concerns here.

    local decls, i, start, dType = List { }, 1

    while i < #tks do
        local tk = tks[i]

        if not start then
            if tk.Type == TT.BRACKET_OPEN and tk.Value == '[' then
                decls:Append(VDLp.Classes.DeclarationRange(tks:Subset(tk.Index, tk.Pair.Index), scope, "attribute"))

                i = tk.Pair.Index
            elseif tk.Type == TT.IDENTIFIER then
                dType = "composite/property/alias"
                start = tk
            else
                io.stderr:write(string.format("%s: Expected valid beginning of a declaration here.\n", tk))

                return decls, true
            end
        else
            if dType == "composite/property/alias" then
                if tk.Type == TT.SEMICOLON then
                    decls:Append(VDLp.Classes.DeclarationRange(tks:Subset(start.Index, tk.Index), scope, "property"))

                    start = nil
                elseif tk.Type == TT.BRACKET_OPEN and tk.Value == '{' then
                    decls:Append(VDLp.Classes.DeclarationRange(tks:Subset(start.Index, tk.Pair.Index), scope, "composite"))

                    start = nil
                    i = tk.Pair.Index
                elseif tk.Type == TT.OPERATOR and tk.Value == '=' then
                    dType = "alias"
                elseif tk.Type ~= TT.IDENTIFIER and tk.Type ~= TT.OPERATOR and tk.Type ~= TT.INTEGER then
                    io.stderr:write(string.format("%s: Invalid token inside what appears to be a %s.\n", tk, dType))

                    return decls, true
                end
            elseif dType == "alias" then
                if tk.Type == TT.SEMICOLON then
                    decls:Append(VDLp.Classes.DeclarationRange(tks:Subset(start.Index, tk.Index), scope, dType))

                    start = nil
                elseif tk.Type ~= TT.IDENTIFIER and tk.Type ~= TT.OPERATOR and tk.Type ~= TT.INTEGER then
                    io.stderr:write(string.format("%s: Invalid token inside what appears to be a %s.\n", tk, dType))

                    return decls, true
                end
            else
                io.stderr:write(string.format("%s: Expected declaration type \"%s\" is invalid!\n", tk, dType))

                return decls, true
            end
        end

        i = i + 1
    end

    if start then
        io.stderr:write(string.format("%s: Declaration that is meant to begin here doesn't seem to have an end.\n", start.Location))

        return decls, true
    end

    if tks[#tks].Type ~= TT.EOF then
        io.stderr:write(string.format("Token list should end with an EOF token, not %s!\n", TT[tks[#tks].Type] or tks[#tks].Type))

        return decls, true
    end

    return decls
end

----------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------
--  Operation

CmdOpt "output" "o" {
    Description = "Puts compilation results in the given file. If it already exists,"
             .. "\nit is overwritten. Use '-' for standard output, which is also"
             .. "\nthe default.",
    ShortPrefix = true,

    Type = "file",

    Handler = function(val)
        
    end,
}

CmdOpt "include" "I" {
    Description = "Adds a directory to the list searched when importing declarations.",
    ShortPrefix = true,

    Type = "directory",
    Many = true,

    Handler = function(val)
        
    end,
}

VDLp.CheckArguments()

inputs:ForEach(function(file)
    local ok = VDLp.Parse(file)

    if not ok then
        io.stderr:write("Stopping.\n")
        os.exit(1)
    end
end)








