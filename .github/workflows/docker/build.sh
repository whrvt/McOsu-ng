#!/bin/bash
# Simple script to build McOsu-ng in Docker

echo "Building for target: $HOST"
set -e

# Building..
export MAKEFLAGS="-j$(nproc)"

./autogen.sh
cd build-out
../configure --disable-system-deps --enable-static --disable-native --with-audio="bass,soloud" --host=$HOST
make install

# symlinks turn into copies in .zip files for GHA artifacts, so we need to make a zip of the zip...
zip -r -y -8 bin.zip ./dist/bin-*/
rm -rf dist/*
mv bin.zip dist/

echo "Done!"
