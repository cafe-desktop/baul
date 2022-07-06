CLEANFILES = $(NULL)

include $(top_srcdir)/Makefile.shared

lib_LTLIBRARIES=libbaul-extension.la

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(LIBBAUL_EXTENSION_CFLAGS) \
	$(WARNING_CFLAGS) \
	$(DISABLE_DEPRECATED_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	$(NULL)

libbaul_extension_la_LDFLAGS=\
	-version-info @BAUL_EXTENSION_VERSION_INFO@ \
	-no-undefined \
	$(NULL)

libbaul_extension_la_LIBADD = $(LIBBAUL_EXTENSION_LIBS)

libbaul_extension_includedir=$(includedir)/baul/libbaul-extension

libbaul_extension_include_HEADERS = \
	baul-column-provider.h \
	baul-column.h \
	baul-extension-types.h \
	baul-widget-view-provider.h \
	baul-file-info.h \
	baul-info-provider.h \
	baul-location-widget-provider.h \
	baul-menu-item.h \
	baul-menu-provider.h \
	baul-property-page-provider.h \
	baul-property-page.h \
	baul-menu.h \
	baul-configurable.h \
	$(NULL)

libbaul_extension_la_SOURCES = \
	baul-column-provider.c \
	baul-column.c \
	baul-widget-view-provider.c \
	baul-extension-i18n.h \
	baul-extension-types.c \
	baul-extension-private.h \
	baul-file-info.c \
	baul-info-provider.c \
	baul-location-widget-provider.c \
	baul-menu-item.c \
	baul-menu-provider.c \
	baul-property-page-provider.c \
	baul-property-page.c \
	baul-menu.c \
	baul-configurable.c \
	$(NULL)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libbaul-extension.pc

EXTRA_DIST = \
	libbaul-extension.pc.in \
	$(NULL)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_files = \
	$(libbaul_extension_include_HEADERS) \
	$(libbaul_extension_la_SOURCES)
Baul-2.0.gir: libbaul-extension.la Makefile
Baul_2_0_gir_INCLUDES = Ctk-3.0 Gio-2.0 GLib-2.0
Baul_2_0_gir_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(LIBBAUL_EXTENSION_CFLAGS)
Baul_2_0_gir_LIBS = libbaul-extension.la
Baul_2_0_gir_FILES = $(addprefix $(srcdir)/, $(introspection_files))
INTROSPECTION_GIRS += Baul-2.0.gir

girdir = $(datadir)/gir-1.0/
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif

-include $(top_srcdir)/git.mk
