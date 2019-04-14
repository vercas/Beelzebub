CFLAGS+=-iquote ../../sysheaders/common/

all: bin/aladin

bin/aladin: main.o | bin
	$(CC) -o $@ $<

bin:
	mkdir bin

