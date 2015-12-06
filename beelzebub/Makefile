.SUFFIXES:  

########################
# Default architecture #
all: amd64

###############
# Directories #
PREFIX		:= ./../build
INC_COMMON	:= ./../include
SRC_DIR		:= ./src
ARC_DIR		:= ./arc
AUX_DIR		:= ./arc
SRCARC_DIR	:= ./src/arc
INC_DIR		:= ./inc
INCARC_DIR	:= ./inc/arc
RUNTIME_DIR := $(SRC_DIR)/runtime

include ../Beelzebub.mk

.PHONY: all install uninstall clean linku linko build headers $(ARC) $(SETTINGS)

#############
# Toolchain #
include ../Toolchain.mk
#	That easy!

#############################
# Objects and linker script #
OBJECTS		:=  
HEADERS		:=  
LDFILE		:=  

##########################
#   Toolchain settings   #
##########################

GCCFLAGS	:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_KERNEL
GCCFLAGS	+= -ffreestanding -Wall -mcmodel=kernel 
GCCFLAGS	+= -mno-red-zone -O2 -flto -nostdlib -pipe 
GCCFLAGS	+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

CFLAGS		:= $(GCCFLAGS) -std=gnu99

CXXFLAGS	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions

#DFLAGS		:= -Wall -Werror -mcmodel=kernel -nostdinc
#DFLAGS      += -I$(RUNTIME_DIR) -I$(SRC_DIR)
#DFLAGS      += -fno-assert -fno-emit-moduleinfo -fd-vtls -fproperty
#DLFAGS      += -fno-bounds-check

ASFLAGS		:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_KERNEL

LOFLAGS		:= -ffreestanding -Wall -mcmodel=kernel 
LOFLAGS		+= -mno-red-zone -O2 -flto -nostdlib -pipe 
LOFLAGS		+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

LDFLAGS		:= -z max-page-size=0x1000 -nostdlib -nodefaultlibs

######################################
#	ARCHITECTURE-SPECIFIC SETTINGS   #
######################################

##############
# 64-bit x86 #
ifeq ($(ARC),amd64)
	LDFILE		:= $(ARC_DIR)/amd64/link.ld

	ASFLAGS		+= -f elf64
	CFLAGS		+= -m64
	CXXFLAGS	+= -m64

####################################
# 32-bit x86 with 36-bit addresses #
else ifeq ($(ARC),ia32pae)
	LDFILE		:= $(ARC_DIR)/ia32/link.ld
	#	There will be no specific linker file for PAE.

	ASFLAGS		+= -f elf32
	CFLAGS		+= -m32
	CXXFLAGS	+= -m32

##############
# 32-bit x86 #
else ifeq ($(ARC),ia32)
	LDFILE		:= $(ARC_DIR)/ia32/link.ld

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
KERNEL_PATH		:= $(BUILD_HOST)/$(KERNEL_BIN)

###############################################
# Build everything and link the target binary #
$(ARC):	linko

####################################
#   OBJECTS, SOURCES AND HEADERS   #
####################################

# Standard
# INCFLAGS	:= -I$(INCPCH_DIR) -I$(INC_DIR) -I$(PREFIX)/include
INCFLAGS	:= -I$(INC_COMMON) -I$(INC_DIR) -I$(PREFIX)/include
INCFLAGS	+= -I./../acpica/include

OBJECTS		+= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SRC_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.asm.o,$(shell find $(SRC_DIR) -name "*.asm"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.cpp.o,$(shell find $(SRC_DIR) -name "*.cpp"))

HEADERS		+= $(patsubst $(INC_DIR)/%.hpp,$(INCPCH_DIR)/%.hpp.gch,$(shell find $(INC_DIR) -name "*.hpp"))
HEADERS		+= $(patsubst $(INC_DIR)/%.h,$(INCPCH_DIR)/%.h.gch,$(shell find $(INC_DIR) -name "*.h"))

# When architecture-specific files are present...
ifneq (,$(ARC))
	INCFLAGS	+= -I$(INC_COMMON)/$(ARC) -I$(ARC_DIR)/$(ARC)/inc

	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.c,$(BUILD_DIR)/%.c.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.asm,$(BUILD_DIR)/%.asm.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.asm"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.cpp,$(BUILD_DIR)/%.cpp.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.cpp"))

	HEADERS		+= $(patsubst $(ARC_DIR)/$(ARC)/inc/%.hpp,$(INCPCH_DIR)/%.hpp.arc.gch,$(shell find $(ARC_DIR)/$(ARC)/inc -name "*.hpp"))
	HEADERS		+= $(patsubst $(ARC_DIR)/$(ARC)/inc/%.h,$(INCPCH_DIR)/%.h.arc.gch,$(shell find $(ARC_DIR)/$(ARC)/inc -name "*.h"))
endif

# When auxiliary files are present...
ifneq (,$(AUX))
	INCFLAGS	+= -I$(INC_COMMON)/$(AUX) -I$(AUX_DIR)/$(AUX)/inc

	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.c,$(BUILD_DIR)/%.c.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.asm,$(BUILD_DIR)/%.asm.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.asm"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.cpp,$(BUILD_DIR)/%.cpp.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.cpp"))

	HEADERS		+= $(patsubst $(AUX_DIR)/$(AUX)/inc/%.hpp,$(INCPCH_DIR)/%.hpp.aux.gch,$(shell find $(AUX_DIR)/$(AUX)/inc -name "*.hpp"))
	HEADERS		+= $(patsubst $(AUX_DIR)/$(AUX)/inc/%.h,$(INCPCH_DIR)/%.h.aux.gch,$(shell find $(AUX_DIR)/$(AUX)/inc -name "*.h"))
endif

# Bootstrapping
CFLAGS		+= $(INCFLAGS)
CXXFLAGS	+= $(INCFLAGS)

#build::
#	@ echo $(shell find $(ARC_DIR)/$(ARC) -name "*.c")
#	@ echo $(MAKECMDGOALS) $(AUX) $(ARC) $(OBJECTS)

####################################### BASICS ##########

###############################
# Install to prefix directory #
install: $(KERNEL_INSTALL_PATH)

####################################
# Uninstalls from prefix directory #
uninstall:
	@ rm $(KERNEL_INSTALL_PATH)

##############################
# Cleans the build directory #
clean:
	@ echo "/REM:" $(BUILD_HOST)
	@ rm -Rf $(BUILD_HOST)

##################################################
# Link the target binary with extra optimization #
linko: $(KERNEL_PATH)

##########################
# Link the target binary #
linku: $(OBJECTS)
	@ echo "/LNK/UNO:" $(KERNEL_PATH) "("$(LDFILE)")"
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(KERNEL_PATH) $(OBJECTS)

##########################
# Build all object files #
build: $(OBJECTS)

###############################
# Precompile all header files #
headers: $(HEADERS)

####################################### DEPENDENCIES ##########

#################################
# Obtain depdendency file names #
DEPENDENCIES = $(OBJECTS:%.o=%.d)

####################################### BINARY BLOBS ##########

###########################################
# Install binary blob to prefix directory #
$(KERNEL_INSTALL_PATH): $(KERNEL_PATH)
	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(KERNEL_INSTALL_DIR)
	@ cp $< $@

##################################################
# Link the target binary with extra optimization #
$(KERNEL_PATH): $(LDFILE) $(OBJECTS)
	@ echo "/LNK/OPT:" $@ "("$(LDFILE)")"
	@ $(LO) -T $(LDFILE) -o $@ $(LOFLAGS) $(OBJECTS)

####################################### NORMAL FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@ echo "/STD/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@ echo "/STD/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -MMD -MP -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@ echo "/STD/C++:" $<
	@ mkdir -p $(@D)
	@ $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

##################
# Build a D file #
#$(BUILD_DIR)/%.d.o: $(SRC_DIR)/%.d
#	@ echo "/STD/D__:" $<
#	@ mkdir -p $(@D)
#	@ $(DC) $(DFLAGS) -c $< -o $@

####################################### ARCHITECTURE FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.asm.arc.o: $(ARC_DIR)/$(ARC)/src/%.asm
	@ echo "/ARC/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.arc.o: $(ARC_DIR)/$(ARC)/src/%.c
	@ echo "/ARC/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -MMD -MP -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.arc.o: $(ARC_DIR)/$(ARC)/src/%.cpp
	@ echo "/ARC/C++:" $<
	@ mkdir -p $(@D)
	@ $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

####################################### AUXILIARY FILES ##########

##########################
# Build an assembly file #
$(BUILD_DIR)/%.asm.aux.o: $(AUX_DIR)/$(AUX)/src/%.asm
	@ echo "/AUX/Asm:" $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

##################
# Build a C file #
$(BUILD_DIR)/%.c.aux.o: $(AUX_DIR)/$(AUX)/src/%.c
	@ echo "/AUX/C__:" $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -MMD -MP -c $< -o $@

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.aux.o: $(AUX_DIR)/$(AUX)/src/%.cpp
	@ echo "/AUX/C++:" $<
	@ mkdir -p $(@D)
	@ $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

####################################### DEPENDENCIES ##########

################################
# Include all dependency files #
-include $(DEPENDENCIES)

####################################### NORMAL HEADER FILES ##########

##############################
# Precompile a C header file #
$(INCPCH_DIR)/%.h.gch: $(INC_DIR)/%.h
	@ echo "/HED/C__:" $^
	@ mkdir -p $(@D)
	@# $(CC) $(CFLAGS) -c $< -o $@
	@ touch $@

################################
# Precompile a C++ header file #
$(INCPCH_DIR)/%.hpp.gch: $(INC_DIR)/%.hpp
	@ echo "/HED/C++:" $^
	@ mkdir -p $(@D)
	@# $(CXX) $(CXXFLAGS) -c $< -o $@
	@ touch $@

####################################### ARCHITECTURE HEADER FILES ##########

##############################
# Precompile a C header file #
$(INCPCH_DIR)/%.h.arc.gch: $(ARC_DIR)/$(ARC)/inc/%.h
	@ echo "/HED/C__:" $^
	@ mkdir -p $(@D)
	@# $(CC) $(CFLAGS) -c $< -o $(@:arc.gch=gch)
	@ touch $@

################################
# Precompile a C++ header file #
$(INCPCH_DIR)/%.hpp.arc.gch: $(ARC_DIR)/$(ARC)/inc/%.hpp
	@ echo "/HED/C++:" $^
	@ mkdir -p $(@D)
	@# $(CXX) $(CXXFLAGS) -c $< -o $(@:arc.gch=gch)
	@ touch $@

####################################### AUXILIARY HEADER FILES ##########

##############################
# Precompile a C header file #
$(INCPCH_DIR)/%.h.aux.gch: $(AUX_DIR)/$(AUX)/inc/%.h
	@ echo "/HED/C__:" $^
	@ mkdir -p $(@D)
	@# $(CC) $(CFLAGS) -c $< -o $(@:aux.gch=gch)
	@ touch $@

################################
# Precompile a C++ header file #
$(INCPCH_DIR)/%.hpp.aux.gch: $(AUX_DIR)/$(AUX)/inc/%.hpp
	@ echo "/HED/C++:" $^
	@ mkdir -p $(@D)
	@# $(CXX) $(CXXFLAGS) -c $< -o $(@:aux.gch=gch)
	@ touch $@



#	And it's done...?


