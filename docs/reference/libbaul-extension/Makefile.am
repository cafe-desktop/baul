## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=libbaul-extension

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../libbaul-extension
DOC_SOURCE_DIR=$(top_srcdir)/libbaul-extension

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="CTK_DISABLE_DEPRECATED" 
#SCAN_OPTIONS=--deprecated-guards="G_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../cdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB=$(top_srcdir)/libbaul-extension/*.h
CFILE_GLOB=$(top_srcdir)/libbaul-extension/*.c

# Header files to ignore when scanning.
IGNORE_HFILES = \
	config.h \
	baul-extension-i18n.h \
	$(NULL)

# Images to copy into HTML directory
HTML_IMAGES =

content_files = \
	version.xml \
	$(NULL)

# SGML files where gtk-doc abbrevations (#CtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files= \
		$(NULL)

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. INCLUDES=-I$(top_srcdir) -I$(top_builddir) $(CTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/ctk/$(ctktargetlib)
GTKDOC_CFLAGS = \
	$(CTK_DEBUG_FLAGS)                         \
	-I$(srcdir)                                \
	-I$(top_srcdir)                            \
	-I$(top_srcdir)/libbaul-extension      \
	-I$(top_builddir)                          \
	-I$(top_builddir)/libbaul-extension    \
	$(LIBBAUL_EXTENSION_CFLAGS)            \
	$(NULL)

GTKDOC_LIBS = \
	$(top_builddir)/libbaul-extension/libbaul-extension.la       \
	$(LIBBAUL_EXTENSION_LIBS)                                        \
	$(NULL)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += version.xml.in

-include $(top_srcdir)/git.mk
