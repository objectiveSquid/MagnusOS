echo WARNING! This script will install gcc-11.1.0 and bintuils-2.37 to $(pwd)/.build-toolchain/i686-elf
read -p "Do you want to continue? [Y/n] " continue_setup

case $continue_setup in
    [yY] )  echo "Running setup";
            ;;

    * )     echo "Quitting";
            exit;;
esac

export PREFIX="$(pwd)/.build-toolchain/i686-elf"
export TARGET="i686-elf"
export PATH="$PREFIX/bin:$PATH"
sudo apt-get update
sudo apt-get install gcc-multilib

rm -rf .build-toolchain
mkdir .build-toolchain
cd .build-toolchain

echo "Downloading the source code of gcc-11.1.0 and binutils-2.37 (These were just the versions used by the guide I was following ¯\_(ツ)_/¯)"
wget "https://ftp.gnu.org/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.xz" -O "gcc-11.1.0.tar.xz"
wget "https://ftp.gnu.org/gnu/binutils/binutils-2.37.tar.xz" -O "binutils-2.37.tar.xz"

echo "Extracting gcc-11.1.0 and binutils-2.37"
tar -xf gcc-11.1.0.tar.xz
tar -xf binutils-2.37.tar.xz

# Build BinUtils
mkdir binutils-build
cd binutils-build
../binutils-2.37/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werr
make -j $(nproc --all)
make install

cd ..
# Build GCC
cd gcc-11.1.0
./contrib/download_prerequisites
cd ..
mkdir gcc-build
cd gcc-build
../gcc-11.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make -j $(nproc --all) all-gcc
make -j $(nproc --all) all-target-libgcc
make install-gcc
make install-target-libgcc