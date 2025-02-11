bin_PROGRAMS = McOsu

# generated by ./autogen.sh
include $(top_srcdir)/src/Makefile.sources

CLEANFILES = \
	*.o \
	src/*/*.o \
	src/*/*/*.o \
	*~ \
	*.bak

# build rules for dependencies when system packages aren't used
#$(WGET) -qO- "https://github.com/libsdl-org/SDL/releases/download/preview-$(SDL3_VERSION)/SDL3-$(SDL3_VERSION).tar.gz" | $(TAR) xz -C build/deps/SDL3 --strip-components=1
build/deps/SDL3.built:
	$(MKDIR_P) build/deps build/deps/lib/pkgconfig
	$(GIT) clone https://github.com/libsdl-org/SDL build/deps/SDL3
	$(MKDIR_P) build/deps/SDL3/build
	cd build/deps/SDL3 && \
	cmake -S . -B build -G Ninja \
	-D CMAKE_BUILD_TYPE="$(if $(ENABLE_DEBUG),Debug,None)" \
	-D SDL_HIDAPI_LIBUSB=OFF \
	-D CMAKE_INSTALL_PREFIX="$(DEPS_PREFIX)" \
	-D SDL_SHARED="$(if $(SDL3_STATIC),OFF,ON)" \
	-D SDL_STATIC="$(if $(SDL3_STATIC),ON,OFF)" \
	-D SDL_RPATH=OFF \
	-D SDL_CCACHE="$(if $(filter ccache,$(CC)),ON,OFF)" \
	-D CMAKE_C_COMPILER="$(CC_NOCCACHE)" \
	-D CMAKE_C_FLAGS="$(CFLAGS) -Wno-error=incompatible-pointer-types" && \
	cmake --build build && \
	cmake --install build
	PKG_CONFIG_PATH="$(DEPS_PREFIX)/lib/pkgconfig:$(DEPS_PREFIX)/lib64/pkgconfig:$(PKG_CONFIG_PATH)" \
		$(PKG_CONFIG) --exists --print-errors sdl3
	touch $@

# sus
build/deps/GLEW.built:
	$(MKDIR_P) build/deps/GLEW build/deps/lib/pkgconfig
	$(WGET) -qO- "https://github.com/nigels-com/glew/releases/download/glew-$(GLEW_VERSION)/glew-$(GLEW_VERSION).tgz" | $(TAR) xz -C build/deps/GLEW --strip-components=1
	cd build/deps/GLEW && \
		$(MAKE) SYSTEM=$(GLEW_SYSTEM) GLEW_DEST="$(DEPS_PREFIX)" \
		CC="$(CC)" CXX="$(CXX)" LD="$(CC)" \
		CFLAGS.EXTRA="$(CFLAGS) -fPIC -Wno-strict-prototypes" \
		LDFLAGS.EXTRA="$(LDFLAGS) -lX11" \
		$(if $(SDL3_STATIC),GLEW_STATIC=1,)
	cd build/deps/GLEW && \
		$(MAKE) SYSTEM=$(GLEW_SYSTEM) GLEW_DEST="$(DEPS_PREFIX)" \
		CC="$(CC)" CXX="$(CXX)" LD="$(CC)" \
		$(if $(SDL3_STATIC),GLEW_STATIC=1,) \
		install
	$(MKDIR_P) "$(DEPS_PREFIX)/lib/pkgconfig"
	( echo 'prefix=$(DEPS_PREFIX)'; \
	  echo 'exec_prefix=$${prefix}'; \
	  echo 'libdir=$${exec_prefix}/lib'; \
	  echo 'includedir=$${prefix}/include'; \
	  echo ''; \
	  echo 'Name: glew'; \
	  echo 'Description: The OpenGL Extension Wrangler library'; \
	  echo 'Version: $(GLEW_VERSION)'; \
	  echo 'Cflags: -I$${includedir}'; \
	  echo 'Libs: -L$${libdir} $(if $(SDL3_STATIC),-l:libGLEW.a,-lGLEW) -lEGL -lGL -lGLU -lOpenGL' ) > "$(DEPS_PREFIX)/lib/pkgconfig/glew.pc"
	PKG_CONFIG_PATH="$(DEPS_PREFIX)/lib/pkgconfig:$(DEPS_PREFIX)/lib64/pkgconfig:$(PKG_CONFIG_PATH)" \
		$(PKG_CONFIG) --exists --print-errors glew
	touch $@

build/deps/FREETYPE2.built:
	$(MKDIR_P) build/deps/FREETYPE2/build build/deps/lib/pkgconfig
	$(WGET) -qO- "https://download.savannah.gnu.org/releases/freetype/freetype-$(FREETYPE2_VERSION).tar.gz" | $(TAR) xz -C build/deps/FREETYPE2 --strip-components=1
	cd build/deps/FREETYPE2 && ./configure --prefix="$(DEPS_PREFIX)" \
		$(if $(SDL3_STATIC),--enable-static --enable-shared,--enable-shared --disable-static) \
		CC="$(CC)" CXX="$(CXX)" LD="$(LD)" CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
	$(MAKE) -C build/deps/FREETYPE2
	$(MAKE) -C build/deps/FREETYPE2 install
	PKG_CONFIG_PATH="$(DEPS_PREFIX)/lib/pkgconfig:$(DEPS_PREFIX)/lib64/pkgconfig:$(PKG_CONFIG_PATH)" \
		$(PKG_CONFIG) --exists --print-errors freetype2
	touch $@

DEPS_BUILT =
if BUILD_SDL3
DEPS_BUILT += build/deps/SDL3.built
endif
if BUILD_GLEW
DEPS_BUILT += build/deps/GLEW.built
endif
if BUILD_FREETYPE2
DEPS_BUILT += build/deps/FREETYPE2.built
endif

# making sure deps are built before main compile
BUILT_SOURCES = $(DEPS_BUILT)

# FIXME: redundant?
define get_pkgconfig_flags
$(shell PKG_CONFIG_PATH="$(DEPS_PREFIX)/lib/pkgconfig:$(DEPS_PREFIX)/lib64/pkgconfig:$(PKG_CONFIG_PATH)" \
	$(PKG_CONFIG) --$(1) $(2))
endef

# re-evaluated after deps are built
SDL3_CFLAGS = $(call get_pkgconfig_flags,cflags,sdl3)
SDL3_LIBS = $(call get_pkgconfig_flags,libs,sdl3)
GLEW_CFLAGS = $(call get_pkgconfig_flags,cflags,glew)
GLEW_LIBS = $(call get_pkgconfig_flags,libs,glew)
FREETYPE2_CFLAGS = $(call get_pkgconfig_flags,cflags,freetype2)
FREETYPE2_LIBS = $(call get_pkgconfig_flags,libs,freetype2)

# making sure deps are built before main compile (continued)
$(McOsu_OBJECTS): $(DEPS_BUILT)

# FIXME: just fixme
MCOSU_INCLUDE_FLAGS = $(shell find $(srcdir)/src -not -path '*/.*' -type d -printf "-I%p ")

McOsu_CXXFLAGS = \
	$(MCOSU_INCLUDE_FLAGS) \
	-I$(srcdir)/$(BUNDLED_BASS_DIR)/include \
	-I$(srcdir)/$(BUNDLED_BASS_FX_DIR)/include \
	$(SDL3_CFLAGS) \
	$(X11_CFLAGS) \
	$(XI_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLU_CFLAGS) \
	$(GLEW_CFLAGS) \
	$(FREETYPE2_CFLAGS) \
	$(LIBJPEG_CFLAGS) \
	$(ZLIB_CFLAGS) \
	-pthread

McOsu_CFLAGS = $(McOsu_CXXFLAGS)

McOsu_LDFLAGS = \
	-Wl,-rpath='$$ORIGIN/lib',-rpath='$$ORIGIN' \
	-L$(srcdir)/$(BUNDLED_BASS_DIR)/lib/linux \
	-L$(srcdir)/$(BUNDLED_BASS_FX_DIR)/lib/linux \
	-lbass -lbass_fx \
	$(SDL3_LIBS) \
	$(X11_LIBS) \
	$(XI_LIBS) \
	$(GL_LIBS) \
	$(GLU_LIBS) \
	$(GLEW_LIBS) \
	$(FREETYPE2_LIBS) \
	$(LIBJPEG_LIBS) \
	$(ZLIB_LIBS) \
	-lpthread

install-libs:
	$(MKDIR_P) "$(DESTDIR)$(bindir)/lib"
	$(INSTALL) -m 755 $(srcdir)/$(BUNDLED_BASS_DIR)/lib/linux/libbass.so "$(DESTDIR)$(bindir)/lib/"
	$(INSTALL) -m 755 $(srcdir)/$(BUNDLED_BASS_FX_DIR)/lib/linux/libbass_fx.so "$(DESTDIR)$(bindir)/lib/"
if BUILD_SDL3
	$(if $(SDL3_STATIC),,@cp -P "$(SDL3_PREFIX)/lib"/libSDL3*.so* "$(DESTDIR)$(bindir)/lib/")
endif
if BUILD_GLEW
	$(if $(SDL3_STATIC),,@libdir=$(GLEW_PREFIX)/`test -d "$(GLEW_PREFIX)/lib64" && echo "lib64" || echo "lib"`; \
	cp -P "$$libdir"/libGLEW*.so* "$(DESTDIR)$(bindir)/lib/")
endif
if BUILD_FREETYPE2
	@cp -P "$(FREETYPE2_PREFIX)/lib"/libfreetype*.so* "$(DESTDIR)$(bindir)/lib/"
endif
# FIXME: need transitive deps (libbrotlidec, libbrotlicommon, libharfbuzz, etc)
#	$(if $(SDL3_STATIC),,@cp -P "$(FREETYPE2_PREFIX)/lib"/libfreetype*.so* "$(DESTDIR)$(bindir)/lib/")

# install hooks
install-exec-hook: install-libs
	cd "$(DESTDIR)$(bindir)" && $(LN_S) -rf McOsu McEngine

uninstall-hook:
	rm -f "$(DESTDIR)$(bindir)/McEngine"
	rm -rf "$(DESTDIR)$(bindir)/lib"

# cleanup targets
clean-local:
	rm -rf $(builddir)/build/obj
	rm -f McOsu

clean-deps:
	rm -rf build/deps
.PHONY: clean-deps install-libs

maintainer-clean-local: clean-deps

distclean-local: clean-deps

# TODO: distribution packaging
dist-hook:
	cp -r $(srcdir)/$(BUNDLED_BASS_DIR)/lib $(distdir)/$(BUNDLED_BASS_DIR)/
	cp -r $(srcdir)/$(BUNDLED_BASS_DIR)/include $(distdir)/$(BUNDLED_BASS_DIR)/
	cp -r $(srcdir)/$(BUNDLED_BASS_FX_DIR)/lib $(distdir)/$(BUNDLED_BASS_FX_DIR)/
	cp -r $(srcdir)/$(BUNDLED_BASS_FX_DIR)/include $(distdir)/$(BUNDLED_BASS_FX_DIR)/
	find $(distdir) -type f -name ".git*" -delete
	find $(distdir) -type f -name "*.a" -delete
	find $(distdir) -type f -name "*.dll" -delete
	find $(distdir) -type f -name "*.dylib" -delete

# TODO: distribution package files
EXTRA_DIST = \
	$(BUNDLED_BASS_DIR)/include \
	$(BUNDLED_BASS_FX_DIR)/include \
	$(BUNDLED_BASS_DIR)/lib/linux/libbass.so \
	$(BUNDLED_BASS_FX_DIR)/lib/linux/libbass_fx.so \
	autogen.sh

MAINTAINERCLEANFILES = $(top_srcdir)/src/Makefile.sources
