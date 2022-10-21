export PREFIX=/usr/local/cross
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

if [ ! -d "binutils-2.39" ]; then
    wget https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz
    tar -xf binutils-2.39.tar.gz
fi

if [ ! -d "binutils-build" ]; then
    mkdir binutils-build
    cd binutils-build
    ../binutils-2.39/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make
    sudo -u root make install
    cd ..
fi

if [ ! -d "gcc-12.2.0" ]; then
    wget https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz
    tar -xf gcc-12.2.0.tar.gz
fi

if [ ! -d "gcc-build" ]; then
    mkdir gcc-build
    cd gcc-build
    ../gcc-12.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    make all-gcc
    make all-target-libgcc
    sudo -u root make install-gcc
    sudo -u root make install-target-libgcc
fi



