#PLATFORM ?= X11
PLATFORM ?= FB

CC ?= gcc
LINUX_DIR ?= ../linux-sunxi
TARGET := disp
SRC = main.c video.c display.c
CFLAGS := -g -I$(LINUX_DIR)/include
EXTRA_LIBS += -lpthread -lEGL -lGLESv2

ifeq ($(PLATFORM),X11)
CFLAGS += -D_X_WINDOW_SYSTEM_
EXTRA_LIBS += -lX11
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(EXTRA_LIBS)

clean:
	rm -rf $(TARGET)
