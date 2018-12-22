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
PROJ_SUBDIR	:= jegudiel
PREFIX2		:= ./..
PREFIX		:= ./../build
INC_COMMON	:= ./../sysheaders

# Local directories
SOURCE_DIR	:= ./src
INC_DIR		:= ./inc
BUILD_HOST	:= ./build

# Common settings
include ../Beelzebub.mk

# Fake targets.
.PHONY: all install uninstall clean link build amd64 $(SETTINGS)
#	I write 'amd64' and not '$(ARC)' so other architectures will error out.

# Output files
BUILD_DIR			:= $(BUILD_HOST)
BTL_BIN				:= jegudiel.bin
BTL_PATH			:= $(BUILD_HOST)/$(BTL_BIN)
BTL_INSTALL_DIR		:= $(PREFIX)/bin
BTL_INSTALL_PATH	:= $(BTL_INSTALL_DIR)/$(BTL_BIN)

################################################################################
#                             TOOLCHAIN & SETTINGS                             #
################################################################################

# Toolchain
include ../Toolchain.mk

# Toolchain options/flags
INCFLAGS	:= -I$(INC_DIR) -I$(INC_COMMON)/$(ARC) 
CFLAGS		:= -m64 -ffreestanding -Wall -Wsystem-headers -mcmodel=kernel $(GCC_PRECOMPILER_FLAGS) $(INCFLAGS) -O0 -static-libgcc 
ASFLAGS		:= -f elf64
LDFLAGS		:= -z max-page-size=0x1000

# Link File
LDFILE		:= link.ld 

# Strip options
STRIPFLAGS	:= -s -K "multiboot_header" 

################################################################################
#                             OBJECTS AND SOURCES                              #
################################################################################

# Object files
OBJECTS		:= $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SOURCE_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SOURCE_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SOURCE_DIR) -name "*.s"))

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

###############################
# Install to prefix directory #
install: $(BTL_INSTALL_PATH)
	@ true

###################################
# Uninstall from prefix directory #
uninstall:
	@ rm $(BTL_INSTALL_PATH)

#############################
# Clean the build directory #
clean:
#	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*

##########################
# Link the target binary #
link: $(BTL_PATH)
	@ true

##########################
# Build all object files #
build: $(OBJECTS)
	@ true

####################################### BINARY BLOBS ##########

###########################################
# Install binary blob to prefix directory #
$(BTL_INSTALL_PATH): $(BTL_PATH)
#	@ echo "/STRIP:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(STRIP) $(STRIPFLAGS) -o $@ $<

##########################
# Link the target binary #
$(BTL_PATH): $(LDFILE) $(OBJECTS)
#	@ echo "/LNK/UNO:" $@
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $@ $(OBJECTS)

####################################### NORMAL FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.s.o: $(SOURCE_DIR)/%.s
#	@ echo "/ASM:" $<
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(AS) $(ASFLAGS) $< -o $@    $(POST_COMPILATION)

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SOURCE_DIR)/%.c
#	@ echo "/C__" $<
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -c $< -o $@    $(POST_COMPILATION)
