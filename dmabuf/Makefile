#!makefile

CC ?= gcc
TARGET := dmabuf
SRC = main.c
CFLAGS := -g
EXTRA_LIBS += -lX11 -lepoxy

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(EXTRA_LIBS)

clean:
	rm -rf $(TARGET)
