CFLAGS := -Wall -Werror

FUSE_CFLAGS = $(shell pkg-config fuse --cflags)

TARGET = tcfs

all:: $(TARGET)

tcfs.o: CFLAGS += $(FUSE_CFLAGS)

tcfs: tcfs.o utils.o encrypt.o rc4.o md5.o
	$(CC) $^ -o $@ -lfuse

clean::
	-rm -f $(TARGET) *.o
