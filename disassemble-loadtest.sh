#!/bin/bash

OBJDUMP=""

if [ -v CROSSCOMPILERS_DIR ]
then
	OBJDUMP="$CROSSCOMPILERS_DIR/gcc-x86_64-beelzebub/bin/x86_64-beelzebub-objdump"
else
	OBJDUMP="/usr/local/gcc-x86_64-beelzebub/bin/x86_64-beelzebub-objdump"
fi

$OBJDUMP -M intel -CdlSw --no-show-raw-insn .vmake/amd64.debug/apps/loadtest/loadtest.exe | less
