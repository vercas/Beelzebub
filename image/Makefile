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

# ISO_SOURCE_INITRD	:= $(BUILD_DIR)/initrd.tar

ISO_TARGET_BOOT		:= boot
ISO_TARGET_GRUB		:= $(ISO_TARGET_BOOT)/grub
ISO_TARGET_ELTORITO	:= $(ISO_TARGET_GRUB)/stage2_eltorito
ISO_TARGET_GRUBCONF	:= $(ISO_TARGET_GRUB)/grub.cfg
ISO_TARGET_FONT		:= $(ISO_TARGET_GRUB)/font.pf2
ISO_TARGET_KERNEL	:= $(ISO_TARGET_BOOT)/$(KERNEL_BIN).gz
ISO_TARGET_INITRD	:= $(ISO_TARGET_BOOT)/initrd.tar.gz

ifneq (,$(SYSROOT))
	SYSROOT_FILES	:= $(shell find $(SYSROOT))
else
	SYSROOT_FILES	:=  
endif

################################################################################
#                        ARCHITECTURE-SPECIFIC SETTINGS                        #
################################################################################

MISCELLANEOUS_TARGETS		:=  

ifeq ($(ARC),amd64)
	MISCELLANEOUS_TARGETS	+= $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/jegudiel.bin.gz 
endif

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
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_INITRD) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_ELTORITO) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_GRUBCONF) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_FONT) \
			 $(MISCELLANEOUS_TARGETS)
#	@ echo "/ISO:" $@
	@ $(MKISO) -R -b $(ISO_TARGET_ELTORITO) -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ $(ISO_TARGET_DIR)
######################################
# The dependencies are the following:
# - Kernel binary blob
# - InitRD
# - GRUB El Torito bootloader binary blob
# - GRUB configuration file
# - GRUB font

############
# Clean up #
clean:
#	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)

####################################### INITRD ##########

##############################
# Create InitRD tape archive #
$(ISO_TARGET_DIR)/$(ISO_TARGET_INITRD): $(SYSROOT_FILES)
#	@ echo "/TAR:" $(SYSROOT) ">" $@
	@ tar czf $@ --owner=root --group=root -C $(SYSROOT) --exclude="*.d" --exclude="libcommon.*.a" .

# ################################
# # Compress InitRD tape archive #
# $(ISO_TARGET_DIR)/$(ISO_TARGET_INITRD): $(ISO_SOURCE_INITRD)
# 	@ echo "/ZIP:" $< ">" $@
# 	@ gzip -cfkn9 $< > $@ 

####################################### ISO: MANDATORY FILES AND FOLDERS ##########

###########################
# Copy kernel binary blob #
$(ISO_TARGET_DIR)/$(ISO_TARGET_KERNEL): $(KERNEL_INSTALL_PATH)
#	@ echo "/ZIP:" $< ">" $@
	@ mkdir -p $(@D)
	@ gzip -9 < $< > $@

#########################
# Copy GRUB binary blob #
$(ISO_TARGET_DIR)/$(ISO_TARGET_ELTORITO): $(ISO_SOURCE_ELTORITO)
#	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

#########################
# Copy GRUB config file #
$(ISO_TARGET_DIR)/$(ISO_TARGET_GRUBCONF): $(ISO_SOURCE_GRUBCONF)
#	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

#######################
# Copy GRUB font file #
$(ISO_TARGET_DIR)/$(ISO_TARGET_FONT): $(ISO_SOURCE_FONT)
#	@ echo "/CPY:" $< ">" $@
	@ mkdir -p $(@D)
	@ cp $< $@

####################################### ISO: OTHER FILES AND FOLDERS ##########

ifeq ($(ARC),amd64)
$(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/jegudiel.bin.gz: $(PREFIX)/bin/jegudiel.bin
#	@ echo "/ZIP:" $< ">" $@
	@ mkdir -p $(@D)
	@ gzip -9 < $< > $@
endif
