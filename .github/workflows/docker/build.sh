#!/bin/bash
# Simple script to build McOsu-ng in Docker

# Setting up build directory..
cd /build
cp -r /src/* .

echo "Building for target: $HOST"
set -e

# Building..
autoreconf
./configure --disable-system-deps --enable-static --disable-native --with-audio="bass,soloud" --host=$HOST
make -j$(nproc) install

rm -f ./dist/bin-*/{McOsu,McEngine} # symlinks turn into copies in .zip files for GHA artifacts

echo "Done!"