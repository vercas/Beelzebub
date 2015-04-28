.PHONY: run clean loader image kernel

PREFIX    := ./build
EMU       := bochs
EMUFLAGS  := -q

all: loader kernel image

run: image
	@ $(EMU) $(EMUFLAGS)
	
qemu: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4

qemu-serial: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4 -nographic
	
loader:
	@ $(MAKE) -C jegudiel/ install
	
image: loader
	@ $(MAKE) -C image/

kernel:
	@ $(MAKE) -C beelzebub/ install

clean:
	@ $(MAKE) -C image/ clean
	@ $(MAKE) -C beelzebub/ clean
	@ $(MAKE) -C jegudiel/ clean
	@ rm -Rf $(PREFIX)/*

