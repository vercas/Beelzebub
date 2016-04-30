################################################################################
#                                   PROLOGUE                                   #
################################################################################

.SUFFIXES:  

# There is no default target.
all:
	@ echo -n "Currently supported target architectures are: " 1>&2
	@ echo "amd64, ia32pae, ia32" 1>&2
	@ echo "Please choose one of them as a target!" 1>&2
	@ return 42 # Yes, the answer to 'all', basically.

# Solution directories
PROJ_SUBDIR	:= libs/runtime
PREFIX2		:= ./../..
PREFIX		:= ./../../build
INC_COMMON	:= ./../../sysheaders

# Local directories
SRC_DIR		:= ./src
INC_DIR		:= ./inc
ARC_DIR		:= .
AUX_DIR		:= .
BUILD_HOST	:= ./build

# Common settings
include ../../Beelzebub.mk

# Fake targets.
.PHONY: all install uninstall clean build $(ARC) $(SETTINGS)

# Output files
BUILD_DIR			:= $(BUILD_HOST)/$(ARC)
LIB_BIN				:= libbeelzebub.$(ARC).so
LIB_PATH			:= $(BUILD_HOST)/$(LIB_BIN)
LIB_INSTALL_DIR		:= $(SYSROOT)/usr/lib
LIB_INSTALL_PATH	:= $(LIB_INSTALL_DIR)/$(LIB_BIN)

################################################################################
#                             TOOLCHAIN & SETTINGS                             #
################################################################################

# Toolchain
include ../../Toolchain.mk

# Common options for GCC
GCCFLAGS	:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_DYNAMIC_LIBRARY 
GCCFLAGS	+= -ffreestanding -Wall -Wsystem-headers 
GCCFLAGS	+= -O3 -flto -nodefaultlibs -pipe -fPIC -static-libgcc 
GCCFLAGS	+= --sysroot=$(SYSROOT) 

# C/C++ options
CFLAGS		:= $(GCCFLAGS) -std=gnu99
CXXFLAGS	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions

# Assembler options
ASFLAGS		:= $(GCC_PRECOMPILER_FLAGS)

# Linker options, with optimization
LOFLAGS		:= -shared -Wl,-z,max-page-size=0x1000 -fuse-linker-plugin 
LOFLAGS		+= -ffreestanding -Wall -Wsystem-headers -Wl,-Bsymbolic 
LOFLAGS		+= -O3 -flto=jobserver -nodefaultlibs -pipe -fPIC -static-libgcc 
LOFLAGS		+= --sysroot=$(SYSROOT) 

# Strip options
STRIPFLAGS	:= -s 

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

##############
# 64-bit x86 #
ifeq ($(ARC),amd64)
	ASFLAGS		+= -f elf64 
	CFLAGS		+= -m64 -mno-red-zone 
	CXXFLAGS	+= -m64 -mno-red-zone 

	LOFLAGS		+= -m64 -mno-red-zone 

####################################
# 32-bit x86 with 36-bit addresses #
else ifeq ($(ARC),ia32pae)
	ASFLAGS		+= -f elf32 
	CFLAGS		+= -m32 
	CXXFLAGS	+= -m32 

	LOFLAGS		+= -m32 

##############
# 32-bit x86 #
else ifeq ($(ARC),ia32)
	ASFLAGS		+= -f elf32 
	CFLAGS		+= -m32 
	CXXFLAGS	+= -m32 

	LOFLAGS		+= -m32 

endif

##############
# Common x86 #
ifeq ($(AUX),x86)
	ifneq (,$(MTUNE))
		CFLAGS		+= -mtune=$(MTUNE)
		CXXFLAGS	+= -mtune=$(MTUNE)
	endif
endif

################################################################################
#                   OBJECTS, SOURCES, HEADERS and LIBRARIES                    #
################################################################################

include ../../Sources.mk

# Libraries
SLIBRARIES	:= common.$(ARC)
LIBRARIES	:= $(patsubst %,-l%,$(SLIBRARIES)) 
SLIBFILES	:= $(patsubst %,$(SYSROOT)/usr/lib/lib%.a,$(SLIBRARIES)) 

# Bootstrapping
CFLAGS		+= $(INCFLAGS) 
CXXFLAGS	+= $(INCFLAGS) 

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

###############################
# Install to prefix directory #
install: $(LIB_INSTALL_PATH)
	@ true

####################################
# Uninstalls from prefix directory #
uninstall:
	@ rm $(LIB_INSTALL_PATH)

##############################
# Cleans the build directory #
clean:
	@ echo "/REM:" $(BUILD_HOST)
	@ rm -Rf $(BUILD_HOST)

##########################
# Build all object files #
build: $(OBJECTS)
	@ true

####################################### BINARY BLOBS ##########

###########################################
# Install binary blob to prefix directory #
$(LIB_INSTALL_PATH): $(LIB_PATH)
	@ echo "/STRIP:" $< ">" $@
	@ mkdir -p $(LIB_INSTALL_DIR)
#	@ cp $< $@
	@ $(STRIP) $(STRIPFLAGS) -o $@ $<

##################################################
# Link the target binary with extra optimization #
$(LIB_PATH): $(SLIBFILES) $(OBJECTS)
	@ echo "/LNK/OPT:" $@
	@ mkdir -p $(@D)
	+@ $(LO) $(LOFLAGS) -o $@ $(OBJECTS) $(LIBRARIES)

####################################### CODE FILES ##########

include ../../Files.mk
