# Oi, this goes into your /usr/bin directory in Cygwin.
# AKA $(CYGWIN_BIN)

OUT="$(make $* 2>&1)"
RES=$?

# sed magic courtesy of Halofreak1990
sed -e 's/\(.[a-zA-Z]\+\):\([0-9]\+\):\([0-9]\+\):/\1(\2,\3):/' <<EOF
${OUT}
EOF

exit $RES
