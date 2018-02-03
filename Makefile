# CC=musl-gcc -Os -static -std=c99 -Werror \
# 	-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
CC=gcc -g -std=gnu99 -Werror \
	-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition

all: import export

test: test.img test.copy.img
	diff test.img test.copy.img

test.img:
	rm -f .$@ && truncate -s 100M .$@
	mkfs.ext4 -L caify-test-fs .$@
	sudo umount mnt; rm -rf mnt; mkdir mnt
	sudo mount .$@ mnt
	sudo git clone . mnt/caify; sleep 1
	sudo umount mnt; sudo rm -rf mnt
	mv .$@ $@

test.copy.img: test.img import export
	./import -v -s test.obj < $< | tee log | ./export -s test.obj > $@

clean:
	rm -rf import export test.*

BLAKE2/ref/blake2b-ref.c:
	git submodule update --init

%: %.c shared.c shared.h BLAKE2/ref/blake2b-ref.c
	$(CC) $< shared.c -o $@

install: import export
	cp import /usr/local/bin/caify-import
	cp export /usr/local/bin/caify-export

uninstall:
	rm -f /usr/local/bin/caify-import /usr/local/bin/caify-export
