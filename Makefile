CC=musl-gcc -Os -static -std=c99 -Werror \
	-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
# CC=gcc -g -std=gnu99 -Werror \
# 	-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition

COMMANDS=\
  bin/import \
	bin/export \
	bin/want

all: $(COMMANDS)

test: test.img bin/import bin/export
	bin/import -v -s test.obj < $< | bin/export -s test.obj > test2.img
	bin/import -s test.obj -i $< -o test.idx
	bin/export -s test.obj -i test.idx -o test2.img
	diff test.img test.copy.img
	rm -rf test.obj/a* test.obj/b* test.obj/c* test.obj/d*
	bin/want -s test.obj -i test.idx | hexdump -e '32/1 "%02x" "\n"'

test.img:
	rm -f .$@ && truncate -s 100M .$@
	mkfs.ext4 -L caify-test-fs .$@
	sudo umount mnt; rm -rf mnt; mkdir mnt
	sudo mount .$@ mnt
	sudo git clone . mnt/caify; sleep 1
	sudo umount mnt; sudo rm -rf mnt
	mv .$@ $@


clean:
	rm -rf bin test.*

BLAKE2/ref/blake2b-ref.c:
	git submodule update --init

bin:
	mkdir bin

bin/%: %.c shared.c shared.h BLAKE2/ref/blake2b-ref.c bin
	$(CC) $< shared.c -o $@

install: $(COMMANDS)
	cp bin/import /usr/local/bin/caify-import
	cp bin/export /usr/local/bin/caify-export
	cp bin/want /usr/local/bin/caify-want

uninstall:
	rm -f \
		/usr/local/bin/caify-import \
		/usr/local/bin/caify-export \
		/usr/local/bin/caify-want
