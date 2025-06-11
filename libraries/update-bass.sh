#!/bin/bash

# https://github.com/ppy/osu-framework/blob/eed788fd166540f7e219e1e48a36d0bf64f07cc4/osu.Framework.NativeLibs/update-bass.sh

mkdir -p {bass,bassfx,bassflac,bassmix,basswasapi,bassasio}/include
mkdir -p {bass,bassfx,bassflac,bassmix}/lib/{windows,linux}/{x86_64,i686}
mkdir -p {basswasapi,bassasio}/lib/windows/{x86_64,i686}

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

# bassflac
curl -Lso bassflac24.zip https://www.un4seen.com/files/bassflac24.zip
unzip -qjo bassflac24.zip bassflac.dll -d bassflac/lib/windows/i686
unzip -qjo bassflac24.zip x64/bassflac.dll -d bassflac/lib/windows/x86_64
unzip -qjo bassflac24.zip c/bassflac.h -d bassflac/include/

curl -Lso bassflac24-linux.zip https://www.un4seen.com/files/bassflac24-linux.zip
unzip -qjo bassflac24-linux.zip libs/x86/libbassflac.so -d bassflac/lib/linux/i686
unzip -qjo bassflac24-linux.zip libs/x86_64/libbassflac.so -d bassflac/lib/linux/x86_64

# bassmix
curl -Lso bassmix24.zip https://www.un4seen.com/stuff/bassmix.zip
unzip -qjo bassmix24.zip bassmix.dll -d bassmix/lib/windows/i686
unzip -qjo bassmix24.zip x64/bassmix.dll -d bassmix/lib/windows/x86_64
unzip -qjo bassmix24.zip bassmix.h -d bassmix/include/

curl -Lso bassmix24-linux.zip https://www.un4seen.com/stuff/bassmix-linux.zip
unzip -qjo bassmix24-linux.zip x86/libbassmix.so -d bassmix/lib/linux/i686
unzip -qjo bassmix24-linux.zip x86_64/libbassmix.so -d bassmix/lib/linux/x86_64

# basswasapi (windows)
curl -Lso basswasapi24.zip https://www.un4seen.com/stuff/basswasapi.zip
curl -Lso basswasapi24-header.zip https://www.un4seen.com/files/basswasapi24.zip # the header isnt in the stuff version
unzip -qjo basswasapi24.zip basswasapi.dll -d basswasapi/lib/windows/i686
unzip -qjo basswasapi24.zip x64/basswasapi.dll -d basswasapi/lib/windows/x86_64
unzip -qjo basswasapi24-header.zip c/basswasapi.h -d basswasapi/include/

# bassasio (windows)
curl -Lso bassasio24.zip https://www.un4seen.com/stuff/bassasio.zip
unzip -qjo bassasio24.zip bassasio.dll -d bassasio/lib/windows/i686
unzip -qjo bassasio24.zip x64/bassasio.dll -d bassasio/lib/windows/x86_64
unzip -qjo bassasio24.zip bassasio.h -d bassasio/include/

# clean up
rm bass*.zip