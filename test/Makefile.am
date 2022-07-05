NULL=

AM_CPPFLAGS =\
	-I$(top_srcdir) \
	$(CORE_CFLAGS) \
	$(WARNING_CFLAGS) \
	-DVERSION="\"$(VERSION)\"" \
	-DBAUL_DATADIR=\""$(datadir)/baul"\" \
	-DCAFELOCALEDIR=\""$(prefix)/${DATADIRNAME}/locale"\" \
	$(NULL)

LDADD =\
	$(top_builddir)/libbaul-private/libbaul-private.la \
	$(CORE_LIBS) \
	$(NULL)

noinst_PROGRAMS =\
	test-baul-wrap-table \
	test-baul-search-engine \
	test-baul-directory-async \
	test-baul-copy \
	test-eel-background \
	test-eel-editable-label \
	test-eel-image-table \
	test-eel-labeled-image \
	test-eel-pixbuf-scale \
	$(NULL)

test_baul_copy_SOURCES = test-copy.c test.c

test_baul_wrap_table_SOURCES = test-baul-wrap-table.c test.c

test_baul_search_engine_SOURCES = test-baul-search-engine.c 

test_baul_directory_async_SOURCES = test-baul-directory-async.c

test_eel_background_SOURCES = test-eel-background.c
test_eel_image_table_SOURCES = test-eel-image-table.c test.c
test_eel_labeled_image_SOURCES = test-eel-labeled-image.c test.c test.h
test_eel_pixbuf_scale_SOURCES = test-eel-pixbuf-scale.c test.c test.h

EXTRA_DIST = \
	test.h \
	$(NULL)

-include $(top_srcdir)/git.mk
