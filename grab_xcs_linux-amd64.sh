#!/bin/bash

PRECMD=""

if [ ! -v CROSSCOMPILERS_DIR ]
then
	export CROSSCOMPILERS_DIR="/usr/local"

	PRECMD="sudo"
fi

mkdir -p $CROSSCOMPILERS_DIR

if [ ! -d "$CROSSCOMPILERS_DIR/gcc-x86_64-elf" ]
then
	pushd "$CROSSCOMPILERS_DIR"

	$PRECMD wget "http://u.vercas.com/gcc-x86_64-elf.tar.xz" -O - | $PRECMD tar --no-same-owner -xJ

	popd
fi
