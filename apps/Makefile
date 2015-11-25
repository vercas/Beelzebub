.SUFFIXES:  

include ../Beelzebub.mk

.PHONY: clean loadtest loadtest-install loadtest-clean $(ARC) $(SETTINGS)

include ../Toolchain.mk
#	This one is needed to determine Make flags.

####################################### BASICS ##########

$(ARC): install

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
	
clean: loadtest-clean
