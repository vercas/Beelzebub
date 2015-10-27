#!/bin/bash

PRECMD=""

if [ ! -v MISC_TOOLS_DIR ]
then
	export MISC_TOOLS_DIR="/usr/local/beelzebub-tools"

	PRECMD="sudo"
fi

mkdir -p $MISC_TOOLS_DIR

pushd "$MISC_TOOLS_DIR"

if [ ! -e "$MISC_TOOLS_DIR/genisoimage" ]
then
	$PRECMD wget "http://u.vercas.com/genisoimage"

	chmod +x genisoimage
fi

popd
