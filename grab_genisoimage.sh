#!/bin/bash

PRECMD=""

if [ ! -v MISC_TOOLS_DIR ]
then
	export MISC_TOOLS_DIR="/usr/local/beelzebub-tools"

	PRECMD="sudo"
fi

mkdir -p $MISC_TOOLS_DIR

if [ ! -e "$MISC_TOOLS_DIR/genisoimage" ]
then
	pushd "$MISC_TOOLS_DIR"

	$PRECMD wget "http://u.vercas.com/genisoimage"
	RES=$?
	
	if [ $RES != 0 ]
	then
		$PRECMD wget "https://dl.dropboxusercontent.com/u/1217587/genisoimage"
	fi

	chmod +x genisoimage

	popd
fi
