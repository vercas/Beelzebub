CROSSCOMPILER_DIRECTORY		:= /usr/local/gcc-x86_64-elf/bin
#	Default

MAKE_FLAGS	:= -j

ifdef CROSSCOMPILERS_DIR
	CROSSCOMPILER_DIRECTORY	:= $(CROSSCOMPILER_DIRECTORY)

	MAKE_FLAGS	:=  
	#	The environment variables will fail to propagate...
endif

#############
# Toolchain #
CC			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-elf-gcc
CXX			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-elf-gcc
DC			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-elf-gdc
AS			:= nasm
LO			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-elf-gcc
LD			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-elf-ld

#################
# Target Tuning #
MTUNE		:= corei7-avx
#	This is only used when targetting x86.

#	You should modify this file according to your needs and add it to your
#	.gitignore.

# Some X-plainations:
#	CXXC = C++ Compiler (not sure if this is standard or not, lel)
#	DC   = D Compiler
#	LO   = Linker-optimizer
# Should be 'nuff.
