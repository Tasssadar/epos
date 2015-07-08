##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common
include data_dirs.am
include data_files.am

pkgdata_DATA = version
noinst_DATA = $(DATA_DIRS) $(DATA_FILES)

version: $(top_srcdir)/configure.ac
	@echo $(VERSION) > version

# In case of problems with the installation please note
# that all you need is a recursive copy
# of the content of this directory to the "right" place,
# such as cp -r * /usr/local/share/epos

install-data-local: installdirs
	@cd $(srcdir) && for i in $(DATA_DIRS); do \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/$$i;\
	done
	cd $(srcdir) && for i in $(DATA_FILES); do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdatadir)/$$i;\
	done

uninstall-local:
	for i in $(DATA_FILES); do \
		rm -f $(DESTDIR)$(pkgdatadir)/$$i;\
	done
	DATA_DIRS_REVERSED=;\
	for i in $(DATA_DIRS); do \
		DATA_DIRS_REVERSED="$$i $$DATA_DIRS_REVERSED";\
	done;\
	for i in $$DATA_DIRS_REVERSED; do \
		chmod o+w $(DESTDIR)$(pkgdatadir)/$$i/..;\
		rmdir $(DESTDIR)$(pkgdatadir)/$$i;\
	done


EXTRA_DIST = $(pkgdata_DATA) $(DATA_FILES) data_dirs.am data_files.am
