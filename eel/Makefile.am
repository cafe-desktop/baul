NULL=

noinst_LTLIBRARIES=libeel-2.la

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Eel\" \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	$(CORE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(DISABLE_DEPRECATED_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DSOURCE_DATADIR=\""$(top_srcdir)/data"\" \
	-DCAFELOCALEDIR=\""$(prefix)/${DATADIRNAME}/locale"\" \
	$(NULL)

BUILT_SOURCES = \
	eel-marshal.c \
	eel-marshal.h \
	$(NULL)

libeel_2_la_LDFLAGS = \
	-no-undefined \
	$(CORE_CFLAGS) \
	$(NULL)

libeel_2_la_LIBADD = \
	$(CORE_LIBS) \
	$(RENDER_LIBS) \
	$(X_LIBS) \
	$(NULL)

eel_headers = \
	eel-accessibility.h \
	eel-art-extensions.h \
	eel-art-ctk-extensions.h \
	eel-background.h \
	eel-background-box.h \
	eel-canvas.h \
	eel-canvas-rect-ellipse.h \
	eel-debug.h \
	eel-editable-label.h \
	eel-cdk-extensions.h \
	eel-gdk-pixbuf-extensions.h \
	eel-glib-extensions.h \
	eel-cafe-extensions.h \
	eel-graphic-effects.h \
	eel-ctk-container.h \
	eel-ctk-extensions.h \
	eel-ctk-macros.h \
	eel-image-table.h \
	eel-labeled-image.h \
	eel-self-checks.h \
	eel-stock-dialogs.h \
	eel-string.h \
	eel-vfs-extensions.h \
	eel-wrap-table.h \
	eel-xml-extensions.h \
	eel.h \
	$(NULL)

libeel_2_la_SOURCES = \
	eel-accessibility.c \
	eel-art-extensions.c \
	eel-art-ctk-extensions.c \
	eel-background.c \
	eel-background-box.c \
	eel-canvas.c \
	eel-canvas-rect-ellipse.c \
	eel-debug.c \
	eel-editable-label.c \
	eel-cdk-extensions.c \
	eel-gdk-pixbuf-extensions.c \
	eel-glib-extensions.c \
	eel-cafe-extensions.c \
	eel-graphic-effects.c \
	eel-ctk-container.c \
	eel-ctk-extensions.c \
	eel-image-table.c \
	eel-labeled-image.c \
	eel-lib-self-check-functions.c \
	eel-self-checks.c \
	eel-stock-dialogs.c \
	eel-string.c \
	eel-vfs-extensions.c \
	eel-wrap-table.c \
	eel-xml-extensions.c \
	eel-lib-self-check-functions.h \
	$(eel_headers) \
	$(NULL)

nodist_libeel_2_la_SOURCES = \
	$(BUILT_SOURCES) \
	$(NULL)

eel-marshal.list: $(libeel_2_la_SOURCES) Makefile.am
	$(AM_V_GEN)( cd $(srcdir) && \
	sed -n -e 's/.*eel_marshal_\([[:upper:][:digit:]]*__[[:upper:][:digit:]_]*\).*/\1/p' \
	$(libeel_2_la_SOURCES) ) \
	| sed -e 's/__/:/' -e 'y/_/,/' | sort -u > $@.tmp
	@if cmp -s $@.tmp $@; then \
		rm $@.tmp; \
	else \
		mv $@.tmp $@; \
	fi

%-marshal.c: %-marshal.list Makefile
	$(AM_V_GEN)echo "#include \"eel-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) --body --prefix=$(subst -,_,$*)_marshal $< >> $*-marshal.c

%-marshal.h: %-marshal.list Makefile
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --prefix=$(subst -,_,$*)_marshal $< > $*-marshal.h

noinst_PROGRAMS = check-program

check_program_SOURCES = check-program.c
check_program_DEPENDENCIES = libeel-2.la
check_program_LDADD = $(EEL_LIBS)
check_program_LDFLAGS = $(check_program_DEPENDENCIES) -lm

TESTS = check-eel

EXTRA_DIST = \
	check-eel \
	eel-marshal.list \
	$(NULL)

CLEANFILES = \
	eel-marshal.list \
	$(BUILT_SOURCES) \
	$(NULL)

-include $(top_srcdir)/git.mk
