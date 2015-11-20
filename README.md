# Beelzebub [![Build Status](https://travis-ci.org/vercas/Beelzebub.svg?branch=master)](https://travis-ci.org/vercas/Beelzebub)

### Building

Currently, Beelzebub requires the following to be built:  

 1. GNU Make
 2. genisoimage (aka mkisofs)
 3. GCC
 4. binutils

*Make* is required for the whole process.  
*genisoimage* is required for constructing an ISO file. You can skip this if you really want, but should you?  
*GCC* is the only compiler supported right now, mainly because of the advanced inline assembly syntax. Also, **AMD64 code and ELF64 are required**.  
*binutils* are required (used indirectly through GCC) for binary linkage.  

Preferably, a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) would be used, but one targetted for AMD64 Linuces should work just as well.  
The compiler paths are partially hardcoded into the makefiles, for now. Sorry.  

Besides these, Visual Studio (2013+ I think) solution and project files are supplied as a nice wrapper. These are outdated, but the build code works.

Also, your output file of interest is `/build/beelzebub.amd64.x86.iso`.

#### Makefiles

Every component of Beelzebub has its own makefile, as does the whole project.  
To build Beelzebub, simply run `make amd64`.  

Yes, a target architecture must be specified to every makefile and AMD64 is the only supported architecture right now.  

Everything is documented in `MAKEFILES.md`

**The makefiles fully support partial builds**. Well, except when changing headers in Jegudiel. But it works perfectly with Beelzebub.  

#### Visual Studio solution and projects

These have been created for convenience, mainly. They fully support partial builds, complete (re)builds and cleanup.  

**However, Cygwin is required** to run the GCC toolchain.  
You require an environment variable named `CYGWIN_BIN` that points to your Cygwin installation's */bin* directory, e.g. `C:\cygwin64\bin`.  
Make sure you restart Visual Studio if you added/changed this variable while it was running, otherwise it won't update.  

The solution, projects and dependency settings are organized in such a way that the projects are built in the right order and you get all the output files where they belong.  
Sadly, it doesn't support debugging or running a VM/emulator, at least yet.  

**Also**, take the `makevs.sh` file and put it in *$(CYGWIN_BIN)* (*/usr/bin*).  
It wraps *make* and translates its output in a way that Visual Studio can understand. (check file for credits)  

#### (Pre-)build scripts

I have included some Bash scripts for grabbing the build tools, mainly for Travis.
For usage, check `.travis.yml`.  

The cross-compiler and `genisoimage` are guarenteed to work on Ubuntu 12.04 and 14.04. It is very likely that it will work on all version in-between, and other distros as well.

##### `grab_xcs_linux-amd64.sh`

This one obtains my pre-built cross-compilers for 64-bit Linux hosts. Feel free to use if you trust me and the security of my webserver.

##### `grab_genisoimage.sh`

This one grabs `genisoimage`. The binary comes from Ubuntu Server 14.04's package.

#### License

Mostly `University of Illinois - NCSA Open Source License`, text and exceptions explained in `LICENSE.md`.
[TLDRLegal summary](https://tldrlegal.com/l/ncsa) for those who seek legal advice on the internet.

#### Miscellaneous

The Travis bot notifies of build statuses on `##beelzebub` at Freenode.
You should be able to find me there all the time.

