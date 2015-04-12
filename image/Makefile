.PHONY: prepare iso clean

BUILD_DIR 		:= ./build
GRUB_DIR		:= ./grub

PREFIX			:= ./../build

ISO				:= $(PREFIX)/boot.iso
ISO_DIR			:= $(BUILD_DIR)/iso
ISO_BOOT		:= boot
ISO_ELTORITO	:= $(ISO_BOOT)/grub/stage2_eltorito

KERNEL			:= $(PREFIX)/bin/beelzebub_64.bin
BOOTLOADER		:= $(PREFIX)/bin/jegudiel.bin
MODULES			:=

iso: prepare
	@ echo " Creating ISO file..."
	@ mkisofs -R -b $(ISO_ELTORITO) -no-emul-boot -boot-load-size 4 \
	          -boot-info-table -o $(ISO) $(ISO_DIR) 2> /dev/null

prepare:
ifneq ($(strip $(KERNEL)),)
	@ echo " Creating directory structure..."
	@ mkdir -p $(ISO_DIR)/$(ISO_BOOT)/grub
	@ echo " Copying GRUB files..."
	@ cp $(GRUB_DIR)/* $(ISO_DIR)/$(ISO_BOOT)/grub
	@ echo " Copying binaries..."
	@ cp $(KERNEL) $(ISO_DIR)/$(ISO_BOOT)/
	@ cp $(BOOTLOADER) $(ISO_DIR)/$(ISO_BOOT)/
  	ifneq ($(strip $(MODULES)),)
		@ cp $(MODULES) $(ISO_DIR)/$(ISO_BOOT)/
  	endif
else
	$(error Specify a kernel binary in KERNEL.)
endif  
	
clean:
	@ echo " RM " $(BUILD_DIR)
	@ rm -Rf $(BUILD_DIR)/*
