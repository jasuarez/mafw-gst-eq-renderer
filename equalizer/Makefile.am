#
# Makefile.am for MAFW gst equalizer renderer library.
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
# Copyright (C) 2009 Igalia S.L.
#

lib_LTLIBRARIES = libmafw-equalizer.la

libmafw_equalizer_la_SOURCES = mafw-equalizer.c $(top_srcdir)/constants.h
libmafw_equalizer_la_LIBADD = $(MAFW_EQUALIZER_CPA_LIBS)
libmafw_equalizer_la_CPPFLAGS = $(MAFW_EQUALIZER_CPA_CFLAGS)

libdir = $(CPA_PLUGINDIR)

desktop_DATA = mafw-equalizer.desktop
desktopdir = $(CPA_DESKTOPDIR)
EXTRA_DIST = $(desktop_DATA)
