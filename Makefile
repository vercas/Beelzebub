.SUFFIXES:  

########################
# Default architecture #
all:
	@ echo -n "Currently supported target architectures are: " 1>&2
	@ echo "amd64, ia32pae, ia32" 1>&2
	@ echo "Please choose one of them as a target!" 1>&2
	@ return 42 # Yes, the answer to 'all', basically.

PREFIX		:= ./build

include Beelzebub.mk

KERNEL_DIR	:= ./$(KERNEL_NAME)

####################################### BASICS ##########

.PHONY: run qemu qemu-serial clean jegudiel image kernel $(ARC) $(SETTINGS)

ia32:		image
ia32pae:	image
amd64:		jegudiel image

run: $(ISO_PATH)
	@ bochs -q
	
qemu: $(ISO_PATH)
	@ qemu-system-x86_64 -cdrom $(ISO_PATH) -smp 4

qemu-serial: $(ISO_PATH)
	@ qemu-system-x86_64 -cdrom $(ISO_PATH) -smp 4 -nographic
	
jegudiel:
	@ echo "/MAK:" $@
	@ $(MAKE) -C jegudiel/ $(ARC) $(SETTINGS) install -j 8
	
image: kernel
	@ echo "/MAK:" $@
	@ $(MAKE) -C image/ $(ARC) $(SETTINGS) iso

kernel:
	@ echo "/MAK:" $@
	@ $(MAKE) -C $(KERNEL_DIR)/ $(ARC) $(SETTINGS) install -j 8

clean:
	@ $(MAKE) -C image/ clean
	@ $(MAKE) -C $(KERNEL_DIR)/ clean
	@ $(MAKE) -C jegudiel/ clean
	@ rm -Rf $(PREFIX)

