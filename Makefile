CC=musl-gcc -Os -static -std=c99
CFLAGS= -Werror \
	-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition

COMMANDS=\
  bin/import \
	bin/export \
	bin/want

all: $(COMMANDS)

test-all: test.img $(COMMANDS)
	rm -rf test ; mkdir -p test
	# Developer imports image into local store on dev machine.
	bin/import test.img test/dev.idx test/dev.obj
	# Developer uploads index to server
	cp test/dev.idx test/server.idx
	# Developer uploads objects to server that server is missing
	bin/want -s test/server.obj -i test/server.idx \
	 | bin/export -s test/dev.obj \
	 | bin/import -s test/server.obj \
	 | tee test/server.delta \
	 | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'
	# Device in field downloads index from server
	cp test/server.idx test/client.idx
	# Device syncs down missing objects.
	bin/want -s test/client.obj < test/client.idx \
	 | bin/export -s test/server.obj \
	 | bin/import -s test/client.obj \
	 | tee test/client.delta \
	 | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'
	# Device writes new image to block device
	bin/export -s test/client.obj -i test/client.idx -o test/client.img
	# Check images match on dev machine and client device
	diff test.img test/client.img
	# Simulate partial update by deleting part of client store
	rm -rf test/client.obj/0* test/client.obj/1* test/client.obj/2*
	# Device syncs down again
	bin/want -s test/client.obj < test/client.idx \
	 | bin/export -s test/server.obj \
	 | bin/import -s test/client.obj \
	 | tee test/client.2.delta \
	 | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'

test.img:
	rm -f .$@ && truncate -s 100M .$@
	mkfs.ext4 -L caify-test-fs .$@
	sudo umount mnt; rm -rf mnt; mkdir mnt
	sudo mount .$@ mnt
	sudo git clone . mnt/caify; sleep 1
	sudo umount mnt; sudo rm -rf mnt
	mv .$@ $@


clean:
	rm -rf bin test*

BLAKE2/ref/blake2b-ref.c:
	git submodule update --init

bin/%: %.c shared.c shared.h BLAKE2/ref/blake2b-ref.c
	mkdir -p bin
	$(CC) $(CFLAGS) $< shared.c -o $@

install: $(COMMANDS)
	cp bin/import /usr/local/bin/caify-import
	cp bin/export /usr/local/bin/caify-export
	cp bin/want /usr/local/bin/caify-want

uninstall:
	rm -f \
		/usr/local/bin/caify-import \
		/usr/local/bin/caify-export \
		/usr/local/bin/caify-want
