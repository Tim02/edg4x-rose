
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_subdirectory(util)
add_subdirectory(xml)
add_subdirectory(profir)
add_subdirectory(xml2profir)
add_subdirectory(sage)
add_subdirectory(profir2sage)
add_subdirectory(xml-xercesc)
add_subdirectory(gprof)


########### install files ###############




#original Makefile.am contents follow:

## ROSE-HPCT sources
#
#SUBDIRS = util xml profir xml2profir sage profir2sage \
#          xml-xercesc gprof
#
#include ../Make.inc
#
#EXTRA_DIST = rosehpct.cc
#
#if ROSE_BUILD_ROSEHPCT
#
#rosehpctlibdir = $(libdir)
#rosehpctlib_LTLIBRARIES = librosehpct.la
#librosehpct_la_SOURCES = rosehpct.cc
#librosehpct_la_LIBADD = \
#	util/libutil.la \
#	xml/libxml.la \
#	profir/libprofir.la \
#	xml2profir/libxml2profir.la \
#	sage/libsage.la \
#	gprof/libgprof.la \
#	profir2sage/libprofir2sage.la
#
#clean-local:
#	rm -rf $(CXX_TEMPLATE_OBJECTS) Templates.DB ii_files ti_files
#
#distclean-local:
#	rm -rf Templates.DB
#
#endif # ROSE_BUILD_ROSEHPCT
#
## eof
