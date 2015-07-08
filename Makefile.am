##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common
export VERSION

# Minimum automake version:

AUTOMAKE_OPTIONS=1.4 foreign

SUBDIRS = libs src cfg arch doc

EXTRA_DIST=Makefile.common Changes WELCOME WELCOME.MacOSX

#distcleancheck:
#	@@:

dist-hook:
	@. $(distdir)/arch/unix/test.version

upload:
	@. $(srcdir)/arch/unix/upload/upload $(VERSION)
