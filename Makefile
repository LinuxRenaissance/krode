CC	= gcc
CFLAGS	= -Wall -Wextra -O2
LDFLAGS	= -lhidapi-hidraw
BINARY	= krode

all: $(BINARY)

$(BINARY): krode.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(BINARY)

.PHONY: all clean
