#!/usr/bin/env lua

local FUNC = false
local MODE = false
local PROJ = false

local FUNC_READELF = "rdelf"
local FUNC_DISASSEMBLE = "dsasm"
local FUNC_GRAB_XC = "grab-cross-compiler"
local FUNC_GRAB_GENISOIMAGE = "grab-genisoimage"
local FUNC_SETUP_LR = "setup-luarocks"

local MODE_TARGET_PROJECT = 1

if not arg[1] then
    error("Expected at least one argument.")
end

local funcs = {
    readelf = FUNC_READELF,
    rdelf = FUNC_READELF,

    disassemble = FUNC_DISASSEMBLE,
    disasm = FUNC_DISASSEMBLE,
    dsasm = FUNC_DISASSEMBLE,

    ["grab-cross-compiler"] = FUNC_GRAB_XC,
    ["grab-xc"] = FUNC_GRAB_XC,

    ["grab-genisoimage"] = FUNC_GRAB_GENISOIMAGE,
    ["grab-mkisofs"] = FUNC_GRAB_GENISOIMAGE,

    ["setup-luarocks"] = FUNC_SETUP_LR,
}

FUNC = funcs[string.lower(arg[1])]

if not FUNC then
    local err = "Argument #1 should be a valid function:"

    for a, b in pairs(funcs) do
        if a == b then
            err = err .. "\n" .. a
        else
            err = err .. "\n" .. a .. " (" .. b .. ")"
        end
    end

    error(err)
end

local modes = {
    [FUNC_READELF] = MODE_TARGET_PROJECT,
    [FUNC_DISASSEMBLE] = MODE_TARGET_PROJECT,
}

MODE = modes[FUNC]

local projects = {
    kernel = ".vmake/amd64.debug/beelzebub/beelzebub.bin",
    loadtest = ".vmake/amd64.debug/apps/loadtest/loadtest.exe",
    testmod = ".vmake/amd64.debug/kmods/test/test.kmod",
    libruntime = ".vmake/amd64.debug/libs/runtime/libbeelzebub.amd64.so",
    libkmod = ".vmake/amd64.debug/libs/kmod/libbeelzebub.kmod.so",
}

if MODE == MODE_TARGET_PROJECT then
    --  Needs a valid project.

    PROJ = projects[string.lower(arg[2])]

    if not PROJ then
        local err = "Argument #2 should be a valid project:"

        for a, b in pairs(projects) do
            if a == b then
                err = err .. "\n" .. a
            else
                err = err .. "\n" .. a .. " (" .. b .. ")"
            end
        end

        error(err)
    end
end

--------------------------------------------------------------------------------

local function escapeForShell(s)
    if #s < 1 then
        return "\"\""
    end

    s = s:gsub('"', "\\\"")

    if s:find("[^a-zA-Z0-9%+%-%*/=%.,_\"|]") then
        return '"' .. s .. '"'
    end

    return s
end

local function shell(cmd, ...)
    local function appendArg(arg, tab, errlvl)
        errlvl = errlvl + 1

        if arg == nil then
            error("util.lua error: Shell command argument cannot be nil.", errlvl)
        end

        local argType = type(arg)

        if argType == "table" then
            for i = 1, #arg do
                appendArg(arg[i], tab, errlvl + 1)
            end
        else
            tab[#tab + 1] = escapeForShell(tostring(arg), tab)
        end
    end

    local args, tab = {...}, {cmd}

    for i = 1, #args do
        appendArg(args[i], tab, 2)
    end

    cmd = table.concat(tab, " ")

    local printCmd = false --   TODO

    if printCmd then
        print(cmd)
    end

    local okay, exitReason, code = os.execute(cmd)

    if not okay then
        error("util.lua failed to execute shell command"
            .. (printCmd and "; " or (":\n" .. cmd .. "\n"))
            .. "exit reason: " .. tostring(exitReason) .. "; status code: " .. tostring(code), 2)
    end

    return okay or false
end

--------------------------------------------------------------------------------

if FUNC == FUNC_READELF then

    shell("readelf", "-ateW", PROJ, "|", "less")

--------------------------------------------------------------------------------

elseif FUNC == FUNC_DISASSEMBLE then

    shell("objdump", "-M", "intel", "-CdlSw", PROJ, "|", "less")

--------------------------------------------------------------------------------

elseif FUNC == FUNC_GRAB_XC then

    shell("bash", "scripts/grab_xcs_linux-amd64.sh")

--------------------------------------------------------------------------------

elseif FUNC == FUNC_GRAB_GENISOIMAGE then

    shell("bash", "scripts/grab_genisoimage.sh")

--------------------------------------------------------------------------------

elseif FUNC == FUNC_SETUP_LR then

    shell("luarocks", "--local", "install", "luafilesystem")
    shell("luarocks", "--local", "install", "vmake")

--------------------------------------------------------------------------------

end
