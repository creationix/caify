CC=musl-gcc -Os -static -std=c99 -Werror \
	-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
	
all: import export

clean:
	rm -f import export

BLAKE2/ref/blake2b-ref.c:
	git submodule update --init

%: %.c shared.c BLAKE2/ref/blake2b-ref.c
	$(CC) $< -o $@
