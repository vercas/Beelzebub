.SUFFIXES:  

###############
# Directories #
PREFIX2		:= ./..
PREFIX		:= ./../build

include ../Beelzebub.mk

.PHONY: clean common common-install common-clean $(ARC) $(SETTINGS)

PROJ_SUBDIR	:= libs
include ../Toolchain.mk
#	This one is needed to determine Make flags.

####################################### BASICS ##########

$(ARC): install

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
	
clean: common-clean
