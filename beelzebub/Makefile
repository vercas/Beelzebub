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
PROJ_SUBDIR	:= beelzebub
PREFIX2		:= ./..
PREFIX		:= ./../build
INC_COMMON	:= ./../include

# Local directories
SRC_DIR		:= ./src
INC_DIR		:= ./inc
ARC_DIR		:= ./arc
AUX_DIR		:= ./arc
RUNTIME_DIR := $(SRC_DIR)/runtime
INCPCH_DIR	:= $(BUILD_DIR)/inc
BUILD_HOST	:= ./build

# Common settings
include ../Beelzebub.mk

# Fake targets.
.PHONY: all install uninstall clean linku build headers $(ARC) $(SETTINGS)

# Output files
BUILD_DIR			:= $(BUILD_HOST)/$(ARC)
KERNEL_PATH			:= $(BUILD_HOST)/$(KERNEL_BIN)

################################################################################
#                             TOOLCHAIN & SETTINGS                             #
################################################################################

# Toolchain
include ../Toolchain.mk

# Common options for GCC
GCCFLAGS	:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_KERNEL
GCCFLAGS	+= -ffreestanding -Wall -mcmodel=kernel -fno-omit-frame-pointer 
GCCFLAGS	+= -mno-red-zone -O2 -flto -nostdlib -pipe -lgcc -static-libgcc 
GCCFLAGS	+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

# C/C++ options
CFLAGS		:= $(GCCFLAGS) -std=gnu99
CXXFLAGS	:= $(GCCFLAGS) -std=gnu++14 -fno-rtti -fno-exceptions

# D options
#DFLAGS		:= -Wall -Werror -mcmodel=kernel -nostdinc
#DFLAGS      += -I$(RUNTIME_DIR) -I$(SRC_DIR)
#DFLAGS      += -fno-assert -fno-emit-moduleinfo -fd-vtls -fproperty
#DLFAGS      += -fno-bounds-check

# Assembler options
ASFLAGS		:= $(GCC_PRECOMPILER_FLAGS) -D __BEELZEBUB_KERNEL
GASFLAGS	:= $(GCCFLAGS) 

# Linker options, with optimization
LOFLAGS		:= -ffreestanding -Wall -mcmodel=kernel -fno-omit-frame-pointer -fuse-linker-plugin 
LOFLAGS		+= -mno-red-zone -O2 -flto=jobserver -nostdlib -pipe -lgcc -static-libgcc -Wl,-z,max-page-size=0x1000 
LOFLAGS		+= -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3 

# Linker options, without optimization
LDFLAGS		:= -z max-page-size=0x1000 -nostdlib -nodefaultlibs 

# Strip options
STRIPFLAGS	:= -s -K "jegudiel_header" 

################################################################################
#                        CONFIGURATION-SPECIFIC SETTINGS                       #
################################################################################

#########
# Debug #
ifneq (,$(findstring conf-debug,$(SETTINGS)))
	GCCFLAGS	+= -fno-omit-frame-pointer 
	LOFLAGS		+= -fno-omit-frame-pointer 
endif

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

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

################################################################################
#                   OBJECTS, SOURCES, HEADERS and LIBRARIES                    #
################################################################################

include ../Sources.mk

# A small extra
INCFLAGS	+= -I./../acpica/include

# Libraries
SLIBRARIES	:= common.$(ARC)
LIBRARIES	:= $(patsubst %,-l%,$(SLIBRARIES)) 
SLIBFILES	:= $(patsubst %,$(SYSROOT)/usr/lib/lib%.a,$(SLIBRARIES)) 

# Bootstrapping
CFLAGS		+= $(INCFLAGS)
CXXFLAGS	+= $(INCFLAGS)
LDFLAGS		+= -L$(SYSROOT)/usr/lib 
LOFLAGS		+= -L$(SYSROOT)/usr/lib 
#	I don't want to set the sysroot here.

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

###############################
# Install to prefix directory #
install: $(KERNEL_INSTALL_PATH)
	@ true

####################################
# Uninstalls from prefix directory #
uninstall:
	@ rm $(KERNEL_INSTALL_PATH)

##############################
# Cleans the build directory #
clean:
	@ echo "/REM:" $(BUILD_HOST)
	@ rm -Rf $(BUILD_HOST)

##########################
# Link the target binary #
linku: $(LDFILE) $(SLIBFILES) $(OBJECTS)
	@ echo "/LNK/UNO:" $(KERNEL_PATH) "("$(LDFILE)")"
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(KERNEL_PATH) $(OBJECTS) $(LIBRARIES) 

##########################
# Build all object files #
build: $(OBJECTS)
	@ true

###############################
# Precompile all header files #
headers: $(HEADERS)
	@ true

####################################### BINARY BLOBS ##########

###########################################
# Install binary blob to prefix directory #
$(KERNEL_INSTALL_PATH): $(KERNEL_PATH)
	@ echo "/STRIP:" $< ">" $@
	@ mkdir -p $(KERNEL_INSTALL_DIR)
#	@ cp $< $@
	@ $(STRIP) $(STRIPFLAGS) -o $@ $<

##################################################
# Link the target binary with extra optimization #
$(KERNEL_PATH): $(LDFILE) $(SLIBFILES) $(OBJECTS)
	@ echo "/LNK/OPT:" $@ "("$(LDFILE)")"
	+@ $(LO) -T $(LDFILE) -o $@ $(LOFLAGS) $(OBJECTS) $(LIBRARIES)

####################################### CODE FILES ##########

include ../Files.mk

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
