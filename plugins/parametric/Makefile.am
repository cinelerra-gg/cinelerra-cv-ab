plugin_LTLIBRARIES = parametric.la
parametric_la_LDFLAGS = -avoid-version -module -shared 
parametric_la_LIBADD = libfourier.la
parametric_la_SOURCES = parametric.C


noinst_LTLIBRARIES=libfourier.la
libfourier_la_SOURCES=fourier.C
libfourier_la_LDFLAGS=
libfourier_la_LIBADD=-lfftw3

AM_CXXFLAGS = $(LARGEFILE_CFLAGS)
INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime

noinst_HEADERS = parametric.h picon_png.h fourier.h
EXTRA_DIST = picon.png
