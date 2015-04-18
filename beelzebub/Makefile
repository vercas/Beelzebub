.PHONY: all build link install uninstall clean

########################
# Default architecture #
all: amd64

##########
# Target #
TARGET		:= beelzebub
TARGET_BIN	:= $(TARGET).bin
ARC			:=  

###############
# Directories #
PREFIX		:= ./../build
SRC_DIR		:= ./src
ARC_DIR		:= ./arc
SRCARC_DIR	:= ./src/arc
BUILD_DIR	:= ./build
INC_DIR		:= ./inc
INCARC_DIR	:= ./inc/arc
RUNTIME_DIR := $(SRC_DIR)/runtime

#############
# Toolchain #
CC			:= gcc
CPPC		:= gcc
DC			:= gdc
AS			:= nasm
LD			:= ld

#############################
# Objects and linker script #
OBJECTS		:=  
LDFILE		:=  

##########################
#   Toolchain settings   #
##########################

INCFLAGS	:= -I$(INC_DIR) -I$(PREFIX)/include

CFLAGS		:= -ffreestanding -Wall -Werror -mcmodel=large
CFLAGS      += -D __BEELZEBUB -std=gnu99
CFLAGS      += -mno-red-zone

CPPFLAGS	:= -ffreestanding -Wall -Werror -mcmodel=large
CPPFLAGS    += -D __BEELZEBUB -std=gnu++11 -fno-rtti -fno-exceptions
CPPFLAGS    += -mno-red-zone

DFLAGS		:= -Wall -Werror -mcmodel=large -nostdinc
DFLAGS      += -I$(RUNTIME_DIR) -I$(SRC_DIR) $(INCFLAGS)
DFLAGS      += -fno-assert -fno-emit-moduleinfo -fd-vtls -fproperty
DLFAGS      += -fno-bounds-check

ASFLAGS		:=  

LDFLAGS		:= -z max-page-size=0x1000 -nostdlib -nodefaultlibs

######################################
#	ARCHITECTURE-SPECIFIC SETTINGS   #
######################################

##############
# 32-bit x86 #
ia32:	ARC			:= ia32
ia32:	CFLAGS		+= -m32 -D __BEELZEBUB__ARCH_X86 -D __BEELZEBUB__ARCH_IA32
ia32:	CPPFLAGS	+= -m32 -D __BEELZEBUB__ARCH_X86 -D __BEELZEBUB__ARCH_IA32
ia32:	ASFLAGS		+= -f elf32
ia32:	LDFILE		+= $(ARC_DIR)/ia32/link.ld

##############
# 64-bit x86 #
amd64:	ARC			:= amd64
amd64:	CFLAGS		+= -m64 -D __BEELZEBUB__ARCH_X86 -D __BEELZEBUB__ARCH_AMD64
amd64:	CPPFLAGS	+= -m64 -D __BEELZEBUB__ARCH_X86 -D __BEELZEBUB__ARCH_AMD64
amd64:	ASFLAGS		+= -f elf64
amd64:	LDFILE		+= $(ARC_DIR)/amd64/link.ld

##################################
#	ARCHITECTURE-BASED ACTIONS   #
##################################

##########################################
# Remove old architecture-specific stuff #
rmarcdir::
	@ echo "/REM:" $(SRCARC_DIR)
	@ rm -Rf $(SRCARC_DIR)
	@ echo "/REM:" $(INCARC_DIR)
	@ rm -Rf $(INCARC_DIR)

###########################################
# Include new architecture-specific stuff #
mkarcdir:: rmarcdir
	@ echo "/CPY:" $(ARC_DIR)/$(ARC)/src ">" $(SRCARC_DIR)
	@ cp -R $(ARC_DIR)/$(ARC)/src $(SRCARC_DIR)
	@ echo "/CPY:" $(ARC_DIR)/$(ARC)/inc ">" $(INCARC_DIR)
	@ cp -R $(ARC_DIR)/$(ARC)/inc $(INCARC_DIR)

#############################################
# Both of these will share some common code #
ia32 amd64:: mkarcdir
	@ echo "/CPY:" $(ARC_DIR)/x86/src/* ">" $(SRCARC_DIR)/
	@ cp -R $(ARC_DIR)/x86/src/* $(SRCARC_DIR)/
	@ echo "/CPY:" $(ARC_DIR)/x86/inc/* ">" $(INCARC_DIR)/
	@ cp -R $(ARC_DIR)/x86/inc/* $(INCARC_DIR)/

###############################################
# Build everything and link the target binary #
ia32 amd64::	build link

CFLAGS		+= $(INCFLAGS) -D __BEELZEBUB__ARCH=$(ARC)
CPPFLAGS	+= $(INCFLAGS) -D __BEELZEBUB__ARCH=$(ARC)

################
# Object Files #
OBJECTS		+= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SRC_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SRC_DIR) -name "*.s"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.cpp.o,$(shell find $(SRC_DIR) -name "*.cpp"))
#OBJECTS		+= $(patsubst $(SRC_DIR)/%.d,$(BUILD_DIR)/%.d.o,$(shell find $(SRC_DIR) -name "*.d"))

###############################
# Install to prefix directory #
install: all
	@ echo " CP" $(TARGET_BIN)
	@ mkdir -p $(PREFIX)/bin
	@ cp $(BUILD_DIR)/$(TARGET_BIN) $(PREFIX)/bin

####################################
# Uninstalls from prefix directory #
uninstall:
	@ rm $(PREFIX)/bin/$(TARGET_BIN)

##########################
# Build all object files #
build:: mkarcdir $(OBJECTS)

##########################
# Build an assembly file #
$(BUILD_DIR)/%.s.o: $(SRC_DIR)/%.s
	@ echo "/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@ echo "/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@ echo "/C++:" $<
	@ mkdir -p $(@D)
	@ $(CPPC) $(CPPFLAGS) -c $< -o $@

##################
# Build a D file #
$(BUILD_DIR)/%.d.o: $(SRC_DIR)/%.d
	@ echo "/D__:" $<
	@ mkdir -p $(@D)
	@ $(DC) $(DFLAGS) -c $< -o $@

##########################
# Link the target binary #
link:
	@ echo "/LNK:" $(TARGET_BIN) " (" $(LDFILE) ")"
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(BUILD_DIR)/$(TARGET_BIN) $(OBJECTS)

##############################
# Cleans the build directory #
clean:
	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*



#	And we're done...?


