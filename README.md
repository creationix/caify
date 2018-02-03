## Import

Import a large filesystem image into the local object store and write index file
needed to rebuild filesystem image.

- Input: The filesystem image you with to import.
- Output: The index file you with to generate.

```sh
caify-import < rootfs.img > rootfs.idx
```

## Export

Export is the inverse of import.  Given an index file, it will reconstitute a
filesystem image using the data in the local object store.

- Input: The index file.
- Output: The filesystem image.

```sh
caify-export < rootfs.idx > rootfs.img
```

## Upload

- Input: Raw blocks
- Output: Raw hashes

To upload a new release to a server, do something like the following:

```sh
# Import the rootfs into the local object store.
caify-import < rootfs.img > rootfs.idx
# Upload the index file to the update server.
scp rootfs.idx update.server:rootfs.idx
# Sync up new objects to server's object store.
ssh update.server 'caify-want -i rootfs.idx' \
 | caify-upload \
 | ssh update.server caify-download \
 | hexdump -e '32/1 "%02x" "\n"'
```

First we upload the index to the server.  Once there, we ask the server what
hashes it's missing and stream that to a local uploader which streams to a remote downloader with streams to a local hexdump so we can watch progress.

## Download

- Input: Raw hashes
- Output: Raw blocks

To download a a new release on a device, do something like the following:

```sh
# Download the index file for the new release.
scp update.server:rootfs.idx ./
# Sync down new objects from server to local store.
caify-want < rootfs.idx \
 | ssh update.server caify-upload \
 | caify-download \
 | hexdump -e '32/1 "%02x" "\n"'
# Write the image to the partition
caify-export < rootfs.idx > /dev/mmcblk0p2
```

## Want

Want is a helper that consumes an index file and emits a raw stream of hashes
not found in the local object store.

- Input: Index file.
- Output: Raw hashes.

```sh
caify-want < rootfs.idx | hexdump -e '32/1 "%02x" "\n"'
```
