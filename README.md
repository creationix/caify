# Commands

There are 3 main commands in the Caify system, they are `caify-import`,
`caify-export`, and `caify-filter`.

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

## Filter

Filter is a helper that consumes an index file and emits a filtered index stream
of hashes not found in the local object store.

- Input: Filtered Index
- Output: Raw blocks

```sh
caify-filter < rootfs.idx | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'
```

# Use Cases

There are two main use cases for this system, though the tools are flexible and
other workflows can be constructed easily.  The main purpose of this is to publish
new rootfs images to internet servers and then allow devices in the fields to
download updates and write them to block devices.

In both of these use cases, only the blocks that are new since the last release
are sent over the internet because of the de-duplication inherent in
content-addressable systems.

## Publish new Release

To upload a new release to a server, do something like the following:

```sh
# Import the rootfs into the local object store.
caify-import < rootfs.img > rootfs.idx
# Upload the index file to the update server.
scp rootfs.idx update.server:rootfs.idx
# Sync up new objects to server's object store.
ssh update.server 'caify-filter -i rootfs.idx' \
 | caify-export \
 | ssh update.server 'caify-import' \
 | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'
```

First we upload the index to the server.  Once there, we ask the server what
hashes it's missing and stream that to a local uploader which streams to a remote downloader with streams to a local hexdump so we can watch progress.

## Download new Release

To download a a new release on a device, do something like the following:

```sh
# Download the index file for the new release.
scp update.server:rootfs.idx rootfs.idx
# Sync down new objects from server to local store.
caify-filter < rootfs.idx \
 | ssh update.server 'caify-export' \
 | caify-import \
 | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'
# Write the image to the partition
caify-export < rootfs.idx > /dev/mmcblk0p2
```

Or if you don't have ssh connectivity to the updates server, a socat based flow
works too.

On the server host the index files over https and spin up a tcp server for
objects with `socat` and `caify-export`:

```sh
socat tcp-listen:4000,fork exec:caify-export
```

Then on the client, do something like:

```sh
curl https://update.server/rootfs.idx \
 | tee rootfs.idx \
 | caify-filter \
 | socat tcp:update.server:4000 - \
 | caify-import \
 | hexdump -e '32/1 "%02x" 1/4 " %x" "\n"'
```
