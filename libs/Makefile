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
.PHONY: install clean common common-install common-clean \
                      runtime runtime-install runtime-clean \
                      $(ARC) $(SETTINGS)

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
#	@ echo "/MAK:" $@
	@ $(MAKE) -C common/ $(ARC) $(SETTINGS) $(MAKE_FLAGS)

runtime: common-install
#	@ echo "/MAK:" $@
	@ $(MAKE) -C runtime/ $(ARC) $(SETTINGS) $(MAKE_FLAGS)

####################################### INSTALLATION ##########

install: common-install runtime-install
	@ true

common-install:
#	@ echo "/MAK:" $@
	@ $(MAKE) -C common/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

runtime-install: common-install
#	@ echo "/MAK:" $@
	@ $(MAKE) -C runtime/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

####################################### CLEAN UP ##########

clean: common-clean runtime-clean
	@ true

common-clean:
#	@ echo "/MAK:" $@
	@ $(MAKE) -C common/ clean $(MAKE_FLAGS)

runtime-clean:
#	@ echo "/MAK:" $@
	@ $(MAKE) -C runtime/ clean $(MAKE_FLAGS)
