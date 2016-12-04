#!/usr/bin/env lua
require "vmake"

--[[
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/vMake

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
]]

--  Configurations

Configuration "debug" {
    Data = {
        Opts_GCC = List { "-fno-omit-frame-pointer", "-g" }
    },
}

Configuration "release" {
    Data = {
        Opts_GCC = List { }
    },
}

Configuration "profile" {
    Data = {
        Opts_GCC = List { }
    },

    Base = "release",
}

--  Architectures

Architecture "amd64" {
    Data = {
        Opts_GCC = List { "-m64" },

        Opts_NASM = List { "-f", "elf64" },
    },

    Base = "x86",
}

Architecture "ia32" {
    Data = {
        Opts_GCC = List { "-m32" },

        Opts_NASM = List { "-f", "elf32" },
    },

    Base = "x86",
}

Architecture "x86" {
    Data = {
        ISO = true,

        CrtFiles = List { "crt0.o", "crti.o", "crtn.o" },
    },

    Auxiliary = true,
}

--  Toolchain

local CROSSCOMPILER_DIRECTORY = Path "/usr/local/gcc-x86_64-beelzebub/bin"
--  Default

local CROSSCOMPILERS_DIR = os.getenv("CROSSCOMPILERS_DIR")
local MISC_TOOLS_DIR = os.getenv("MISC_TOOLS_DIR")

if CROSSCOMPILERS_DIR then
    CROSSCOMPILER_DIRECTORY = Path(CROSSCOMPILERS_DIR) + "gcc-x86_64-beelzebub/bin"
end

local CC    = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-gcc"
local CXX   = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-gcc"
local GAS   = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-gcc"
local DC    = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-gdc"
local AS    = "nasm"
local LO    = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-gcc"
local LD    = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-ld"
local AR    = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-gcc-ar"
local STRIP = CROSSCOMPILER_DIRECTORY + "x86_64-beelzebub-strip"
local MKISO = "mkisofs"
local TAR   = "tar"
local GZIP  = "gzip"

if not os.execute(MKISO .. " --version > /dev/null 2> /dev/null") then
    --  So, mkisofs may not be completely absent.

    if os.execute("genisoimage --version > /dev/null 2> /dev/null") then
        --  Maybe genisoimage no longer symlinks it.

        MKISO = "genisoimage"
    elseif MISC_TOOLS_DIR then
        --  Or maybe it comes from an external source?

        MKISO = MISC_TOOLS_DIR .. "/genisoimage"
    end
end

--  Tests

local availableTests = List {
    "MT",
    "STR",
    --"OBJA",
    "METAP",
    "EXCP",
    "APP",
    "KMOD",
    "TIMER",
    "MAILBOX",
    "STACKINT",
    "AVL_TREE",
    "TERMINAL",
    "CMDO",
    "FPU",
    "BIGINT",
    "LOCK_ELISION",
    "RW_SPINLOCK",
    "VAS",
    "INTERRUPT_LATENCY",
}

local availableApicModes = List {
    "legacy", "x2apic", "flexible"
}

for i = 1, #availableTests do availableTests[availableTests[i]] = true end
for i = 1, #availableApicModes do availableApicModes[availableApicModes[i]] = true end
--  Makes lookup easier.

local testOptions, specialOptions = List { }, List { }
local settSmp, settInlineSpinlocks, settUnitTests = true, true, true
local settApicMode = "FLEXIBLE"
local settMakeDeps = true

CmdOpt "tests" "t" {
    Description = "Specifies which tests to include in the Beelzebub build.",
    Display = "name-1,name 2;name_3,...|all",

    Type = "string",
    Many = true,

    Handler = function(_, val)
        if val == "all" then
            availableTests:ForEach(function(testName)
                testOptions:AppendUnique("__BEELZEBUB__TEST_" .. testName)
            end)
        else
            for test in string.iteratesplit(val, "[,;]") do
                if #test == 0 or string.find(test, "[^%a%d%s%-_]") then
                    error("Beelzebub test \"" .. test .. "\" contains invalid characters.")
                end

                local testName = string.upper(string.gsub(test, "[%s%-]", "_"))

                if not availableTests[testName] then
                    error("Unknown Beelzebub test \"" .. testName .. "\".")
                end

                testOptions:AppendUnique("__BEELZEBUB__TEST_" .. testName)
            end
        end
    end,
}

CmdOpt "smp" {
    Description = "Specifies whether the Beelzebub build includes symmetric multiprocessing support; defaults to yes.",

    Type = "boolean",

    Handler = function(_, val) settSmp = val end,
}

CmdOpt "inline-spinlocks" {
    Description = "Specifies whether spinlock operations are inlined in the kernel code; defaults to yes.",

    Type = "boolean",

    Handler = function(_, val) settInlineSpinlocks = val end,
}

CmdOpt "unit-tests" {
    Description = "Specifies whether kernel unit tests are included in the kernel or not, or if they will be quieted; defaults to yes (included but not quieted).",

    Type = "string",
    Display = "boolean|quiet",

    Handler = function(_, val)
        local bVal = toboolean(val)

        if bVal == nil then
            if string.lower(val) ~= "quiet" then
                error("Invalid value given to \"unit-tests\" command-line option, expected a boolean value or \"quiet\", not \""
                    .. val .. "\".")
            end

            settUnitTests = "quiet"
        else
            settUnitTests = bVal
        end
    end,
}

CmdOpt "apic-mode" {
    Description = "Specifies the APIC mode(s) supported by the kernel. Defaults to flexible.",

    Type = "string",
    Display = availableApicModes:Print("|"),

    Handler = function(_, val)
        if not availableApicModes[string.lower(val)] then
            error("Invalid value given to \"apic-mode\" command-line option: \""
                .. val .. "\".")
        end

        settApicMode = string.upper(val)
    end,
}

CmdOpt "march" {
    Description = "Specifies an `-march=` option to pass on to GCC on compilation.",

    Type = "string",

    Handler = function(_, val)
        if val == "" then
            error("Value given to \"march\" command-line option must be non-empty.")
        end

        specialOptions:Append("-march=" .. val)
    end,
}

CmdOpt "mtune" {
    Description = "Specifies an `-mtune=` option to pass on to GCC on compilation.",

    Type = "string",

    Handler = function(_, val)
        if val == "" then
            error("Value given to \"mtune\" command-line option must be non-empty.")
        end

        specialOptions:Append("-mtune=" .. val)
    end,
}

CmdOpt "no-make-deps" {
    Description = "Specifies that GNU Make dependency files are not to be generated by GCC.",

    Handler = function(_, val) settMakeDeps = false end,
}

--  Utilities

local function parseObjectExtension(path)
    local part1, part2 = tostring(path):match("^(.+)%.([^%.]+)%.o$")

    if not part1 then
        return nil, nil
    end

    local arch = GetArchitecture(part2)

    if arch then
        return part1, arch
    else
        return part1 .. "." .. part2, nil
    end
end

local function checkObjectExtension(path, ext)
    local part1, part2 = tostring(path):match("^(.+)%.([^%.]+)%.o$")

    if not part1 then
        return false
    end

    if GetArchitecture(part2) then
        return part1:sub(-#ext - 1) == "." .. ext
    else
        return part2 == ext
    end
end

--  Templates

local function sourceArchitectural(_, dst)
    local codeFile, arch, src = parseObjectExtension(dst)

    if arch then
        src = _.ArchitecturesDirectory + arch.Name + "src" + Path(codeFile):Skip(_.ObjectsDirectory)
    else
        src = _.CommonDirectory + "src" + Path(codeFile):Skip(_.ObjectsDirectory)
    end

    if settMakeDeps then
        return ParseGccDependencies(dst, src, true) + List { dst:GetParent() }
    else
        return List { src, dst:GetParent() }
    end
end

local function gzipSingleFile(_, dst, src)
    local tmp = dst:TrimEnd(3)  --  3 = #".gz"
    fs.Copy(tmp, src[1])
    sh.silent(GZIP, "-f", "-9", tmp)
end

local function ArchitecturalComponent(name)
    local comp = Component(name)({
        Rule "Compile C" {
            Filter = function(_, dst) return checkObjectExtension(dst, "c") end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                if settMakeDeps then
                    sh.silent(CC, _.Opts_C, "-MD", "-MP", "-c", src[1], "-o", dst)
                else
                    sh.silent(CC, _.Opts_C, "-c", src[1], "-o", dst)
                end
            end,
        },

        Rule "Compile C++" {
            Filter = function(_, dst) return checkObjectExtension(dst, "cpp") end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                if settMakeDeps then
                    sh.silent(CXX, _.Opts_CXX, "-MD", "-MP", "-c", src[1], "-o", dst)
                else
                    sh.silent(CXX, _.Opts_CXX, "-c", src[1], "-o", dst)
                end
            end,
        },

        Rule "Assemble w/ NASM" {
            Filter = function(_, dst) return checkObjectExtension(dst, "asm") end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                sh.silent(AS, _.Opts_NASM, src[1], "-o", dst)
            end,
        },

        Rule "Assemble w/ GAS" {
            Filter = function(_, dst) return checkObjectExtension(dst, "s") end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                sh.silent(GAS, _.Opts_GAS, "-c", src[1], "-o", dst)
            end,
        },

        ExcuseMissingFilesRule { "h", "hpp", "inc", "hh" },
    })

    local data = {
        CommonDirectory = function(_) return _.comp.Directory end,

        ArchitecturesDirectory = function(_) return _.comp.Directory end,

        IncludeDirectories = function(_)
            local res = List {
                _.comp.Directory + "inc",
            }

            for arch in _.selArch:Hierarchy() do
                res:Append(_.ArchitecturesDirectory + arch.Name + "inc")
            end

            return res
        end,

        Opts_Includes_Base = function(_)
            return _.IncludeDirectories:Select(function(val) return "-I" .. tostring(val) end)
        end,

        Opts_Includes_Nasm_Base = function(_)
            return _.Opts_Includes_Base
                :Select(function(dir) return dir .. "/" end)
                :Append("-I" .. tostring(_.SysheadersPath) .. "/")
        end,

        Opts_Includes      = function(_) return _.Opts_Includes_Base      end,
        Opts_Includes_Nasm = function(_) return _.Opts_Includes_Nasm_Base end,

        Opts_Libraries = function(_)
            return _.Libraries:Select(function(val)
                return "-l" .. val
            end)
        end,

        Libraries = List { },   --  Default to none.

        ObjectsDirectory = function(_) return _.outDir + _.comp.Directory end,

        Objects = function(_)
            local comSrcDir = _.CommonDirectory + "src"

            local objects = fs.ListDir(comSrcDir)
                :Where(function(val) return val:EndsWith(".c", ".cpp", ".asm", ".s") end)
                :Select(function(val)
                    return _.ObjectsDirectory + val:Skip(comSrcDir) .. ".o"
                end)

            for arch in _.selArch:Hierarchy() do
                local arcSrcDir = _.ArchitecturesDirectory + arch.Name + "src"
                local suffix = "." .. arch.Name .. ".o"

                if fs.GetInfo(arcSrcDir) then
                    objects:AppendMany(fs.ListDir(arcSrcDir)
                        :Where(function(val) return val:EndsWith(".c", ".cpp", ".asm", ".s") end)
                        :Select(function(val)
                            return _.ObjectsDirectory + val:Skip(arcSrcDir) .. suffix
                        end))
                end
            end

            return objects
        end,
    }

    return function(tab)
        for k, v in pairs(tab) do
            if k == "Data" then
                for dk, dv in pairs(v) do
                    data[dk] = dv
                end

                v = data
            end

            if type(k) == "string" then
                comp[k] = v
            end
        end

        for i = 1, #tab do
            comp:AddMember(tab[i])
        end

        return comp
    end
end

--  Projects

Project "Beelzebub" {
    Data = {
        Sysroot                 = function(_) return _.outDir + "sysroot" end,
        SysheadersPath          = function(_) return _.Sysroot + "usr/include" end,
        JegudielPath            = function(_) return _.outDir + "jegudiel.bin" end,
        CommonLibraryPath       = function(_) return _.Sysroot + ("usr/lib/libcommon." .. _.selArch.Name .. ".a") end,
        RuntimeLibraryPath      = function(_) return _.Sysroot + ("usr/lib/libbeelzebub." .. _.selArch.Name .. ".so") end,
        KernelModuleLibraryPath = function(_) return _.Sysroot + "usr/lib/libbeelzebub.kmod.so" end,
        TestKernelModulePath    = function(_) return _.Sysroot + "kmods/test.kmod" end,
        KernelPath              = function(_) return _.outDir + "beelzebub.bin" end,
        LoadtestAppPath         = function(_) return _.Sysroot + "apps/loadtest.exe" end,

        CrtFiles = function(_)
            return _.selArch.Data.CrtFiles:Select(function(val)
                return _.Sysroot + "usr/lib" + val
            end)
        end,

        IsoFile = function(_)
            if _.selArch.Base.Data.ISO then
                return _.outDir + ("beelzebub." .. _.selArch.Name .. "." .. _.selConf.Name .. ".iso")
            end
        end,

        Opts_GCC_Precompiler = function(_)
            local res = List {
                "-D__BEELZEBUB",
                "-D__BEELZEBUB__ARCH=" .. _.selArch.Name,
                "-D__BEELZEBUB__CONF=" .. _.selConf.Name,
            } + testOptions:Select(function(val) return "-D" .. val end)

            for arch in _.selArch:Hierarchy() do
                res:Append("-D__BEELZEBUB__ARCH_" .. string.upper(arch.Name))
            end

            for conf in _.selConf:Hierarchy() do
                res:Append("-D__BEELZEBUB__" .. string.upper(conf.Name))
            end

            res:Append(settSmp
                and "-D__BEELZEBUB_SETTINGS_SMP"
                or "-D__BEELZEBUB_SETTINGS_NO_SMP")

            res:Append(settInlineSpinlocks
                and "-D__BEELZEBUB_SETTINGS_INLINE_SPINLOCKS"
                or "-D__BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS")

            res:Append(settUnitTests
                and "-D__BEELZEBUB_SETTINGS_UNIT_TESTS"
                or "-D__BEELZEBUB_SETTINGS_NO_UNIT_TESTS")

            if settUnitTests == "quiet" then
                res:Append("-D__BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET")
            end

            res:Append("-D__BEELZEBUB_SETTINGS_APIC_MODE_" .. settApicMode)

            return res
        end,
    },

    Description = "Lord of Flies",

    Output = function(_)
        if _.selArch.Base.Data.ISO then
            return _.IsoFile
        else
            error("TODO")
        end
    end,

    Component "System Headers" {
        Data = {
            Headers = function(_)
                local files = fs.ListDir(_.comp.Directory + "common")

                for arch in _.selArch:Hierarchy() do
                    files:AppendMany(fs.ListDir(_.comp.Directory + arch.Name))
                end

                files = files:Where(function(val)
                    return val.IsFile and val:EndsWith(".h", ".hpp", ".inc")
                end)

                return files
            end,
        },

        Directory = "sysheaders",

        Output = function(_)
            local files = _.Headers:Select(function(val)
                return _.SysheadersPath + val:SkipDirs(2)
            end)

            return files
        end,

        Rule "Create Directory" {
            Filter = function(_, dst) return dst.IsDirectory end,

            Action = function(_, dst)
                fs.MkDir(dst)
            end,
        },

        Rule "Copy Header" {
            Filter = function(_, dst)
                return dst:StartsWith(_.SysheadersPath) and not dst.IsDirectory
            end,

            Source = function(_, dst)
                local parent = dst:GetParent()
                dst = dst:Skip(_.SysheadersPath)

                for arch in _.selArch:Hierarchy() do
                    local src = _.comp.Directory + arch.Name + dst

                    if fs.GetInfo(src) then return src end
                end

                return List { _.comp.Directory + "common" + dst, parent }
            end,

            Action = function(_, dst, src)
                fs.Copy(dst, src[1])
            end,
        },
    },

    Component "Jegudiel" {
        Data = {
            SourceDirectory = function(_) return _.comp.Directory + "src" end,

            ObjectsDirectory = function(_) return _.outDir + "jegudiel" end,
            BinaryPath = function(_) return _.ObjectsDirectory + "jegudiel.bin" end,

            Objects = function(_)
                return fs.ListDir(_.SourceDirectory)
                    :Where(function(val) return val:EndsWith(".c", ".s") end)
                    :Select(function(val)
                        return _.ObjectsDirectory + val:Skip(_.SourceDirectory) .. ".o"
                    end)
            end,

            LinkerScript = function(_) return _.comp.Directory + "link.ld" end,

            Opts_Includes = function(_)
                return List {
                    "-I" .. tostring(_.comp.Directory + "inc"),
                    "-Isysheaders/" .. _.selArch.Name, --  Harcoded but meh...
                }
            end,

            Opts_C = function(_)
                return List {
                    "-m64", "-ffreestanding",
                    "-Wall", "-Wsystem-headers",
                    "-mcmodel=kernel", "-static-libgcc",
                    "-O0",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + specialOptions
            end,

            Opts_NASM = List { "-f", "elf64" },

            Opts_LD = List { "-z", "max-page-size=0x1000" },

            Opts_STRIP = List { "-s", "-K", "multiboot_header" },
        },

        Directory = "jegudiel",

        Dependencies = "System Headers",

        Output = function(_) return _.outDir + "jegudiel.bin" end,

        Rule "Link Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst)
                return _.Objects + List { _.LinkerScript, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(LD, _.Opts_LD, "-T", _.LinkerScript, "-o", dst, _.Objects)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.comp.Output end,

            Source = function(_, dst)
                return List {_.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },

        Rule "Compile C" {
            Filter = function(_, dst) return dst:EndsWith(".c.o") end,

            Source = function(_, dst)
                return List { _.SourceDirectory + dst:Skip(_.ObjectsDirectory):TrimEnd(2), dst:GetParent() }
                --  2 = #".o"
            end,

            Action = function(_, dst, src)
                sh.silent(CC, _.Opts_C, "-c", src[1], "-o", dst)
            end,
        },

        Rule "Assemble" {
            Filter = function(_, dst) return dst:EndsWith(".s.o") end,

            Source = function(_, dst)
                return List { _.SourceDirectory + dst:Skip(_.ObjectsDirectory):TrimEnd(2), dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(AS, _.Opts_NASM, src[1], "-o", dst)
            end,
        },

        ExcuseMissingFilesRule { "h", "hpp", "inc", "hh" },
    },

    ArchitecturalComponent "Common Library" {
        Data = {
            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden",
                    "-ffreestanding", "-nostdlib", "-static-libgcc",
                    "-Wall", "-Wsystem-headers",
                    "-pipe",
                    "-mno-aes", "-mno-mmx", "-mno-pclmul", "-mno-sse", "-mno-sse2",
                    "-mno-sse3", "-mno-sse4", "-mno-sse4a", "-mno-fma4", "-mno-ssse3",
                    "--sysroot=" .. tostring(_.Sysroot),
                    "-D__BEELZEBUB_STATIC_LIBRARY",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
                + specialOptions

                if _.selArch.Name == "amd64" then
                    res:Append("-mcmodel=large"):Append("-mno-red-zone")
                end

                return res
            end,

            Opts_C       = function(_) return _.Opts_GCC + List { "-std=gnu99", "-O2", "-flto", } end,
            Opts_CXX_CRT = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_CXX     = function(_) return _.Opts_CXX_CRT + List { "-O2", "-flto", } end,
            Opts_NASM    = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS_CRT = function(_) return _.Opts_GCC end,
            Opts_GAS     = function(_) return _.Opts_GAS_CRT + List { "-flto" } end,

            Opts_AR = List { "rcs" },
        },

        Directory = "libs/common",

        Dependencies = "System Headers",

        Output = function(_) return List { _.CommonLibraryPath } + _.CrtFiles end,

        Rule "Archive Objects" {
            Filter = function(_, dst) return _.CommonLibraryPath end,

            Source = function(_, dst) return _.Objects + List { dst:GetParent() } end,

            Action = function(_, dst, src)
                sh.silent(AR, _.Opts_AR, dst, _.Objects)
            end,
        },

        Rule "C Runtime Objects" {
            Filter = function(_, dst) return _.CrtFiles:Contains(dst) end,

            Source = function(_, dst)
                local crtName = dst:GetName():TrimEnd(2)
                --  2 = #".o"

                for arch in _.selArch:Hierarchy() do
                    local archCrtBase = _.ArchitecturesDirectory + arch.Name + "crt" + crtName
                    local cpp, gas = archCrtBase .. ".cpp", archCrtBase .. ".s"

                    if fs.GetInfo(cpp) then
                        return List { cpp, dst:GetParent() }
                    elseif fs.GetInfo(gas) then
                        return List { gas, dst:GetParent() }
                    end
                end

                error("Unable to find source of CRT file \"" .. tostring(crtName) .. "\".")
            end,

            Action = function(_, dst, src)
                if src[1]:EndsWith(".cpp") then
                    sh.silent(CXX, _.Opts_CXX_CRT, "-MD", "-MP", "-c", src[1], "-o", dst)
                else
                    sh.silent(GAS, _.Opts_GAS_CRT, "-c", src[1], "-o", dst)
                end
            end,
        },
    },

    ArchitecturalComponent "Runtime Library" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.RuntimeLibraryPath:GetName() end,

            Opts_GCC = function(_)
                return List {
                    "-fvisibility=hidden",
                    "-ffreestanding", "-nodefaultlibs", "-static-libgcc",
                    "-fPIC",
                    "-Wall", "-Wsystem-headers",
                    "-O3", "-flto",
                    "-pipe",
                    "--sysroot=" .. tostring(_.Sysroot),
                    "-D__BEELZEBUB_DYNAMIC_LIBRARY",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
                + specialOptions
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return List { "-shared", "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", "-Wl,-Bsymbolic", } + _.Opts_GCC end,

            Opts_STRIP = List { "-s" },

            Libraries = function(_) return List { "common." .. _.selArch.Name, } end,
        },

        Directory = "libs/runtime",

        Dependencies = "System Headers",

        Output = function(_) return _.RuntimeLibraryPath end,

        Rule "Link-Optimize Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst)
                return _.Objects + _.CrtFiles
                    + List { _.CommonLibraryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.RuntimeLibraryPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Kernel Module Library" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.KernelModuleLibraryPath:GetName() end,

            Opts_GCC = function(_)
                return List {
                    "-fvisibility=hidden",
                    "-ffreestanding", "-nodefaultlibs", "-static-libgcc",
                    "-fPIC",
                    "-Wall", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-pipe",
                    "--sysroot=" .. tostring(_.Sysroot),
                    "-D__BEELZEBUB_DYNAMIC_LIBRARY",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
                + specialOptions
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return List { "-shared", "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", "-Wl,-Bsymbolic", } + _.Opts_GCC end,

            Opts_STRIP = List { "-s" },
        },

        Directory = "libs/kmod",

        Dependencies = "System Headers",

        Output = function(_) return _.KernelModuleLibraryPath end,

        Rule "Link-Optimize Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst) return _.Objects + List { dst:GetParent() } end,

            Action = function(_, dst, src)
                sh.silent(LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.KernelModuleLibraryPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Loadtest Application" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.LoadtestAppPath:GetName() end,

            Opts_GCC = function(_)
                return List {
                    "-fvisibility=hidden",
                    "-Wall", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-pipe",
                    "--sysroot=" .. tostring(_.Sysroot),
                    "-D__BEELZEBUB_APPLICATION",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
                + specialOptions
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return List { "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", } + _.Opts_GCC end,

            Opts_STRIP = List { "-s" },
        },

        Directory = "apps/loadtest",

        Dependencies = "System Headers",

        Output = function(_) return _.LoadtestAppPath end,

        Rule "Link-optimize Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst)
                return _.Objects
                    + List { _.RuntimeLibraryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.LoadtestAppPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Kernel" {
        Data = {
            ArchitecturesDirectory = function(_) return _.comp.Directory + "arc" end,
            BinaryPath             = function(_) return _.ObjectsDirectory + _.KernelPath:GetName() end,
            LinkerScript           = function(_) return _.ArchitecturesDirectory + _.selArch.Name + "link.ld" end,

            Opts_Includes = function(_) return List { "-Iacpica/include", } + _.Opts_Includes_Base end,

            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden",
                    "-ffreestanding", "-nostdlib", "-static-libgcc",
                    "-Wall", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-pipe",
                    "-mno-aes", "-mno-mmx", "-mno-pclmul", "-mno-sse", "-mno-sse2",
                    "-mno-sse3", "-mno-sse4", "-mno-sse4a", "-mno-fma4", "-mno-ssse3",
                    "--sysroot=" .. tostring(_.Sysroot),
                    "-D__BEELZEBUB_KERNEL",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
                + specialOptions

                if _.selArch.Name == "amd64" then
                    res:Append("-mcmodel=kernel"):Append("-mno-red-zone")
                end

                return res
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return _.Opts_GCC + List { "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", } end,

            Opts_STRIP = List { "-s", "-K", "jegudiel_header" },

            Libraries = function(_) return List { "common." .. _.selArch.Name, } end,
        },

        Directory = "beelzebub",

        Dependencies = "System Headers",

        Output = function(_) return _.KernelPath end,

        Rule "Link-Optimize Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst)
                return _.Objects
                    + List { _.LinkerScript, _.CommonLibraryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(LO, _.Opts_LO, "-T", _.LinkerScript, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.KernelPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Test Kernel Module" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.TestKernelModulePath:GetName() end,

            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden",
                    "-ffreestanding", "-nodefaultlibs", "-static-libgcc",
                    "-fPIC",
                    "-Wall", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-pipe",
                    "--sysroot=" .. tostring(_.Sysroot),
                    "-D__BEELZEBUB_KERNEL_MODULE",
                } + _.Opts_GCC_Precompiler + _.Opts_Includes
                + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
                + specialOptions

                if _.selArch.Name == "amd64" then
                    res:Append("-mno-red-zone")
                end

                return res
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return List { "-shared", "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", "-Wl,-Bsymbolic", } + _.Opts_GCC end,

            Opts_STRIP = List { "-s" },

            Libraries = function(_) return List { "beelzebub.kmod", } end,
        },

        Directory = "kmods/test",

        Dependencies = "System Headers",

        Output = function(_) return _.TestKernelModulePath end,

        Rule "Link-Optimize Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst)
                return _.Objects + _.CrtFiles
                    + List { _.KernelModuleLibraryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.TestKernelModulePath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    Component "ISO Image" {
        Data = {
            SysrootFiles = function(_)
                local res

                if fs.GetInfo(_.Sysroot) then
                    res = fs.ListDir(_.Sysroot):Append(_.Sysroot)
                else
                    res = List { }
                end

                res:AppendUnique(_.LoadtestAppPath)
                res:AppendUnique(_.TestKernelModulePath)

                return res
            end,

            Opts_TAR = function(_)
                return List {
                    "--owner=root", "--group=root",
                    "-C", _.Sysroot,
                    "--exclude=*.d",
                    "--exclude=libcommon.*.a",
                }
            end,

            IsoDirectory = function(_) return _.outDir + "iso" end,
            IsoBootPath = function(_) return _.IsoDirectory + "boot" end,
            IsoGrubDirectory = function(_) return _.IsoBootPath + "grub" end,
            IsoJegudielPath = function(_) return _.IsoBootPath + "jegudiel.bin.gz" end,
            IsoInitRdPath = function(_) return _.IsoBootPath + "initrd.tar.gz" end,
            IsoKernelPath = function(_) return _.IsoBootPath + ("beelzebub." .. _.selArch.Name .. ".bin.gz") end,
            IsoEltoritoPath = function(_) return _.IsoGrubDirectory + "stage2_eltorito" end,
            IsoGrubfontPath = function(_) return _.IsoGrubDirectory + "font.pf2" end,
            IsoGrubconfPath = function(_) return _.IsoGrubDirectory + "grub.cfg" end,

            IsoSources = function(_, dst)
                local res = List {
                    _.IsoKernelPath,
                    _.IsoInitRdPath,
                    _.IsoEltoritoPath,
                    _.IsoGrubfontPath,
                    _.IsoGrubconfPath,

                    _.TestKernelModulePath,
                    _.LoadtestAppPath,
                }

                if _.selArch.Name == "amd64" then
                    res:Append(_.IsoJegudielPath)
                end

                return res
            end,

            SourceGrub2 = function(_) return _.comp.Directory + "grub2" end,
            SourceEltorito = function(_) return _.SourceGrub2 + "stage2_eltorito" end,
            SourceGrubfont = function(_) return _.SourceGrub2 + "font.pf2" end,
            SourceGrubconf = function(_) return _.SourceGrub2 + ("grub." .. _.selArch.Name .. ".cfg") end,
        },

        Directory = "image",

        Dependencies = List {
            "System Headers"
        },

        Output = function(_) return _.IsoFile end,

        Rule "Make ISO" {
            Filter = function(_, dst) return _.IsoFile end,
            Source = function(_, dst) return _.IsoSources end,

            Action = function(_, dst, src)
                sh.silent(MKISO, "-R", "-b", _.IsoEltoritoPath:Skip(_.IsoDirectory), "-no-emul-boot", "-boot-load-size", 4, "-boot-info-table", "-o", _.IsoFile, _.IsoDirectory)
            end,
        },

        Rule "Archive Sysroot" {
            Filter = function(_, dst) return _.IsoInitRdPath end,
            Source = function(_, dst) return _.SysrootFiles end,

            Action = function(_, dst, src)
                sh.silent(TAR, "czf", dst, _.Opts_TAR, ".")
            end,
        },

        Rule "GZip Jegudiel" {
            Filter = function(_, dst) return _.IsoJegudielPath end,
            
            Source = function(_, dst)
                return List { _.JegudielPath, dst:GetParent() }
            end,

            Action = gzipSingleFile,
        },

        Rule "GZip Beelzebub" {
            Filter = function(_, dst) return _.IsoKernelPath end,
            
            Source = function(_, dst)
                return List { _.KernelPath, dst:GetParent() }
            end,

            Action = gzipSingleFile,
        },

        Rule "Copy GRUB El Torito" {
            Filter = function(_, dst) return _.IsoEltoritoPath end,
            
            Source = function(_, dst)
                return List { _.SourceEltorito, dst:GetParent() }
            end,

            Action = CopySingleFileAction,
        },

        Rule "Copy GRUB Font" {
            Filter = function(_, dst) return _.IsoGrubfontPath end,
            
            Source = function(_, dst)
                return List { _.SourceGrubfont, dst:GetParent() }
            end,

            Action = CopySingleFileAction,
        },

        Rule "Copy GRUB Configuration" {
            Filter = function(_, dst) return _.IsoGrubconfPath end,
            
            Source = function(_, dst)
                return List { _.SourceGrubconf, dst:GetParent() }
            end,

            Action = CopySingleFileAction,
        },
    },

    Rule "Create Directory" {
        Shared = true,

        Filter = function(_, dst) return dst == _.ObjectsDirectory or dst.IsDirectory end,

        Action = function(_, dst, src)
            fs.MkDir(dst)
        end,
    },
}

Default "Beelzebub" "amd64" "debug"

vmake()
