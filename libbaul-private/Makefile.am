include $(top_srcdir)/Makefile.shared

noinst_LTLIBRARIES=libbaul-private.la

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(CORE_CFLAGS) \
	$(WARNING_CFLAGS) \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(TRACKER_CFLAGS) \
	$(BEAGLE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DBAUL_DATADIR=\""$(datadir)/baul"\" \
	-DBAUL_EXTENSIONDIR=\""$(libdir)/baul/extensions-2.0"\" \
	$(NULL)

BUILT_SOURCES = \
	baul-marshal.c \
	baul-marshal.h \
	$(NULL)

dependency_static_libs = \
	$(top_builddir)/libegg/libegg.la \
	$(NULL)

libbaul_private_la_LDFLAGS = \
	-no-undefined \
	$(NULL)

libbaul_private_la_LIBADD = \
	$(dependency_static_libs) \
	$(SELINUX_LIBS) \
	$(BEAGLE_LIBS) \
	$(TRACKER_LIBS) \
	$(top_builddir)/eel/libeel-2.la \
	$(top_builddir)/libbaul-extension/libbaul-extension.la \
	$(CORE_LIBS) \
    -lnotify
	$(NULL)

libbaul_private_la_SOURCES = \
	baul-autorun.c \
	baul-autorun.h \
	baul-bookmark.c \
	baul-bookmark.h \
	baul-cell-renderer-text-ellipsized.c \
	baul-cell-renderer-text-ellipsized.h \
	baul-clipboard-monitor.c \
	baul-clipboard-monitor.h \
	baul-clipboard.c \
	baul-clipboard.h \
	baul-column-chooser.c \
	baul-column-chooser.h \
	baul-column-utilities.c \
	baul-column-utilities.h \
	baul-customization-data.c \
	baul-customization-data.h \
	baul-debug-log.c \
	baul-debug-log.h \
	baul-default-file-icon.c \
	baul-default-file-icon.h \
	baul-desktop-directory-file.c \
	baul-desktop-directory-file.h \
	baul-desktop-directory.c \
	baul-desktop-directory.h \
	baul-desktop-icon-file.c \
	baul-desktop-icon-file.h \
	baul-desktop-link-monitor.c \
	baul-desktop-link-monitor.h \
	baul-desktop-link.c \
	baul-desktop-link.h \
	baul-desktop-metadata.c \
	baul-desktop-metadata.h \
	baul-directory-async.c \
	baul-directory-background.c \
	baul-directory-background.h \
	baul-directory-notify.h \
	baul-directory-private.h \
	baul-directory.c \
	baul-directory.h \
	baul-dnd.c \
	baul-dnd.h \
	baul-emblem-utils.c \
	baul-emblem-utils.h \
	baul-extensions.c \
	baul-extensions.h \
	baul-entry.c \
	baul-entry.h \
	baul-file-attributes.h \
	baul-file-changes-queue.c \
	baul-file-changes-queue.h \
	baul-file-conflict-dialog.c \
	baul-file-conflict-dialog.h \
	baul-file-dnd.c \
	baul-file-dnd.h \
	baul-file-operations.c \
	baul-file-operations.h \
	baul-file-private.h \
	baul-file-queue.c \
	baul-file-queue.h \
	baul-file-utilities.c \
	baul-file-utilities.h \
	baul-file.c \
	baul-file.h \
	baul-global-preferences.c \
	baul-global-preferences.h \
	baul-icon-canvas-item.c \
	baul-icon-canvas-item.h \
	baul-icon-container.c \
	baul-icon-container.h \
	baul-icon-dnd.c \
	baul-icon-dnd.h \
	baul-icon-private.h \
	baul-icon-info.c \
	baul-icon-info.h \
	baul-icon-names.h \
	baul-keep-last-vertical-box.c \
	baul-keep-last-vertical-box.h \
	baul-lib-self-check-functions.c \
	baul-lib-self-check-functions.h \
	baul-link.c \
	baul-link.h \
	baul-metadata.h \
	baul-metadata.c \
	baul-mime-actions.c \
	baul-mime-actions.h \
	baul-mime-application-chooser.c \
	baul-mime-application-chooser.h \
	baul-module.c \
	baul-module.h \
	baul-monitor.c \
	baul-monitor.h \
	baul-open-with-dialog.c \
	baul-open-with-dialog.h \
	baul-progress-info.c \
	baul-progress-info.h \
	baul-program-choosing.c \
	baul-program-choosing.h \
	baul-recent.c \
	baul-recent.h \
	baul-saved-search-file.c \
	baul-saved-search-file.h \
	baul-search-directory.c \
	baul-search-directory.h \
	baul-search-directory-file.c \
	baul-search-directory-file.h \
	baul-search-engine.c \
	baul-search-engine.h \
	baul-search-engine-simple.c \
	baul-search-engine-simple.h \
	baul-search-engine-beagle.c \
	baul-search-engine-beagle.h \
	baul-search-engine-tracker.c \
	baul-search-engine-tracker.h \
	baul-sidebar-provider.c \
	baul-sidebar-provider.h \
	baul-sidebar.c \
	baul-sidebar.h \
	baul-signaller.h \
	baul-signaller.c \
	baul-query.c \
	baul-query.h \
	baul-thumbnails.c \
	baul-thumbnails.h \
	baul-trash-monitor.c \
	baul-trash-monitor.h \
	baul-tree-view-drag-dest.c \
	baul-tree-view-drag-dest.h \
	baul-ui-utilities.c \
	baul-ui-utilities.h \
	baul-vfs-directory.c \
	baul-vfs-directory.h \
	baul-vfs-file.c \
	baul-vfs-file.h \
	baul-view-factory.c \
	baul-view-factory.h \
	baul-view.c \
	baul-view.h \
	baul-window-info.c \
	baul-window-info.h \
	baul-window-slot-info.c \
	baul-window-slot-info.h \
	baul-undostack-manager.c \
	baul-undostack-manager.h \
	$(NULL)

nodist_libbaul_private_la_SOURCES =\
	$(BUILT_SOURCES) \
	$(NULL)

$(lib_LTLIBRARIES): $(dependency_static_libs)

baul-marshal.list: $(libbaul_private_la_SOURCES) Makefile.am
	$(AM_V_GEN)( cd $(srcdir) && \
	sed -n -e 's/.*baul_marshal_\([[:upper:][:digit:]]*__[[:upper:][:digit:]_]*\).*/\1/p' \
	$(libbaul_private_la_SOURCES) ) \
	| sed -e 's/__/:/' -e 'y/_/,/' | sort -u > $@.tmp
	@if cmp -s $@.tmp $@; then \
		rm $@.tmp; \
	else \
		mv $@.tmp $@; \
	fi

%-marshal.h: %-marshal.list Makefile
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --prefix=$(subst -,_,$*)_marshal $< > $*-marshal.h

%-marshal.c: %-marshal.list Makefile
	$(AM_V_GEN)echo "#include \"baul-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) --body --prefix=$(subst -,_,$*)_marshal $< >> $*-marshal.c

gsettings_SCHEMAS = org.cafe.baul.gschema.xml org.cafe.media-handling.gschema.xml

@GSETTINGS_RULES@

EXTRA_DIST = \
	baul-marshal.list \
	$(gsettings_SCHEMAS) \
	$(BEAGLE_SOURCES) \
	$(TRACKER_SOURCES) \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES) \
	baul-marshal.list \
	$(NULL)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)


-include $(top_srcdir)/git.mk
