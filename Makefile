.PHONY: run clean jegudiel image kernel
all: amd64

PREFIX    := ./build
EMU       := bochs
EMUFLAGS  := -q

ia32:		kernel image
ia32pae:	kernel image
amd64:		jegudiel kernel image

run: image
	@ $(EMU) $(EMUFLAGS)
	
qemu: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4

qemu-serial: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4 -nographic
	
jegudiel:
	@ $(MAKE) -C jegudiel/ $(MAKECMDGOALS) install
	
image:
	@ $(MAKE) -C image/

kernel:
	@ $(MAKE) -C beelzebub/ $(MAKECMDGOALS) install -j 9

clean:
	@ $(MAKE) -C image/ clean
	@ $(MAKE) -C beelzebub/ clean
	@ $(MAKE) -C jegudiel/ clean
	@ rm -Rf $(PREFIX)/*

