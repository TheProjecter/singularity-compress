
CC = i586-mingw32msvc-gcc
AR = i586-mingw32msvc-ar

#CC = gcc
#AR = ar

STRIP = strip

UPX = upx

#CFLAGS = -Wall -g3
CFLAGS = -Wall -DNDEBUG -Os -s
LDFLAGS = -Wl,-O2 -s 
STRIP_FLAGS= -S
STRIP_FLAGS2 = --remove-section=.comment
LIB= libunpacker.a
UPXFLAGS=--brute

all: unpacker-stub

clean:
	rm -f unpacker unpacker-stub unpacker-stripped unpacker-small simple_d.o libunpacker.a unpacker_main.o

simple_d.o: simple_d.c ../common/code_tables.h range_decod.h ../common/rangecod.h ../common/model.h
	$(CC) $(CFLAGS) -o $@ -c $<

unpacker_main.o: unpacker_main.c simple_d.h
	$(CC) $(CFLAGS) -o $@ -c $<

unpacker: simple_d.o unpacker_main.o
	$(CC) $(LDFLAGS) -o $@ simple_d.o unpacker_main.o

unpacker-stub: unpacker libunpacker.a
	$(STRIP) $(STRIP_FLAGS)  -o unpacker-stripped $<
	$(STRIP) $(STRIP_FLAGS2) -o $@ unpacker-stripped
	rm unpacker-stripped
#	rm $<
	$(UPX) $(UPXFLAGS) $@
	echo "#define SELF_SIZE " `wc -c $@|cut -f1 -d\ ` >sfx.h


libunpacker.a: simple_d.o
	$(AR) rcs $@ $@ $<

