##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common

SUBDIRS = nnet . tests

bin_PROGRAMS = eposd say-epos
noinst_PROGRAMS = eposm gauge-epos

SRC_BASE = parser.cc unit.cc rule.cc \
text.cc encoding.cc interf.cc function.cc options.cc \
synth.cc waveform.cc voice.cc client.cc \
lpcsyn.cc tdpsyn.cc mbrsyn.cc tcpsyn.cc \
hash.cc hashi.cc \
common.h epos.h unit.h parser.h rule.h text.h encoding.h interf.h \
function.h options.h hash.h hashtmpl.h synth.h voice.h exc.h endian_utils.h \
waveform.h lpcsyn.h tdpsyn.h mbrsyn.h tcpsyn.h \
slab.h client.h agent.h 

eposd_SOURCES = daemon.cc agent.cc ttscp.cc nonblock.cc hashd.cc $(SRC_BASE)
eposm_SOURCES = monolith.cc $(SRC_BASE)

say_epos_SOURCES = say-epos.cc
gauge_epos_SOURCES = gauge-epos.cc

AM_CXXFLAGS+=@EXTRA_PORTAUDIO_DEFS@

LDADD_OTHER = nnet/libeposnnet.la @EXTRA_REGEX_OBJ@ @EXTRA_PORTAUDIO_LIBS@

eposd_LDADD = $(LDADD_OTHER)
eposm_LDADD = $(LDADD_OTHER)

nnet/libeposnnet.la:
	@cd nnet && $(MAKE) $(AM_MAKEFLAGS) libeposnnet.la

EXTRA_DIST = options.lst marker.cc block.cc exc.cc \
qnxipc.cc

cfginst:
	cd .. && make install
