# Beelzebub [![Build Status](https://travis-ci.org/vercas/Beelzebub.svg?branch=master)](https://travis-ci.org/vercas/Beelzebub)

### Building

Currently, Beelzebub requires the following to be built:  

 1. GNU Make or [vMake](https://github.com/vercas/vMake)
 2. genisoimage (aka mkisofs)
 3. [GCC targetting Beelzebub](https://github.com/vercas/Beelzebub-toolchains)
 4. [GNU Binutils targetting Beelzebub](https://github.com/vercas/Beelzebub-toolchains)
 5. `gzip` and `tar` utilities.

*Make* or *vMake* are required for the whole process.  
*genisoimage* is required for constructing an ISO file. You can skip this if you really want, but should you?  
*GCC* is the only compiler supported right now. This will likely change in the future.  
*GNU Binutils* is required (used indirectly through GCC) for binary linkage, binary stripping, and by accompanying utilitary scripts.  

Use the links above to acquire the cross-compiler required to build Beelzebub. Don't forget to place the tools where they need to be.  
The compiler paths are partially hardcoded into the makefiles, for now. Sorry.  
You may use the `grab_xcs_linux-amd64.sh` file for more specific instructions.  

Also, your output file of interest is `/build/beelzebub.amd64.x86.iso` when using GNU Make, or `.vmake\amd64.debug\beelzebub.amd64.debug.iso` when using vMake.  

#### vMake

I have written my own build system to tackle the complex task of assembling, compiling, linking, archiving, compressing, copying, and all the miscellaneous operations involved in building the OS, as well as configuring the build.  

As of right now, the build system is stable and usable, supports all the features of GNU Make that Beelzebub's makefiles use, and provides the same options which the makefiles emulated through targets.  

It requires Lua 5.1 or 5.2 installed, and can benefit greatly from having LFS (LuaFileSystem) available. In absence of LFS, it will simply use the `find` tool for every filesystem operation unavailable in ANSI C (and, thus, Lua).  
Besides this, it only requires the tools that the makefiles use.  

A simple way to build Beelzebub is to run `./vmakefile.lua`. Providing the `--help` argument will show all the available options.  
An example of a more complex invocation is `./vmakefile.lua -t all --mtune=corei7-avx -j 8`, which includes all tests, tunes the output code for a specific CPU feature set and runs the build in parallel.  

vMake is the preferred tool to build Beelzebub.

#### Makefiles

These are documented in [`MAKEFILES.md`](https://github.com/vercas/Beelzebub/blob/master/MAKEFILES.md). These will be deprecated soon in favor of vMake.

#### Utility scripts

I have included some Bash scripts for grabbing the build tools, mainly for Travis.  
For usage, check `.travis.yml`.  

The cross-compiler and `genisoimage` are guarenteed to work on Ubuntu 12.04 and 14.04. It is very likely that it will work on all versions in-between, and other distros as well.  

##### `grab_xcs_linux-amd64.sh`

This one obtains my pre-built cross-compilers for 64-bit Linux hosts. Feel free to use if you trust me and the security of my webserver.  

##### `grab_genisoimage.sh`

This one grabs `genisoimage`. The binary comes from Ubuntu Server 14.04's package.  

#### License

Mostly `University of Illinois - NCSA Open Source License`, text and exceptions explained in `LICENSE.md`.  
[TLDRLegal summary](https://tldrlegal.com/l/ncsa) for those who seek legal advice on the internet.  
