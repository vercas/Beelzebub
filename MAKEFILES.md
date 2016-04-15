## Beelzebub's Makefiles

All the options for building Beelzebub are given in its makefiles.
In other words, the whole build process can be controlled imperatively.

### Partial builds, full builds

Beelzebub (all components) fully supports partial builds. This means that, no matter what file you change (assembly, C/C++ source or headers), you can start a normal build and only the files that need to be recompiled will be compiled.
Note, however, that the linkage still needs to happen, and it will not get any faster. The whole process is several times faster, though.  
Jegudiel doesn't keep track of headers.  

A "normal" build is always partial. To issue a full build, add the target `clear` as **the first target**. (e.g. `make clean amd64`)

Full builds are only required when some things change in the makefile or the build targets/settings. The only "change" that does not require rebuild is the architecture because all the intermediary and final files have the target architecture in their path/name. One could even build for all architectures in parallel. (support for this in the makefiles will be added eventually)  

The script `runmake.sh` allows automating the decision between full and partial builds, by running `make clean` before running `make` with the given arguments, if they were not the same as last time the script was used.  

### Architectures

Specifying a target architecture is mandatory, and only one can be specified as target (for now).  

The following architectures are currently supported in the makefiles:
- `amd64` - 64-bit x86 (aka x64 on other OSes)
- `ia32` - 32-bit x86
- `ia32pae` - 32-bit x86 with Physical Address Extension (may be merged with `ia32` later)

### Configurations

There are 3 available configurations so far: `release`, `profile` and `debug`.  
`debug` is the default configuration. This is so that the other configurations, which may let some erroneous situations slip because assertions are not included, require explicit targetting, hopefully making the user more aware of the situation.

The following targets are used to choose the configuration:
- `conf-profile` for `profile`;
- `conf-release` for `release`;
- `conf-debug` or nothing for `debug`.

The `assert`, `msg`, `msg_` and `breakpoint` *macros*/*functions* will only only do their function in the `debug` configuration.  

### Settings

Various settings that affect the output of the build process (and, thus, the behaviour of the OS) are available.  

They are all documented below.

#### `smp` (default) and `no-smp`

Decides whether the built components (bootloader[s], kernel, system-bound libraries, system-bound services and system-bound applications) support symmetric multiprocessing (multi-core, hyperthreaded, possibly multi-CPU) or not.  

When SMP support is enabled, the kernel will initialize APs, the kernel's SMP-related spinlocks will activate, TLB shootdowns will be broadcast to other cores and mutexes will be hybridized.  
AP booting can be stopped through a command-line argument to the kernel.  

When SMP support is disabled, the kernel will not attempt to initialize APs, the kernel's SMP-related spinlocks will be disabled (always appearing free), TLB shootdowns will not require any broadcasts and mutexes will be simpler.  

More differences will be listed here as they surface.  

When you plan on running Beelzebub on a single-core (and single-hyperthreaded) system, a build with SMP support disabled will be much more efficient: cache lines won't be locked unnecessarily, page/TLB changes don't need broadcasting and mutexes will never be busy-waited for.

#### `unit-tests` (default), `unit-tests-quiet` and `no-unit-tests`

Chooses whether unit tests are included in the kernel or not, and whether they'll be quieted or not when they execute.  
Although included, these will not execute without an explicit command-line argument.

### Tests

The codebases includes tests for various libraries, (sub)systems and implementations. These are normally completely ignored when building, unless enabled specifically.  

To enable all available tests, use the `test-all` target.
The rest can be enabled individually:
- `test-mt` for a basic multitasking test;
- `test-str` for the *string.h* implementation;
- `test-obja` for the fixed-size object allocator;
- `test-metap` for the correctness of metaprogramming facilities;
- `test-excp` for kernel exceptions;
- `test-app` for loading a process with an application;
- `test-stackint` for stack integrity (for interrupts and pre-emption);
- `test-avl-tree` for AVL tree implementation;
- `test-terminal` for terminal interface, base and implementations;
- `test-cmdo` for command-line options;
- `test-fpu` for floating point instructions (FPU & SSE2);
- `test-bigint` for big integer operations.
