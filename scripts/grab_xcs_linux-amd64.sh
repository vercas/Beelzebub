#!/bin/bash

PRECMD=""

if [ ! -v CROSSCOMPILERS_DIR ]
then
	export CROSSCOMPILERS_DIR="/usr/local"

	PRECMD="sudo"
fi

mkdir -p $CROSSCOMPILERS_DIR

if [ ! -d "$CROSSCOMPILERS_DIR/gcc-x86_64-beelzebub" ]
then
	TMP="$(mktemp)"

	pushd "$CROSSCOMPILERS_DIR"

	$PRECMD wget "http://u.vercas.com/gcc-x86_64-beelzebub.tar.xz" -O $TMP
	RES=$?
	
	if [ $RES != 0 ]
	then
		$PRECMD wget "https://www.dropbox.com/s/zzmjcsd4d8wdiym/gcc-x86_64-beelzebub.tar.xz?dl=1" -O $TMP
	fi

	$PRECMD tar --no-same-owner -xJf $TMP

	rm $TMP

	popd
fi
