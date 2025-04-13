AC_PREREQ([2.71])
AC_INIT([McOsu-ng], [33.11], [william@horvath.blog], [], [https://github.com/whrvt/McOsu-ng]) # don't report bugs to McKay :)
AC_CONFIG_SRCDIR([src/Engine/Engine.cpp])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_HEADERS([src/config.h])
AC_PREFIX_DEFAULT([dist]) # default `make install`s to ./dist/bin/ (modify with --prefix=)

# no-define needed to avoid setting e.g. VERSION which conflicts with McEngine stuff
# dist-xz for `make dist` tarball target
AM_INIT_AUTOMAKE([1.16 foreign subdir-objects no-define dist-xz parallel-tests])
AM_SILENT_RULES([yes])

AC_USE_SYSTEM_EXTENSIONS
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

# needed for dep resolver
AC_PROG_WGET
AC_PROG_GIT
AC_PROG_TAR

AC_ARG_ENABLE([optimize],
    AS_HELP_STRING([--enable-optimize], [Optimize for speed (default: yes)]))
AC_ARG_ENABLE([native],
    AS_HELP_STRING([--enable-native], [Compile for -march=native (default: yes)]))
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debug build (default: no)]))
AC_ARG_ENABLE([clang],
    AS_HELP_STRING([--enable-clang], [Use clang/clang++ instead of gcc/g++ (default: no)]))
AC_ARG_ENABLE([lto],
    AS_HELP_STRING([--enable-lto], [Enable Link Time Optimization (default: auto, use if functional)]))
AC_ARG_ENABLE([system-deps],
    AS_HELP_STRING([--disable-system-deps], [Always build dependencies from source (default: no)]))
AC_ARG_ENABLE([static],
    AS_HELP_STRING([--enable-static], [Enable static linking where possible (default: no)]))

PKG_PROG_PKG_CONFIG

AS_IF([test "x$enable_static" = "xyes"], [
    PKGTYPE="--static"
])
PKG_CONFIG="$PKG_CONFIG ${PKGTYPE:-}"

AC_SUBST([PKGTYPE])
AC_SUBST([PKG_CONFIG])

AC_CHECK_HEADERS([X11/Xlib.h GL/gl.h GL/glu.h], [],
    [AC_MSG_ERROR([Missing required X11/OpenGL development headers])])

AS_IF([test "x$enable_clang" = "xyes"], [
    CC=clang
    CXX=clang++
    AC_PROG_CC([clang gcc cc])
    AC_PROG_CXX([clang++ g++ c++])
],[
    AC_PROG_CC
    AC_PROG_CXX
    CXXFLAGS="$CXXFLAGS -Wno-sign-compare"
])

# use c/c++17 because we can
AC_LANG([C++])
AC_LANG_PUSH(C++)
AX_CHECK_COMPILE_FLAG([-std=gnu++23], [
    CXXFLAGS="$CXXFLAGS -std=gnu++23"
], [
    AC_MSG_ERROR([Compiler does not support gnu++23])
])

AC_LANG([C])
AX_CHECK_COMPILE_FLAG([-std=gnu23], [
    CFLAGS="$CFLAGS -std=gnu23"
], [
    AC_MSG_ERROR([Compiler does not support gnu23])
])

AC_LANG_POP

AC_CHECK_PROG([BEAR], [bear], [bear])
AC_SUBST([BEAR])

AC_CHECK_PROG([CCACHE], [ccache], [ccache])
AS_IF([test "x$CCACHE" = "xccache"], [
    CC_NOCCACHE="$CC"
    CXX_NOCCACHE="$CXX"
    AC_SUBST([CC_NOCCACHE])
    AC_SUBST([CXX_NOCCACHE])
    CC="ccache $CC"
    CXX="ccache $CXX"
])

AS_IF([test "x$enable_native" != "xno"], [
    CXXFLAGS="$CXXFLAGS -march=native -mtune=native"
    CFLAGS="$CFLAGS -march=native -mtune=native"
])

AS_IF([test "x$enable_debug" = "xyes"], [
    CXXFLAGS="$CXXFLAGS -ggdb3 -gdwarf-5 -Og -fasynchronous-unwind-tables -fsanitize=address,undefined"
    CFLAGS="$CFLAGS -ggdb3 -gdwarf-5 -Og -fasynchronous-unwind-tables -fsanitize=address,undefined"
    ENABLE_DEBUG=$enable_debug
    AC_SUBST([ENABLE_DEBUG])
], [
    AS_IF([test "x$enable_optimization" != "xno"], [
        CPPFLAGS="$CPPFLAGS -DNDEBUG -D_NDEBUG"
        CXXFLAGS="${CXXFLAGS//-g -O2/} -O3 -g0" # where do those -g -O2 flags even come from...
        CFLAGS="${CFLAGS//-g -O2/} -O3 -g0"
    ])
    LDFLAGS="$LDFLAGS -Wl,-s,--gc-sections" # strip and GC unless debug
])

AS_IF([test "x$enable_lto" != "xno"], [
    AX_CHECK_COMPILE_FLAG([-flto=thin], [
        AX_CHECK_LINK_FLAG([-fuse-ld=lld -flto=thin -fno-fat-lto-objects -fwhole-program-vtables], [
            CXXFLAGS="$CXXFLAGS -flto=thin -fno-fat-lto-objects -fwhole-program-vtables"
            CFLAGS="$CFLAGS -flto=thin -fno-fat-lto-objects -fwhole-program-vtables"
            LDFLAGS="$LDFLAGS -fuse-ld=lld -flto=thin -fno-fat-lto-objects -fwhole-program-vtables -Wl,--lto-whole-program-visibility"
        ])], [
        # gcc
        AX_CHECK_LINK_FLAG([-flto=auto -ffat-lto-objects -fuse-linker-plugin], [
            CXXFLAGS="$CXXFLAGS -flto=auto -ffat-lto-objects -fuse-linker-plugin"
            CFLAGS="$CFLAGS -flto=auto -ffat-lto-objects -fuse-linker-plugin"
            LDFLAGS="$LDFLAGS -flto=auto -ffat-lto-objects -fuse-linker-plugin"
        ])
    ])
])

COMMON_FLAGS="-mcmodel=small -ffunction-sections -fdata-sections -fwrapv -fno-strict-aliasing -fno-semantic-interposition"
CXXFLAGS="$CXXFLAGS $COMMON_FLAGS"
CFLAGS="$CFLAGS $COMMON_FLAGS"
LDFLAGS="$LDFLAGS -static-libstdc++ -static-libgcc"

MCOSU_CXXFLAGS="-Wall"
AC_SUBST([MCOSU_CXXFLAGS])

AC_DEFUN([PKG_CHECK_AUTO], [
    AS_IF([test "x$enable_static" = "xyes"],
        [PKG_CHECK_MODULES_STATIC([$1], [$2], [$3], [$4])],
        [PKG_CHECK_MODULES([$1], [$2], [$3], [$4])])
])

AC_SUBST([DEPS_PREFIX], ['${abs_top_builddir}/build/deps'])
AC_SUBST([DEPS_CACHE], ['${abs_top_builddir}/build/cache'])

$MKDIR_P build/deps

PKG_CHECK_AUTO([X11], [x11], [have_x11=yes], [have_x11=no])
PKG_CHECK_AUTO([XI], [xi], [have_xi=yes], [have_xi=no])  # required for noSDL (TODO: ./configure --disable-sdl)
PKG_CHECK_AUTO([GL], [gl], [have_gl=yes], [have_gl=no])
PKG_CHECK_AUTO([GLU], [glu], [have_glu=yes], [have_glu=no])
PKG_CHECK_AUTO([EGL], [egl], [have_egl=yes], [have_egl=no])  # FIXME: transitive GLEW dep

SDL3_MIN_VERSION=3.2.0
SDL3_BUILD_VERSION=3.99.99 # git
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

AC_SUBST([MIMALLOC_VERSION], [3.0.3])
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
])

AS_IF([test "x$have_sdl3" != "xyes"], [have_sdl3="build from source"])
SDL3_VERSION=$SDL3_BUILD_VERSION
SDL3_PREFIX="${DEPS_PREFIX}"
SDL3_STATIC=$enable_static
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
    AC_DEFUN([AC_PROG_NASM], [
        AC_CHECK_PROG([NASM], [nasm], [nasm])
        if test -z "$NASM"; then
            AC_MSG_ERROR([nasm not found - required for building libjpeg])
        fi
    ])
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

AM_CONDITIONAL([BUILD_SDL3], [test "x$have_sdl3" != "xyes"])
AM_CONDITIONAL([BUILD_GLEW], [test "x$have_glew" != "xyes"])
AM_CONDITIONAL([BUILD_LIBJPEG], [test "x$have_libjpeg" != "xyes"])
AM_CONDITIONAL([BUILD_LIBPNG], [test "x$have_libpng" != "xyes"])
AM_CONDITIONAL([BUILD_BZIP2], [test "x$have_bzip2" != "xyes"])
AM_CONDITIONAL([BUILD_ZLIB], [test "x$have_zlib" != "xyes"])
AM_CONDITIONAL([BUILD_FREETYPE2], [test "x$have_freetype2" != "xyes"])
AM_CONDITIONAL([BUILD_MIMALLOC], [true])

BUNDLED_LIBS_DIR=libraries
BUNDLED_BASS_DIR=$BUNDLED_LIBS_DIR/bass
BUNDLED_BASS_FX_DIR=$BUNDLED_LIBS_DIR/bassfx
AC_SUBST([BUNDLED_LIBS_DIR])
AC_SUBST([BUNDLED_BASS_DIR])
AC_SUBST([BUNDLED_BASS_FX_DIR])

ASSETS_DIR=assets
AC_SUBST([ASSETS_DIR])

# sanity check
AC_CHECK_FILE([$srcdir/$BUNDLED_BASS_DIR/lib/linux/libbass.so],
    [],
    [AC_MSG_ERROR([Required bundled library libbass.so not found])])
AC_CHECK_FILE([$srcdir/$BUNDLED_BASS_FX_DIR/lib/linux/libbass_fx.so],
    [],
    [AC_MSG_ERROR([Required bundled library libbass_fx.so not found])])

AC_CHECK_FILE([$srcdir/$ASSETS_DIR/models],
    [],
    [AC_MSG_ERROR([McOsu-ng model assets are missing!])])
AC_CHECK_FILE([$srcdir/$ASSETS_DIR/shaders],
    [],
    [AC_MSG_ERROR([McOsu-ng shader assets are missing!])])

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
    Install prefix:     ${prefix}
    Preprocessor flags: ${CPPFLAGS}
    ccache:             ${CCACHE-no}
    C++ compiler:       ${CXX}
    C++ flags:          ${CXXFLAGS}
    C compiler:         ${CC}
    C flags:            ${CFLAGS}
    Linker flags:       ${LDFLAGS}
    LTO:                ${enable_lto-auto}
    Debug build:        ${enable_debug-no}
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
])
