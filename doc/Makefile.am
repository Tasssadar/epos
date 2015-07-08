##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common

all: epos.html

CONTENTS = intro.sgml arch.sgml rules.sgml options.sgml ttscp.sgml security.sgml devel.sgml 

epos.sgml: header.sgml $(CONTENTS) footer.sgml
	@echo Updating epos.sgml
	@cat $^ | sed							\
		-e "s/CURRENT_EPOS_VERSION/$(VERSION)/g"	\
		-e "s/CURRENT_EPOS_DATE/`$(top_srcdir)/arch/unix/makedate`/g"	\
		-f $(top_srcdir)/arch/unix/doctwk.sed   > $@

epos.tex: epos.sgml
	@sgml2latex $^ --output=tex

epostwk.sgml: epos.sgml
	@sed '/^<sect/s:</\?tt>::g' <$^ >$@

epos.txt: epostwk.sgml
	@sgml2txt $^
	@mv epostwk.txt epos.txt

epos.html: epos.sgml
	@echo Converting to HTML
	@-sgml2html $^
	@-$(top_srcdir)/arch/unix/commat

epos.dvi: epos.sgml
	@sgml2latex --output=dvi --papersize=a4 --language=en $^

epos.ps: epos.sgml
	@sgml2latex --output=ps --papersize=a4 --language=en $^

epos.info: epos.sgml
	@sgml2info $^

#/usr/lib/texmf/source/latex/misc/linuxdoc-sgml.sty: /usr/lib/sgml-tools/linuxdoc-sgml.sty
#	@ln -s /usr/lib/sgml-tools/linuxdoc-sgml.sty $@

header.sgml: ../configure.ac
	@ echo Source code version may have changed, documentation adjusted.
	@ touch header.sgml

EXTRA_DIST = $(CONTENTS) header.sgml footer.sgml Todo Intro Files FAQ FORMATS

