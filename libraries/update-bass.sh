#!/bin/bash

# https://github.com/ppy/osu-framework/blob/eed788fd166540f7e219e1e48a36d0bf64f07cc4/osu.Framework.NativeLibs/update-bass.sh

# bass
curl -Lso bass.zip https://www.un4seen.com/stuff/bass.zip
unzip -qjo bass.zip bass.dll -d bass/lib/windows/i686
unzip -qjo bass.zip x64/bass.dll -d bass/lib/windows/x86_64
unzip -qjo bass.zip bass.h -d bass/include/

curl -Lso bass-linux.zip https://www.un4seen.com/stuff/bass-linux.zip
unzip -qjo bass-linux.zip x86/libbass.so -d bass/lib/linux/i686
unzip -qjo bass-linux.zip x86_64/libbass.so -d bass/lib/linux/x86_64

# bassfx
curl -Lso bass_fx.zip https://www.un4seen.com/stuff/bass_fx.zip
unzip -qjo bass_fx.zip bass_fx.dll -d bassfx/lib/windows/i686
unzip -qjo bass_fx.zip x64/bass_fx.dll -d bassfx/lib/windows/x86_64
unzip -qjo bass_fx.zip bass_fx.h -d bassfx/include/

curl -Lso bass_fx-linux.zip https://www.un4seen.com/stuff/bass_fx-linux.zip
unzip -qjo bass_fx-linux.zip x86/libbass_fx.so -d bassfx/lib/linux/i686
unzip -qjo bass_fx-linux.zip x86_64/libbass_fx.so -d bassfx/lib/linux/x86_64

# curl -Lso bass_fx-osx.zip https://www.un4seen.com/stuff/bass_fx-osx.zip
# unzip -qjo bass_fx-osx.zip libbass_fx.dylib -d runtimes/osx/native/

# CURRENTLY UNUSED
# # bassmix
# curl -Lso bassmix24.zip https://www.un4seen.com/stuff/bassmix.zip
# unzip -qjo bassmix24.zip x64/bassmix.dll -d runtimes/win-x64/native/
# unzip -qjo bassmix24.zip bassmix.dll -d runtimes/win-x86/native/

# curl -Lso bassmix24-linux.zip https://www.un4seen.com/stuff/bassmix-linux.zip
# unzip -qjo bassmix24-linux.zip x86/libbassmix.so -d bassmix/lib/linux/i686
# unzip -qjo bassmix24-linux.zip x86_64/libbassmix.so -d bassmix/lib/linux/x86_64

# curl -Lso bassmix24-osx.zip https://www.un4seen.com/stuff/bassmix-osx.zip
# unzip -qjo bassmix24-osx.zip libbassmix.dylib -d runtimes/osx/native/

# clean up
rm bass*.zip