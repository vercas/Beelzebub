#	Argument #1 must be the configuration.
#	Argument #2 must be the architecture.
#	The rest must be the build settings.

TMP="$(mktemp)"

echo "$*" > $TMP
cmp $TMP last_settings.txt
#	Compares the build settings with the last ones.

if [ $? != 0 ]
then
	make clean
fi

make $@

MAKE_RES=$?

if [ $? == 0 ]
then
	mv -f $TMP last_settings.txt
fi

exit $MAKE_RES

#	This mainly exists because GNU Make only works well (for me, at least) when
#	run under Bash.
#	But it allowed me to add one extra feature...
