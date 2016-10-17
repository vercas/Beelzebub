#	Argument #1 must be the folder to add.
#	The rest are the actual compilation command. 

TMP="$(mktemp)"

${@:2} &> $TMP
RES=$?

sed -r 's,(In file included from | *)(.+:[0-9]+:[0-9]+:),\1'$1'/\2,' < $TMP

rm $TMP

exit $RES

#	This file runs the compilation command, capturing its output in a temporary
#	file, as well as its return code.
#	It then filters the output through a substitution rule, which preppends the
#	project's directory to all file file names present in such a way that
#	Sublime can retrieve all the info it needs from the build output.
#	Then, it exits with the same code as the compilation command, to make sure
#	that GNU Make can halt.
