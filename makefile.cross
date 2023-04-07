CFLAGS = -Wall -I include

ifdef DEBUG
CFLAGS += -ggdb -g3
endif

ifdef SYSROOT
CFLAGS += --sysroot=$(SYSROOT)
endif

all: i3cblocks i3ccmd i3cget i3creset i3cset i3ctransfer

clean:
	rm -f i3cblocks i3ccmd i3cget i3creset i3cset i3ctransfer

i3cblocks: i3cblocks.c include/i3c/i3cdev.h include/uapi/linux/i3c-dev.h include/i3c/i3clib.h
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3cblocks i3cblocks.c i3clib.c

i3ccmd: i3ccmd.c include/i3c/i3cdev.h include/uapi/linux/i3c-dev.h include/i3c/i3clib.h
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3ccmd i3ccmd.c i3clib.c

i3cget: i3cget.c include/i3c/i3cdev.h include/uapi/linux/i3c-dev.h include/i3c/i3clib.h
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3cget i3cget.c i3clib.c

i3creset: i3creset.c include/i3c/i3cdev.h include/uapi/linux/i3c-dev.h include/i3c/i3clib.h
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3creset i3creset.c i3clib.c

i3cset: i3cset.c include/i3c/i3cdev.h include/uapi/linux/i3c-dev.h include/i3c/i3clib.h
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3cset i3cset.c i3clib.c

i3ctransfer: i3ctransfer.c include/i3c/i3cdev.h include/uapi/linux/i3c-dev.h include/i3c/i3clib.h
	${CROSS_COMPILE}gcc $(CFLAGS) -o i3ctransfer i3ctransfer.c i3clib.c
