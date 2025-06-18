CFLAGS = -Wall -I include

ifdef DEBUG
CFLAGS += -ggdb
endif

ifdef SYSROOT
CFLAGS += --sysroot=$(SYSROOT)
endif

all: i3ctransfer ibitest

clean:
	rm -f i3ctransfer ibitest

i3ctransfer: i3ctransfer.c
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3ctransfer i3ctransfer.c

ibitest: ibitest.c
	${CROSS_COMPILE}gcc $(CFLAGS) -o ibitest ibitest.c
