#execute makefile at least twice: once to generate the stub, and calculate its size, and once again to store that size inside it

CC = i586-mingw32msvc-gcc
AR = i586-mingw32msvc-ar

#CC = gcc
#AR = ar

STRIP = strip

UPX = upx

CFLAGS = -Wall -DNDEBUG -Os
LDFLAGS = 
STRIP_FLAGS= -S
STRIP_FLAGS2 = --remove-section=.comment
LIB= ../unpacker/libunpacker.a
UPXFLAGS=--brute

all: sfx-stub

clean:
	rm -f sfx sfx-stub sfx-stripped sfx-small sfx.o unpack.o libunpack.a

sfx.o: sfx.c sfx.h
	$(CC) $(CFLAGS)  -o $@ -c $<

sfx: sfx.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ $< $(LIB)

sfx-stub: sfx
	$(STRIP) $(STRIP_FLAGS)  -o sfx-stripped $<
	$(STRIP) $(STRIP_FLAGS2) -o $@ sfx-stripped
	rm sfx-stripped
	rm $<
	$(UPX) $(UPXFLAGS) $@
	echo "#define SELF_SIZE " `wc -c $@|cut -f1 -d\ ` >sfx.h

unpack.o: unpack.c
	$(CC) $(CFLAGS) -o $@ -c $<

