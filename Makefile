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
PREFIX		:= ./build

# Common settings
include ./Beelzebub.mk

# Fake targets.
.PHONY: run qemu qemu-serial clean jegudiel image kernel apps libs sysheaders $(ARC) $(SETTINGS)

# Output file
KERNEL_DIR	:= ./$(KERNEL_NAME)

################################################################################
#                             TOOLCHAIN & SETTINGS                             #
################################################################################

# Toolchain
include ./Toolchain.mk
#	This one is needed to determine Make flags.

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

##############
# 64-bit x86 #
ifeq ($(ARC),amd64)
image:: jegudiel
endif

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC): image
	@ true

####################################### CLEANING ##########

clean:: clean-libs clean-jegudiel \
        clean-kernel clean-apps clean-image
	@ rm -Rf $(PREFIX)
	@ rm -f last_settings.txt

#clean-sysheaders:
#	@ $(MAKE) -C sysheaders/ clean

clean-libs:
	@ $(MAKE) -C libs/ clean

clean-jegudiel:
	@ $(MAKE) -C jegudiel/ clean

clean-kernel:
	@ $(MAKE) -C $(KERNEL_DIR)/ clean

clean-apps:
	@ $(MAKE) -C apps/ clean

clean-image:
	@ $(MAKE) -C image/ clean

####################################### TESTING & RUNNING ##########

run: $(ISO_PATH)
	@ bochs -q

qemu: $(ISO_PATH)
	@ qemu-system-x86_64 -cdrom $(ISO_PATH) -smp 4

qemu-serial: $(ISO_PATH)
	@ qemu-system-x86_64 -cdrom $(ISO_PATH) -smp 4 -nographic

vmware:
#	@ echo a | /cygdrive/c/Users/rada/Dropbox/Projects/Named\ Pipe\ Server/Named\ Pipe\ Server/bin/Release/Named\ Pipe\ Server
	@ vmrun start $(VMX_PATH)

vmware2:
	@ vmrun start $(VMX_PATH)
	@ sleep 5
	@ vmrun stop $(VMX_PATH)

####################################### COMPONENTS ##########

jegudiel: sysheaders
#	@ echo "/MAK:" $@
	@ $(MAKE) -C jegudiel/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

image:: kernel apps libs sysheaders
#	@ echo "/MAK:" $@
	@ $(MAKE) -C image/ $(ARC) $(SETTINGS) iso $(MAKE_FLAGS)

kernel: libs sysheaders
#	@ echo "/MAK:" $@
	@ $(MAKE) -C $(KERNEL_DIR)/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

apps: kernel libs sysheaders
#	@ echo "/MAK:" $@
	@ $(MAKE) -C apps/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

libs: sysheaders
#	@ echo "/MAK:" $@
	@ $(MAKE) -C libs/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)

sysheaders:
#	@ echo "/MAK:" $@
	@ $(MAKE) -C sysheaders/ $(ARC) $(SETTINGS) install $(MAKE_FLAGS)
