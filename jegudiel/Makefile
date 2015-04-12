.PHONY: all build link install uninstall clean

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
CC			:= gcc
AS			:= nasm
LD			:= ld

# Toolchain Flags
INCFLAGS	:= -I$(INC_DIR) -I$(PREFIX)/include
CFLAGS		:= -m64 -ffreestanding -Wall -Werror -mcmodel=large $(INCFLAGS)
ASFLAGS		:= -f elf64
LDFLAGS		:= -z max-page-size=0x1000

# Link File
LDFILE		:= link.ld

# Build everything and link the target binary.
all: build link

# Install to prefix directory.
install: all
	@ mkdir -p $(PREFIX)/bin
	@ mkdir -p $(PREFIX)/include
	
	@ echo " CP" $(TARGET_BIN)
	@ cp $(BUILD_DIR)/$(TARGET_BIN) $(PREFIX)/bin
	
	@ echo " CP jegudiel.h"
	@ cp $(INC_DIR)/jegudiel.h $(PREFIX)/include
	
# Uninstalls from prefix directory.
uninstall:
	@ rm $(PREFIX)/bin/$(TARGET_BIN)
	@ rm $(PREFIX)/include/jegudiel.h

# Build all C and assembly files.
build: $(OBJECTS)

# Build an assembly file.
$(BUILD_DIR)/%.s.o: $(SOURCE_DIR)/%.s
	@ echo " AS " $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

# Build a C file.
$(BUILD_DIR)/%.c.o: $(SOURCE_DIR)/%.c
	@ echo " CC " $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@

# Link the target binary.
link:
	@ echo " LD " $(TARGET_BIN)
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(BUILD_DIR)/$(TARGET_BIN) $(OBJECTS)

# Cleans the build directory	
clean:
	@ echo " RM " $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*
