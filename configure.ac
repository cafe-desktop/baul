AC_PREREQ(2.54)

dnl ===========================================================================

m4_define(gdk-pixbuf_minver,           2.36.5)
m4_define(glib_minver,                 2.67.3)
m4_define(gio_minver,                  2.50.0)
m4_define(cafe_desktop_minver,         1.17.3)
m4_define(pango_minver,                1.1.2)
m4_define(ctk_minver,                  3.22.0)
m4_define(xml_minver,                  2.4.7)
m4_define(exif_minver,                 0.6.14)
m4_define(exempi_minver,               1.99.5)
m4_define(cail_minver,                 3.0.0)
m4_define(notify_minver,               0.7.0)


dnl 1. If the library code has changed at all since last release, then increment revision.
dnl 2. If any interfaces have been added, then increment current and set revision to 0.
dnl Interface break is not allowed.
m4_define(baul_extension_current,  5)
m4_define(baul_extension_revision, 0)

AC_INIT([baul], [1.25.0], [https://cafe-desktop.org])

dnl ---------------------------------------------------------------------------

AC_CONFIG_SRCDIR(src)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4])

CAFE_COMPILE_WARNINGS

AM_INIT_AUTOMAKE([1.9 tar-ustar dist-xz no-dist-gzip check-news])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE
AC_SUBST([ACLOCAL_AMFLAGS], ["\${ACLOCAL_FLAGS}"])

AC_SUBST(CDKPIXBUF_REQUIRED, [gdk-pixbuf_minver])
AC_SUBST(GLIB_REQUIRED, [glib_minver])
AC_SUBST(GIO_REQUIRED, [gio_minver])
AC_SUBST(CAFE_DESKTOP_REQUIRED, [cafe_desktop_minver])
AC_SUBST(PANGO_REQUIRED, [pango_minver])
AC_SUBST(CTK_REQUIRED, [ctk_minver])
AC_SUBST(XML_REQUIRED, [xml_minver])
AC_SUBST(CAIL_REQUIRED, [cail_minver])

dnl We need to decrement current by one in the calculation of the age because
dnl the library was started with version "1:0:0" instead of "0:0:0"
AC_SUBST(BAUL_EXTENSION_VERSION_INFO, [baul_extension_current]:[baul_extension_revision]:`expr [baul_extension_current] - 1`)

AC_C_BIGENDIAN
AC_C_CONST
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT([disable-static])
PKG_PROG_PKG_CONFIG
dnl Give error and exit if we dont have pkgconfig
if test "x$PKG_CONFIG" = "x"; then
  AC_MSG_ERROR([you need to have pkgconfig installed!])
fi
AC_PATH_PROG([GLA11Y], [gla11y], [true])

AC_CHECK_LIB(m, floor)

PKG_CHECK_MODULES(ALL, [
    gdk-pixbuf-2.0 >= gdk-pixbuf_minver
    glib-2.0 >= glib_minver
    cafe-desktop-2.0 >= cafe_desktop_minver
    gthread-2.0
    gio-unix-2.0
    gio-2.0 >= gio_minver
    pango >= pango_minver
    ctk+-3.0 >= ctk_minver
    libnotify
    libxml-2.0 >= xml_minver
    cail-3.0 >= cail_minver
])
dnl ==========================================================================

GETTEXT_PACKAGE=baul
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",
    [the gettext translation domain])

AH_TEMPLATE([ENABLE_INSTALLER])
AH_TEMPLATE([ENABLE_NLS])
AH_TEMPLATE([HAVE_CATGETS])
AH_TEMPLATE([HAVE_LC_MESSAGES])
AH_TEMPLATE([HAVE_LIBBZ2])
AH_TEMPLATE([HAVE_LIBJPEG])
AH_TEMPLATE([HAVE_MEDUSA])
AH_TEMPLATE([HAVE_STPCPY])
AH_TEMPLATE([bzclose])
AH_TEMPLATE([bzCompress])
AH_TEMPLATE([bzCompressInit])
AH_TEMPLATE([bzDecompress])
AH_TEMPLATE([bzDecompressInit])
AH_TEMPLATE([bzdopen])
AH_TEMPLATE([bzerror])
AH_TEMPLATE([bzflush])
AH_TEMPLATE([bzopen])
AH_TEMPLATE([bzread])
AH_TEMPLATE([bzwrite])

AC_DEFINE([HAVE_CTK_MULTIHEAD], [], [needed for egg-screen-exec functions])

AH_TEMPLATE([HAVE_STARTUP_NOTIFICATION])
AH_TEMPLATE([HAVE_EXIF])

AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

dnl ==========================================================================

GTK_DOC_CHECK([1.4])

dnl ==========================================================================

AC_CHECK_PROGS(PERL, perl5 perl)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl ==========================================================================

AC_CHECK_HEADERS(sys/mount.h sys/vfs.h sys/param.h malloc.h)
AC_CHECK_FUNCS(mallopt)

dnl ==========================================================================

AM_CONDITIONAL(HAVE_EXIF, false)

dnl libexif checking

PKG_CHECK_MODULES(EXIF, libexif >= exif_minver,
    [AM_CONDITIONAL(HAVE_EXIF, true)
     AC_DEFINE(HAVE_EXIF, 1, [Define to enable EXIF support])
    ],
    [AM_CONDITIONAL(HAVE_EXIF, false)])

AC_SUBST(EXIF_CFLAGS)
AC_SUBST(EXIF_LIBS)

dnl ==========================================================================

dnl exempi checking

AM_CONDITIONAL(HAVE_EXEMPI, false)

AC_ARG_ENABLE(xmp,
    AS_HELP_STRING([--disable-xmp],
                   [build without xmp support]))
msg_xmp=no
if test "x$enable_xmp" != "xno"; then
    PKG_CHECK_MODULES(EXEMPI, exempi-2.0 >= exempi_minver,
        [AM_CONDITIONAL(HAVE_EXEMPI, true)
         AC_DEFINE(HAVE_EXEMPI, 1, [Define to enable xmp support])]
        msg_xmp=yes,
        [AM_CONDITIONAL(HAVE_EXEMPI, false)])

    AC_SUBST(EXEMPI_CFLAGS)
    AC_SUBST(EXEMPI_LIBS)
fi

dnl ==========================================================================

dnl ****************************
dnl *** Check for libselinux ***
dnl ****************************

SELINUX_LIBS=
msg_selinux=no
AC_CHECK_LIB(selinux, is_selinux_enabled,
   [AC_CHECK_HEADERS(selinux/selinux.h,
     [AC_SEARCH_LIBS(selinux_raw_to_trans_context, selinux,
       [AC_DEFINE(HAVE_SELINUX, 1, [Define to 1 if libselinux is available])
        SELINUX_LIBS="-lselinux"
        msg_selinux=yes])
     ])
   ])
AC_SUBST(SELINUX_LIBS)


AC_ARG_ENABLE(empty_view,
 AS_HELP_STRING([--enable-empty-view], [Enable empty view]),
 [ENABLE_EMPTY_VIEW=1
  AC_DEFINE(ENABLE_EMPTY_VIEW, 1, [define to enable the empty view that is used for performance measurement])])

AC_SUBST(ENABLE_EMPTY_VIEW)
AM_CONDITIONAL(ENABLE_EMPTY_VIEW, test "x$ENABLE_EMPTY_VIEW" = "x1")

dnl ==========  EEL/BAUL OMIT SELF CHECK  ====================================
AC_ARG_ENABLE(self-check,
    AS_HELP_STRING([--disable-self-check], [build without self check]))
msg_self_check=yes
if test "x$enable_self_check" = "xno"; then
    msg_self_check=no
    AC_DEFINE(EEL_OMIT_SELF_CHECK, 1, [define to disable eel self check])
    AC_DEFINE(BAUL_OMIT_SELF_CHECK, 1, [define to disable baul self check])
fi
AM_CONDITIONAL(ENABLE_SELF_CHECK, test "x$msg_self_check" = "xyes")

dnl ==========================================================================

AC_ARG_ENABLE(packagekit,
    AS_HELP_STRING([--disable-packagekit],
        [build without PackageKit support]))
msg_packagekit=no
if test "x$enable_packagekit" != "xno"; then
    msg_packagekit=yes
    AC_DEFINE(ENABLE_PACKAGEKIT, 1, [define to enable PackageKit mimetype installer])
fi

dnl ==========================================================================

dnl libegg
LIBEGG_MODULES="ctk+-3.0"
LIBEGG_CFLAGS="`$PKG_CONFIG --cflags $LIBEGG_MODULES`"
AC_SUBST(LIBEGG_CFLAGS)
LIBEGG_LIBS="`$PKG_CONFIG --libs $LIBEGG_MODULES`"
AC_SUBST(LIBEGG_LIBS)

dnl libbaul-extension
LIBBAUL_EXTENSION_MODULES="glib-2.0 ctk+-3.0"
LIBBAUL_EXTENSION_CFLAGS="`$PKG_CONFIG --cflags $LIBBAUL_EXTENSION_MODULES`"
AC_SUBST(LIBBAUL_EXTENSION_CFLAGS)
LIBBAUL_EXTENSION_LIBS="`$PKG_CONFIG --libs $LIBBAUL_EXTENSION_MODULES`"
AC_SUBST(LIBBAUL_EXTENSION_LIBS)

dnl core baul
PKG_CHECK_MODULES(GMODULE, gmodule-2.0, [GMODULE_ADD="gmodule-2.0"],[GMODULE_ADD=""])
CORE_MODULES="glib-2.0 $GMODULE_ADD cafe-desktop-2.0 gthread-2.0 gio-2.0 gio-unix-2.0 cail-3.0 libxml-2.0 $EXTRA_CORE_MODULES ctk+-3.0 x11"
CORE_CFLAGS="`$PKG_CONFIG --cflags $CORE_MODULES`"
AC_SUBST(CORE_CFLAGS)
CORE_LIBS="`$PKG_CONFIG --libs $CORE_MODULES`"
AC_SUBST(CORE_LIBS)

DISABLE_DEPRECATED_CFLAGS="-DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED"
AC_SUBST(DISABLE_DEPRECATED_CFLAGS)

dnl Multimedia keys
AC_CHECK_HEADERS([X11/XF86keysym.h])

dnl ==============================================
dnl Special GSettings section
dnl ==============================================

GLIB_GSETTINGS

##################################################
# Check for introspection
##################################################
GOBJECT_INTROSPECTION_CHECK([0.6.4])

dnl ==========================================================================

AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)

AC_ARG_ENABLE(update-mimedb,
   AS_HELP_STRING([--disable-update-mimedb],
                   [disable the update-mime-database after install [default=no]]),,
    enable_update_mimedb=yes)
AM_CONDITIONAL(ENABLE_UPDATE_MIMEDB, test x$enable_update_mimedb = xyes)

AC_ARG_ENABLE(icon-update, AS_HELP_STRING([--disable-icon-update],
                                        [Disable icon cache update]))
if (test "$enable_icon_update" != no); then
        AC_PATH_PROG(UPDATE_ICON_CACHE, [ctk-update-icon-cache])
fi
AM_CONDITIONAL([ICON_UPDATE], [test -n "$UPDATE_ICON_CACHE"])

AC_CONFIG_FILES([
Makefile
libegg/Makefile
data/Makefile
data/icons/Makefile
data/patterns/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/libbaul-extension/Makefile
docs/reference/libbaul-extension/version.xml
eel/Makefile
icons/Makefile
libbaul-private/Makefile
libbaul-extension/Makefile
libbaul-extension/libbaul-extension.pc
libbaul-extension/libbaul-extension-uninstalled.pc
po/Makefile.in
src/Makefile
src/file-manager/Makefile
test/Makefile
])

AC_OUTPUT

dnl ==========================================================================

echo "
baul-$VERSION:

    prefix:                       ${prefix}
    source code location:         ${srcdir}
    compiler:                     ${CC}
    compiler flags:               ${CFLAGS}
    warning flags:                ${WARN_CFLAGS}
    xmp support:                  $msg_xmp
    PackageKit support:           $msg_packagekit
    Self check:                   $msg_self_check

    baul-extension documentation: ${enable_gtk_doc}
    baul-extension introspection: ${found_introspection}
"
