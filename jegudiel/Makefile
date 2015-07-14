.PHONY: all amd64 install uninstall clean link build
.SUFFIXES:  

# Target
TARGET		:= jegudiel
TARGET_BIN	:= $(TARGET).bin

# Directories
PREFIX		:= ./../build
SOURCE_DIR	:= ./src
BUILD_DIR	:= ./build
INC_DIR		:= ./inc

# Object Files
OBJECTS		:= $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SOURCE_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SOURCE_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SOURCE_DIR) -name "*.s"))

# Toolchain
CC			:= /usr/local/cross/bin/x86_64-elf-gcc
AS			:= nasm
LD			:= /usr/local/cross/bin/x86_64-elf-ld

# Toolchain Flags
INCFLAGS	:= -I$(INC_DIR) -I$(PREFIX)/include
CFLAGS		:= -m64 -ffreestanding -Wall -Werror -mcmodel=large $(INCFLAGS)
ASFLAGS		:= -f elf64
LDFLAGS		:= -z max-page-size=0x1000

# Link File
LDFILE		:= link.ld

# Build everything and link the target binary.
all: amd64
amd64: link

####################################### BASICS ##########

###############################
# Install to prefix directory #
install: $(PREFIX)/bin/$(TARGET_BIN) $(PREFIX)/include/jegudiel.h

###################################
# Uninstall from prefix directory #
uninstall:
	@ rm $(PREFIX)/bin/$(TARGET_BIN)
	@ rm $(PREFIX)/include/jegudiel.h

#############################
# Clean the build directory #
clean:
	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*

##########################
# Link the target binary #
link: $(BUILD_DIR)/$(TARGET_BIN)

##########################
# Build all object files #
build: $(OBJECTS)

####################################### BINARY BLOBS ##########

###########################################
# Install binary blob to prefix directory #
$(PREFIX)/bin/$(TARGET_BIN): $(BUILD_DIR)/$(TARGET_BIN)
	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(PREFIX)/bin
	@ cp $< $@

##########################
# Link the target binary #
$(BUILD_DIR)/$(TARGET_BIN): $(LDFILE) $(OBJECTS)
	@ echo "/LNK/UNO:" $@
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(BUILD_DIR)/$(TARGET_BIN) $(OBJECTS)

####################################### OTHER OUTPUT ##########

##########################################
# Install jegudiel.h to prefix directory #
$(PREFIX)/include/jegudiel.h: $(INC_DIR)/jegudiel.h
	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(PREFIX)/include
	@ cp $< $@

####################################### NORMAL FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.s.o: $(SOURCE_DIR)/%.s
	@ echo "/ASM:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SOURCE_DIR)/%.c
	@ echo "/C__" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@
