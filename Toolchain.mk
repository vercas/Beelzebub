CROSSCOMPILER_DIRECTORY		:= /usr/local/gcc-x86_64-beelzebub/bin
#	Default

MAKE_FLAGS	:= -j 

ifdef CROSSCOMPILERS_DIR
	CROSSCOMPILER_DIRECTORY	:= $(CROSSCOMPILERS_DIR)/gcc-x86_64-elf/bin

	#MAKE_FLAGS	:=  
	#	The environment variables will fail to propagate...
endif

#############
# Toolchain #
CC			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-beelzebub-gcc 
CXX			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-beelzebub-gcc 
DC			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-beelzebub-gdc
AS			:= nasm
LO			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-beelzebub-gcc 
LD			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-beelzebub-ld
AR			:= $(CROSSCOMPILER_DIRECTORY)/x86_64-beelzebub-gcc-ar 
MKISO		:= mkisofs

PRE_COMPILATION		:=  
POST_COMPILATION	:=  

ifneq ($(shell $(MKISO) --version; echo $$?),0)
	#	So, mkisofs may not be absent.
	#	Maybe it comes from an external source?
	ifdef MISC_TOOLS_DIR
		MKISO	:= $(MISC_TOOLS_DIR)/genisoimage
	endif
endif


#################
# Target Tuning #
MTUNE		:= corei7-avx
#	This is only used when targetting x86.


#####################
# Testing w/ VMWare #
ifndef VMX_PATH
	VMX_PATH	:= "H:\Virtual Machines\Beelzebub\Beelzebub.vmx"
endif


#######################
# Testing for Sublime #
ifdef IN_SUBLIME
	PRE_COMPILATION		:= $(PREFIX2)/run-compiler-add-dir.sh $(PROJ_SUBDIR) 
	#POST_COMPILATION	:= 2>&1 | sed -r 's,( *)(.+:[0-9]+:[0-9]+:),\1$(PROJ_SUBDIR)/\2,'
endif


#	You should modify this file according to your needs and add it to your
#	.gitignore.

# Some X-plainations:
#	CXX  = C++ Compiler (not sure if this is standard or not, lel)
#	DC   = D Compiler
#	LO   = Linker-optimizer
# Should be 'nuff.

#ifeq ($(shell clang --version; echo $$?),0)
#	CC			:= clang -target x86_64-elf 
#	CXX			:= clang -target x86_64-elf 
#	LO			:= clang -target x86_64-elf -v 
#endif
