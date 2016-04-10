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
PREFIX2		:= ./..
PREFIX		:= ./../build
PROJ_SUBDIR	:= image

# Local directories
BUILD_DIR 	:= ./build

# Common settings
include ../Beelzebub.mk

# Fake targets.
.PHONY: iso clean $(ARC) $(SETTINGS)

# Toolchain
include ../Toolchain.mk

################################################################################
#                                    FILES                                     #
################################################################################

ISO_TARGET_DIR		:= $(BUILD_DIR)/iso

ISO_SOURCE_GRUB		:= ./grub2
ISO_SOURCE_ELTORITO	:= $(ISO_SOURCE_GRUB)/stage2_eltorito
ISO_SOURCE_GRUBCONF	:= $(ISO_SOURCE_GRUB)/grub.$(ARC).cfg
ISO_SOURCE_FONT		:= $(ISO_SOURCE_GRUB)/font.pf2

ISO_TARGET_BOOT		:= boot
ISO_TARGET_GRUB		:= $(ISO_TARGET_BOOT)/grub
ISO_TARGET_ELTORITO	:= $(ISO_TARGET_GRUB)/stage2_eltorito
ISO_TARGET_GRUBCONF	:= $(ISO_TARGET_GRUB)/grub.cfg
ISO_TARGET_FONT		:= $(ISO_TARGET_GRUB)/font.pf2
ISO_TARGET_KERNEL	:= $(ISO_TARGET_BOOT)/$(KERNEL_BIN)

ISO_TARGET_APPS		:= apps
ISO_TARGET_LIBS		:= libs

BOOTLOADER			:= $(PREFIX)/bin/jegudiel.bin

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

MISCELLANEOUS_FILES		:=  
MISCELLANEOUS_TARGETS	:= $(addprefix $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT), $(notdir $(MISCELLANEOUS_FILES)))

ifeq ($(ARC),amd64)
	MISCELLANEOUS_FILES		+= $(PREFIX)/bin/jegudiel.bin 
	MISCELLANEOUS_TARGETS	+= $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/jegudiel.bin 
endif

MISCELLANEOUS_FILES		+= $(PREFIX)/bin/loadtest.exe $(SYSROOT)/usr/lib/libbeelzebub.$(ARC).so 
MISCELLANEOUS_TARGETS	+= $(ISO_TARGET_DIR)/$(ISO_TARGET_APPS)/loadtest.exe \
                           $(ISO_TARGET_DIR)/$(ISO_TARGET_LIBS)/libbeelzebub.$(ARC).so 

################################################################################
#                                   TARGETS                                    #
################################################################################

# Do nothing for the architecture as a target.
$(ARC):
	@ true

iso: $(ISO_PATH)
	@ true

###################
# Create ISO file #
$(ISO_PATH): $(ISO_TARGET_DIR)/$(ISO_TARGET_KERNEL) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_ELTORITO) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_GRUBCONF) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_FONT) \
			 $(MISCELLANEOUS_TARGETS)
	@ echo "/ISO:" $@
	@ $(MKISO) -R -b $(ISO_TARGET_ELTORITO) -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ $(ISO_TARGET_DIR)
######################################
# The dependencies are the following:
# - Kernel binary blob
# - GRUB El Torito bootloader binary blob
# - GRUB configuration file

############
# Clean up #
clean:
	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*

####################################### ISO: MANDATORY FILES AND FOLDERS ##########

###########################
# Copy kernel binary blob #
$(ISO_TARGET_DIR)/$(ISO_TARGET_KERNEL): $(KERNEL_INSTALL_PATH) $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@

#########################
# Copy GRUB binary blob #
$(ISO_TARGET_DIR)/$(ISO_TARGET_ELTORITO): $(ISO_SOURCE_ELTORITO) $(ISO_TARGET_DIR)/$(ISO_TARGET_GRUB)
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@

#########################
# Copy GRUB config file #
$(ISO_TARGET_DIR)/$(ISO_TARGET_GRUBCONF): $(ISO_SOURCE_GRUBCONF) $(ISO_TARGET_DIR)/$(ISO_TARGET_GRUB)
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@

#######################
# Copy GRUB font file #
$(ISO_TARGET_DIR)/$(ISO_TARGET_FONT): $(ISO_SOURCE_FONT) $(ISO_TARGET_DIR)/$(ISO_TARGET_GRUB)
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@

#######################
# Make GRUB directory #
$(ISO_TARGET_DIR)/$(ISO_TARGET_GRUB):
	@ echo "/MKD:" $@
	@ mkdir -p $@

#######################
# Make boot directory #
$(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT):
	@ echo "/MKD:" $@
	@ mkdir -p $@

#######################
# Make apps directory #
$(ISO_TARGET_DIR)/$(ISO_TARGET_APPS):
	@ echo "/MKD:" $@
	@ mkdir -p $@

#######################
# Make libs directory #
$(ISO_TARGET_DIR)/$(ISO_TARGET_LIBS):
	@ echo "/MKD:" $@
	@ mkdir -p $@

####################################### ISO: OTHER FILES AND FOLDERS ##########

ifeq ($(ARC),amd64)
$(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/jegudiel.bin: $(PREFIX)/bin/jegudiel.bin
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@
endif

$(ISO_TARGET_DIR)/$(ISO_TARGET_APPS)/loadtest.exe: $(PREFIX)/bin/loadtest.exe $(ISO_TARGET_DIR)/$(ISO_TARGET_APPS)
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@

$(ISO_TARGET_DIR)/$(ISO_TARGET_LIBS)/libbeelzebub.$(ARC).so: $(SYSROOT)/usr/lib/libbeelzebub.$(ARC).so $(ISO_TARGET_DIR)/$(ISO_TARGET_LIBS)
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@
