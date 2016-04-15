# Beelzebub [![Build Status](https://travis-ci.org/vercas/Beelzebub.svg?branch=master)](https://travis-ci.org/vercas/Beelzebub) [![IRC Channel](https://img.shields.io/badge/IRC-beelzebub-blue.svg)](http://webchat.freenode.net/?channels=%23%23beelzebub)

### Building

Currently, Beelzebub requires the following to be built:  

 1. GNU Make
 2. genisoimage (aka mkisofs)
 3. [GCC targetting Beelzebub](https://github.com/vercas/Beelzebub-toolchains)
 4. [GNU Binutils targetting Beelzebub](https://github.com/vercas/Beelzebub-toolchains)

*Make* is required for the whole process.  
*genisoimage* is required for constructing an ISO file. You can skip this if you really want, but should you?  
*GCC* is the only compiler supported right now.  
*GNU Binutils* is required (used indirectly through GCC) for binary linkage, binary stripping, and by accompanying util scripts.  

Use the links above to acquire the cross-compiler required to build Beelzebub. Don't forget to place the tools where they need to be.  
The compiler paths are partially hardcoded into the makefiles, for now. Sorry.  
You may use the `grab_xcs_linux-amd64.sh` file for more specific instructions.  

Besides these, Visual Studio (2013+ I think) solution and project files are supplied as a nice wrapper. These are severely outdated and are likely to just not work.  

Also, your output file of interest is `/build/beelzebub.amd64.x86.iso`.  

#### Makefiles

Every component of Beelzebub has its own makefile, as does the whole project.  
To build Beelzebub, simply run `make amd64`.  

Yes, a target architecture must be specified to every makefile and AMD64 is the only supported architecture right now.  

Everything is documented in [`MAKEFILES.md`](https://github.com/vercas/Beelzebub/blob/master/MAKEFILES.md)

**The makefiles fully support partial builds**. Well, except when changing headers in Jegudiel. But it works perfectly with everything else.  

#### Visual Studio solution and projects

These will be gone soon.  

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

#### Miscellaneous

The Travis bot notifies of build statuses on `##beelzebub` at Freenode.
You should be able to find me there all the time.  

