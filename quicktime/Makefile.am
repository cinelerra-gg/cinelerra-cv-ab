SUBDIRS = thirdparty/encore50


lib_LTLIBRARIES = libquicktimehv.la
AM_CFLAGS = \
	$(LARGEFILE_CFLAGS) \
	$(CPU_CFLAGS) \
	$(FIREWIRE_CFLAGS) \
	$(XIPH_CFLAGS) \
	$(MJPEG_CFLAGS) \
	$(LIBDV_CFLAGS) \
	-DENCORE_INCLUDE=\"thirdparty/encore50/encore.h\" \
	@FFMPEG_CFLAGS@

libquicktimehv_la_LDFLAGS = -version-info 1:0:0 -release 2.3.0 -Wl,--no-undefined 

libquicktimehv_la_LIBADD = \
	@FFMPEG_LIBS@ \
	$(XIPH_LIBS) \
	-lmp3lame \
	$(FAAD_LIBS) \
	$(top_builddir)/quicktime/thirdparty/encore50/libencore.la \
	-lfaac \
	$(top_builddir)/libmpeg3/libmpeg3hv.la \
	$(LIBX264_LIBS) \
	$(LIBDV_LIBS) \
	-ljpeg -lpng \
	-lz -lpthread -lm

libquicktimehv_la_SOURCES = \
	atom.c \
	avcc.c \
	avi_hdrl.c \
	avi_ix.c \
	avi_idx1.c \
	avi_indx.c \
	avi_movi.c \
	avi_odml.c \
	avi_riff.c \
	avi_strl.c \
	cmodel_default.c \
	cmodel_float.c \
	cmodel_yuv420p.c \
	cmodel_yuv422.c \
	codecs.c \
	colormodels.c \
	ctab.c \
	dinf.c \
	dref.c \
	edts.c \
	elst.c \
	esds.c \
	frma.c \
	graphics.c \
	hdlr.c \
	ima4.c \
	interlacemodes.c \
	jpeg.c \
	libdv.c \
	libmjpeg.c \
	matrix.c \
	mdat.c \
	mdhd.c \
	mdia.c \
	minf.c \
	moov.c \
	mp4a.c \
	mpeg4.c \
	mvhd.c \
	plugin.c \
	qdm2.c \
	qtasf.c \
	qtcache.c \
	qtdv.c \
	qtffmpeg.c \
	qtpng.c \
	qth264.c \
	qtmp3.c \
	quicktime.c \
	raw.c \
	rawaudio.c \
	rle.c \
	smhd.c \
	stbl.c \
	stco.c \
	stsc.c \
	stsd.c \
	stsdtable.c \
	stss.c \
	stsz.c \
	stts.c \
	tkhd.c \
	trak.c \
	twos.c \
	udta.c \
	ulaw.c \
	util.c \
	v308.c \
	v408.c \
	v410.c \
	vbraudio.c \
	vmhd.c \
	vorbis.c \
	workarounds.c \
	wma.c \
	wmx2.c \
	yuv2.c \
	yuv4.c \
	yv12.c

noinst_HEADERS = cmodel_permutation.h \
	codecs.h \
	colormodels.h \
	div3.h \
	divx.h \
	fastjpg.h \
	fastjpgmacro.h \
	funcprotos.h \
	graphics.h \
	ima4.h \
	interlacemodes.h \
	jpeg.h \
	jpeg_old.h \
	libdv.h \
	libmjpeg.h \
	mmx.h \
	mp4a.h \
	mpeg4.h \
	qdm2.h \
	qtasf.h \
	qtasf_codes.h \
	qtdv.h \
	qtffmpeg.h \
	qth264.h \
	qtmp3.h \
	qtpng.h \
	qtprivate.h \
	qtvorbis.h \
	quicktime.h \
	raw.h \
	rawaudio.h \
	rle.h \
	rtjpeg.h \
	rtjpeg_core.h \
	twos.h \
	ulaw.h \
	v308.h \
	v408.h \
	v410.h \
	wma.h \
	wmx1.h \
	wmx2.h \
	workarounds.h \
	xanimjpeg.h \
	yuv2.h \
	yuv4.h \
	yuv9.h \
	yv12.h
	
EXTRA_DIST = docs

INCLUDES = -I$(top_srcdir)/libmpeg3

pkgincludedir=$(includedir)/quicktime
pkginclude_HEADERS=quicktime.h qtprivate.h

dist-hook:
	-rm -rf $(distdir)/docs/CVS
