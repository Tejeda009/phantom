CC = gcc
CFLAGS = -static -s -no-pie -fno-stack-protector -znoexecstack -O0 
TARGET = phantom

.PHONY: all clean

all: $(TARGET)

$(TARGET): phantom.c
	$(CC) $(CFLAGS) -o $@ $<
	strip $@

clean:
	rm -f $(TARGET)
