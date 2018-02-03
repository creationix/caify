CC=musl-gcc -Os -static -Wall -std=c99

all: import export

clean:
	rm -f import export

BLAKE2/ref/blake2b-ref.c:
	git submodule update --init

%: %.c config.h BLAKE2/ref/blake2b-ref.c
	$(CC) $< -o $@
