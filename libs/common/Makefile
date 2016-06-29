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
PROJ_SUBDIR	:= libs/common
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
.PHONY: all install uninstall clean linku build headers $(ARC) $(SETTINGS)

# Output files
BUILD_DIR			:= $(BUILD_HOST)/$(ARC)
LIB_BIN				:= libcommon.$(ARC).a
LIB_PATH			:= $(BUILD_HOST)/$(LIB_BIN)
LIB_INSTALL_DIR		:= $(SYSROOT)/usr/lib
LIB_INSTALL_PATH	:= $(LIB_INSTALL_DIR)/$(LIB_BIN)

################################################################################
#                             TOOLCHAIN & SETTINGS                             #
################################################################################

# Toolchain
include ../../Toolchain.mk

# Common options for GCC
GCCFLAGS		:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_STATIC_LIBRARY 
GCCFLAGS		+= -ffreestanding -Wall -Wsystem-headers 
GCCFLAGS		+= -mno-red-zone -nostdlib -pipe -lgcc -static-libgcc 
GCCFLAGS		+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 
GCCFLAGS		+= --sysroot=$(SYSROOT) 

# C/C++ options
CFLAGS			:= $(GCCFLAGS) -std=gnu99 -flto -O2 
CXXFLAGS_CRT	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions
CXXFLAGS		:= $(CXXFLAGS_CRT) -flto -O2 

# Assembler options
ASFLAGS			:= $(GCC_PRECOMPILER_FLAGS)
GASFLAGS_CRT	:= $(GCCFLAGS) 
GASFLAGS		:= $(GASFLAGS_CRT) -flto 

# Archiver options
ARFLAGS			:= rcs 

################################################################################
#                        CONFIGURATION-SPECIFIC SETTINGS                       #
################################################################################

#########
# Debug #
ifneq (,$(findstring conf-debug,$(SETTINGS)))
	GCCFLAGS		+= -fno-omit-frame-pointer 
	CFLAGS			+= -fno-omit-frame-pointer 
	CXXFLAGS		+= -fno-omit-frame-pointer 
	CXXFLAGS_CRT	+= -fno-omit-frame-pointer 
endif

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

##############
# 64-bit x86 #
ifeq ($(ARC),amd64)
	ASFLAGS			+= -f elf64 
	CFLAGS			+= -m64 -mno-red-zone -mcmodel=large 
	CXXFLAGS		+= -m64 -mno-red-zone -mcmodel=large 
	CXXFLAGS_CRT	+= -m64 -mno-red-zone -mcmodel=large 

####################################
# 32-bit x86 with 36-bit addresses #
else ifeq ($(ARC),ia32pae)
	ASFLAGS			+= -f elf32 
	CFLAGS			+= -m32 
	CXXFLAGS		+= -m32 
	CXXFLAGS_CRT	+= -m32 

##############
# 32-bit x86 #
else ifeq ($(ARC),ia32)
	ASFLAGS			+= -f elf32 
	CFLAGS			+= -m32 
	CXXFLAGS		+= -m32 
	CXXFLAGS_CRT	+= -m32 

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

# CRTs
CRTS		:= 0 i n
CRT_TARGETS	:= $(patsubst %,$(SYSROOT)/usr/lib/crt%.o,$(CRTS)) 

# Bootstrapping
CFLAGS			+= $(INCFLAGS)
CXXFLAGS		+= $(INCFLAGS)
CXXFLAGS_CRT	+= $(INCFLAGS)

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

###############################
# Install to prefix directory #
install: $(LIB_INSTALL_PATH) $(CRT_TARGETS)
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
	@ mkdir -p $(@D)
	@ cp $< $@

##################################################
# Link the target binary with extra optimization #
$(LIB_PATH): $(OBJECTS)
	@ echo "/AR:" $@
	 $(AR) $(ARFLAGS) $@ $(OBJECTS)

####################################### CRTS ##########

#	crt0.o needs to be compiled without LTO, otherwise it won't be linkable.

$(SYSROOT)/usr/lib/crt0.o: $(ARC_DIR)/$(ARC)/crt/crt0.cpp
	@ echo "/CRT/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS_CRT) -MMD -MP -c $< -o $@    $(POST_COMPILATION)

$(SYSROOT)/usr/lib/crti.o: $(ARC_DIR)/$(ARC)/crt/crti.s
	@ echo "/CRT/Gas:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(GAS) $(GASFLAGS_CRT) -c $< -o $@    $(POST_COMPILATION)

$(SYSROOT)/usr/lib/crtn.o: $(ARC_DIR)/$(ARC)/crt/crtn.s
	@ echo "/CRT/Gas:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(GAS) $(GASFLAGS_CRT) -c $< -o $@    $(POST_COMPILATION)

####################################### CODE FILES ##########

include ../../Files.mk
