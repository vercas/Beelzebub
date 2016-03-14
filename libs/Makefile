################################################################################
#                                   PROLOGUE                                   #
################################################################################

.SUFFIXES:  

###########
# Default #
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

include ../Beelzebub.mk

.PHONY: install clean common common-install common-clean $(ARC) $(SETTINGS)

include ../Toolchain.mk
#	This one is needed to determine Make flags.

################################################################################
#                                   TARGETS                                    #
################################################################################

$(ARC): install

####################################### BASICS ##########

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
