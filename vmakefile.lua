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
        Opts_GCC = List { "-fno-omit-frame-pointer", "-g3" }
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

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Architectures
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Architecture "amd64" {
    Data = {
        GccTargetName = "x86_64",

        Opts_GCC = function(_)
            return List {
                "-m64",
            } + _.Opts_GCC_x86
        end,

        Opts_GCC_Kernel = function(_)
            return List {
                "-mno-red-zone", "-mtls-gs",
            } + _.Opts_GCC_Kernel_x86
        end,

        Opts_NASM = List { "-f", "elf64" },
    },

    Description = "64-bit x86",

    Base = "x86",
}

Architecture "ia32" {
    Data = {
        Opts_GCC = function(_)
            return List {
                "-m32",
            } + _.Opts_GCC_x86
        end,

        Opts_GCC_Kernel = function(_) return _.Opts_GCC_Kernel_x86 end,

        Opts_NASM = List { "-f", "elf32" },
    },

    Description = "32-bit x86",

    Base = "x86",
}

Architecture "x86" {
    Data = {
        Opts_GCC_x86 = List {
            "-mtls-direct-seg-refs",
        },

        Opts_GCC_Kernel_x86 = List {
            "-ffreestanding", "-nostdlib", "-static-libgcc",
            "-mno-aes", "-mno-mmx", "-mno-pclmul", "-mno-sse", "-mno-sse2",
            "-mno-sse3", "-mno-sse4", "-mno-sse4a", "-mno-fma4", "-mno-ssse3",
        },

        ISO = true,

        CrtFiles = List { "crt0.o", "crti.o", "crtn.o" },
    },

    Auxiliary = true,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Toolchain
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local xcsDir, settXcDir = Path(os.getenv("CROSSCOMPILERS_DIR") or "/usr/local"), nil

local MISC_TOOLS_DIR = os.getenv("MISC_TOOLS_DIR")

CmdOpt "xc-dir" {
    Description = "The directory containing the cross-compiler's binaries for the"
             .. "\nselected architecture."
             .. "\nBy default, `$CROSSCOMPILERS_DIR/gcc-<arch>-beelzebub/bin` is used."
             .. "\nThe <arch> is substituted with GCC's name for the selected architecture."
             .. "\nIf CROSSCOMPILERS_DIR is undefined in the environment,"
             .. "\nit defaults to `/usr/local`.",

    Type = "string",
    Display = "directory",

    Handler = function(_, val)
        settXcDir = val
    end,
}

GlobalData {
    XCDirectory = function(_)
        return settXcDir or (xcsDir + ("gcc-" .. _.selArch.Data.GccTargetName .. "-beelzebub/bin"))
    end,

    CC    = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-gcc"    end,
    CXX   = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-g++"    end,
    GAS   = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-gcc"    end,
    LO    = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-gcc"    end,
    LD    = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-ld"     end,
    AR    = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-gcc-ar" end,
    STRIP = function(_) return _.XCDirectory + _.selArch.Data.GccTargetName .. "-beelzebub-strip"  end,
    AS    = "nasm",
    TAR   = "tar",
    GZIP  = "gzip",

    MKISO = function(_)
        if os.execute("mkisofs --version > /dev/null 2> /dev/null") then
            return "mkisofs"
        elseif os.execute("genisoimage --version > /dev/null 2> /dev/null") then
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
    "MT",
    -- "STR",
    -- "PMM",
    "VMM",
    -- "OBJA",
    --"METAP",
    --"EXCP",
    "APP",
    "KMOD",
    -- "TIMER",
    -- "MAILBOX",
    --"STACKINT",
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

    Handler = function(_, val)
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

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Component Choices
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local availableDynamicAllocators = List {
    "valloc", "streamflow", "ptmalloc3", "jemalloc", "none"
}

local settKrnDynAlloc, settUsrDynAlloc = "VALLOC", "NONE"

local dynAllocLibs = {
    VALLOC = "valloc",
    STREAMFLOW = "streamflow",
    PTMALLOC3 = "ptmalloc3",
    JEMALLOC = "jemalloc",
}

CmdOpt "kernel-dynalloc" {
    Description = "The dynamic allocator used in the kernel; defaults to " .. settKrnDynAlloc .. ".",

    Type = "string",
    Display = availableDynamicAllocators:Print("|"),

    Handler = function(_, val)
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

    Handler = function(_, val)
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

CmdOpt "smp" {
    Description = "Specifies whether the Beelzebub build includes symmetric"
             .. "\nmultiprocessing support; defaults to yes.",

    Type = "boolean",

    Handler = function(_, val) settSmp = val end,
}

CmdOpt "inline-spinlocks" {
    Description = "Specifies whether spinlock operations are inlined in the"
             .. "\nkernel code; defaults to yes.",

    Type = "boolean",

    Handler = function(_, val) settInlineSpinlocks = val end,
}

CmdOpt "apic-mode" {
    Description = "The APIC mode(s) supported by the kernel. Defaults to flexible.",

    Type = "string",
    Display = availableApicModes:Print("|"),

    Handler = function(_, val)
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

    Handler = function(_, val) settMakeDeps = false end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Options and Parameters
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

GlobalData {
    Opts_GCC_Tests = function(_)
        return settSelTests:Select(function(val)
            return "-D__BEELZEBUB__TEST_" .. val
        end)
    end,

    Opts_GCC_Precompiler = function(_)
        local res = List {
            "-D__BEELZEBUB",
            "-D__BEELZEBUB__ARCH=" .. _.selArch.Name,
            "-D__BEELZEBUB__CONF=" .. _.selConf.Name,
            "-D__BEELZEBUB_SETTINGS_APIC_MODE=" .. settApicMode,
            "-D__BEELZEBUB_SETTINGS_APIC_MODE_" .. settApicMode,
            "-D__BEELZEBUB_SETTINGS_KRNDYNALLOC=" .. settKrnDynAlloc,
            "-D__BEELZEBUB_SETTINGS_KRNDYNALLOC_" .. settKrnDynAlloc,
            "-D__BEELZEBUB_SETTINGS_USRDYNALLOC=" .. settUsrDynAlloc,
            "-D__BEELZEBUB_SETTINGS_USRDYNALLOC_" .. settUsrDynAlloc,

            settSmp             and "-D__BEELZEBUB_SETTINGS_SMP"                or "-D__BEELZEBUB_SETTINGS_NO_SMP",
            settInlineSpinlocks and "-D__BEELZEBUB_SETTINGS_INLINE_SPINLOCKS"   or "-D__BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS",
            settUnitTests       and "-D__BEELZEBUB_SETTINGS_UNIT_TESTS"         or "-D__BEELZEBUB_SETTINGS_NO_UNIT_TESTS"
        } + _.Opts_GCC_Tests

        for arch in _.selArch:Hierarchy() do
            res:Append("-D__BEELZEBUB__ARCH_" .. string.upper(arch.Name))
        end

        for conf in _.selConf:Hierarchy() do
            res:Append("-D__BEELZEBUB__CONF_" .. string.upper(conf.Name))
        end

        if settUnitTests == "quiet" then
            res:Append("-D__BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET")
        end

        return res
    end,

    Opts_GCC_Common = function(_)
        local res = List {
            --"-fdollars-in-identifiers",
            "-pipe", "--sysroot=" .. tostring(_.Sysroot),
        } + _.Opts_GCC_Precompiler
        + _.selArch.Data.Opts_GCC + _.selConf.Data.Opts_GCC
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

    VallocKernelLibraryPath         = function(_) return _.Sysroot + "usr/lib/libvalloc.kernel.a" end,
    VallocUserlandLibraryPath       = function(_) return _.Sysroot + "usr/lib/libvalloc.userland.a" end,
    StreamflowKernelLibraryPath     = function(_) return _.Sysroot + "usr/lib/libstreamflow.kernel.a" end,
    StreamflowUserlandLibraryPath   = function(_) return _.Sysroot + "usr/lib/libstreamflow.userland.a" end,
    JemallocKernelLibraryPath       = function(_) return _.Sysroot + "usr/lib/libjemalloc.kernel.a" end,
    JemallocUserlandLibraryPath     = function(_) return _.Sysroot + "usr/lib/libjemalloc.userland.a" end,

    KernelDynamicAllocatorPath = function(_)
        return _.Sysroot + ("usr/lib/lib" .. dynAllocLibs[settKrnDynAlloc] .. ".kernel.a")
    end,

    UserlandDynamicAllocatorPath = function(_)
        return _.Sysroot + ("usr/lib/lib" .. dynAllocLibs[settUsrDynAlloc] .. ".userland.a")
    end,
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Utilities
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

local function parseObjectExtension(path, header)
    if header then
        return tostring(path):match("^(.+)%.gch$")
    else
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
end

local function checkObjectExtension(path, ext, header)
    local part1, part2 = tostring(path):match(header and "^(.+)%.([^%.]+)%.gch$"
                                                     or  "^(.+)%.([^%.]+)%.o$")

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

local function sourceArchitecturalCode(_, dst)
    local file, arch, src, res = parseObjectExtension(dst, false)

    if arch then
        src = _.ArchitecturesDirectory + arch.Name + "src" + Path(file):Skip(_.ObjectsDirectory)
    else
        src = _.CommonDirectory + "src" + Path(file):Skip(_.ObjectsDirectory)
    end

    if settMakeDeps then
        res = ParseGccDependencies(dst, src, true) + List { dst:GetParent() }
    else
        res = List { src, dst:GetParent() }
    end

    if _.PrecompiledCppHeader then
        res:Append(_.PrecompiledCppHeaderPath .. ".gch")
    end

    return res
end

local function sourceArchitecturalHeader(_, dst)
    local file, src, res = parseObjectExtension(dst, true), nil
    file = Path(file):Skip(_.PchDirectory)

    for arch in _.selArch:Hierarchy() do
        local pth = _.ArchitecturesDirectory + arch.Name + "inc" + file

        if fs.GetInfo(pth) then
            src = pth

            break
        end
    end

    if not src then
        src = _.comp.Directory + "inc" + file
    end

    if settMakeDeps then
        res = ParseGccDependencies(dst, src, true) + List { dst:GetParent() }
    else
        res = List { src, dst:GetParent() }
    end

    return res
end

local function gzipSingleFile(_, dst, src)
    local tmp = dst:TrimEnd(3)  --  3 = #".gz"
    fs.Copy(tmp, src[1])
    sh.silent(_.GZIP, "-f", "-9", tmp)
end

local function ArchitecturalComponent(name)
    local comp = Component(name)({
        Rule "Compile C" {
            Filter = function(_, dst) return checkObjectExtension(dst, "c", false) end,

            Source = sourceArchitecturalCode,

            Action = function(_, dst, src)
                sh.silent(_.CC, _.Opts_C, "-x", "c", "-c", src[1], "-o", dst)
            end,
        },

        Rule "Compile C++" {
            Filter = function(_, dst) return checkObjectExtension(dst, "cpp", false) end,

            Source = sourceArchitecturalCode,

            Action = function(_, dst, src)
                sh.silent(_.CXX, _.Opts_CXX, "-x", "c++", "-c", src[1], "-o", dst)
            end,
        },

        Rule "Assemble w/ NASM" {
            Filter = function(_, dst) return checkObjectExtension(dst, "asm", false) end,

            Source = sourceArchitecturalCode,

            Action = function(_, dst, src)
                sh.silent(_.AS, _.Opts_NASM, src[1], "-o", dst)
            end,
        },

        Rule "Assemble w/ GAS" {
            Filter = function(_, dst) return checkObjectExtension(dst, "S", false) end,

            Source = sourceArchitecturalCode,

            Action = function(_, dst, src)
                sh.silent(_.GAS, _.Opts_GAS, "-x", "assembler-with-cpp", "-c", src[1], "-o", dst)
            end,
        },

        Rule "Precompile C Header" {
            Filter = function(_, dst) return checkObjectExtension(dst, "h", true) end,

            Source = sourceArchitecturalHeader,

            Action = function(_, dst, src)
                sh.silent(_.CC, _.Opts_C, "-x", "c-header", "-c", src[1], "-o", dst)
            end,
        },

        Rule "Precompile C++ Header" {
            Filter = function(_, dst) return checkObjectExtension(dst, "hpp", true) end,

            Source = sourceArchitecturalHeader,

            Action = function(_, dst, src)
                sh.silent(_.CXX, _.Opts_CXX, "-x", "c++-header", "-c", src[1], "-o", dst)
            end,
        },

        ExcuseMissingFilesRule { "h", "hpp", "inc", "hh" },
    })

    local data = {
        CommonDirectory = function(_) return _.comp.Directory end,

        ArchitecturesDirectory = function(_) return _.comp.Directory end,

        IncludeDirectories = function(_)
            local res = List { }

            if _.PrecompiledCHeader or _.PrecompiledCppHeader then
                res:Append(_.PchDirectory)
            end

            res:Append(_.comp.Directory + "inc")

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
                :Where(function(val) return val:EndsWith(".c", ".cpp", ".asm", ".S") end)
                :Select(function(val)
                    return _.ObjectsDirectory + val:Skip(comSrcDir) .. ".o"
                end)

            for arch in _.selArch:Hierarchy() do
                local arcSrcDir = _.ArchitecturesDirectory + arch.Name + "src"
                local suffix = "." .. arch.Name .. ".o"

                if fs.GetInfo(arcSrcDir) then
                    objects:AppendMany(fs.ListDir(arcSrcDir)
                        :Where(function(val) return val:EndsWith(".c", ".cpp", ".asm", ".S") end)
                        :Select(function(val)
                            return _.ObjectsDirectory + val:Skip(arcSrcDir) .. suffix
                        end))
                end
            end

            return objects
        end,

        PchDirectory = function(_) return _.outDir + _.comp.Directory + ".pch" end,

        PrecompiledCHeaderPath   = function(_) return _.PchDirectory + _.PrecompiledCHeader   end,
        PrecompiledCppHeaderPath = function(_) return _.PchDirectory + _.PrecompiledCppHeader end,
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

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Projects
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Project "Beelzebub" {
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
                    return val.IsFile --and val:EndsWith(".h", ".hpp", ".inc")
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

        Rule "Copy Header" {
            Filter = function(_, dst)
                return dst:StartsWith(_.SysheadersPath) and not dst.IsDirectory
            end,

            Source = function(_, dst)
                local parent = dst:GetParent()
                dst = dst:Skip(_.SysheadersPath)

                for arch in _.selArch:Hierarchy() do
                    local src = _.comp.Directory + arch.Name + dst

                    if fs.GetInfo(src) then
                        return List { src, parent }
                    end
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
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
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
                sh.silent(_.LD, _.Opts_LD, "-T", _.LinkerScript, "-o", dst, _.Objects)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.comp.Output end,

            Source = function(_, dst)
                return List {_.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },

        Rule "Compile C" {
            Filter = function(_, dst) return dst:EndsWith(".c.o") end,

            Source = function(_, dst)
                return List { _.SourceDirectory + dst:Skip(_.ObjectsDirectory):TrimEnd(2), dst:GetParent() }
                --  2 = #".o"
            end,

            Action = function(_, dst, src)
                sh.silent(_.CC, _.Opts_C, "-c", src[1], "-o", dst)
            end,
        },

        Rule "Assemble" {
            Filter = function(_, dst) return dst:EndsWith(".s.o") end,

            Source = function(_, dst)
                return List { _.SourceDirectory + dst:Skip(_.ObjectsDirectory):TrimEnd(2), dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.AS, _.Opts_NASM, src[1], "-o", dst)
            end,
        },

        ExcuseMissingFilesRule { "h", "hpp", "inc", "hh" },
    },

    ArchitecturalComponent "Common Library" {
        Data = {
            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-D__BEELZEBUB_STATIC_LIBRARY",
                } + _.Opts_GCC_Common + _.Opts_Includes
                + _.selArch.Data.Opts_GCC_Kernel

                if _.selArch.Name == "amd64" then
                    res:Append("-mcmodel=large")
                end

                return res
            end,

            Opts_C       = function(_) return _.Opts_GCC + List { "-std=gnu99", "-O2", "-flto", } end,
            Opts_CXX_CRT = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_CXX     = function(_) return _.Opts_CXX_CRT + List { "-O2", "-flto", } end,
            Opts_NASM    = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS_CRT = function(_) return _.Opts_GCC end,
            Opts_GAS     = function(_) return _.Opts_GAS_CRT + List { "-flto", } end,

            Opts_AR = List { "rcs" },
        },

        Directory = "libs/common",

        Dependencies = "System Headers",

        Output = function(_) return List { _.CommonLibraryPath } + _.CrtFiles end,

        Rule "Archive Objects" {
            Filter = function(_, dst) return _.CommonLibraryPath end,

            Source = function(_, dst) return _.Objects + List { dst:GetParent() } end,

            Action = function(_, dst, src)
                sh.silent(_.AR, _.Opts_AR, dst, _.Objects)
            end,
        },

        Rule "C Runtime Objects" {
            Filter = function(_, dst) return _.CrtFiles:Contains(dst) end,

            Source = function(_, dst)
                local crtName = dst:GetName():TrimEnd(2)
                --  2 = #".o"

                for arch in _.selArch:Hierarchy() do
                    local archCrtBase = _.ArchitecturesDirectory + arch.Name + "crt" + crtName
                    local cpp, gas = archCrtBase .. ".cpp", archCrtBase .. ".S"

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
                    sh.silent(_.CXX, _.Opts_CXX_CRT, "-MD", "-MP", "-c", src[1], "-o", dst)
                else
                    sh.silent(_.GAS, _.Opts_GAS_CRT, "-c", src[1], "-o", dst)
                end
            end,
        },
    },

    ArchitecturalComponent "vAlloc - Kernel" {
        Data = {
            ObjectsDirectory = function(_) return _.outDir + (_.comp.Directory .. ".kernel") end,

            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-D__BEELZEBUB_STATIC_LIBRARY", "-D__BEELZEBUB_KERNEL",
                } + _.Opts_GCC_Common + _.Opts_Includes
                + _.selArch.Data.Opts_GCC_Kernel

                if _.selArch.Name == "amd64" then
                    res:Append("-mcmodel=kernel")
                end

                return res
            end,

            Opts_C       = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX     = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM    = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS     = function(_) return _.Opts_GCC end,

            Opts_AR = List { "rcs" },
        },

        Directory = "libs/valloc",

        Output = function(_) return List { _.VallocKernelLibraryPath } end,

        Rule "Archive Objects" {
            Filter = function(_, dst) return _.VallocKernelLibraryPath end,

            Source = function(_, dst) return _.Objects + List { dst:GetParent() } end,

            Action = function(_, dst, src)
                sh.silent(_.AR, _.Opts_AR, dst, _.Objects)
            end,
        },
    },

    -- ArchitecturalComponent "Streamflow - Kernel" {
    --     Data = {
    --         ObjectsDirectory = function(_) return _.outDir + (_.comp.Directory .. ".kernel") end,

    --         Opts_GCC = function(_)
    --             local res = List {
    --                 "-fvisibility=hidden",
    --                 "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers", "-Wno-int-to-pointer-cast", "-Wno-strict-aliasing",
    --                 "-O2", "-flto",
    --                 "-D__BEELZEBUB_STATIC_LIBRARY", "-D__BEELZEBUB_KERNEL",
    --                 "-DRADIX_TREE",
    --             } + _.Opts_GCC_Common + _.Opts_Includes
    --             + _.selArch.Data.Opts_GCC_Kernel

    --             if _.selArch.Name == "amd64" then
    --                 res:Append("-mcmodel=kernel")
    --             end

    --             return res
    --         end,

    --         Opts_C       = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
    --         Opts_CXX     = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
    --         Opts_NASM    = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
    --         Opts_GAS     = function(_) return _.Opts_GCC end,

    --         Opts_AR = List { "rcs" },
    --     },

    --     Directory = "libs/streamflow",

    --     Output = function(_) return List { _.StreamflowKernelLibraryPath } end,

    --     Rule "Archive Objects" {
    --         Filter = function(_, dst) return _.StreamflowKernelLibraryPath end,

    --         Source = function(_, dst) return _.Objects + List { dst:GetParent() } end,

    --         Action = function(_, dst, src)
    --             sh.silent(_.AR, _.Opts_AR, dst, _.Objects)
    --         end,
    --     },
    -- },

    -- ArchitecturalComponent "jemalloc - Kernel" {
    --     Data = {
    --         ObjectsDirectory = function(_) return _.outDir + (_.comp.Directory .. ".kernel") end,

    --         Opts_GCC = function(_)
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
    --             } + _.Opts_GCC_Common + _.Opts_Includes
    --             + _.selArch.Data.Opts_GCC_Kernel

    --             if _.selArch.Name == "amd64" then
    --                 res:Append("-mcmodel=kernel")
    --             end

    --             return res
    --         end,

    --         Opts_C       = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
    --         Opts_CXX     = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
    --         Opts_NASM    = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
    --         Opts_GAS     = function(_) return _.Opts_GCC end,

    --         Opts_AR = List { "rcs" },
    --     },

    --     Directory = "libs/jemalloc",

    --     Output = function(_) return List { _.JemallocKernelLibraryPath } end,

    --     Rule "Archive Objects" {
    --         Filter = function(_, dst) return _.JemallocKernelLibraryPath end,

    --         Source = function(_, dst) return _.Objects + List { dst:GetParent() } end,

    --         Action = function(_, dst, src)
    --             sh.silent(_.AR, _.Opts_AR, dst, _.Objects)
    --         end,
    --     },
    -- },

    ArchitecturalComponent "Runtime Library" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.RuntimeLibraryPath:GetName() end,

            Opts_GCC = function(_)
                return List {
                    "-fvisibility=hidden", "-fPIC",
                    "-ffreestanding", "-nodefaultlibs", "-static-libgcc",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-O3", "-flto",
                    "-D__BEELZEBUB_DYNAMIC_LIBRARY",
                } + _.Opts_GCC_Common + _.Opts_Includes
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return List { "-shared", "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", "-Wl,-Bsymbolic", } + _.Opts_GCC end,

            Opts_STRIP = List { "-s" },

            Libraries = function(_)
                local res = List {
                    "common." .. _.selArch.Name,
                }

                if settUsrDynAlloc ~= "NONE" then
                    res:Append(dynAllocLibs[settUsrDynAlloc] .. ".userland")
                end

                return res;
            end,
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
                sh.silent(_.LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.RuntimeLibraryPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Kernel Module Library" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.KernelModuleLibraryPath:GetName() end,

            Opts_GCC = function(_)
                return List {
                    "-fvisibility=hidden", "-fPIC",
                    "-ffreestanding", "-nodefaultlibs", "-static-libgcc",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-D__BEELZEBUB_DYNAMIC_LIBRARY",
                } + _.Opts_GCC_Common + _.Opts_Includes
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
                sh.silent(_.LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.KernelModuleLibraryPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
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
                    "-D__BEELZEBUB_APPLICATION",
                } + _.Opts_GCC_Common + _.Opts_Includes
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
                sh.silent(_.LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.LoadtestAppPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Kernel" {
        Data = {
            ArchitecturesDirectory = function(_) return _.comp.Directory + "arc" end,
            BinaryPath             = function(_) return _.ObjectsDirectory + _.KernelPath:GetName() end,
            LinkerScript           = function(_) return _.ArchitecturesDirectory + _.selArch.Name + "link.ld" end,

            PrecompiledCppHeader = "common.hpp",

            Opts_Includes = function(_) return List { "-Iacpica/include", } + _.Opts_Includes_Base end,

            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden", "-fno-PIE", "-fno-PIC",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-D__BEELZEBUB_KERNEL",
                } + _.Opts_GCC_Common + _.Opts_Includes
                + _.selArch.Data.Opts_GCC_Kernel

                if _.selArch.Name == "amd64" then
                    res:Append("-mcmodel=kernel")
                end

                return res
            end,

            Opts_C    = function(_) return _.Opts_GCC + List { "-std=gnu99", } end,
            Opts_CXX  = function(_) return _.Opts_GCC + List { "-std=gnu++14", "-fno-rtti", "-fno-exceptions", } end,
            Opts_NASM = function(_) return _.Opts_GCC_Precompiler + _.Opts_Includes_Nasm + _.selArch.Data.Opts_NASM end,
            Opts_GAS  = function(_) return _.Opts_GCC end,

            Opts_LO = function(_) return _.Opts_GCC + List { "-fuse-linker-plugin", "-Wl,-z,max-page-size=0x1000", } end,

            Opts_STRIP = List { "-s", "-K", "jegudiel_header" },

            Libraries = function(_)
                local res = List {
                    "common." .. _.selArch.Name,
                }

                if settKrnDynAlloc ~= "NONE" then
                    res:Append(dynAllocLibs[settKrnDynAlloc] .. ".kernel")
                end

                return res;
            end,
        },

        Directory = "beelzebub",

        Dependencies = "System Headers",

        Output = function(_) return _.KernelPath end,

        Rule "Link-Optimize Binary" {
            Filter = function(_, dst) return _.BinaryPath end,

            Source = function(_, dst)
                local res = _.Objects
                    + List {
                        _.LinkerScript,
                        _.CommonLibraryPath,
                        dst:GetParent(),
                    }

                if settKrnDynAlloc ~= "NONE" then
                    res:Append(_.KernelDynamicAllocatorPath)
                end

                return res;
            end,

            Action = function(_, dst, src)
                sh.silent(_.LO, _.Opts_LO, "-T", _.LinkerScript, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.KernelPath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
            end,
        },
    },

    ArchitecturalComponent "Test Kernel Module" {
        Data = {
            BinaryPath = function(_) return _.ObjectsDirectory + _.TestKernelModulePath:GetName() end,

            Opts_GCC = function(_)
                local res = List {
                    "-fvisibility=hidden", "-fPIC",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wsystem-headers",
                    "-O2", "-flto",
                    "-D__BEELZEBUB_KERNEL_MODULE",
                } + _.Opts_GCC_Common + _.Opts_Includes
                + _.selArch.Data.Opts_GCC_Kernel

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
                sh.silent(_.LO, _.Opts_LO, "-o", dst, _.Objects, _.Opts_Libraries)
            end,
        },

        Rule "Strip Binary" {
            Filter = function(_, dst) return _.TestKernelModulePath end,

            Source = function(_, dst)
                return List { _.BinaryPath, dst:GetParent() }
            end,

            Action = function(_, dst, src)
                sh.silent(_.STRIP, _.Opts_STRIP, "-o", dst, _.BinaryPath)
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
                sh.silent(_.MKISO, "-R", "-b", _.IsoEltoritoPath:Skip(_.IsoDirectory), "-no-emul-boot", "-boot-load-size", 4, "-boot-info-table", "-o", _.IsoFile, _.IsoDirectory)
            end,
        },

        Rule "Archive Sysroot" {
            Filter = function(_, dst) return _.IsoInitRdPath end,
            Source = function(_, dst) return _.SysrootFiles end,

            Action = function(_, dst, src)
                sh.silent(_.TAR, "czf", dst, _.Opts_TAR, ".")
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

    CreateMissingDirectoriesRule(true),
}

--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --
--  Wrap up
--  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --

Default "Beelzebub" "amd64" "debug"

vmake()
