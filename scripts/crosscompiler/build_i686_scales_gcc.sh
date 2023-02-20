#!/bin/bash

set -e

PREFIX=/usr/local/cross
TARGET=i686-scales
export PATH="$PREFIX/bin:$PATH"

ROOT=$(realpath ../..)
SYSROOT=$ROOT/sysroot

if [ ! -d "hosted" ]; then
	mkdir hosted
fi

cd hosted

if [ ! -d "binutils-gdb" ]; then
	git clone https://sourceware.org/git/binutils-gdb.git
fi

cd binutils-gdb

git fetch origin
git reset --hard origin/master
git checkout binutils-2_39
rm -rf ld/emulparams/*_scales.sh || 1
git apply $ROOT/toolchain/binutils.patch
cd  ld
automake-1.15

cd ../..

if [ ! -d "binutils-build" ]; then
	mkdir binutils-build
	cd binutils-build
	../binutils-gdb/configure --target="$TARGET" --prefix="$PREFIX" --with-sysroot="$SYSROOT" --disable-werror
	make
	sudo -u root make install
	cd ..
else
	echo "Skipping binutils build..."
fi

if [ ! -d "gcc" ]; then
	git clone https://gcc.gnu.org/git/gcc.git 
fi

cd gcc

git fetch origin
git reset --hard origin/master
git checkout -f releases/gcc-12.2.0
rm -rf gcc/config/scales.h
git apply $ROOT/toolchain/gcc.patch
cd libstdc++-v3
autoconf

cd ../..

if [ ! -d "gcc-build" ]; then
	mkdir gcc-build
	cd gcc-build
	../gcc/configure --target="$TARGET" --prefix="$PREFIX" --with-sysroot="$SYSROOT" --enable-languages=c,c++
	make all-gcc all-target-libgcc
	sudo -u root make install-gcc install-target-libgcc
else
	echo "Skipping gcc build..."
fi
