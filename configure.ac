#
# configure.ac for MAFW gstreamer renderer with equalizer library
# 
# For original mafw-gst-renderer code:
#     Copyright (C) 2007, 2008, 2009 Nokia. All rights reserved.
#     Author: Visa Smolander <visa.smolander@nokia.com>
#
# For mafw-gst-eq-renderer fork:
#     Copyright (C) 2009, 2010 Igalia S.L.
#     Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#

AC_PREREQ([2.53])
AC_INIT([mafw-gst-eq-renderer], [0.2.2010.07-2-1mtr])

AC_CONFIG_SRCDIR([libmafw-gst-renderer/mafw-gst-renderer.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign tar-ustar])
AM_MAINTAINER_MODE

AC_DISABLE_STATIC

dnl Prevent AC_PROG_CC adding '-g -O2' to CFLAGS.
SAVEDCFLAGS="$CFLAGS"
AC_PROG_CC
if test "x$GCC" = xyes; then
	CFLAGS="$SAVEDCFLAGS"
fi

AC_PROG_LIBTOOL
AC_PROG_INSTALL

# DISABLED_BY_DEFAULT(NAME, DESCRIPTION)
# ---------------------------------
# Creates a new --enable-* option, with default value `no'.
AC_DEFUN([DISABLED_BY_DEFAULT], [dnl
         AC_ARG_ENABLE([$1], AS_HELP_STRING([--enable-$1], [$2]), [],[dnl
         m4_bpatsubst([enable_$1], [[^0-9a-z]], [_])=no])dnl
])# DISABLED_BY_DEFAULT

# ENABLED_BY_DEFAULT(NAME, DESCRIPTION)
# ---------------------------------
# Creates a new --disable-* option, with default value `yes'.
AC_DEFUN([ENABLED_BY_DEFAULT], [dnl
         AC_ARG_ENABLE([$1], AS_HELP_STRING([--disable-$1], [$2]), [],[dnl
         m4_bpatsubst([enable_$1], [[^0-9a-z]], [_])=yes])dnl
])# ENABLED_BY_DEFAULT

dnl Prerequisites.

GSTREAMER_VERSION=0.10.20

AM_PATH_GLIB_2_0(2.15.0, [], [], [glib])
PKG_CHECK_MODULES(DEPS,
		  gobject-2.0 >= 2.0
		  gstreamer-0.10 >= $GSTREAMER_VERSION
		  gstreamer-plugins-base-0.10 >= $GSTREAMER_VERSION
		  mafw >= 0.1
		  libosso >= 2.0
		  x11
		  hal
		  totem-plparser
                  gconf-2.0 >= 2.0
		  gnome-vfs-2.0
                  mce
                  dbus-1
)

dnl Check for GdkPixbuf, needed for dumping current frame
GDKPIXBUF_REQUIRED=2.12.0
AC_ARG_ENABLE(gdkpixbuf,
	      AS_HELP_STRING([--disable-gdkpixbuf],
			     [Disable GdkPixbuf support, required for current frame dumping]),,
	      [enable_gdkpixbuf=auto])

if test "x$enable_gdkpixbuf" != "xno"; then
   PKG_CHECK_MODULES(GDKPIXBUF,
		     [gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED],
		     [have_gdkpixbuf=yes],
		     [have_gdkpixbuf=no])
   AC_SUBST(GDKPIXBUF_LIBS)
   AC_SUBST(GDKPIXBUF_CFLAGS)

   if test "x$have_gdkpixbuf" = "xyes"; then
      AC_DEFINE(HAVE_GDKPIXBUF, [], [Define if we have GdkPixbuf])
   fi
else
   have_gdkpixbuf="no (disabled)"
fi

if test "x$enable_gdkpixbuf" = "xyes"; then
   if test "x$have_gdkpixbuf" != "xyes"; then
      AC_MSG_ERROR([Couldn't find GdkPixbuf >= $GDKPIXBUF_REQUIRED.])
   fi
fi

AM_CONDITIONAL(HAVE_GDKPIXBUF, test "x$have_gdkpixbuf" = "xyes")


dnl Check for Conic, needed connection error handling
CONIC_REQUIRED=0.16
AC_ARG_ENABLE(conic,
	      AS_HELP_STRING([--disable-conic],
			     [Disable Conic support, required to handle network errors]),,
	      [enable_conic=auto])

if test "x$enable_conic" != "xno"; then
   PKG_CHECK_MODULES(CONIC,
		     [conic >= $CONIC_REQUIRED],
		     [have_conic=yes],
		     [have_conic=no])
   AC_SUBST(CONIC_LIBS)
   AC_SUBST(CONIC_CFLAGS)

   if test "x$have_conic" = "xyes"; then
      AC_DEFINE(HAVE_CONIC, [], [Define if we have Conic])
   fi
else
   have_conic="no (disabled)"
fi

if test "x$enable_conic" = "xyes"; then
   if test "x$have_conic" != "xyes"; then
      AC_MSG_ERROR([Couldn't find Conic >= $CONIC_REQUIRED.])
   fi
fi

AM_CONDITIONAL(HAVE_CONIC, test "x$have_conic" = "xyes")



plugindir=`$PKG_CONFIG --variable=plugindir mafw`
AC_SUBST(plugindir)

dnl Default compile flags.  (NOTE: CFLAGS is reserved for the user!)

AC_SUBST([_CFLAGS])
AC_SUBST([_LDFLAGS])
_CFLAGS="-Wall -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations"
_CFLAGS="$_CFLAGS -ggdb3"

dnl Configure-time options.

dnl Debugging.
DISABLED_BY_DEFAULT([debug], [compile with debug flags and extra output])
if test "x$enable_debug" = xyes; then
	AC_DEFINE([MAFW_DEBUG], [1], [Enable debugging related parts.])
	_CFLAGS="$_CFLAGS -O0 -Werror -DGTK_DISABLE_DEPRECATED"
else
	AC_DEFINE([G_DEBUG_DISABLE], [1], [Disable g_debug() calls.])
	_CFLAGS="$_CFLAGS -O2"
fi
AS_IF([test "x$enable_debug" = xyes],
	[AC_DEFINE([MAFW_DEBUG], [1], [Enable extra debug messages])
	 CFLAGS="$CFLAGS -Werror -O0 -ggdb3 -DGTK_DISABLE_DEPRECATED"],
	[AC_DEFINE([G_DEBUG_DISABLE], [1], [Disable g_debug calls])
	 CFLAGS="$CFLAGS -O2"])


dnl Tests.
DISABLED_BY_DEFAULT([tests], [disable unit tests])
if test "x${SBOX_DPKG_INST_ARCH}" = "xarmel"; then
   AC_MSG_WARN([Tests are disabled for compilation in armel])
   enable_tests="no"
fi
if test "x$enable_tests" = xyes; then
	PKG_CHECK_MODULES(CHECKMORE, [checkmore, check >= 0.9.4])
	if test -z "$CHECKMORE_LIBS"; then
		AC_MSG_WARN([checkmore is needed for unit tests!])
	fi
fi
AM_CONDITIONAL(ENABLE_TESTS,
               [test "x$enable_tests" = xyes && test -n "$CHECKMORE_LIBS"])

dnl Volume handling
if test "x${SBOX_DPKG_INST_ARCH}" = "xi386"; then
   DISABLED_BY_DEFAULT([pulse-volume], [enable volume handling with pulse])
else
   ENABLED_BY_DEFAULT([pulse-volume], [enable volume handling with pulse])
fi
if test "x$enable_pulse_volume" = xno; then
	AC_DEFINE([MAFW_GST_RENDERER_DISABLE_PULSE_VOLUME], [1], [Disables volume handling with pulse.])
else
        PKG_CHECK_MODULES(VOLUME, libpulse-mainloop-glib >= 0.9.15)
fi


dnl Mute
DISABLED_BY_DEFAULT([mute], [enable mute handling])
if test "x$enable_mute" = xyes; then
	AC_DEFINE([MAFW_GST_RENDERER_ENABLE_MUTE], [1], [Enable mute.])
fi

dnl Tracing.
DISABLED_BY_DEFAULT([tracing], [enable function instrumentation (tracing)])
if test "x$enable_tracing" = xyes; then
	_CFLAGS="$_CFLAGS -finstrument-functions -rdynamic"
fi

dnl Coverage.
DISABLED_BY_DEFAULT([coverage], [enable coverage data generation (gcov)])
if test "x$enable_coverage" = xyes; then
	AC_PATH_PROG(LCOV, [lcov], [lcov])
	if test "x$LCOV" = x; then
		echo You need to install lcov to get actual reports!
		echo See http://ltp.sf.net/coverage/lcov.php
	fi
	if test "x$SBOX_USE_CCACHE" == xyes; then
		AC_MSG_ERROR([Please set SBOX_USE_CCACHE=no to use coverage.])
	fi
	_CFLAGS="$_CFLAGS -fprofile-arcs -ftest-coverage"
	_LDFLAGS="$_LDFLAGS -g -lgcov"
fi
AM_CONDITIONAL(ENABLE_COVERAGE,
               [test "x$enable_coverage" != xno && test -n "$LCOV"])

dnl GConf path
GCONF_MAFW_GST_EQ_RENDERER=/system/mafw/mafw-gst-eq-renderer
AC_SUBST(GCONF_MAFW_GST_EQ_RENDERER)

dnl Min/Max gain for each band in equalizer
EQ_GAIN_MIN=-24
EQ_GAIN_MAX=+12
AC_SUBST(EQ_GAIN_MIN)
AC_SUBST(EQ_GAIN_MAX)

dnl Path were presets will be stored
HOME_PRESETS=/home/user/.mafw-gst-eq-presets
AC_SUBST(HOME_PRESETS)

dnl Control Panel
PKG_CHECK_MODULES([MAFW_EQUALIZER_CPA], [libosso >= 2.0
                                        hildon-1 >= 2.1
                                        hildon-control-panel
                                        gtk+-2.0
                                        gconf-2.0
                                        libxml-2.0])

CPA_PLUGINDIR=`pkg-config hildon-control-panel --variable=pluginlibdir`
CPA_DESKTOPDIR=`pkg-config hildon-control-panel --variable=plugindesktopentrydir`

AC_SUBST(MAFW_EQUALIZER_CPA_CFLAGS)
AC_SUBST(MAFW_EQUALIZER_CPA_LIBS)
AC_SUBST(CPA_DESKTOPDIR)
AC_SUBST(CPA_PLUGINDIR)

dnl Output files.

AC_CONFIG_FILES([
  Makefile
  constants.h
  mafw-gst-renderer-uninstalled.pc
  libmafw-gst-renderer/Makefile
  equalizer/Makefile
  presets/Makefile
  tests/Makefile
  debian/mafw-gst-eq-renderer.install
  debian/mafw-gst-eq-renderer.postinst
])

AC_OUTPUT
