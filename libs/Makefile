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
PROJ_SUBDIR	:= libs

# Local directories
PREFIX2		:= ./..
PREFIX		:= ./../build

# Common settings
include ../Beelzebub.mk

# Fake targets.
.PHONY: install clean common common-install common-clean $(ARC) $(SETTINGS)

# Toolchain
include ../Toolchain.mk
#	This one is needed to determine Make flags.

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

common:
	@ echo "/MAK:" $@
	@ $(MAKE) -C common/ $(ARC) $(SETTINGS) $(MAKE_FLAGS)

common-install:
	@ echo "/MAK:" $@
	@ $(MAKE) -C common/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

common-clean:
	@ echo "/MAK:" $@
	@ $(MAKE) -C common/ clean $(MAKE_FLAGS)

install: common-install
	@ true
	
clean: common-clean
	@ true
