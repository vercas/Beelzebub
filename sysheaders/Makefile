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
PROJ_SUBDIR	:= sysheaders
PREFIX2		:= ./..
PREFIX		:= ./../build
INC_COMMON	:= ./../sysheaders

# Local directories
INC_DIR		:= ./common
ARC_DIR		:= .
AUX_DIR		:= .
BUILD_HOST	:= ./build

# Common settings
include ../Beelzebub.mk

# Fake targets.
.PHONY: all install uninstall clean $(ARC) $(SETTINGS)

# Output files
HDR_INSTALL_DIR		:= $(SYSROOT)/usr/include

################################################################################
#                                    HEADERS                                   #
################################################################################

HEADERS		:= $(patsubst $(INC_DIR)/%.hpp,$(HDR_INSTALL_DIR)/%.hpp,$(shell find $(INC_DIR) -name "*.hpp"))
HEADERS		+= $(patsubst $(INC_DIR)/%.h,$(HDR_INSTALL_DIR)/%.h,$(shell find $(INC_DIR) -name "*.h"))
HEADERS		+= $(patsubst $(INC_DIR)/%.inc,$(HDR_INSTALL_DIR)/%.inc,$(shell find $(INC_DIR) -name "*.inc"))

# When architecture-specific files are present...
ifneq (,$(ARC))
	HEADERS		+= $(patsubst $(ARC_DIR)/$(ARC)/%.hpp,$(HDR_INSTALL_DIR)/%.hpp,$(shell find $(ARC_DIR)/$(ARC) -name "*.hpp"))
	HEADERS		+= $(patsubst $(ARC_DIR)/$(ARC)/%.h,$(HDR_INSTALL_DIR)/%.h,$(shell find $(ARC_DIR)/$(ARC) -name "*.h"))
	HEADERS		+= $(patsubst $(ARC_DIR)/$(ARC)/%.inc,$(HDR_INSTALL_DIR)/%.inc,$(shell find $(ARC_DIR)/$(ARC) -name "*.inc"))
endif

# When auxiliary files are present...
ifneq (,$(AUX))
	HEADERS		+= $(patsubst $(AUX_DIR)/$(AUX)/%.hpp,$(HDR_INSTALL_DIR)/%.hpp,$(shell find $(AUX_DIR)/$(AUX) -name "*.hpp"))
	HEADERS		+= $(patsubst $(AUX_DIR)/$(AUX)/%.h,$(HDR_INSTALL_DIR)/%.h,$(shell find $(AUX_DIR)/$(AUX) -name "*.h"))
	HEADERS		+= $(patsubst $(AUX_DIR)/$(AUX)/%.inc,$(HDR_INSTALL_DIR)/%.inc,$(shell find $(AUX_DIR)/$(AUX) -name "*.inc"))
endif

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

###############################
# Install to prefix directory #
install: $(HEADERS)
	@ true

####################################
# Uninstalls from prefix directory #
uninstall clean:
	@ rm -Rf $(HDR_INSTALL_DIR)

####################################### ARCHITECTURE HEADER FILES ##########

###############################################
# Copy an architecture-specific C header file #
$(HDR_INSTALL_DIR)/%.h: $(ARC_DIR)/$(ARC)/%.h
	@ echo "/CPY/C__:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

#################################################
# Copy an architecture-specific C++ header file #
$(HDR_INSTALL_DIR)/%.hpp: $(ARC_DIR)/$(ARC)/%.hpp
	@ echo "/CPY/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

###########################################
# Copy an architecture-specific .inc file #
$(HDR_INSTALL_DIR)/%.inc: $(ARC_DIR)/$(ARC)/%.inc
	@ echo "/CPY/INC:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

####################################### AUXILIARY HEADER FILES ##########

################################################
# Copy an architecture-auxiliary C header file #
$(HDR_INSTALL_DIR)/%.h: $(AUX_DIR)/$(AUX)/%.h
	@ echo "/CPY/C__:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

##################################################
# Copy an architecture-auxiliary C++ header file #
$(HDR_INSTALL_DIR)/%.hpp: $(AUX_DIR)/$(AUX)/%.hpp
	@ echo "/CPY/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

############################################
# Copy an architecture-auxiliary .inc file #
$(HDR_INSTALL_DIR)/%.inc: $(AUX_DIR)/$(AUX)/%.inc
	@ echo "/CPY/INC:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

####################################### NORMAL HEADER FILES ##########

#################################
# Copy a standard C header file #
$(HDR_INSTALL_DIR)/%.h: $(INC_DIR)/%.h
	@ echo "/CPY/C__:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

###################################
# Copy a standard C++ header file #
$(HDR_INSTALL_DIR)/%.hpp: $(INC_DIR)/%.hpp
	@ echo "/CPY/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

#############################
# Copy a standard .inc file #
$(HDR_INSTALL_DIR)/%.inc: $(INC_DIR)/%.inc
	@ echo "/CPY/INC:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@
