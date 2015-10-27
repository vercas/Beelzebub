#!/bin/bash

if [ ! -v CROSSCOMPILERS_DIR ]
then
	export CROSSCOMPILERS_DIR="/usr/local"

	echo "CROSSCOMPILERS_DIR is set to $CROSSCOMPILERS_DIR"
else
	echo "CROSSCOMPILERS_DIR was $CROSSCOMPILERS_DIR"
fi

mkdir -p $CROSSCOMPILERS_DIR

if [ ! -d "$CROSSCOMPILERS_DIR/gcc-x86_64-elf" ]
then
	pushd "$CROSSCOMPILERS_DIR"

	sudo wget "http://u.vercas.com/gcc-x86_64-elf.tar.xz" -O - | sudo tar --no-same-owner -xJ

	popd
fi
