NULL =

xml_in_files = baul.xml.in
xml_files = $(xml_in_files:.xml.in=.xml)

%.xml: %.xml.in
	$(AM_V_GEN) GETTEXTDATADIR=$(top_srcdir) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

desktopdir = $(datadir)/applications
desktop_DATA = \
	baul.desktop \
	baul-home.desktop \
	baul-computer.desktop \
	baul-folder-handler.desktop \
	baul-file-management-properties.desktop  \
	baul-browser.desktop \
	baul-autorun-software.desktop \
	mate-network-scheme.desktop
desktop_in_files = $(desktop_DATA:.desktop=.desktop.in)
desktop_in_in_files = $(desktop_DATA:.desktop=.desktop.in.in)
%.desktop.in: %.desktop.in.in
	$(AM_V_GEN) GETTEXTDATADIR=$(top_srcdir) $(MSGFMT) --desktop \
		--keyword --keyword=Name --keyword=GenericName --keyword=Comment --keyword=Keywords \
		--template $< -d $(top_srcdir)/po -o $@
%.desktop: %.desktop.in
	$(AM_V_GEN) sed -e 's|@bindir[@]|$(bindir)|g' $< > $@

mimedir = $(datadir)/mime/packages
mime_DATA = $(xml_files)

servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)
service_in_files = 				\
	org.mate.freedesktop.FileManager1.service.in

org.mate.freedesktop.FileManager1.service: org.mate.freedesktop.FileManager1.service.in Makefile
	$(AM_V_GEN) sed -e "s|\@bindir\@|$(bindir)|" $< > $@

bauldatadir = $(datadir)/baul
bauldata_in_files = browser.xml.in
bauldata_DATA = \
	browser.xml \
	baul-extras.placeholder  \
	baul-suggested.placeholder \
	baul.css \
	baul-desktop.css \
	a11y-baul-desktop-base.css \
	baul-desktop-ContrastHigh.css \
	baul-desktop-ContrastHighInverse.css \
	baul-desktop-HighContrast.css \
	baul-desktop-HighContrastInverse.css \
	$(NULL)

# app data file
appdatadir = $(datadir)/metainfo
appdata_in_files = baul.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(bauldata_DATA) \
	baul.css \
	freedesktop-dbus-interfaces.xml \
	$(xml_in_files) \
	$(bauldata_in_files) \
	$(desktop_in_files) \
	$(desktop_in_in_files) \
	$(service_in_files) \
	$(appdata_in_files) \
	$(NULL)

CLEANFILES = $(xml_files) \
	$(desktop_DATA) \
	$(desktop_in_files) \
	$(service_DATA) \
	baul.appdata.xml \
	$(NULL)

SUBDIRS = \
	icons \
	patterns \
	$(NULL)

install-data-hook:
if ENABLE_UPDATE_MIMEDB
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

uninstall-hook:
if ENABLE_UPDATE_MIMEDB
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

-include $(top_srcdir)/git.mk
