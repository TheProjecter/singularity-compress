
#CC = i586-mingw32msvc-gcc
#AR = i586-mingw32msvc-ar

CC = gcc
AR = ar

STRIP = strip

UPX = upx

CFLAGS = -Wall -DNDEBUG -O2
LDFLAGS = -lJudy 
STRIP_FLAGS= -S
STRIP_FLAGS2 = --remove-section=.comment
LIB= libpacker.a
UPXFLAGS=--brute

all: packer-stub

clean:
	rm -f packer packer-stub packer-stripped packer-small lz_coder.o simple_c.o libpacker.a

simple_c.o: simple_c.c range_encoder.h ../common/rangecod.h lz_coder.h ../common/code_tables.h codes.h
	$(CC) $(CFLAGS)  -o $@ -c $<

lz_coder.o: lz_coder.c lz_coder.h
	$(CC) $(CFLAGS) -o $@ -c $<

packer: lz_coder.o simple_c.o 
	$(CC) $(LDFLAGS) -o $@ lz_coder.o simple_c.o

packer-stub: packer
	$(STRIP) $(STRIP_FLAGS)  -o packer-stripped $<
	$(STRIP) $(STRIP_FLAGS2) -o $@ packer-stripped
	rm packer-stripped
	rm $<
	$(UPX) $(UPXFLAGS) $@
	echo "#define SELF_SIZE " `wc -c $@|cut -f1 -d\ ` >sfx.h


libpacker.a: lz_coder.o simple_c.o
	$(AR) rcs $@ $@ $<
