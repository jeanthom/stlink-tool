ifeq ($(ASAN), 1)
CFLAGS += -fsanitize=address  -Wno-format-truncation
LDFLAGS += -lasan
endif

CFLAGS := -Wall -Wextra -Werror $(shell pkg-config --cflags libusb-1.0) -g -Og
LDFLAGS := $(shell pkg-config --libs libusb-1.0)

OBJS := src/main.o src/stlink.o src/crypto.o tiny-AES-c/aes.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

stlink-tool: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f src/*.o
	rm -f stlink-tool
