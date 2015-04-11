.PHONY: all build link install uninstall clean

# Target
TARGET		:= beelzebub_64
TARGET_BIN	:= $(TARGET).bin

# Directories
PREFIX		:= ./../build
SOURCE_DIR	:= ./src
BUILD_DIR	:= ./build
INC_DIR		:= ./inc

# Object Files
OBJECTS		:= $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SOURCE_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SOURCE_DIR)/%.d,$(BUILD_DIR)/%.d.o,$(shell find $(SOURCE_DIR) -name "*.d"))
OBJECTS		+= $(patsubst $(SOURCE_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SOURCE_DIR) -name "*.s"))

# Toolchain
CC			:= gcc
DC			:= gdc
AS			:= nasm
LD			:= ld

# Toolchain Flags
INCFLAGS	:= -I$(INC_DIR) -I$(PREFIX)/include
CFLAGS		:= -m64 -ffreestanding -Wall -Werror -mcmodel=large $(INCFLAGS)
DFLAGS		:= -m64 -ffreestanding -Wall -Werror -mcmodel=large
ASFLAGS		:= -f elf64
LDFLAGS		:= -z max-page-size=0x1000

# Link File
LDFILE		:= link.ld

# Build everything and link the target binary.
all: build link

# Install to prefix directory.
install: all
	@ echo " CP" $(TARGET_BIN)
	@ mkdir -p $(PREFIX)/bin
	@ cp $(BUILD_DIR)/$(TARGET_BIN) $(PREFIX)/bin
	
# Uninstalls from prefix directory.
uninstall:
	@ rm $(PREFIX)/bin/$(TARGET_BIN)

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

# Build a D file.
$(BUILD_DIR)/%.d.o: $(SOURCE_DIR)/%.d
	@ echo " DC " $<
	@ mkdir -p $(@D)
	@ $(DC) $(DFLAGS) -c $< -o $@

# Link the target binary.
link:
	@ echo " LD " $(TARGET_BIN)
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(BUILD_DIR)/$(TARGET_BIN) $(OBJECTS)

# Cleans the build directory	
clean:
	@ echo " RM " $(BUILD_DIR)
	@ rm -R $(BUILD_DIR)/*
