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
PROJ_SUBDIR	:= apps

# Local directories
PREFIX2		:= ./..
PREFIX		:= ./../build

# Common settings
include ../Beelzebub.mk

# Fake targets.
.PHONY: install clean loadtest loadtest-install loadtest-clean $(ARC) $(SETTINGS)

# Toolchain
include ../Toolchain.mk
#	This one is needed to determine Make flags.

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

loadtest:
	@ echo "/MAK:" $@
	@ $(MAKE) -C loadtest/ $(ARC) $(SETTINGS) $(MAKE_FLAGS)

loadtest-install:
	@ echo "/MAK:" $@
	@ $(MAKE) -C loadtest/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

loadtest-clean:
	@ echo "/MAK:" $@
	@ $(MAKE) -C loadtest/ clean $(MAKE_FLAGS)

install: loadtest-install
	@ true
	
clean: loadtest-clean
	@ true
