#!/bin/bash

OBJDUMP=""

if [ -v CROSSCOMPILERS_DIR ]
then
	OBJDUMP="$CROSSCOMPILERS_DIR/gcc-x86_64-elf/bin/x86_64-elf-objdump"
else
	OBJDUMP="/usr/local/gcc-x86_64-elf/bin/x86_64-elf-objdump"
fi

$OBJDUMP -M intel -CdlSw --no-show-raw-insn beelzebub/build/beelzebub.amd64.bin | less
