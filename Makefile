CFLAGS := -Wall -Werror

FUSE_CFLAGS = $(shell pkg-config fuse --cflags)

TARGET = tcfsd tcfs

all:: $(TARGET)

tcfsd: tcfsd.o
	$(CC) $^ -o $@ -lev

tcfs.o: CFLAGS += $(FUSE_CFLAGS)

tcfs: tcfs.o
	$(CC) $^ -o $@ -lfuse

clean::
	-rm -f $(TARGET) *.o
