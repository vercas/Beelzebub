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

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Configurations
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Configuration "debug" {
    Data = {
        Opts_GCC = List "-fno-omit-frame-pointer -g3",
    },
}

Configuration "release" {
    Data = {
        Opts_GCC = List { },
    },
}

Configuration "profile" {
    Data = {
        Opts_GCC = List { },
    },

    Base = "release",
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Architectures
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Architecture "amd64" {
    Data = {
        GccTargetName   = "x86_64",

        Opts_GCC        = LST "-m64 !Opts_GCC_x86",
        Opts_GCC_Kernel = LST "-mno-red-zone -mtls-gs !Opts_GCC_Kernel_x86",
        Opts_NASM       = List "-f elf64",
    },

    Description = "64-bit x86",

    Base = "x86",
}

Architecture "ia32" {
    Data = {
        Opts_GCC        = LST "-m32 !Opts_GCC_x86",
        Opts_GCC_Kernel = LST "!Opts_GCC_Kernel_x86",
        Opts_NASM       = List "-f elf32",
    },

    Description = "32-bit x86",

    Base = "x86",
}

Architecture "x86" {
    Data = {
        Opts_GCC_x86        = List "-mtls-direct-seg-refs",
        Opts_GCC_Kernel_x86 = List [[
            -ffreestanding -nostdlib -static-libgcc
            -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2
            -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3
            -mno-bmi -mno-bmi2
        ]],

        ISO = true,

        CrtFiles = List "crt0.o crti.o crtn.o",
    },

    Auxiliary = true,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Toolchain
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local xcsDir, settXcDir = Path(env.CROSSCOMPILERS_DIR or "/usr/local"), nil

local MISC_TOOLS_DIR = env.MISC_TOOLS_DIR

CmdOpt "xc-dir" {
    Description = "The directory containing the cross-compiler's binaries for the"
             .. "\nselected architecture."
             .. "\nBy default, `$CROSSCOMPILERS_DIR/gcc-<arch>-beelzebub/bin` is used."
             .. "\nThe <arch> is substituted with GCC's name for the selected architecture."
             .. "\nIf CROSSCOMPILERS_DIR is undefined in the environment,"
             .. "\nit defaults to `/usr/local`.",

    Type = "string",
    Display = "directory",

    Handler = function(val)
        settXcDir = val
    end,
}

GlobalData {
    XCDirectory = function()
        return settXcDir or (xcsDir + ("gcc-" .. selArch.Data.GccTargetName .. "-beelzebub/bin"))
    end,

    CC    = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-gcc'",
    CXX   = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-g++'",
    GAS   = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-gcc'",
    LO    = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-gcc'",
    LD    = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-ld'",
    AR    = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-gcc-ar'",
    STRIP = DAT "XCDirectory + selArch.Data.GccTargetName .. '-beelzebub-strip'",
    AS    = "nasm",
    TAR   = "tar",
    GZIP  = "gzip",

    MKISO = function()
        if _G.os.execute("mkisofs --version > /dev/null 2> /dev/null") then
            return "mkisofs"
        elseif _G.os.execute("genisoimage --version > /dev/null 2> /dev/null") then
            return "genisoimage"
        elseif MISC_TOOLS_DIR then
            --  Or maybe it comes from an external source?

            return MISC_TOOLS_DIR .. "/genisoimage"
        end

        error("Could not find mkisofs or genisoimage, or $MISC_TOOLS_DIR in the environment.")
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Tests
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local availableTests = List {
    -- "MT",
    -- "STR",
    -- "PMM",
    "VMM",
    -- "OBJA",
    --"METAP",
    --"EXCP",
    -- "APP",
    -- "KMOD",
    -- "TIMER",
    -- "MAILBOX",
    -- "STACKINT",
    -- "AVL_TREE",
    --"TERMINAL",
    --"CMDO",
    -- "FPU",
    -- "BIGINT",
    -- "LOCK_ELISION",
    -- "RW_SPINLOCK",
    -- "RW_TICKETLOCK",
    -- "VAS",
    --"INTERRUPT_LATENCY",
    "MALLOC",
}

local settSelTests, settUnitTests = List { }, true

CmdOpt "tests" "t" {
    Description = "Specifies which tests to include in the Beelzebub build.",
    Display = "name-1,name 2;name_3,...|all",

    Type = "string",
    Many = true,

    Handler = function(val)
        if val == "all" then
            availableTests:ForEach(function(testName)
                settSelTests:AppendUnique(testName)
            end)
        else
            for test in string.iteratesplit(val, "[,;]") do
                if #test == 0 or string.find(test, "[^%a%d%s%-_]") then
                    error("Beelzebub test \"" .. test .. "\" contains invalid characters.")
                end

                local testName = string.upper(string.gsub(test, "[%s%-]", "_"))

                if not availableTests:Contains(testName) then
                    error("Unknown Beelzebub test \"" .. testName .. "\".")
                end

                settSelTests:AppendUnique(testName)
            end
        end
    end,
}

CmdOpt "unit-tests" {
    Description = "Specifies whether kernel unit tests are included in the"
             .. "\nkernel or not, or if they will be quieted; defaults to yes"
             .. "\n(included but not quieted).",

    Type = "string",
    Display = "boolean|quiet",

    Handler = function(val)
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

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Component Choices
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local availableDynamicAllocators = List {
    "valloc", "streamflow", "ptmalloc3", "jemalloc", "none"
}

local settKrnDynAlloc, settUsrDynAlloc = "VALLOC", "NONE"

local dynAllocLibs = {
    VALLOC      = "valloc",
    STREAMFLOW  = "streamflow",
    PTMALLOC3   = "ptmalloc3",
    JEMALLOC    = "jemalloc",
}

CmdOpt "kernel-dynalloc" {
    Description = "The dynamic allocator used in the kernel; defaults to " .. settKrnDynAlloc .. ".",

    Type = "string",
    Display = availableDynamicAllocators:Print("|"),

    Handler = function(val)
        if not availableDynamicAllocators:Contains(string.lower(val)) then
            error("Invalid value given to \"kernel-dynalloc\" command-line option: \""
                .. val .. "\".")
        end

        settKrnDynAlloc = string.upper(val)
    end,
}

CmdOpt "userland-dynalloc" {
    Description = "The dynamic allocator used in the userland; defaults to " .. settUsrDynAlloc .. ".",

    Type = "string",
    Display = availableDynamicAllocators:Print("|"),

    Handler = function(val)
        if not availableDynamicAllocators:Contains(string.lower(val)) then
            error("Invalid value given to \"userland-dynalloc\" command-line option: \""
                .. val .. "\".")
        end

        settUsrDynAlloc = string.upper(val)
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Code Generation Options
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local availableApicModes = List {
    "legacy", "x2apic", "flexible"
}

local specialOptions = List { }
local settApicMode = "FLEXIBLE"
local settSmp, settInlineSpinlocks = true, true

CmdOpt "march" {
    Description = "Specifies an `-march=` option to pass on to GCC on compilation.",

    Type = "string",

    Handler = function(val)
        if val == "" then
            error("Value given to \"march\" command-line option must be non-empty.")
        end

        specialOptions:Append("-march=" .. val)
    end,
}

CmdOpt "mtune" {
    Description = "Specifies an `-mtune=` option to pass on to GCC on compilation.",

    Type = "string",

    Handler = function(val)
        if val == "" then
            error("Value given to \"mtune\" command-line option must be non-empty.")
        end

        specialOptions:Append("-mtune=" .. val)
    end,
}

CmdOpt "smp" {
    Description = "Specifies whether the Beelzebub build includes symmetric"
             .. "\nmultiprocessing support; defaults to yes.",

    Type = "boolean",

    Handler = function(val) settSmp = val end,
}

CmdOpt "inline-spinlocks" {
    Description = "Specifies whether spinlock operations are inlined in the"
             .. "\nkernel code; defaults to yes.",

    Type = "boolean",

    Handler = function(val) settInlineSpinlocks = val end,
}

CmdOpt "apic-mode" {
    Description = "The APIC mode(s) supported by the kernel. Defaults to flexible.",

    Type = "string",
    Display = availableApicModes:Print("|"),

    Handler = function(val)
        if not availableApicModes:Contains(string.lower(val)) then
            error("Invalid value given to \"apic-mode\" command-line option: \""
                .. val .. "\".")
        end

        settApicMode = string.upper(val)
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Dependency Management
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local settMakeDeps = true

CmdOpt "no-make-deps" {
    Description = "Indicates that GNU Make dependency files are not to be"
             .. "\ngenerated by GCC, G++ or GAS.",

    Handler = function(val) settMakeDeps = false end,
}

GlobalData {
    UseMakeDependencies = function()
        return settMakeDeps
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Options and Parameters
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

GlobalData {
    Opts_GCC_Tests = function()
        return settSelTests:Select(function(val)
            return "-D__BEELZEBUB__TEST_" .. val
        end)
    end,

    Opts_GCC_Precompiler = function()
        local res = List {
            "-D__BEELZEBUB",
            "-D__BEELZEBUB__ARCH=" .. selArch.Name,
            "-D__BEELZEBUB__CONF=" .. selConf.Name,
            "-D__BEELZEBUB_SETTINGS_APIC_MODE=" .. settApicMode,
            "-D__BEELZEBUB_SETTINGS_APIC_MODE_" .. settApicMode,
            "-D__BEELZEBUB_SETTINGS_KRNDYNALLOC=" .. settKrnDynAlloc,
            "-D__BEELZEBUB_SETTINGS_KRNDYNALLOC_" .. settKrnDynAlloc,
            "-D__BEELZEBUB_SETTINGS_USRDYNALLOC=" .. settUsrDynAlloc,
            "-D__BEELZEBUB_SETTINGS_USRDYNALLOC_" .. settUsrDynAlloc,

            settSmp             and "-D__BEELZEBUB_SETTINGS_SMP"                or "-D__BEELZEBUB_SETTINGS_NO_SMP",
            settInlineSpinlocks and "-D__BEELZEBUB_SETTINGS_INLINE_SPINLOCKS"   or "-D__BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS",
            settUnitTests       and "-D__BEELZEBUB_SETTINGS_UNIT_TESTS"         or "-D__BEELZEBUB_SETTINGS_NO_UNIT_TESTS"
        } + Opts_GCC_Tests

        for arch in selArch:Hierarchy() do
            res:Append("-D__BEELZEBUB__ARCH_" .. _G.string.upper(arch.Name))
        end

        for conf in selConf:Hierarchy() do
            res:Append("-D__BEELZEBUB__CONF_" .. _G.string.upper(conf.Name))
        end

        if settUnitTests == "quiet" then
            res:Append("-D__BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET")
        end

        return res
    end,

    Opts_GCC_Common = function()
        local res = List {
            --"-fdollars-in-identifiers",
            "-pipe", "--sysroot=" .. Sysroot,
            "-Wshadow", "-Wframe-larger-than=300",
        } + Opts_GCC_Precompiler
        + selArch.Data.Opts_GCC + selConf.Data.Opts_GCC
        + specialOptions

        if settMakeDeps then
            res:Append("-MD"):Append("-MP")
        end

        return res
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Main File Locations
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

GlobalData {
    Sysroot                 = DAT "outDir + 'sysroot'",
    SysheadersPath          = DAT "Sysroot + 'usr/include'",
    JegudielPath            = DAT "outDir  + 'jegudiel.bin'",
    CommonLibraryPath       = DAT "Sysroot + 'usr/lib/libcommon.'    .. selArch.Name .. '.a'",
    RuntimeLibraryPath      = DAT "Sysroot + 'usr/lib/libbeelzebub.' .. selArch.Name .. '.so'",
    KernelModuleLibraryPath = DAT "Sysroot + 'usr/lib/libbeelzebub.kmod.so'",
    TestKernelModulePath    = DAT "Sysroot + 'kmods/test.kmod'",
    KernelPath              = DAT "outDir  + 'beelzebub.bin'",
    LoadtestAppPath         = DAT "Sysroot + 'apps/loadtest.exe'",

    CrtFiles = function()
        return selArch.Data.CrtFiles:Select(function(val)
            return Sysroot + "usr/lib" + val
        end)
    end,

    IsoFile = function()
        if selArch.Base.Data.ISO then
            return outDir + ("beelzebub." .. selArch.Name .. "." .. selConf.Name .. ".iso")
        end
    end,

    VallocKernelLibraryPath         = DAT "Sysroot + 'usr/lib/libvalloc.kernel.a'",
    VallocUserlandLibraryPath       = DAT "Sysroot + 'usr/lib/libvalloc.userland.a'",
    StreamflowKernelLibraryPath     = DAT "Sysroot + 'usr/lib/libstreamflow.kernel.a'",
    StreamflowUserlandLibraryPath   = DAT "Sysroot + 'usr/lib/libstreamflow.userland.a'",
    JemallocKernelLibraryPath       = DAT "Sysroot + 'usr/lib/libjemalloc.kernel.a'",
    JemallocUserlandLibraryPath     = DAT "Sysroot + 'usr/lib/libjemalloc.userland.a'",

    KernelDynamicAllocatorPath = function()
        return Sysroot + ("usr/lib/lib" .. dynAllocLibs[settKrnDynAlloc] .. ".kernel.a")
    end,

    UserlandDynamicAllocatorPath = function()
        return Sysroot + ("usr/lib/lib" .. dynAllocLibs[settUsrDynAlloc] .. ".userland.a")
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Utilities
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local function gzipSingleFile(dst, src)
    local tmp = dst:TrimEnd(3)  --  3 = #".gz"
    fs.Copy(tmp, src[1])
    sh.silent(GZIP, "-f", "-9", tmp)
end

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Projects
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Project "Beelzebub" {
    Description = "Lord of Flies",

    Output = function()
        if selArch.Base.Data.ISO then
            return IsoFile
        else
            error("TODO")
        end
    end,

    Component "System Headers" {
        Data = {
            Headers = function()
                local files = fs.ListDir(comp.Directory + "common")

                for arch in selArch:Hierarchy() do
                    files:AppendMany(fs.ListDir(comp.Directory + arch.Name))
                end

                files = files:Where(function(val)
                    return val.IsFile --and val:EndsWith(".h", ".hpp", ".inc")
                end)

                return files
            end,
        },

        Directory = "sysheaders",

        Output = function()
            local files = Headers:Select(function(val)
                return SysheadersPath + val:SkipDirs(2)
            end)

            return files
        end,

        Rule "Copy Header" {
            Filter = function(dst)
                return dst:StartsWith(SysheadersPath) and not dst.IsDirectory
            end,

            Source = function(dst)
                local parent = dst:GetParent()
                dst = dst:Skip(SysheadersPath)

                for arch in selArch:Hierarchy() do
                    local src = comp.Directory + arch.Name + dst

                    if fs.GetInfo(src) then
                        return List { src, parent }
                    end
                end

                return List { comp.Directory + "common" + dst, parent }
            end,

            Action = function(dst, src)
                fs.Copy(dst, src[1])
            end,
        },
    },

    Component "Jegudiel" {
        Data = {
            SourceDirectory     = DAT "comp.Directory + 'src'",

            ObjectsDirectory    = DAT "outDir + 'jegudiel'",
            BinaryPath          = DAT "ObjectsDirectory + 'jegudiel.bin'",

            Objects = function()
                return fs.ListDir(SourceDirectory)
                    :Where(function(val) return val:EndsWith(".c", ".s") end)
                    :Select(function(val)
                        return ObjectsDirectory + val:Skip(SourceDirectory) .. ".o"
                    end)
            end,

            LinkerScript        = DAT "comp.Directory + 'link.ld'",

            Opts_Includes = function()
                return List {
                    "-I" .. (comp.Directory + "inc"),
                    "-Isysheaders/" .. selArch.Name, --  Harcoded but meh...
                }
            end,

            Opts_C = function()
                return List [[-m64 -ffreestanding
                    -Wall -Wextra -Wpedantic
                    -Wsystem-headers -mcmodel=kernel -static-libgcc
                    -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3
                    -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 -mno-bmi -mno-bmi2
                    -O0
                ]] + Opts_GCC_Precompiler + Opts_Includes
                   + specialOptions
            end,

            Opts_NASM       = List "-f elf64",
            Opts_LD         = List "-z max-page-size=0x1000",
            Opts_STRIP      = List "-s -K multiboot_header",
        },

        Directory = "jegudiel",

        Dependencies = "System Headers",

        Output = DAT "JegudielPath",

        Rule "Link Binary" {
            Filter = FLT "BinaryPath",

            Source = function(dst)
                return Objects + List { LinkerScript, dst:GetParent() }
            end,

            Action = ACT "!LD !Opts_LD -T !LinkerScript -o !dst !Objects",
        },

        Rule "Strip Binary" {
            Filter = FLT "JegudielPath",

            Source = function(dst)
                return List {BinaryPath, dst:GetParent() }
            end,

            Action = ACT "!STRIP !Opts_STRIP -o !dst !BinaryPath",
        },

        Rule "Compile C" {
            Filter = function(dst) return dst:EndsWith(".c.o") end,

            Source = function(dst)
                return List { SourceDirectory + dst:Skip(ObjectsDirectory):TrimEnd(2), dst:GetParent() }
                --  2 = #".o"
            end,

            Action = ACT "!CC !Opts_C -x c -c !src[1] -o !dst",
        },

        Rule "Assemble" {
            Filter = function(dst) return dst:EndsWith(".s.o") end,

            Source = function(dst)
                return List { SourceDirectory + dst:Skip(ObjectsDirectory):TrimEnd(2), dst:GetParent() }
            end,

            Action = ACT "!AS !Opts_NASM !src[1] -o !dst",
        },

        ExcuseMissingFilesRule { "h", "hpp", "inc", "hh" },
    },

    ManagedComponent "Common Library" {
        Languages = { "C", "C++", "GAS", "NASM", },
        Target = "Static Library",
        ExcuseHeaders = true,

        Data = {
            SourcesSubdirectory     = "src",
            HeadersSubdirectory     = "inc",

            Opts_GCC = function()
                local res = List [[
                    -fvisibility=hidden
                    -Wall -Wextra -Wpedantic -Wsystem-headers
                    -D__BEELZEBUB_STATIC_LIBRARY
                ]] + Opts_GCC_Common + Opts_Includes
                   + selArch.Data.Opts_GCC_Kernel

                if selArch.Name == "amd64" then
                    res:Append("-mcmodel=large")
                end

                return res
            end,

            Opts_C          = LST "!Opts_GCC -std=gnu99 -O0 -flto",
            Opts_CXX_CRT    = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",
            Opts_CXX        = LST "!Opts_CXX_CRT -O0 -flto",
            Opts_NASM       = LST "!Opts_GCC_Precompiler !Opts_Includes_Nasm !selArch.Data.Opts_NASM",
            Opts_GAS_CRT    = LST "!Opts_GCC",
            Opts_GAS        = LST "!Opts_GAS_CRT -flto",

            Opts_AR         = List "rcs",
        },

        Directory = "libs/common",

        Dependencies = "System Headers",

        Output = function() return List { CommonLibraryPath } + CrtFiles end,
        --  First one is the output archive/binary for the ManagedComponent.

        Rule "C Runtime Objects" {
            Filter = function(dst) return CrtFiles:Contains(dst) end,

            Source = function(dst)
                local crtName = dst:GetName():TrimEnd(2)
                --  2 = #".o"

                for arch in selArch:Hierarchy() do
                    local archCrtBase = ArchitecturesDirectory + arch.Name + "crt" + crtName
                    local cpp, gas = archCrtBase .. ".cpp", archCrtBase .. ".S"

                    if fs.GetInfo(cpp) then
                        return List { cpp, dst:GetParent() }
                    elseif fs.GetInfo(gas) then
                        return List { gas, dst:GetParent() }
                    end
                end

                error("Unable to find source of CRT file \"" .. tostring(crtName) .. "\".")
            end,

            Action = function(dst, src)
                if src[1]:EndsWith(".cpp") then
                    sh.silent(CXX, Opts_CXX_CRT, "-MD", "-MP", "-c", src[1], "-o", dst)
                else
                    sh.silent(GAS, Opts_GAS_CRT, "-c", src[1], "-o", dst)
                end
            end,
        },
    },

    ManagedComponent "vAlloc - Kernel" {
        Languages = { "C++", },
        Target = "Static Library",
        ExcuseHeaders = true,

        Data = {
            SourcesSubdirectory     = "src",
            HeadersSubdirectory     = "inc",

            ObjectsDirectory        = DAT "outDir + (comp.Directory .. '.kernel')",

            Opts_GCC = function()
                local res = List {
                    "-fvisibility=hidden",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-O0", "-flto",
                    "-D__BEELZEBUB_STATIC_LIBRARY", "-D__BEELZEBUB_KERNEL",
                } + Opts_GCC_Common + Opts_Includes
                + selArch.Data.Opts_GCC_Kernel

                if selArch.Name == "amd64" then
                    res:Append("-mcmodel=kernel")
                end

                return res
            end,

            Opts_CXX        = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",
            Opts_AR         = List "rcs",
        },

        Directory = "libs/valloc",

        Output = function() return List { VallocKernelLibraryPath } end,
    },

    -- ArchitecturalComponent "Streamflow - Kernel" {
    --     Data = {
    --         ObjectsDirectory = function() return outDir + (comp.Directory .. ".kernel") end,

    --         Opts_GCC = function()
    --             local res = List {
    --                 "-fvisibility=hidden",
    --                 "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers", "-Wno-int-to-pointer-cast", "-Wno-strict-aliasing",
    --                 "-O2", "-flto",
    --                 "-D__BEELZEBUB_STATIC_LIBRARY", "-D__BEELZEBUB_KERNEL",
    --                 "-DRADIX_TREE",
    --             } + Opts_GCC_Common + Opts_Includes
    --             + selArch.Data.Opts_GCC_Kernel

    --             if selArch.Name == "amd64" then
    --                 res:Append("-mcmodel=kernel")
    --             end

    --             return res
    --         end,

    --         Opts_C       = function() return Opts_GCC + List { "-std=gnu99", } end,
    --         Opts_CXX     = function() return Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
    --         Opts_NASM    = function() return Opts_GCC_Precompiler + Opts_Includes_Nasm + selArch.Data.Opts_NASM end,
    --         Opts_GAS     = function() return Opts_GCC end,

    --         Opts_AR = List { "rcs" },
    --     },

    --     Directory = "libs/streamflow",

    --     Output = function() return List { StreamflowKernelLibraryPath } end,

    --     Rule "Archive Objects" {
    --         Filter = FLT "StreamflowKernelLibraryPath",

    --         Source = function(dst) return Objects + List { dst:GetParent() } end,

    --         Action = function(dst, src)
    --             sh.silent(AR, Opts_AR, dst, Objects)
    --         end,
    --     },
    -- },

    -- ArchitecturalComponent "jemalloc - Kernel" {
    --     Data = {
    --         ObjectsDirectory = function() return outDir + (comp.Directory .. ".kernel") end,

    --         Opts_GCC = function()
    --             local res = List {
    --                 "-fvisibility=hidden",
    --                 --"-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
    --                 "-Wno-int-to-pointer-cast", "-Wno-strict-aliasing",
    --                 "-Wno-implicit-function-declaration", "-Wno-unused-parameter",
    --                 "-O2", "-flto",
    --                 "-D__BEELZEBUB_STATIC_LIBRARY", "-D__BEELZEBUB_KERNEL",
    --                 "-DJEMALLOC_MALLOC_THREAD_CLEANUP",
    --                 "-DJEMALLOC_PURGE_MADVISE_FREE",
    --                 "-DJEMALLOC_NO_RENAME",
    --             } + Opts_GCC_Common + Opts_Includes
    --             + selArch.Data.Opts_GCC_Kernel

    --             if selArch.Name == "amd64" then
    --                 res:Append("-mcmodel=kernel")
    --             end

    --             return res
    --         end,

    --         Opts_C       = function() return Opts_GCC + List { "-std=gnu99", } end,
    --         Opts_CXX     = function() return Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
    --         Opts_NASM    = function() return Opts_GCC_Precompiler + Opts_Includes_Nasm + selArch.Data.Opts_NASM end,
    --         Opts_GAS     = function() return Opts_GCC end,

    --         Opts_AR = List { "rcs" },
    --     },

    --     Directory = "libs/jemalloc",

    --     Output = function() return List { JemallocKernelLibraryPath } end,

    --     Rule "Archive Objects" {
    --         Filter = FLT "JemallocKernelLibraryPath",

    --         Source = function(dst) return Objects + List { dst:GetParent() } end,

    --         Action = function(dst, src)
    --             sh.silent(AR, Opts_AR, dst, Objects)
    --         end,
    --     },
    -- },

    ManagedComponent "Runtime Library" {
        Languages = { "C", "C++", "GAS", "NASM", },
        Target = "Dynamic Library",
        ExcuseHeaders = true,

        Data = {
            SourcesSubdirectory     = "src",
            HeadersSubdirectory     = "inc",

            Opts_Includes_Nasm = function()
                return Opts_Includes_Nasm_Base:Append("-I" .. tostring(SysheadersPath) .. "/")
            end,

            Opts_GCC = function()
                return List [[
                    -fvisibility=hidden -fPIC
                    -ffreestanding -nodefaultlibs -static-libgcc
                    -Wall -Wextra -Wpedantic -Wsystem-headers
                    -O0 -flto
                    -D__BEELZEBUB_DYNAMIC_LIBRARY
                ]] + Opts_GCC_Common + Opts_Includes
            end,

            Opts_C      = LST "!Opts_GCC -std=gnu99",
            Opts_CXX    = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",
            Opts_NASM   = LST "!Opts_GCC_Precompiler !Opts_Includes_Nasm !selArch.Data.Opts_NASM",
            Opts_GAS    = LST "!Opts_GCC",

            LD          = DAT "LO",
            Opts_LD     = LST "-fuse-linker-plugin -Wl,-z,max-page-size=0x1000 -Wl,-Bsymbolic !Opts_GCC",
            Opts_STRIP  = List "-s",

            Libraries = function()
                local res = List {
                    "common." .. selArch.Name,
                }

                if settUsrDynAlloc ~= "NONE" then
                    res:Append(dynAllocLibs[settUsrDynAlloc] .. ".userland")
                end

                return res;
            end,

            BinaryPath  = DAT "ObjectsDirectory + RuntimeLibraryPath:GetName()",

            BinaryDependencies = function()
                local res = List {
                    CommonLibraryPath,
                } + CrtFiles

                return res
            end,
        },

        Directory = "libs/runtime",

        Dependencies = "System Headers",

        Output = DAT "RuntimeLibraryPath",

        Rule "Strip Binary" {
            Filter = FLT "RuntimeLibraryPath",

            Source = function(dst)
                return List { BinaryPath, dst:GetParent() }
            end,

            Action = ACT "!STRIP !Opts_STRIP -o !dst !BinaryPath",
        },
    },

    ManagedComponent "Kernel Module Library" {
        Languages = { "C++", },
        Target = "Dynamic Library",
        ExcuseHeaders = true,

        Data = {
            Opts_GCC = function()
                return List [[
                    -fvisibility=hidden -fPIC
                    -ffreestanding -nodefaultlibs -static-libgcc
                    -Wall -Wextra -Wpedantic -Wsystem-headers
                    -O0 -flto
                    -D__BEELZEBUB_DYNAMIC_LIBRARY
                ]] + Opts_GCC_Common + Opts_Includes
            end,

            Opts_CXX    = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",

            LD          = DAT "LO",
            Opts_LD     = LST "-shared -fuse-linker-plugin -Wl,-z,max-page-size=0x1000 -Wl,-Bsymbolic !Opts_GCC",

            Opts_STRIP  = List "-s",

            BinaryPath  = DAT "ObjectsDirectory + KernelModuleLibraryPath:GetName()",
        },

        Directory = "libs/kmod",

        Dependencies = "System Headers",

        Output = DAT "KernelModuleLibraryPath",

        Rule "Strip Binary" {
            Filter = FLT "KernelModuleLibraryPath",

            Source = function(dst)
                return List { BinaryPath, dst:GetParent() }
            end,

            Action = ACT "!STRIP !Opts_STRIP -o !dst !BinaryPath",
        },
    },

    ManagedComponent "Loadtest Application" {
        Languages = { "C++", },
        Target = "Executable",
        ExcuseHeaders = true,

        Data = {
            Opts_GCC = function()
                return List [[
                    -fvisibility=hidden
                    -Wall -Wsystem-headers
                    -O0 -flto
                    -D__BEELZEBUB_APPLICATION
                ]] + Opts_GCC_Common + Opts_Includes
            end,

            Opts_CXX    = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",

            LD          = DAT "LO",
            Opts_LD     = LST "-fuse-linker-plugin -Wl,-z,max-page-size=0x1000 !Opts_GCC",

            Opts_STRIP  = List "-s",

            BinaryPath  = DAT "ObjectsDirectory + LoadtestAppPath:GetName()",

            BinaryDependencies = function()
                local res = List {
                    RuntimeLibraryPath,
                }

                return res
            end,
        },

        Directory = "apps/loadtest",

        Dependencies = "System Headers",

        Output = DAT "LoadtestAppPath",

        Rule "Strip Binary" {
            Filter = FLT "LoadtestAppPath",

            Source = function(dst)
                return List { BinaryPath, dst:GetParent() }
            end,

            Action = ACT "!STRIP !Opts_STRIP -o !dst !BinaryPath",
        },
    },

    ManagedComponent "Kernel" {
        Languages = { "C", "C++", "GAS", "NASM", },
        Target = "Executable",
        ExcuseHeaders = true,

        Data = {
            ArchitecturesDirectory  = DAT "comp.Directory + 'arc'",
            SourcesSubdirectory     = "src",
            HeadersSubdirectory     = "inc",

            Opts_Includes           = LST "-Iacpica/include !Opts_Includes_Base",

            Opts_Includes_Nasm = function()
                return Opts_Includes_Nasm_Base:Append(("-I" .. SysheadersPath) .. "/")
            end,

            Opts_GCC = function()
                local res = List [[
                    -fvisibility=hidden -fno-PIE -fno-PIC
                    -Wall -Wextra -Wpedantic -Wsystem-headers
                    -O0 -flto
                    -D__BEELZEBUB_KERNEL
                ]] + Opts_GCC_Common + Opts_Includes
                   + selArch.Data.Opts_GCC_Kernel

                if selArch.Name == "amd64" then
                    res:Append("-mcmodel=kernel")
                end

                return res
            end,

            Opts_C      = LST "!Opts_GCC -std=gnu99",
            Opts_CXX    = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",
            Opts_NASM   = LST "!Opts_GCC_Precompiler !Opts_Includes_Nasm !selArch.Data.Opts_NASM",
            Opts_GAS    = LST "!Opts_GCC",

            LD          = DAT "LO",
            Opts_LD     = LST "!Opts_GCC -fuse-linker-plugin -Wl,-z,max-page-size=0x1000",

            Opts_STRIP  = List "-s -K jegudiel_header",

            Libraries = function()
                local res = List {
                    "common." .. selArch.Name,
                }

                if settKrnDynAlloc ~= "NONE" then
                    res:Append(dynAllocLibs[settKrnDynAlloc] .. ".kernel")
                end

                return res;
            end,

            BinaryPath      = DAT "ObjectsDirectory + KernelPath:GetName()",
            LinkerScript    = DAT "ArchitecturesDirectory + selArch.Name + 'link.ld'",

            PrecompiledCppHeader = "common.hpp",

            BinaryDependencies = function()
                local res = List {
                    CommonLibraryPath,
                }

                if settKrnDynAlloc ~= "NONE" then
                    res:Append(KernelDynamicAllocatorPath)
                end

                return res
            end,
        },

        Directory = "beelzebub",

        Dependencies = "System Headers",

        Output = DAT "KernelPath",

        Rule "Strip Binary" {
            Filter = FLT "KernelPath",

            Source = function(dst)
                return List { BinaryPath, dst:GetParent() }
            end,

            Action = ACT "!STRIP !Opts_STRIP -o !dst !BinaryPath",
        },
    },

    ManagedComponent "Test Kernel Module" {
        Languages = { "C++", },
        Target = "Dynamic Library",
        ExcuseHeaders = true,

        Data = {
            Opts_GCC = function()
                local res = List [[
                    -fvisibility=hidden -fPIC
                    -Wall -Wextra -Wpedantic -Wsystem-headers
                    -O0 -flto
                    -D__BEELZEBUB_KERNEL_MODULE
                ]] + Opts_GCC_Common + Opts_Includes
                   + selArch.Data.Opts_GCC_Kernel

                return res
            end,

            Opts_CXX    = LST "!Opts_GCC -std=gnu++14 -fno-rtti -fno-exceptions",

            LD          = DAT "LO",
            Opts_LD     = LST "-fuse-linker-plugin -Wl,-z,max-page-size=0x1000 -Wl,-Bsymbolic !Opts_GCC",

            Opts_STRIP  = List "-s",

            Libraries   = List "beelzebub.kmod",

            BinaryPath  = DAT "ObjectsDirectory + TestKernelModulePath:GetName()",

            BinaryDependencies = function()
                local res = List {
                    KernelModuleLibraryPath,
                }

                return res
            end,
        },

        Directory = "kmods/test",

        Dependencies = "System Headers",

        Output = DAT "TestKernelModulePath",

        Rule "Strip Binary" {
            Filter = FLT "TestKernelModulePath",

            Source = function(dst)
                return List { BinaryPath, dst:GetParent() }
            end,

            Action = ACT "!STRIP !Opts_STRIP -o !dst !BinaryPath",
        },
    },

    Component "ISO Image" {
        Data = {
            SysrootFiles = function()
                local res

                if fs.GetInfo(Sysroot) then
                    res = fs.ListDir(Sysroot):Append(Sysroot)
                else
                    res = List { }
                end

                res:AppendUnique(LoadtestAppPath)
                res:AppendUnique(TestKernelModulePath)

                return res
            end,

            Opts_TAR = LST "--owner=root --group=root -C !Sysroot --exclude=*.d --exclude=libcommon.*.a",

            IsoDirectory        = DAT "outDir + 'iso'",
            IsoBootPath         = DAT "IsoDirectory + 'boot'",
            IsoGrubDirectory    = DAT "IsoBootPath + 'grub'",
            IsoJegudielPath     = DAT "IsoBootPath + 'jegudiel.bin.gz'",
            IsoInitRdPath       = DAT "IsoBootPath + 'initrd.tar.gz'",
            IsoKernelPath       = DAT "IsoBootPath + ('beelzebub.' .. selArch.Name .. '.bin.gz')",
            IsoEltoritoPath     = DAT "IsoGrubDirectory + 'stage2_eltorito'",
            IsoGrubfontPath     = DAT "IsoGrubDirectory + 'font.pf2'",
            IsoGrubconfPath     = DAT "IsoGrubDirectory + 'grub.cfg'",

            IsoSources = function(dst)
                local res = List {
                    IsoKernelPath,
                    IsoInitRdPath,
                    IsoEltoritoPath,
                    IsoGrubfontPath,
                    IsoGrubconfPath,

                    TestKernelModulePath,
                    LoadtestAppPath,
                }

                if selArch.Name == "amd64" then
                    res:Append(IsoJegudielPath)
                end

                return res
            end,

            SourceGrub2     = DAT "comp.Directory + 'grub2'",
            SourceEltorito  = DAT "SourceGrub2 + 'stage2_eltorito'",
            SourceGrubfont  = DAT "SourceGrub2 + 'font.pf2'",
            SourceGrubconf  = DAT "SourceGrub2 + ('grub.' .. selArch.Name .. '.cfg')",
        },

        Directory = "image",

        Dependencies = List {
            "System Headers"
        },

        Output = DAT "IsoFile",

        Rule "Make ISO" {
            Filter = FLT "IsoFile",
            Source = function(dst) return IsoSources end,

            Action = ACT "!MKISO -R -b !IsoEltoritoPath:Skip(!IsoDirectory) -no-emul-boot -boot-load-size 4 -boot-info-table -o !IsoFile !IsoDirectory",
        },

        Rule "Archive Sysroot" {
            Filter = FLT "IsoInitRdPath",
            Source = function(dst) return SysrootFiles end,

            Action = ACT "!TAR czf !dst !Opts_TAR .",
        },

        Rule "GZip Jegudiel" {
            Filter = FLT "IsoJegudielPath",
            
            Source = function(dst)
                return List { JegudielPath, dst:GetParent() }
            end,

            Action = gzipSingleFile,
        },

        Rule "GZip Beelzebub" {
            Filter = FLT "IsoKernelPath",
            
            Source = function(dst)
                return List { KernelPath, dst:GetParent() }
            end,

            Action = gzipSingleFile,
        },

        Rule "Copy GRUB El Torito" {
            Filter = FLT "IsoEltoritoPath",
            
            Source = function(dst)
                return List { SourceEltorito, dst:GetParent() }
            end,

            Action = CopySingleFileAction,
        },

        Rule "Copy GRUB Font" {
            Filter = FLT "IsoGrubfontPath",
            
            Source = function(dst)
                return List { SourceGrubfont, dst:GetParent() }
            end,

            Action = CopySingleFileAction,
        },

        Rule "Copy GRUB Configuration" {
            Filter = FLT "IsoGrubconfPath",
            
            Source = function(dst)
                return List { SourceGrubconf, dst:GetParent() }
            end,

            Action = CopySingleFileAction,
        },
    },

    CreateMissingDirectoriesRule(true),
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Wrap up
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Default "Beelzebub" "amd64" "debug"

vmake()
