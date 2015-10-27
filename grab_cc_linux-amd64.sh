#!/bin/bash

if [ ! -v CROSSCOMPILERS_DIR ]
then
	export CROSSCOMPILERS_DIR="/usr/local"
fi

if [ ! -d "$CROSSCOMPILERS_DIR/gcc-x86_64-elf" ]
then
	pushd "$CROSSCOMPILERS_DIR"

	sudo wget "http://u.vercas.com/gcc-x86_64-elf.tar.xz" -O o | sudo tar --no-same-owner -xJ

	popd
fi
