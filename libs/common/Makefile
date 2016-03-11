.SUFFIXES:  

###############
# Directories #
PREFIX2		:= ./../..
PREFIX		:= ./../../build
INC_COMMON	:= ./../../include
SRC_DIR		:= ./src
INC_DIR		:= ./inc
RUNTIME_DIR := $(SRC_DIR)/runtime

include ../../Beelzebub.mk

.PHONY: all install uninstall clean linku linko build headers $(ARC) $(SETTINGS)

#########
# Files #
LIB_BIN				:= libcommon.$(ARC).a
LIB_INSTALL_DIR		:= $(PREFIX)/libs
LIB_INSTALL_PATH	:= $(LIB_INSTALL_DIR)/$(LIB_BIN)

#############
# Toolchain #
PROJ_SUBDIR	:= libs/common
include ../../Toolchain.mk
#	That easy!

#############################
# Objects and linker script #
OBJECTS		:=  

##########################
#   Toolchain settings   #
##########################

GCCFLAGS	:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_STATIC_LIBRARY 
GCCFLAGS	+= -ffreestanding -Wall 
GCCFLAGS	+= -mno-red-zone -O2 -flto -nostdlib -pipe 
GCCFLAGS	+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

CFLAGS		:= $(GCCFLAGS) -std=gnu99

CXXFLAGS	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions

ASFLAGS		:= $(GCC_PRECOMPILER_FLAGS)

ARFLAGS		:= rcs 

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
LIB_PATH		:= $(BUILD_HOST)/$(LIB_BIN)

###############################################
# Build everything and link the target binary #
$(ARC):	$(LIB_PATH)

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
install: $(LIB_INSTALL_PATH)

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

####################################### DEPENDENCIES ##########

#################################
# Obtain depdendency file names #
DEPENDENCIES = $(OBJECTS:%.o=%.d)

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

####################################### NORMAL FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@ echo "/STD/Asm:" $<
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(AS) $(ASFLAGS) $< -o $@    $(POST_COMPILATION)

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@ echo "/STD/C__:" $<
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@ echo "/STD/C++:" $<
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)

####################################### DEPENDENCIES ##########

################################
# Include all dependency files #
-include $(DEPENDENCIES)



#	And it's done...?


