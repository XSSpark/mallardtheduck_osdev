#!/bin/bash

rm -rf parted-3.2
if [ ! -f parted-3.2.tar.xz ];
then
	wget https://github.com/mallardtheduck/btos-3rdparty-prereqs/raw/master/parted-3.2.tar.xz
fi
tar xf parted-3.2.tar.xz
mkdir -p originals
tar xf parted-3.2.tar.xz -C originals

cd parted-3.2
patch -p1 -R < ../parted/parted.patch
cp ../parted/btos-build.sh .
mkdir -p install
./btos-build.sh
