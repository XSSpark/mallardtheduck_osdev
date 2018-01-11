#!/bin/bash

rm -rf freetype-2.7.1
if [ ! -f freetype-2.7.1.tar.gz ];
then
	wget http://mirrors.fe.up.pt/pub/nongnu/freetype/freetype-2.7.1.tar.gz
fi
tar xf freetype-2.7.1.tar.gz
mkdir -p originals
tar xf freetype-2.7.1.tar.gz -C originals

cd freetype-2.7.1
patch -p1 -R < ../freetype/freetype.patch
cp ../freetype/btos-build.sh .
chmod +x btos-build.sh
mkdir -p install
./btos-build.sh
