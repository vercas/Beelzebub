#!/usr/bin/env lua
require "vmake"

--  Configurations

Configuration "debug"
Configuration "release"
Configuration "profile" { Base = "release" }

--  Architectures

Architecture "amd64" { Base = "x86" }
Architecture "ia32" { Base = "x86" }

Architecture "x86" {
    Auxiliary = true,

    Data = {
        ISO = true,
    },
}

--  Toolchain

local CROSSCOMPILER_DIRECTORY = Path "/usr/local/gcc-x86_64-beelzebub/bin"
--  Default

local CROSSCOMPILERS_DIR = os.getenv("CROSSCOMPILERS_DIR")

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

--  Tests

local testOptions = List { }

CmdOpt "tests" "t" {
    Description = "Specifies which tests to include in the Beelzebub build.",
    Display = "name-1,name 2;name_3,...",

    Type = "string",
    Many = true,
    Mandatory = false,

    Handler = function(_, val)
        for test in string.iteratesplit(val, "[,;]") do
            if #test == 0 or string.find(test, "[^%a%d%s%-_]") then
                error("Test \"" .. test .. "\" contains invalid characters.")
            end

            testOptions:Append("__BEELZEBUB__TEST_" .. string.gsub(test, "[%s%-]", "_"))
        end
    end,
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
    local codeFile, arch = parseObjectExtension(dst)

    if arch then
        return _.data.ArchitecturesDirectory + arch.Name + "src" + Path(codeFile):Skip(_.data.ObjectsDirectory)
    else
        return _.data.CommonDirectory + "src" + Path(codeFile):Skip(_.data.ObjectsDirectory)
    end
end

local function ArchitecturalComponent(name)
    local comp = Component(name)({
        Rule "Compile C" {
            Filter = function(_, dst)
                return checkObjectExtension(dst, "c")
            end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },

        Rule "Compile C++" {
            Filter = function(_, dst)
                return checkObjectExtension(dst, "cpp")
            end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },

        Rule "Assemble w/ NASM" {
            Filter = function(_, dst)
                return checkObjectExtension(dst, "asm")
            end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },

        Rule "Assemble w/ GAS" {
            Filter = function(_, dst)
                return checkObjectExtension(dst, "s")
            end,

            Source = sourceArchitectural,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },
    })

    local data = {
        CommonDirectory = function(_)
            return _.comp.Directory
        end,

        ArchitecturesDirectory = function(_)
            return _.comp.Directory
        end,

        Objects = function(_)
            local comSrcDir = _.CommonDirectory + "src"

            local objects = fs.ListDir(comSrcDir)
                :Where(function(val) return val:EndsWith(".c", ".cpp", ".asm", ".s") end)
                :Select(function(val)
                    return _.data.ObjectsDirectory + val:Skip(comSrcDir) .. ".o"
                end)

            for arch in _.selArch:Hierarchy() do
                local arcSrcDir = _.ArchitecturesDirectory + arch.Name + "src"
                local suffix = "." .. arch.Name .. ".o"

                objects:AppendMany(fs.ListDir(arcSrcDir)
                    :Where(function(val) return val:EndsWith(".c", ".cpp", ".asm", ".s") end)
                    :Select(function(val)
                        return _.data.ObjectsDirectory + val:Skip(arcSrcDir) .. suffix
                    end))
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
        InnerDirectory = function(_)
            return _.outDir + (_.selArch.Name .. "." .. _.selConf.Name)
        end,

        Sysroot = function(_)
            local dir = _.InnerDirectory + "sysroot"

            fs.MkDir(dir, true)

            return dir
        end,

        CommonLibraryPath = function(_)
            return _.Sysroot + ("usr/lib/libcommon." .. _.selArch.Name .. ".a")
        end,

        RuntimeLibraryPath = function(_)
            return _.Sysroot + ("usr/lib/libbeelzebub." .. _.selArch.Name .. ".so")
        end,

        JegudielPath = function(_)
            return _.InnerDirectory + "jegudiel.bin"
        end,

        KernelPath = function(_)
            return _.InnerDirectory + ("beelzebub." .. _.selArch.Name .. ".bin")
        end,

        IsoPath = function(_)
            local dir = _.InnerDirectory + "iso"

            fs.MkDir(dir, true)

            return dir
        end,

        IsoBootPath = function(_)
            local dir = _.IsoPath + "boot"

            fs.MkDir(dir, true)

            return dir
        end,

        IsoGrubPath = function(_)
            local dir = _.IsoBootPath + "grub"

            fs.MkDir(dir, true)

            return dir
        end,

        IsoJegudielPath = function(_)
            return _.IsoBootPath + "jegudiel.bin.gz"
        end,

        IsoInitRdPath = function(_)
            return _.IsoBootPath + "initrd.tar.gz"
        end,

        IsoKernelPath = function(_)
            return _.IsoBootPath + ("beelzebub." .. _.selArch.Name .. ".bin.gz")
        end,

        IsoSources = function(_, dst)
            local res = List {
                _.IsoKernelPath,
                _.IsoInitRdPath,
            }

            if _.selArch.Name == "amd64" then
                res:Append(_.IsoJegudielPath)
            end

            return res
        end,

        Flags_GCC_Precompiler = function(_)
            local res = List { "__BEELZEBUB" }

            for arch in _.selArch:Hierarchy() do
                res:Append("__BEELZEBUB__ARCH_" .. string.upper(arch.Name))
            end

            for conf in _.selConf:Hierarchy() do
                res:Append("__BEELZEBUB__" .. string.upper(conf.Name))
            end

            return res
        end,
    },

    Description = "Lord of Flies",

    Output = function(_)
        if _.selArch.Base.Data.ISO then
            return _.outDir + ("beelzebub." .. _.selArch.Name .. "." .. _.selConf.Name .. ".iso")
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

            SysheadersDirectory = function(_)
                return _.Sysroot + "usr/include"
            end,
        },

        Directory = "sysheaders",

        Output = function(_)
            local files = _.data.Headers:Select(function(val)
                return _.data.SysheadersDirectory + val:SkipDirs(2)
            end)

            return files
        end,

        Rule "Copy Header" {
            Filter = function(_, dst)
                return dst:StartsWith(_.data.SysheadersDirectory)
            end,

            Source = function(_, dst)
                dst = dst:Skip(_.data.SysheadersDirectory)

                for arch in _.selArch:Hierarchy() do
                    local src = _.comp.Directory + arch.Name + dst

                    if fs.GetInfo(src) then return src end
                end

                return _.comp.Directory + "common" + dst
            end,

            Action = function(_, dst, src)
                print("COPYING", src[1], "TO", dst, "FOR", _.rule)
                fs.MkDir(dst:GetParent())
                fs.Copy(dst, src[1])
            end,
        },
    },

    Component "Jegudiel" {
        Data = {
            SourceDirectory = function(_)
                return _.comp.Directory + "src"
            end,

            ObjectsDirectory = function(_)
                local dir = _.InnerDirectory + "jegudiel"

                fs.MkDir(dir, true)

                return dir
            end,

            Objects = function(_)
                return fs.ListDir(_.data.SourceDirectory)
                    :Where(function(val) return val:EndsWith(".c", ".s") end)
                    :Select(function(val)
                        return _.ObjectsDirectory + val:Skip(_.SourceDirectory) .. ".o"
                    end)
            end,

            LinkerScript = function(_)
                return _.comp.Directory + "link.ld"
            end,

            Flags_Includes = function(_)
                return List {
                    "-I" .. tostring(_.comp.Directory + "inc"),
                    "-Isysheaders/" .. _.selArch.Name, --  Harcoded but meh...
                }
            end,

            Flags_C = function(_)
                return List {
                    "-m64", "-ffreestanding",
                    "-Wall", "-Wsystem-headers",
                    "-mcmodel=kernel", "-static-libgcc",
                    "-O0",
                } + _.Flags_Includes
            end,

            Flags_NASM = function(_)
                return List {
                    "-f",
                    "elf64"
                }
            end,

            Flags_LD = function(_)
                return List {
                    "-z",
                    "max-page-size=0x1000"
                }
            end,
        },

        Directory = "jegudiel",

        Dependencies = "System Headers",

        Output = function(_)
            return _.data.InnerDirectory + "jegudiel.bin"
        end,

        Rule "Link Binary" {
            Filter = function(_, dst)
                return dst == _.comp.Output
            end,

            Source = function(_, dst)
                return _.data.Objects + List { _.data.LinkerScript }
            end,

            Action = function(_, dst, src)
                sh(LD, _.data.Flags_LD, "-T", _.data.LinkerScript, "-o", dst, _.data.Objects)
            end,
        },

        Rule "Compile C" {
            Filter = function(_, dst)
                return dst:EndsWith(".c.o")
            end,

            Source = function(_, dst)
                return _.data.SourceDirectory + dst:Skip(_.data.ObjectsDirectory):TrimEnd(2)
                --  2 = #".o"
            end,

            Action = function(_, dst, src)
                sh(CC, _.data.Flags_C, "-c", src, "-o", dst)
            end,
        },

        Rule "Assemble" {
            Filter = function(_, dst)
                return dst:EndsWith(".s.o")
            end,

            Source = function(_, dst)
                return _.data.SourceDirectory + dst:Skip(_.data.ObjectsDirectory):TrimEnd(2)
            end,

            Action = function(_, dst, src)
                sh(AS, _.data.Flags_NASM, src, "-o", dst)
            end,
        },
    },

    ArchitecturalComponent "Common Library" {
        Data = {
            ObjectsDirectory = function(_)
                local dir = _.InnerDirectory + "libcommon"

                fs.MkDir(dir)

                return dir
            end,
        },

        Directory = "libs/common",

        Dependencies = "System Headers",

        Output = function(_)
            return _.data.CommonLibraryPath
        end,

        Rule "Archive Objects" {
            Filter = function(_, dst)
                return dst == _.comp.Output
            end,

            Source = function(_, dst)
                return _.data.Objects
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },
    },

    ArchitecturalComponent "Runtime Library" {
        Data = {
            ObjectsDirectory = function(_)
                local dir = _.InnerDirectory + "libbeelzebub"

                fs.MkDir(dir)

                return dir
            end,
        },

        Directory = "libs/runtime",

        Dependencies = "Common Library",

        Output = function(_)
            return _.data.RuntimeLibraryPath
        end,

        Rule "Link-optimize Binary" {
            Filter = function(_, dst)
                return dst == _.comp.Output
            end,

            Source = function(_, dst)
                return _.data.Objects
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },
    },

    ArchitecturalComponent "Loadtest Application" {
        Data = {
            ObjectsDirectory = function(_)
                local dir = _.InnerDirectory + "loadtest"

                fs.MkDir(dir)

                return dir
            end,
        },

        Directory = "apps/loadtest",

        Dependencies = "Runtime Library",

        Output = function(_)
            return _.data.Sysroot + "apps/loadtest.exe"
        end,

        Rule "Link-optimize Binary" {
            Filter = function(_, dst)
                return dst == _.comp.Output
            end,

            Source = function(_, dst)
                return _.data.Objects
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },
    },

    ArchitecturalComponent "Kernel" {
        Data = {
            ArchitecturesDirectory = function(_)
                return _.comp.Directory + "arc"
            end,

            ObjectsDirectory = function(_)
                local dir = _.InnerDirectory + "kernel"

                fs.MkDir(dir)

                return dir
            end,
        },

        Directory = "beelzebub",

        Dependencies = "Common Library",

        Output = function(_)
            return _.data.KernelPath
        end,

        Rule "Link-optimize Binary" {
            Filter = function(_, dst)
                return dst == _.comp.Output
            end,

            Source = function(_, dst)
                return _.data.Objects
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },
    },

    Component "ISO Image" {
        Data = {
            SysrootFiles = function(_)
                return fs.ListDir(_.Sysroot):Append(_.Sysroot)
            end,
        },

        Directory = "image",

        Dependencies = List {
            "Kernel", "Runtime Library",
            "Jegudiel", "Loadtest Application"
        },

        Output = function(_)
            return _.data.IsoSources:Copy()
        end,

        Rule "Archive Sysroot" {
            Filter = function(_, dst)
                return _.data.IsoInitRdPath == dst
            end,

            Source = function(_, dst)
                return _.data.SysrootFiles
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },

        Rule "GZip Jegudiel" {
            Filter = function(_, dst)
                return _.data.IsoJegudielPath == dst
            end,

            Source = function(_, dst)
                return _.data.JegudielPath
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },

        Rule "GZip Beelzebub" {
            Filter = function(_, dst)
                return _.data.IsoKernelPath == dst
            end,

            Source = function(_, dst)
                return _.data.KernelPath
            end,

            Action = function(_, dst, src)
                sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
                --  TODO
            end,
        },
    },

    Rule "Make ISO" {
        Filter = function(_, dst)
            return _.proj.Output == dst
        end,

        Source = function(_, dst)
            return _.data.IsoSources:Copy()
        end,

        Action = function(_, dst, src)
            sh.silent("echo", "ACTING TOWARDS", dst, "WITH", src, "FOR", _.rule)
            --  TODO
        end,
    },
}

Default "Beelzebub" "amd64" "debug"

vmake()
