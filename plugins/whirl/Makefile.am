plugin_LTLIBRARIES = whirl.la
whirl_la_LDFLAGS = -avoid-version -module -shared 
whirl_la_LIBADD = 
whirl_la_SOURCES = whirl.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h whirl.h
EXTRA_DIST = picon.png
