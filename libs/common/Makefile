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
PROJ_SUBDIR	:= apps/loadtest
PREFIX2		:= ./../..
PREFIX		:= ./../../build
INC_COMMON	:= ./../../include

# Local directories
SRC_DIR		:= ./src
INC_DIR		:= ./inc
ARC_DIR		:= .
AUX_DIR		:= .
BUILD_HOST	:= ./build
BUILD_DIR	:= $(BUILD_HOST)/$(ARC)

# Common settings
include ../../Beelzebub.mk

# Fake targets.
.PHONY: all install uninstall clean linku build headers $(ARC) $(SETTINGS)

# Output files
LIB_BIN				:= libcommon.$(ARC).a
LIB_PATH			:= $(BUILD_HOST)/$(LIB_BIN)
LIB_INSTALL_DIR		:= $(PREFIX)/libs
LIB_INSTALL_PATH	:= $(LIB_INSTALL_DIR)/$(LIB_BIN)

################################################################################
#                             TOOLCHAIN & SETTINGS                             #
################################################################################

# Toolchain
include ../../Toolchain.mk

# Common options for GCC
GCCFLAGS	:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_STATIC_LIBRARY 
GCCFLAGS	+= -ffreestanding -Wall -mcmodel=large -fno-omit-frame-pointer 
GCCFLAGS	+= -mno-red-zone -O2 -flto -nostdlib -pipe 
GCCFLAGS	+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

# C/C++ options
CFLAGS		:= $(GCCFLAGS) -std=gnu99
CXXFLAGS	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions

# Assembler options
ASFLAGS		:= $(GCC_PRECOMPILER_FLAGS)

# Archiver options
ARFLAGS		:= rcs 

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

##############
# 64-bit x86 #
ifeq ($(ARC),amd64)
	ASFLAGS		+= -f elf64
	CFLAGS		+= -m64
	CXXFLAGS	+= -m64

####################################
# 32-bit x86 with 36-bit addresses #
else ifeq ($(ARC),ia32pae)
	ASFLAGS		+= -f elf32
	CFLAGS		+= -m32
	CXXFLAGS	+= -m32

##############
# 32-bit x86 #
else ifeq ($(ARC),ia32)
	ASFLAGS		+= -f elf32
	CFLAGS		+= -m32
	CXXFLAGS	+= -m32

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
#                             OBJECTS AND SOURCES                              #
################################################################################

include ../../Sources.mk

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
	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(LIB_INSTALL_DIR)
	@ cp $< $@

##################################################
# Link the target binary with extra optimization #
$(LIB_PATH): $(OBJECTS)
	@ echo "/AR/OPT:" $@
	@ $(AR) $(ARFLAGS) $@ $(OBJECTS)

####################################### CODE FILES ##########

include ../../Files.mk
