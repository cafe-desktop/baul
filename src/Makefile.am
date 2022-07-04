include $(top_srcdir)/Makefile.shared

SUBDIRS=file-manager

bin_PROGRAMS = \
	baul \
	baul-file-management-properties \
	baul-autorun-software \
	baul-connect-server \
	$(NULL)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/libbaul-private \
	$(CORE_CFLAGS) \
	$(WARNING_CFLAGS) \
	$(EXIF_CFLAGS) \
	$(EXEMPI_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DBAUL_DATADIR=\""$(datadir)/baul"\" \
	-DMATELOCALEDIR=\""$(datadir)/locale"\" \
	-DUIDIR=\""$(datadir)/baul/ui"\" \
	-DBAUL_PIXMAPDIR=\""$(datadir)/pixmaps/baul"\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DVERSION="\"$(VERSION)\"" \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(NULL)

LDADD = \
	$(top_builddir)/src/file-manager/libbaul-file-manager.la \
	$(top_builddir)/libbaul-private/libbaul-private.la \
	$(CORE_LIBS) \
	$(EXIF_LIBS) \
	$(EXEMPI_LIBS) \
	$(POPT_LIBS) \
    -lnotify
	$(NULL)

dbus_freedesktop_built_sources =			\
	baul-freedesktop-generated.c		\
	baul-freedesktop-generated.h

$(dbus_freedesktop_built_sources) : Makefile.am $(top_srcdir)/data/freedesktop-dbus-interfaces.xml
	gdbus-codegen									\
		--interface-prefix org.freedesktop.					\
		--c-namespace CajaFreedesktop					\
		--c-generate-object-manager						\
		--generate-c-code baul-freedesktop-generated			\
		$(top_srcdir)/data/freedesktop-dbus-interfaces.xml			\
		$(NULL)

BUILT_SOURCES = \
	baul-src-marshal.c \
	baul-src-marshal.h \
	$(dbus_freedesktop_built_sources) \
	$(NULL)
baul_SOURCES = \
	baul-actions.h \
	baul-application.c \
	baul-application.h \
	baul-bookmark-list.c \
	baul-bookmark-list.h \
	baul-bookmarks-window.c \
	baul-bookmarks-window.h \
	baul-connect-server-dialog.c \
	baul-connect-server-dialog.h \
	baul-connect-server-dialog-nonmain.c \
	baul-connect-server-operation.c	\
	baul-connect-server-operation.h	\
	baul-desktop-window.c \
	baul-desktop-window.h \
	baul-emblem-sidebar.c \
	baul-emblem-sidebar.h \
	baul-file-management-properties.c \
	baul-file-management-properties.h \
	baul-freedesktop-dbus.c \
	baul-freedesktop-dbus.h \
	baul-history-sidebar.c \
	baul-history-sidebar.h \
	baul-image-properties-page.c \
	baul-image-properties-page.h \
	baul-information-panel.c \
	baul-information-panel.h \
	baul-location-bar.c \
	baul-location-bar.h \
	baul-location-dialog.c \
	baul-location-dialog.h \
	baul-location-entry.c \
	baul-location-entry.h \
	baul-main.c \
	baul-navigation-action.c \
	baul-navigation-action.h \
	baul-navigation-window-menus.c \
	baul-navigation-window.c \
	baul-navigation-window.h \
	baul-navigation-window-pane.c \
	baul-navigation-window-pane.h \
	baul-navigation-window-slot.c \
	baul-navigation-window-slot.h \
	baul-notebook.c \
	baul-notebook.h \
	baul-notes-viewer.c \
	baul-notes-viewer.h \
	baul-pathbar.c \
	baul-pathbar.h \
	baul-places-sidebar.c \
	baul-places-sidebar.h \
	baul-property-browser.c \
	baul-property-browser.h \
	baul-query-editor.c \
	baul-query-editor.h \
	baul-search-bar.c \
	baul-search-bar.h \
	baul-self-check-functions.c \
	baul-self-check-functions.h \
	baul-side-pane.c \
	baul-side-pane.h \
	baul-sidebar-title.c \
	baul-sidebar-title.h \
	baul-spatial-window.c \
	baul-spatial-window.h \
	baul-trash-bar.c \
	baul-trash-bar.h \
	baul-view-as-action.c \
	baul-view-as-action.h \
	baul-window-bookmarks.c \
	baul-window-bookmarks.h \
	baul-window-manage-views.c \
	baul-window-manage-views.h \
	baul-window-menus.c \
	baul-window-pane.c \
	baul-window-pane.h \
	baul-window-private.h \
	baul-window-slot.c \
	baul-window-slot.h \
	baul-window-toolbars.c \
	baul-window.c \
	baul-window.h \
	baul-x-content-bar.c \
	baul-x-content-bar.h \
	baul-zoom-action.c \
	baul-zoom-action.h \
	baul-zoom-control.c \
	baul-zoom-control.h \
	$(NULL)

nodist_baul_SOURCES = \
	$(BUILT_SOURCES) \
	$(NULL)

baul-src-marshal.list: $(baul_SOURCES) Makefile.am
	$(AM_V_GEN)( cd $(srcdir) && \
	sed -n -e 's/.*baul_src_marshal_\([[:upper:][:digit:]]*__[[:upper:][:digit:]_]*\).*/\1/p' \
	$(baul_SOURCES) ) \
	| sed -e 's/__/:/' -e 'y/_/,/' | sort -u > $@.tmp
	@if cmp -s $@.tmp $@; then \
		rm $@.tmp; \
	else \
		mv $@.tmp $@; \
	fi

%-marshal.c: %-marshal.list Makefile
	$(AM_V_GEN)echo "#include \"baul-src-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) --body --prefix=$(subst -,_,$*)_marshal $< >> $*-marshal.c

%-marshal.h: %-marshal.list Makefile
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --prefix=$(subst -,_,$*)_marshal $< > $*-marshal.h

baul_file_management_properties_SOURCES = \
	baul-file-management-properties.c \
	baul-file-management-properties.h \
	baul-file-management-properties-main.c \
	$(NULL)

baul_autorun_software_SOURCES = \
	baul-autorun-software.c \
	$(NULL)

baul_connect_server_SOURCES = \
	baul-bookmark-list.c \
	baul-bookmark-list.h \
	baul-connect-server-dialog.c \
	baul-connect-server-dialog.h \
	baul-connect-server-dialog-main.c \
	baul-connect-server-operation.c     \
	baul-connect-server-operation.h     \
	$(NULL)


aboutdir = $(datadir)/baul
about_DATA = \
	baul.about \
        $(NULL)

if ENABLE_SELF_CHECK
TESTS=check-baul
endif

uidir = $(datadir)/baul/ui

ui_files = \
	baul-file-management-properties.ui \
	baul-bookmarks-window.ui \
	$(NULL)

include $(top_srcdir)/gla11y.mk

ui_DATA = \
	baul-shell-ui.xml \
	baul-navigation-window-ui.xml \
	baul-spatial-window-ui.xml \
	$(ui_files) \
	$(NULL)

CLEANFILES = \
	baul-src-marshal.list \
	$(BUILT_SOURCES) \
	$(server_DATA) \
	$(NULL)

EXTRA_DIST = \
	baul-src-marshal.list \
	$(server_in_files) \
	$(ui_DATA) \
	check-baul \
	$(about_DATA) \
	ui-a11y.suppr \
	$(NULL)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
