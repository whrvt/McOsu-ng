AC_LANG([C++])
AC_PREREQ([2.69])
AC_INIT([McOsu-ng], [34.00], [william@horvath.blog], [], [https://github.com/whrvt/McOsu-ng]) # don't report bugs to McKay :)
AC_CONFIG_SRCDIR([src/Engine/Engine.cpp])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_HEADERS([src/config.h])
AC_PREFIX_DEFAULT([dist]) # default `make install`s to ./dist/bin/ (modify with --prefix=)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_USE_SYSTEM_EXTENSIONS

AC_DEFUN([AX_COUNT_CPUS], [
    AC_CHECK_PROG([GETCONF], [getconf], [getconf])
    AS_IF([test -n "$GETCONF"],
        [n_cpus=$($GETCONF _NPROCESSORS_ONLN 2>/dev/null)],
        [n_cpus=1])
    AS_IF([test -z "$n_cpus" -o "$n_cpus" -le "0"], [n_cpus=1])
    AC_SUBST([CPU_COUNT], [$n_cpus])
])
AX_COUNT_CPUS

export MAKEFLAGS="-j$CPU_COUNT"
AC_SUBST([MAKEFLAGS])

# no-define needed to avoid setting e.g. VERSION which conflicts with McEngine stuff
# dist-xz for `make dist` tarball target
AM_INIT_AUTOMAKE([1.16 foreign subdir-objects no-define dist-xz parallel-tests])
AM_SILENT_RULES([yes])

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_GREP
AC_PROG_SED

AC_DEFUN([AC_PROG_WGET], [
    AC_CHECK_PROG([WGET], [wget], [wget])
    if test -z "$WGET"; then
        AC_MSG_ERROR([wget not found - required for downloading dependencies])
    fi
])
AC_DEFUN([AC_PROG_GIT], [
    AC_CHECK_PROG([GIT], [git], [git])
    if test -z "$GIT"; then
        AC_MSG_ERROR([git not found - required for downloading dependencies])
    fi
])
AC_DEFUN([AC_PROG_TAR], [
    AC_CHECK_PROG([TAR], [tar], [tar])
    if test -z "$TAR"; then
        AC_MSG_ERROR([tar not found - required for extracting dependencies])
    fi
])
AC_DEFUN([AC_PROG_PKGCONF], [
    AC_CHECK_PROG([PKGCONF], [pkgconf], [pkgconf])
    if test -z "$PKGCONF"; then
        AC_MSG_ERROR([pkgconf not found - required for finding include/library paths])
    fi
])
AC_DEFUN([AC_PROG_CMAKE], [
    AC_CHECK_PROG([CMAKE], [cmake], [cmake])
    if test -z "$CMAKE"; then
        AC_MSG_ERROR([cmake not found - required for building dependencies])
    fi
])
AC_DEFUN([AC_PROG_MESON], [
    AC_CHECK_PROG([MESON], [meson], [meson])
    if test -z "$MESON"; then
        AC_MSG_ERROR([meson not found - required for building dependencies])
    fi
])
AC_DEFUN([AC_PROG_NINJA], [
    AC_CHECK_PROG([NINJA], [ninja], [ninja])
    if test -z "$NINJA"; then
        AC_MSG_ERROR([ninja not found - required for building dependencies])
    fi
])
# needed for building SoLoud (if enabled)
AC_DEFUN([AC_PROG_GENIE], [
    AC_CHECK_PROG([GENIE], [genie], [genie])
    if test -z "$GENIE"; then
        AC_MSG_ERROR([GENie (https://github.com/bkaradzic/GENie) not found - required for building SoLoud])
    fi
])
# needed for building libjpeg (if building it is required)
AC_DEFUN([AC_PROG_NASM], [
    AC_CHECK_PROG([NASM], [nasm], [nasm])
    if test -z "$NASM"; then
        AC_MSG_ERROR([nasm not found - required for building libjpeg])
    fi
])

# needed for dep resolver
AC_PROG_WGET
AC_PROG_GIT
AC_PROG_TAR
AC_PROG_PKGCONF
AC_PROG_NINJA
AC_PROG_MESON
AC_PROG_CMAKE

AC_ARG_ENABLE([optimize],
    AS_HELP_STRING([--enable-optimize], [Optimize for speed (default: yes)]))
AC_ARG_ENABLE([native],
    AS_HELP_STRING([--enable-native], [Compile for -march=native (default: yes)]))
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debug build (default: no)]))
AC_ARG_ENABLE([asan],
    AS_HELP_STRING([--enable-asan], [Enable ASan/UBSan (turns debug on as well) (default: no)]))
AC_ARG_ENABLE([clang],
    AS_HELP_STRING([--enable-clang], [Use clang/clang++ instead of gcc/g++ (default: no)]))
AC_ARG_ENABLE([lto],
    AS_HELP_STRING([--enable-lto], [Enable Link Time Optimization (default: auto, use if functional)]))
AC_ARG_ENABLE([system-deps],
    AS_HELP_STRING([--disable-system-deps], [Always build dependencies from source (default: no)]))
AC_ARG_ENABLE([static],
    AS_HELP_STRING([--enable-static], [Enable static linking where possible (default: no)]))

AC_ARG_WITH([sdlmixer],
    AS_HELP_STRING([--with-sdlmixer], [Use the SDL_mixer backend for audio instead of BASS (default: no)]))
AC_ARG_WITH([soloud],
    AS_HELP_STRING([--with-soloud], [Use the SoLoud backend for audio instead of BASS (default: no)]))
AC_ARG_WITH([mimalloc],
    AS_HELP_STRING([--with-mimalloc], [Use mimalloc instead of the default memory allocator (faster) (default: on if not debug)]))

AS_IF([test "x$with_sdlmixer" = "xyes" && test "x$with_soloud" = "xyes"], [
    AC_MSG_ERROR([Only one of --with-sdlmixer or --with-soloud can be used at once!])
])

AS_IF([test "x$with_soloud" = "xyes"], [
    AC_PROG_GENIE
])

AS_IF([test "$host_os" = "mingw32"], [
    # its too much of a headache to cross-build shared with host libs
    AS_IF([test "x$enable_static" != "xno"], [
        enable_static=yes
        disable_shared=yes
    ])
    enable_system_deps=no
    disable_system_deps=yes
    MCOSU_CPPFLAGS="$MCOSU_CPPFLAGS -D_UNICODE -DUNICODE"
    MCOSU_CXXFLAGS="$MCOSU_CXXFLAGS -D_UNICODE -DUNICODE -municode -mwindows"
    MCOSU_LDFLAGS="$MCOSU_LDFLAGS -municode -mwindows"
    AS_IF([test "x$enable_clang" = "xyes"], [
        LLVMMINGW=1
        AC_SUBST([LLVMMINGW])
        CXXFLAGS="$CXXFLAGS --target=$host_cpu-w64-windows-gnu"
        CFLAGS="$CFLAGS --target=$host_cpu-w64-windows-gnu"
    ])
])

AS_IF([test "x$enable_static" = "xyes"], [
    PKGTYPE="--static"
])

AC_SUBST([PKGTYPE])
ac_cv_env_PKG_CONFIG_value="$PKG_CONFIG"
ac_cv_env_PKG_CONFIG_set="set"

PKG_CONFIG_PATH="$PWD/build/deps/lib/pkgconfig:$PWD/build/deps/lib64/pkgconfig"
PKG_CONFIG_PATH_CUSTOM="$PKG_CONFIG_PATH"
AC_SUBST([PKG_CONFIG_PATH])
AC_SUBST([PKG_CONFIG_PATH_CUSTOM])

mkdir -p build/deps

AS_IF([test "x$enable_clang" = "xyes"], [
    CC=clang
    CXX=clang++
    CXXFLAGS="$CXXFLAGS -Qunused-arguments -fuse-ld=lld"
    CFLAGS="$CFLAGS -Qunused-arguments -fuse-ld=lld"
    LDFLAGS="$LDFLAGS -fuse-ld=lld"
],[
    CC=gcc
    CXX=g++
    MCOSU_CXXFLAGS="$MCOSU_CXXFLAGS -fimplicit-constexpr"
])

AS_IF([[case "$host_cpu" in
  i[[3456789]]86*) host_cpu=i686 ;;
esac]])

AS_IF([test "$host_os" = "linux-gnu"], [
    AS_IF([test "$host_cpu" = "i686"], [
        install -Dm755 /dev/stdin $PWD/build/$target-$CC <<EOF
#!/usr/bin/env sh
$CC -m32 "\$@"
EOF
        install -Dm755 /dev/stdin $PWD/build/$target-$CXX <<EOF
#!/usr/bin/env sh
$CXX -m32 "\$@"
EOF
        [[ "x$enable_clang" = "xyes" ]] && MCOSU_LDFLAGS="$MCOSU_LDFLAGS -latomic"
        AC_SUBST([NASMOBJ],[elf32])
    ],[
        install -Dm755 /dev/stdin $PWD/build/$target-$CC <<EOF
#!/usr/bin/env sh
$CC "\$@"
EOF
        install -Dm755 /dev/stdin $PWD/build/$target-$CXX <<EOF
#!/usr/bin/env sh
$CXX "\$@"
EOF
        AC_SUBST([NASMOBJ],[elf64])
    ])

    install -Dm755 /dev/stdin $PWD/build/pkg-config <<__EOF__
#!/usr/bin/env sh
PKG_CONFIG_PATH="$PWD/build/deps/lib/pkgconfig:$PWD/build/deps/lib64/pkgconfig" $PKGCONF --personality=$host_cpu-pc-linux-gnu "$PKGTYPE" "\$@"
__EOF__

    CC="$PWD/build/$target-$CC"
    CXX="$PWD/build/$target-$CXX"
    ac_tool_prefix=
    CMAKE_SYSTEM_NAME="Linux"
    SYSNAME="linux"
    DLLEXT=".so"
    LIBPREFIX="lib"

    AC_DEFINE([MCENGINE_PLATFORM_LINUX], [1], [Define if compiling for Linux.])
],[
    # TODO: don't hardcode mingw paths here
    AS_IF([test "$build_os" = "linux-gnu"], [
        AC_SUBST([PKG_CONFIG_SYSTEM_LIBRARY_PATH],["/usr/$host_cpu-w64-mingw32/lib"])
        AC_SUBST([PKG_CONFIG_SYSTEM_INCLUDE_PATH],["/usr/$host_cpu-w64-mingw32/include"])
        install -Dm755 /dev/stdin $PWD/build/pkg-config <<__EOF__
#!/usr/bin/env sh
PKG_CONFIG_LIBDIR="/usr/$host_cpu-w64-mingw32/lib/pkgconfig" PKG_CONFIG_SYSTEM_INCLUDE_PATH="/usr/$host_cpu-w64-mingw32/include" \
PKG_CONFIG_SYSTEM_LIBRARY_PATH="/usr/$host_cpu-w64-mingw32/lib" PKG_CONFIG_PATH="$PWD/build/deps/lib/pkgconfig:$PWD/build/deps/lib64/pkgconfig" \
$PKGCONF --personality=$host_cpu-w64-mingw32 $PKGTYPE "\$@"
__EOF__
    ], [
        # WIP: building in msys2
        install -Dm755 /dev/stdin $PWD/build/pkg-config <<__EOF__
#!/usr/bin/env sh
PKG_CONFIG_PATH="$PWD/build/deps/lib/pkgconfig:$PWD/build/deps/lib64/pkgconfig" \
$host_cpu-w64-mingw32-pkg-config $PKGTYPE "\$@"
__EOF__
    ])

    CMAKE_SYSTEM_NAME="Windows"
    CMAKERCOPT="-DCMAKE_RC_COMPILER=$host_cpu-w64-mingw32-windres"
    SYSNAME="windows"
    DLLEXT=".dll"
    LIBPREFIX=""

    AS_IF([test "$host_cpu" = "i686"], [ # uber compatibility configuration, works on winxp
        AS_IF([test "x$enable_native" != "xyes"], [ # compatibility, disable unless explicitly requested
            enable_native=no
            disable_native=yes
        ])
        ubercompat=on
        with_mimalloc="no"
        LDFLAGS="$LDFLAGS -m32 -lwinpthread"
        # winxp
        CPPFLAGS="$CPPFLAGS -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32_WINDOWS=0x0501 -D_WIN32_IE=0x0501"
        CFLAGS="$CFLAGS -m32 -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32_WINDOWS=0x0501 -D_WIN32_IE=0x0501 -mfpmath=sse"
        CXXFLAGS="$CXXFLAGS -m32 -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32_WINDOWS=0x0501 -D_WIN32_IE=0x0501 -mfpmath=sse"
        # won't work unfortunately, https://github.com/mstorsjo/llvm-mingw/issues/44
        # but i tried
        AS_IF([test "x$enable_clang" = "xyes"], [LDFLAGS="$LDFLAGS -Xlinker /subsystem:windows,5.0"])
        AC_SUBST([NASMOBJ],[win32])
    ], [
        # win10
        CPPFLAGS="$CPPFLAGS -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_WIN32_WINDOWS=0x0A00 -D_WIN32_IE=0x0A00"
        CFLAGS="$CFLAGS -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_WIN32_WINDOWS=0x0A00 -D_WIN32_IE=0x0A00"
        CXXFLAGS="$CXXFLAGS -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_WIN32_WINDOWS=0x0A00 -D_WIN32_IE=0x0A00"
        LDFLAGS="$LDFLAGS -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic"
        AS_IF([test "x$enable_clang" = "xyes"], [LDFLAGS="$LDFLAGS -Xlinker /subsystem:windows,10.0"])
        AC_SUBST([NASMOBJ],[win64])
    ])

    AC_DEFINE([MCENGINE_PLATFORM_WINDOWS], [1], [Define if compiling for Windows.])
])

PKG_CONFIG="$PWD/build/pkg-config"
AC_SUBST([PKG_CONFIG])
AC_SUBST([CMAKE_SYSTEM_NAME])
AC_SUBST([CMAKERCOPT])
AC_SUBST([SYSNAME])
AC_SUBST([DLLEXT])
AC_SUBST([LIBPREFIX])

AC_PROG_CC([$CC])
AS_IF([[case "$CC" in
  *clang) CXX=$CC++ ;;
  *gcc) CXX=${CC%gcc}g++ ;;
esac]])
AC_PROG_CXX([$CXX])

AC_CHECK_PROG([CCACHE], [ccache], [ccache])

install -Dm755 /dev/stdin $PWD/build/cc <<EOF
#!/usr/bin/env sh
$CCACHE $CC "\$@"
EOF
install -Dm755 /dev/stdin $PWD/build/cxx <<EOF
#!/usr/bin/env sh
$CCACHE $CXX "\$@"
EOF

export CC="$PWD/build/cc"
export CXX="$PWD/build/cxx"

AC_SUBST([CC])
AC_SUBST([CXX])

AS_IF([test "$host_cpu" = "x86_64"], [COMMON_FLAGS="$COMMON_FLAGS -mcmodel=small"])

COMMON_FLAGS="$COMMON_FLAGS -ffunction-sections -fdata-sections -fwrapv -fno-strict-aliasing -fno-semantic-interposition"
CXXFLAGS="$CXXFLAGS $COMMON_FLAGS"
CFLAGS="$CFLAGS $COMMON_FLAGS"
AS_IF([test -z "$ubercompat" || test "x$enable_static" = "xyes"], [
    LDFLAGS="$LDFLAGS -static-libstdc++ -static-libgcc"
])

AC_LANG_PUSH([C++])
AS_IF([test "x$enable_native" != "xno"], [
    CXXFLAGS="$CXXFLAGS -march=native -mtune=native"
    CFLAGS="$CFLAGS -march=native -mtune=native"
],[
    AS_IF([test "$host_cpu" = "i686"], [
        CXXFLAGS="$CXXFLAGS -march=pentium4 -mtune=generic"
        CFLAGS="$CFLAGS -march=pentium4 -mtune=generic"
    ], [
        CXXFLAGS="$CXXFLAGS -march=${host_cpu//_/-} -mtune=generic"
        CFLAGS="$CFLAGS -march=${host_cpu//_/-} -mtune=generic"
    ])

    AC_SUBST([DISABLE_NATIVE],[1])
])
# use c/c++23 because we can
AX_CHECK_COMPILE_FLAG([-std=gnu++23], [
    AX_CHECK_COMPILE_FLAG([-std=gnu++26], [
        MCOSU_CXXFLAGS="$MCOSU_CXXFLAGS -std=gnu++26"
        ], [
        MCOSU_CXXFLAGS="$MCOSU_CXXFLAGS -std=gnu++23"
    ])
], [
    AC_MSG_ERROR([Compiler does not support gnu++23])
])

AC_CHECK_TOOL([LD], [ld], [AC_MSG_ERROR([ld not found])])
AS_IF([test "x$enable_clang" = "xyes" && test "$host_os" = "linux-gnu"], [
    AC_CHECK_PROG([AR], [llvm-ar], [llvm-ar], [AC_MSG_ERROR([llvm-ar not found])])
    AC_CHECK_PROG([RANLIB], [llvm-ranlib], [llvm-ranlib], [AC_MSG_ERROR([llvm-ranlib not found])])
], [
    AC_CHECK_TOOL([AR], [ar], [AC_MSG_ERROR([ar not found])])
    AC_CHECK_TOOL([RANLIB], [ranlib], [AC_MSG_ERROR([ranlib not found])])
])

AS_IF([test "$host_os" = "linux-gnu"],[
    AC_CHECK_HEADERS([X11/Xlib.h GL/gl.h GL/glu.h], [],
        [AC_MSG_ERROR([Missing required X11/OpenGL development headers])])
])

AC_CHECK_PROG([BEAR], [bear], [bear])
AC_SUBST([BEAR])

AS_IF([test "x$enable_asan" = "xyes"], [
    CXXFLAGS="$CXXFLAGS -fsanitize=address,undefined"
    CFLAGS="$CFLAGS -fsanitize=address,undefined"
    AS_IF([test -z "$LLVMMINGW"],[
        LDFLAGS="$LDFLAGS -static-libasan" CFLAGS="$CFLAGS -static-libasan" CXXFLAGS="$CXXFLAGS -static-libasan"
    ])
    enable_debug="yes"
    ENABLE_ASAN=$enable_asan
    AC_SUBST([ENABLE_ASAN])
])

AS_IF([test "x$enable_debug" = "xyes"], [
    with_mimalloc="no"
    AS_IF([test -n "$ubercompat"],[
        CXXFLAGS="$CXXFLAGS -gdwarf-4 -Og"
        CFLAGS="$CFLAGS -gdwarf-4 -Og"
    ], [
        CXXFLAGS="$CXXFLAGS -ggdb3 -gdwarf-5 -Og -fasynchronous-unwind-tables"
        CFLAGS="$CFLAGS -ggdb3 -gdwarf-5 -Og -fasynchronous-unwind-tables"
    ])
    ENABLE_DEBUG=$enable_debug
    MCOSU_CPPFLAGS="$MCOSU_CPPFLAGS -D_DEBUG"
    AC_SUBST([ENABLE_DEBUG])
], [
    AS_IF([test "x$enable_optimization" != "xno"], [
        CPPFLAGS="$CPPFLAGS -DNDEBUG -D_NDEBUG"
        CXXFLAGS="${CXXFLAGS//-g -O2/} -O3 -g0" # where do those -g -O2 flags even come from...
        CFLAGS="${CFLAGS//-g -O2/} -O3 -g0"
        AS_IF([test "x$enable_clang" = "xyes"], [
            CXXFLAGS="$CXXFLAGS"
            CFLAGS="$CFLAGS"
        ],[
            CXXFLAGS="$CXXFLAGS -fipa-pta"
            CFLAGS="$CFLAGS -fipa-pta"
        ])
    ])
    # MCOSU_CXXFLAGS="$MCOSU_CXXFLAGS -fno-rtti" # TODO: remove needless dynamic_casts everywhere
    MCOSU_LDFLAGS="$MCOSU_LDFLAGS -Wl,-s,--gc-sections" # strip and GC unless debug
])

AS_IF([test "x$enable_lto" != "xno"], [
    AX_CHECK_COMPILE_FLAG([-flto=thin], [
        AX_CHECK_LINK_FLAG([-flto=thin -fno-fat-lto-objects -fwhole-program-vtables], [
            CXXFLAGS="$CXXFLAGS -flto=thin -fno-fat-lto-objects -fwhole-program-vtables"
            CFLAGS="$CFLAGS -flto=thin -fno-fat-lto-objects -fwhole-program-vtables"
            LDFLAGS="$LDFLAGS -flto=thin -fno-fat-lto-objects -fwhole-program-vtables"
        ])], [
        # gcc
        AX_CHECK_LINK_FLAG([-flto=auto -ffat-lto-objects -fuse-linker-plugin -fdevirtualize-at-ltrans], [
            CXXFLAGS="$CXXFLAGS -flto=auto -ffat-lto-objects -fuse-linker-plugin -fdevirtualize-at-ltrans"
            CFLAGS="$CFLAGS -flto=auto -ffat-lto-objects -fuse-linker-plugin -fdevirtualize-at-ltrans"
            LDFLAGS="$LDFLAGS -flto=auto -ffat-lto-objects -fuse-linker-plugin -fdevirtualize-at-ltrans"
        ])
    ])
])

MCOSU_CPPFLAGS="$MCOSU_CPPFLAGS -DGLM_ENABLE_EXPERIMENTAL"
MCOSU_CXXFLAGS="$MCOSU_CXXFLAGS -Wall -Wno-sign-compare -Werror=deprecated-declarations -pthread"
MCOSU_LDFLAGS="$MCOSU_LDFLAGS -pthread"
AC_SUBST([MCOSU_CPPFLAGS])
AC_SUBST([MCOSU_CXXFLAGS])
AC_SUBST([MCOSU_LDFLAGS])

AC_DEFUN([PKG_CHECK_AUTO], [
    AS_IF([test "x$enable_static" = "xyes"],
        [PKG_CHECK_MODULES_STATIC([$1], [$2], [$3], [$4])],
        [PKG_CHECK_MODULES([$1], [$2], [$3], [$4])])
])

AC_SUBST([DEPS_PREFIX], ['${abs_top_builddir}/build/deps'])
AC_SUBST([DEPS_CACHE], ['${abs_top_srcdir}/build/cache'])
AC_SUBST([bindir], [$bindir-$host_cpu])

AS_IF([test "$SYSNAME" = "linux"], [
    PKG_CHECK_AUTO([X11], [x11], [have_x11=yes], [have_x11=no])
    PKG_CHECK_AUTO([XI], [xi], [have_xi=yes], [have_xi=no])  # required for noSDL (TODO: ./configure --disable-sdl)
    PKG_CHECK_AUTO([EGL], [egl], [have_egl=yes], [have_egl=no])  # FIXME: transitive GLEW dep
])

PKG_CHECK_AUTO([GL], [gl], [have_gl=yes], [have_gl=no])
PKG_CHECK_AUTO([GLU], [glu], [have_glu=yes], [have_glu=no])

SDL3_MIN_VERSION=3.4.0 # needs SDL_SetRelativeMouseTransform, not in any current (3.2.x) releases
SDL3_BUILD_VERSION=514d96de07c3b1b8603ee4701229bb055a1dd2e3 # git
GLEW_MIN_VERSION=2.2.0
GLEW_BUILD_VERSION=2.2.0
FREETYPE2_MIN_VERSION=2.10.0
FREETYPE2_BUILD_VERSION=2.13.3
LIBJPEG_MIN_VERSION=3.0.0
LIBJPEG_BUILD_VERSION=3.1.0
LIBPNG_MIN_VERSION=1.6.38
LIBPNG_BUILD_VERSION=1.6.47
ZLIB_MIN_VERSION=1.3
ZLIB_BUILD_VERSION=2.2.4 # zlib-ng
BZIP2_MIN_VERSION=1.0.8
BZIP2_BUILD_VERSION=1.0.8

# always build these from source if we want to use them
AC_SUBST([SOLOUD_VERSION], [e82fd32c1f62183922f08c14c814a02b58db1873])
AC_SUBST([SOLOUD_PREFIX],[$DEPS_PREFIX])
AC_SUBST([SOUNDTOUCH_VERSION], [2.4.0])
AC_SUBST([SOUNDTOUCH_PREFIX],[$DEPS_PREFIX])
AC_SUBST([SDL3_MIXER_VERSION], [b8017938de8b31c1962c2c8044a127519c2c1cdf])
AC_SUBST([SDL3_MIXER_PREFIX],[$DEPS_PREFIX])
AC_SUBST([MIMALLOC_VERSION], [bc18e9456ee3fa4449240250e5918175c1898712])
AC_SUBST([MIMALLOC_PREFIX],[$DEPS_PREFIX])

# TODO: add all deps and transitive deps (fun!)
AS_IF([test "x$enable_system_deps" != "xno"], [
    PKG_CHECK_AUTO([SDL3], [sdl3 >= $SDL3_MIN_VERSION], [have_sdl3=yes], [have_sdl3=no])
    PKG_CHECK_AUTO([GLEW], [glew >= $GLEW_MIN_VERSION], [have_glew=yes], [have_glew=no])
    PKG_CHECK_AUTO([LIBJPEG], [libjpeg >= $LIBJPEG_MIN_VERSION], [have_libjpeg=yes], [have_libjpeg=no])
    PKG_CHECK_AUTO([LIBPNG], [libpng >= $LIBPNG_MIN_VERSION], [have_libpng=yes], [have_libpng=no])
    PKG_CHECK_AUTO([ZLIB], [zlib >= $ZLIB_MIN_VERSION], [have_zlib=yes], [have_zlib=no])
    PKG_CHECK_AUTO([BZIP2], [bzip2 >= $BZIP2_MIN_VERSION], [have_bzip2=yes], [have_bzip2=no])
    PKG_CHECK_AUTO([FREETYPE2], [freetype2 >= $FREETYPE2_MIN_VERSION], [have_freetype2=yes], [have_freetype2=no])
    AC_CHECK_HEADERS([glm/vec3.hpp], [have_glm=yes], [have_glm=no])
])

AS_IF([test "x$have_sdl3" != "xyes"], [have_sdl3="build from source"])
SDL3_VERSION=$SDL3_BUILD_VERSION
SDL3_PREFIX="${DEPS_PREFIX}"
AS_IF([test "x$enable_static" = "xyes"], [
    SDL3_STATIC=1
])
AC_SUBST([SDL3_VERSION])
AC_SUBST([SDL3_PREFIX])
AC_SUBST([SDL3_STATIC])

AS_IF([test "x$have_glew" != "xyes"], [have_glew="build from source"])
GLEW_VERSION=$GLEW_BUILD_VERSION
GLEW_PREFIX="${DEPS_PREFIX}"
AC_SUBST([GLEW_VERSION])
AC_SUBST([GLEW_PREFIX])

AS_IF([test "x$have_libjpeg" != "xyes"], [
    have_libjpeg="build from source"
    AC_PROG_NASM
])

LIBJPEG_VERSION=$LIBJPEG_BUILD_VERSION
LIBJPEG_PREFIX="${DEPS_PREFIX}"
AC_SUBST([LIBJPEG_VERSION])
AC_SUBST([LIBJPEG_PREFIX])

AS_IF([test "x$have_libpng" != "xyes"], [have_libpng="build from source"])
LIBPNG_VERSION=$LIBPNG_BUILD_VERSION
LIBPNG_PREFIX="${DEPS_PREFIX}"
AC_SUBST([LIBPNG_VERSION])
AC_SUBST([LIBPNG_PREFIX])

AS_IF([test "x$have_bzip2" != "xyes"], [have_bzip2="build from source"])
BZIP2_VERSION=$BZIP2_BUILD_VERSION
BZIP2_PREFIX="${DEPS_PREFIX}"
AC_SUBST([BZIP2_VERSION])
AC_SUBST([BZIP2_PREFIX])

AS_IF([test "x$have_zlib" != "xyes"], [have_zlib="build from source"])
ZLIB_VERSION=$ZLIB_BUILD_VERSION
ZLIB_PREFIX="${DEPS_PREFIX}"
AC_SUBST([ZLIB_VERSION])
AC_SUBST([ZLIB_PREFIX])

AS_IF([test "x$have_freetype2" != "xyes"], [have_freetype2="build from source"])
FREETYPE2_VERSION=$FREETYPE2_BUILD_VERSION
FREETYPE2_PREFIX="${DEPS_PREFIX}"
AC_SUBST([FREETYPE2_VERSION])
AC_SUBST([FREETYPE2_PREFIX])

AS_IF([test "x$have_glm" != "xyes"], [have_glm="build from source"])
AC_SUBST([GLM_VERSION], [1.0.1])
AC_SUBST([GLM_PREFIX],[$DEPS_PREFIX])
AC_SUBST([GLM_CFLAGS], [""])

AM_CONDITIONAL([BUILD_SDL3], [test "x$have_sdl3" != "xyes"])
AM_CONDITIONAL([BUILD_GLEW], [test "x$have_glew" != "xyes"])
AM_CONDITIONAL([BUILD_LIBJPEG], [test "x$have_libjpeg" != "xyes"])
AM_CONDITIONAL([BUILD_LIBPNG], [test "x$have_libpng" != "xyes"])
AM_CONDITIONAL([BUILD_BZIP2], [test "x$have_bzip2" != "xyes"])
AM_CONDITIONAL([BUILD_ZLIB], [test "x$have_zlib" != "xyes"])
AM_CONDITIONAL([BUILD_FREETYPE2], [test "x$have_freetype2" != "xyes"])
AM_CONDITIONAL([BUILD_GLM], [test "x$have_glm" != "xyes"])
AM_CONDITIONAL([BUILD_MIMALLOC], [test "x$enable_debug" != "xyes" &&
                                { { test "$SYSNAME" = "windows" && test "x$with_mimalloc" = "xyes" ; } ||
                                  { test "$SYSNAME" != "windows" && test "x$with_mimalloc" != "xno" ; };
                                }]) # default off for windows, kinda buggy

AM_CONDITIONAL([USE_BASS], [test "x$with_sdlmixer" != "xyes" && test "x$with_soloud" != "xyes"])
AM_CONDITIONAL([USE_SDLMIXER], [test "x$with_sdlmixer" = "xyes"])
AM_CONDITIONAL([USE_SOLOUD], [test "x$with_soloud" = "xyes"])
AM_CONDITIONAL([WIN_PLATFORM], [test "$SYSNAME" = "windows"])
AM_CONDITIONAL([WIN32_PLATFORM], [test "$SYSNAME" = "windows" && test "$host_cpu" = "i686"]) # idk why this has issues with soloud + GENie

# need dynamic winpthread for old windows 32bit compat builds
AS_IF([test -n "$ubercompat"], [
    AS_IF([test "x$enable_static" = "xyes"], [ # still need winpthread dynamic, breaks otherwise for reasons i dont understand
        WINSHARED_DLLS="$($CC -print-prog-name=ld | sed -e 's|/bin/.*||' | tr -d '\n' | find -files0-from - \
            -iregex ".*$host_cpu"'.*libwinpthread-1.dll' \
            -printf '%h/%f ')"
    ], [
        WINSHARED_DLLS="$($CC -print-prog-name=ld | sed -e 's|/bin/.*||' | tr -d '\n' | find -files0-from - \
            '(' -iregex ".*$host_cpu"'.*libwinpthread-1.dll' -o -iregex ".*$host_cpu"'.*libgcc_.*.dll' -o -iregex ".*$host_cpu"'.*libstdc.*.dll' ')' \
            -printf '%h/%f ')"
    ])

    AC_SUBST([WINSHARED_DLLS])
])
AM_CONDITIONAL([WINSHARED_DLLS_NEEDED], [test -n "$WINSHARED_DLLS"])

ASSETS_DIR=assets
AC_SUBST([ASSETS_DIR])

BUNDLED_LIBS_DIR=libraries
BUNDLED_BASS_DIR=$BUNDLED_LIBS_DIR/bass
BUNDLED_BASS_FX_DIR=$BUNDLED_LIBS_DIR/bassfx

AC_DEFINE([MCENGINE_FEATURE_SDL], [1], [Define if the SDL environment McEngine backend is desired.])

# sanity check
AS_IF([test "x$with_sdlmixer" != "xyes" && test "x$with_soloud" != "xyes"], [
    AS_IF([test -f "$srcdir/$BUNDLED_BASS_DIR/lib/$SYSNAME/$host_cpu/${LIBPREFIX}bass$DLLEXT"], 
        [],
        [AC_MSG_ERROR([Required bundled library ${LIBPREFIX}bass$DLLEXT not found])])
    AS_IF([test -f "$srcdir/$BUNDLED_BASS_FX_DIR/lib/$SYSNAME/$host_cpu/${LIBPREFIX}bass_fx$DLLEXT"],
        [],
        [AC_MSG_ERROR([Required bundled library ${LIBPREFIX}bass_fx$DLLEXT not found])])
    BASS_CFLAGS="-I\$(srcdir)/\$(BUNDLED_BASS_DIR)/include -I\$(srcdir)/\$(BUNDLED_BASS_FX_DIR)/include"
    AC_DEFINE([MCENGINE_FEATURE_BASS], [1], [Define if the BASS audio backend is desired.])
], [
    BASS_CFLAGS=""
    AS_IF([test "x$with_sdlmixer" = "xyes"], [
        AC_DEFINE([MCENGINE_FEATURE_SDL_MIXER], [1], [Define if the SDL_mixer audio backend is desired.])
    ])
    AS_IF([test "x$with_soloud" = "xyes"], [
        AC_DEFINE([MCENGINE_FEATURE_SOLOUD], [1], [Define if the SoLoud audio backend is desired.])
    ])
])

AC_SUBST([BASS_CFLAGS])
AC_SUBST([BUNDLED_LIBS_DIR])
AC_SUBST([BUNDLED_BASS_DIR])
AC_SUBST([BUNDLED_BASS_FX_DIR])

AC_MSG_CHECKING([for default config in $srcdir/$ASSETS_DIR/cfg])
AS_IF([test -f "$srcdir/$ASSETS_DIR/cfg/override.cfg"], [
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([McOsu-ng default config is missing!])
])

AC_MSG_CHECKING([for models in $srcdir/$ASSETS_DIR/models])
AS_IF([test -d "$srcdir/$ASSETS_DIR/models"], [
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([McOsu-ng model assets are missing!])
])

AC_MSG_CHECKING([for shaders in $srcdir/$ASSETS_DIR/shaders])
AS_IF([test -d "$srcdir/$ASSETS_DIR/shaders"], [
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([McOsu-ng shader assets are missing!])
])

AC_MSG_CHECKING([for fonts in $srcdir/$ASSETS_DIR/fonts])
AS_IF([test -d "$srcdir/$ASSETS_DIR/fonts"], [
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([McOsu-ng fonts are missing!])
])

AC_MSG_CHECKING([for src/Makefile.sources])
AS_IF([test -f "$srcdir/src/Makefile.sources"], [
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([src/Makefile.sources not found. Run ./autogen.sh first.])
])

# output makefile
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

AC_MSG_NOTICE([
McOsu-ng build configuration:
    Build target:       ${target}
    Install prefix:     ${prefix}
    Preprocessor flags: ${CPPFLAGS}
    ccache:             ${CCACHE-no}
    C++ compiler:       ${CXX}
    C++ flags:          ${CXXFLAGS}
    C compiler:         ${CC}
    C flags:            ${CFLAGS}
    Linker flags:       ${LDFLAGS}
    mimalloc:           ${with_mimalloc-yes}
    SDL_mixer:          ${with_sdlmixer-no}
    SoLoud:             ${with_soloud-no}
    LTO:                ${enable_lto-auto}
    Debug build:        ${enable_debug-no}
    ASan build:         ${enable_asan-no}
    Native build:       ${enable_native-yes}
    System deps:        ${enable_system_deps-yes}
    Static linking:     ${enable_static-no}

Dependencies:
    libX11              ${have_x11-no}
    libXi (XInput)      ${have_xi-no}
    libGL               ${have_gl-no}
    libGLU              ${have_glu-no}
    libEGL              ${have_egl-no}
    sdl3:               ${have_sdl3-build from source}
    GLEW:               ${have_glew-build from source}
    libjpeg:            ${have_libjpeg-build from source}
    libpng:             ${have_libpng-build from source}
    bzip2:              ${have_bzip2-build from source}
    zlib:               ${have_zlib-build from source}
    freetype2:          ${have_freetype2-build from source}
    glm (math)          ${have_glm-build from source}
])
