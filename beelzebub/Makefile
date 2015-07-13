.PHONY: all build linku linko install uninstall clean

########################
# Default architecture #
all: amd64

##########
# Target #
TARGET		:= beelzebub
TARGET_BIN	:= $(TARGET).bin
ARC			:= ''
AUX			:= ''
MTUNE       := corei7-avx

###############
# Directories #
PREFIX		:= ./../build
SRC_DIR		:= ./src
ARC_DIR		:= ./arc
AUX_DIR		:= ./arc
SRCARC_DIR	:= ./src/arc
BUILD_DIR	:= ./build
INC_DIR		:= ./inc
INCARC_DIR	:= ./inc/arc
RUNTIME_DIR := $(SRC_DIR)/runtime

#############
# Toolchain #
CC			:= /usr/local/cross/bin/x86_64-elf-gcc
CPPC		:= /usr/local/cross/bin/x86_64-elf-gcc
DC			:= /usr/local/cross/bin/x86_64-elf-gdc
AS			:= nasm
LO			:= /usr/local/cross/bin/x86_64-elf-gcc
LD			:= /usr/local/cross/bin/x86_64-elf-ld

#############################
# Objects and linker script #
OBJECTS		:=  
LDFILE		:=  

##########################
#   Toolchain settings   #
##########################

CFLAGS		:= -ffreestanding -Wall -Werror -mcmodel=large
CFLAGS      += -D __BEELZEBUB -std=gnu99
CFLAGS      += -mno-red-zone -O2
CFLAGS		+= -flto -nostdlib -lgcc -static-libgcc
CFLAGS		+= -pipe

CPPFLAGS	:= -ffreestanding -Wall -mcmodel=large
CPPFLAGS    += -D __BEELZEBUB -std=gnu++11 -fno-rtti -fno-exceptions
CPPFLAGS    += -mno-red-zone -O2
CPPFLAGS	+= -flto -nostdlib -lgcc -static-libgcc
CPPFLAGS	+= -pipe

DFLAGS		:= -Wall -Werror -mcmodel=large -nostdinc
DFLAGS      += -I$(RUNTIME_DIR) -I$(SRC_DIR)
DFLAGS      += -fno-assert -fno-emit-moduleinfo -fd-vtls -fproperty
DLFAGS      += -fno-bounds-check

ASFLAGS		:=  

LOFLAGS     := -flto -O2 -ffreestanding -Wall -mcmodel=large -mno-red-zone
LOFLAGS     += -nostdlib -lgcc -static-libgcc -Wl,-z,max-page-size=0x1000
LOFLAGS		+= -pipe

LDFLAGS		:= -z max-page-size=0x1000 -nostdlib -nodefaultlibs

######################################
#	ARCHITECTURE-SPECIFIC SETTINGS   #
######################################

##############
# 64-bit x86 #
ifneq (,$(findstring amd64,$(MAKECMDGOALS)))
	AUX			:= x86
	ARC			:= amd64

	LDFILE		+= $(ARC_DIR)/amd64/link.ld

	ASFLAGS		+= -f elf64
	CFLAGS		+= -m64 -D __BEELZEBUB__ARCH_AMD64
	CPPFLAGS	+= -m64 -D __BEELZEBUB__ARCH_AMD64

####################################
# 32-bit x86 with 36-bit addresses #
else ifneq (,$(findstring ia32pae,$(MAKECMDGOALS)))
	AUX			:= x86
	ARC			:= ia32pae

	LDFILE		+= $(ARC_DIR)/ia32/link.ld
	#	There will be no specific linker file for PAE.

	ASFLAGS		+= -f elf32
	CFLAGS		+= -m32 -D __BEELZEBUB__ARCH_IA32 -D __BEELZEBUB__ARCH_IA32PAE
	CPPFLAGS	+= -m32 -D __BEELZEBUB__ARCH_IA32 -D __BEELZEBUB__ARCH_IA32PAE

##############
# 32-bit x86 #
else ifneq (,$(findstring ia32,$(MAKECMDGOALS)))
	AUX			:= x86
	ARC			:= ia32

	LDFILE		+= $(ARC_DIR)/ia32/link.ld

	ASFLAGS		+= -f elf32
	CFLAGS		+= -m32 -D __BEELZEBUB__ARCH_X86 -D __BEELZEBUB__ARCH_IA32
	CPPFLAGS	+= -m32 -D __BEELZEBUB__ARCH_X86 -D __BEELZEBUB__ARCH_IA32

endif

##############
# Common x86 #
ifeq ($(AUX),x86)
	CFLAGS		+= -D __BEELZEBUB__ARCH_X86 -mtune=$(MTUNE)
	CPPFLAGS	+= -D __BEELZEBUB__ARCH_X86 -mtune=$(MTUNE)
endif

###############################################
# Build everything and link the target binary #
ia32 ia32pae amd64:	linko

####################################
#   OBJECTS, SOURCES AND HEADERS   #
####################################

# Standard
INCFLAGS	:= -I$(INC_DIR) -I$(PREFIX)/include
OBJECTS		+= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SRC_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SRC_DIR) -name "*.s"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.cpp.o,$(shell find $(SRC_DIR) -name "*.cpp"))

# When architecture-specific files are present...
ifneq ($(ARC),'')
	INCFLAGS	+= -I$(ARC_DIR)/$(ARC)/inc
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.c,$(BUILD_DIR)/%.c.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.s,$(BUILD_DIR)/%.s.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.s"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.cpp,$(BUILD_DIR)/%.cpp.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.cpp"))

	CFLAGS		+= -D __BEELZEBUB__ARCH=$(ARC)
	CPPFLAGS	+= -D __BEELZEBUB__ARCH=$(ARC)
endif

# When auxiliary files are present...
ifneq ($(AUX),'')
	INCFLAGS	+= -I$(AUX_DIR)/$(AUX)/inc
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.c,$(BUILD_DIR)/%.c.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.s,$(BUILD_DIR)/%.s.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.s"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.cpp,$(BUILD_DIR)/%.cpp.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.cpp"))

	CFLAGS		+= -D __BEELZEBUB__AUX=$(AUX)
	CPPFLAGS	+= -D __BEELZEBUB__AUX=$(AUX)
endif

# Bootstrapping
CFLAGS		+= $(INCFLAGS)
CPPFLAGS	+= $(INCFLAGS)

#build::
#	@ echo $(shell find $(ARC_DIR)/$(ARC) -name "*.c")
#	@ echo $(MAKECMDGOALS) $(AUX) $(ARC) $(OBJECTS)

####################################### BASICS ##########

###############################
# Install to prefix directory #
install: linko
	@ echo " CP" $(TARGET_BIN)
	@ mkdir -p $(PREFIX)/bin
	@ cp $(BUILD_DIR)/$(TARGET_BIN) $(PREFIX)/bin

####################################
# Uninstalls from prefix directory #
uninstall:
	@ rm $(PREFIX)/bin/$(TARGET_BIN)

##################################################
# Link the target binary with extra optimization #
linko: build
	@ echo "/GCC:" $(TARGET_BIN) " (" $(LDFILE) ")"
	@ $(LO) -T $(LDFILE) -o $(BUILD_DIR)/$(TARGET_BIN) $(LOFLAGS) $(OBJECTS)

##########################
# Link the target binary #
linku: build
	@ echo "/LNK:" $(TARGET_BIN) " (" $(LDFILE) ")"
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(BUILD_DIR)/$(TARGET_BIN) $(OBJECTS)

##############################
# Cleans the build directory #
clean:
	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)

##########################
# Build all object files #
build: $(OBJECTS)

####################################### NORMAL FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.s.o: $(SRC_DIR)/%.s
	@ echo "/STD/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@ echo "/STD/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@ echo "/STD/C++:" $<
	@ mkdir -p $(@D)
	@ $(CPPC) $(CPPFLAGS) -c $< -o $@

##################
# Build a D file #
$(BUILD_DIR)/%.d.o: $(SRC_DIR)/%.d
	@ echo "/STD/D__:" $<
	@ mkdir -p $(@D)
	@ $(DC) $(DFLAGS) -c $< -o $@

####################################### ARCHITECTURE FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.s.arc.o: $(ARC_DIR)/$(ARC)/src/%.s
	@ echo "/ARC/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.arc.o: $(ARC_DIR)/$(ARC)/src/%.c
	@ echo "/ARC/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.arc.o: $(ARC_DIR)/$(ARC)/src/%.cpp
	@ echo "/ARC/C++:" $<
	@ mkdir -p $(@D)
	@ $(CPPC) $(CPPFLAGS) -c $< -o $@

####################################### AUXILIARY FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.s.aux.o: $(AUX_DIR)/$(AUX)/src/%.s
	@ echo "/AUX/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.aux.o: $(AUX_DIR)/$(AUX)/src/%.c
	@ echo "/AUX/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.aux.o: $(AUX_DIR)/$(AUX)/src/%.cpp
	@ echo "/AUX/C++:" $<
	@ mkdir -p $(@D)
	@ $(CPPC) $(CPPFLAGS) -c $< -o $@



#	And we're done...?


