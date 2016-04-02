#!/bin/bash

OBJDUMP=""

if [ -v CROSSCOMPILERS_DIR ]
then
	OBJDUMP="$CROSSCOMPILERS_DIR/gcc-x86_64-beelzebub/bin/x86_64-beelzebub-objdump"
else
	OBJDUMP="/usr/local/gcc-x86_64-beelzebub/bin/x86_64-beelzebub-objdump"
fi

$OBJDUMP -M intel -CdlSw --no-show-raw-insn libs/runtime/build/libbeelzebub.amd64.so | less
