.PHONY: run qemu qemu-serial clean jegudiel image kernel ia32 ia32pae amd64
.SUFFIXES:  

########################
# Default architecture #
all:
	@ echo -n "Currently supported target architectures are: " 1>&2
	@ echo "amd64, ia32pae, ia32" 1>&2
	@ echo "Please choose one of them as a target!" 1>&2
	@ return 42 # Yes, the answer to 'all', basically.

PREFIX		:= ./build

KERNEL_NAME := beelzebub
KERNEL_DIR	:= ./beelzebub

include Beelzebub.mk

####################################### BASICS ##########

ia32:		image
ia32pae:	image
amd64:		jegudiel image

run: image
	@ bochs -q
	
qemu: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4

qemu-serial: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4 -nographic
	
jegudiel:
	@ $(MAKE) -C jegudiel/ $(ARC) install -j 8
	
image: kernel
	@ $(MAKE) -C image/ $(ARC) iso

kernel:
	@ echo "/MAK:" $@
	@ $(MAKE) -C $(KERNEL_DIR)/ $(ARC) install -j 8

clean:
	@ $(MAKE) -C image/ clean
	@ $(MAKE) -C $(KERNEL_DIR)/ clean
	@ $(MAKE) -C jegudiel/ clean
	@ rm -Rf $(PREFIX)

