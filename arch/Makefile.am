##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common
include data_dirs.am
include data_files.am

noinst_HEADERS = version.h

version.h: $(top_srcdir)/configure.ac
	@echo "#define VERSION \"$(VERSION)\"" > version.h

EXTRA_DIST = $(DATA_FILES) data_dirs.am data_files.am
