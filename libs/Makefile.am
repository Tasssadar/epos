##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common
include data_files.am

if PORTAUDIO
MAYBE_PORTAUDIO=portaudio
else !PORTAUDIO
MAYBE_PORTAUDIO=
endif !PORTAUDIO

SUBDIRS = regex $(MAYBE_PORTAUDIO)

EXTRA_DIST = $(DATA_FILES)
