.SUFFIXES:  

###########
# Default #
all:
	@ echo -n "Currently supported target architectures are: " 1>&2
	@ echo "amd64, ia32pae, ia32" 1>&2
	@ echo "Please choose one of them as a target!" 1>&2
	@ return 42 # Yes, the answer to 'all', basically.

###############
# Directories #
PREFIX2		:= ./../..
PREFIX		:= ./../../build
INC_COMMON	:= ./../../include
SRC_DIR		:= ./src
INC_DIR		:= ./inc
ARC_DIR		:= .
AUX_DIR		:= .
RUNTIME_DIR := $(SRC_DIR)/runtime

include ../../Beelzebub.mk

.PHONY: all install uninstall clean linku linko build headers $(ARC) $(SETTINGS)

#########
# Files #
APP_BIN				:= loadtest.exe
APP_INSTALL_DIR		:= $(PREFIX)/bin
APP_INSTALL_PATH	:= $(APP_INSTALL_DIR)/$(APP_BIN)

#############
# Toolchain #
PROJ_SUBDIR	:= apps/loadtest
include ../../Toolchain.mk
#	That easy!

#############################
# Objects and linker script #
OBJECTS		:=  
LDFILE		:= ./link.$(ARC).ld

##########################
#   Toolchain settings   #
##########################

GCCFLAGS	:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_APPLICATION 
GCCFLAGS	+= -ffreestanding -Wall -mcmodel=small 
GCCFLAGS	+= -mno-red-zone -O2 -flto -nostdlib -pipe 
# GCCFLAGS	+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

CFLAGS		:= $(GCCFLAGS) -std=gnu99

CXXFLAGS	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions

#DFLAGS		:= -Wall -Werror -mcmodel=small -nostdinc
#DFLAGS      += -I$(RUNTIME_DIR) -I$(SRC_DIR)
#DFLAGS      += -fno-assert -fno-emit-moduleinfo -fd-vtls -fproperty
#DLFAGS      += -fno-bounds-check

ASFLAGS		:= $(GCC_PRECOMPILER_FLAGS)

LOFLAGS		:= -ffreestanding -Wall -mcmodel=small 
LOFLAGS		+= -mno-red-zone -O2 -flto -nostdlib -pipe 
# LOFLAGS		+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

LDFLAGS		:= -z max-page-size=0x1000 -nostdlib -nodefaultlibs

######################################
#	ARCHITECTURE-SPECIFIC SETTINGS   #
######################################

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

# Output directories
BUILD_HOST		:= ./build
BUILD_DIR		:= $(BUILD_HOST)/$(ARC)
INCPCH_DIR		:= $(BUILD_DIR)/inc

# Binary blob
APP_PATH		:= $(BUILD_HOST)/$(APP_BIN)

###############################################
# Build everything and link the target binary #
$(ARC):	linko

###########################
#   OBJECTS AND SOURCES   #
###########################

# Standard
# INCFLAGS	:= -I$(INCPCH_DIR) -I$(INC_DIR) -I$(PREFIX)/include
INCFLAGS	:= -I$(INC_COMMON) -I$(INC_DIR) -I$(PREFIX)/include

OBJECTS		+= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SRC_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.asm.o,$(shell find $(SRC_DIR) -name "*.asm"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.cpp.o,$(shell find $(SRC_DIR) -name "*.cpp"))

# When architecture-specific files are present...
ifneq (,$(ARC))
	INCFLAGS	+= -I$(INC_COMMON)/$(ARC) -I$(ARC_DIR)/$(ARC)/inc

	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.c,$(BUILD_DIR)/%.c.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.asm,$(BUILD_DIR)/%.asm.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.asm"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.cpp,$(BUILD_DIR)/%.cpp.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.cpp"))
endif

# When auxiliary files are present...
ifneq (,$(AUX))
	INCFLAGS	+= -I$(INC_COMMON)/$(AUX) -I$(AUX_DIR)/$(AUX)/inc

	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.c,$(BUILD_DIR)/%.c.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.asm,$(BUILD_DIR)/%.asm.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.asm"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.cpp,$(BUILD_DIR)/%.cpp.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.cpp"))
endif

# Bootstrapping
CFLAGS		+= $(INCFLAGS)
CXXFLAGS	+= $(INCFLAGS)

####################################### BASICS ##########

###############################
# Install to prefix directory #
install: $(APP_INSTALL_PATH)

####################################
# Uninstalls from prefix directory #
uninstall:
	@ rm $(APP_INSTALL_PATH)

##############################
# Cleans the build directory #
clean:
	@ echo "/REM:" $(BUILD_HOST)
	@ rm -Rf $(BUILD_HOST)

##################################################
# Link the target binary with extra optimization #
linko: $(APP_PATH)

##########################
# Link the target binary #
linku: $(OBJECTS)
	@ echo "/LNK/UNO:" $(APP_PATH) "("$(LDFILE)")"
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(APP_PATH) $(OBJECTS)

##########################
# Build all object files #
build: $(OBJECTS)

####################################### BINARY BLOBS ##########

###########################################
# Install binary blob to prefix directory #
$(APP_INSTALL_PATH): $(APP_PATH)
	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(APP_INSTALL_DIR)
	@ cp $< $@

##################################################
# Link the target binary with extra optimization #
$(APP_PATH): $(LDFILE) $(OBJECTS)
	@ echo "/LNK/OPT:" $@ "("$(LDFILE)")"
	@ $(LO) -T $(LDFILE) -o $@ $(LOFLAGS) $(OBJECTS)

####################################### CODE FILES ##########

include ../../Files.mk
