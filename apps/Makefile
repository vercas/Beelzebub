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
PROJ_SUBDIR	:= apps

# Local directories
PREFIX2		:= ./..
PREFIX		:= ./../build

include ../Beelzebub.mk

.PHONY: install clean loadtest loadtest-install loadtest-clean $(ARC) $(SETTINGS)

include ../Toolchain.mk
#	This one is needed to determine Make flags.

################################################################################
#                                   TARGETS                                    #
################################################################################

$(ARC): install

####################################### BASICS ##########

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
