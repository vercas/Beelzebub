.PHONY: prepare iso clean
.SUFFIXES:  

KERNEL_NAME			:= beelzebub
PREFIX				:= ../build
BUILD_DIR 			:= ./build

include ../Beelzebub.mk

####################################### VARIABLES ##########

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

BOOTLOADER			:= $(PREFIX)/bin/jegudiel.bin

####################################### ARCHITECTURE-SPECIFIC ##########

MISCELLANEOUS_FILES		:=  
MISCELLANEOUS_TARGETS	:= $(addprefix $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT), $(notdir $(MISCELLANEOUS_FILES)))

ifeq ($(ARC),amd64)
	MISCELLANEOUS_FILES		+= $(PREFIX)/bin/jegudiel.bin 
	MISCELLANEOUS_TARGETS	+= $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/jegudiel.bin 
endif

####################################### BASICS ##########

ia32 ia32pae amd64:	iso

iso: $(ISO_PATH)

###################
# Create ISO file #
$(ISO_PATH): $(ISO_TARGET_DIR)/$(ISO_TARGET_KERNEL) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_ELTORITO) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_GRUBCONF) \
			 $(ISO_TARGET_DIR)/$(ISO_TARGET_FONT) \
			 $(MISCELLANEOUS_TARGETS)
	@ echo "/ISO:" $@
	@ mkisofs -R -b $(ISO_TARGET_ELTORITO) -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ $(ISO_TARGET_DIR) 2> /dev/null
######################################
# The dependencies are the following:
# - Kernel binary blob
# - GRUB El Torito bootloader binary blob
# - GRUB configuration file

prepare:
	@ echo "> Copying binaries"
	@ echo "/CPY:" $(BOOTLOADER) ">" $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/
	@ cp $(BOOTLOADER) $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/

	ifneq ($(strip $(MODULES)),)
		@ cp $(MODULES) $(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/
	endif

############
# Clean up #
clean:
	@ echo "/REM:" $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*

####################################### MANDATORY FILES AND FOLDERS ##########

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

####################################### OTHER FILES AND FOLDERS ##########

ifeq ($(ARC),amd64)
$(ISO_TARGET_DIR)/$(ISO_TARGET_BOOT)/jegudiel.bin: $(PREFIX)/bin/jegudiel.bin
	@ echo "/CPY:" $< ">" $@
	@ cp $< $@
endif
