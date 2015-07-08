##	Process this file with automake run in the top directory to yield Makefile.in

TESTS=hard_zero_data_test				\
	intr_test					\
	legal_data_test long_data_test long_strm_test	\
	random_data_test				\
	soft_zero_data_test				\
	syn2_test 					\
	vogon_test 					\
	shutdown_test

include $(top_srcdir)/Makefile.common
OTHER_CXXFLAGS = -DSOURCEDIR=$(srcdir)

noinst_PROGRAMS = $(TESTS)
noinst_LTLIBRARIES = libepostest.la

LDADD = libepostest.la

libepostest_la_SOURCES = testbench.cc testbench.h

hard_zero_data_test_SOURCES = hard_zero_data_test.cc
intr_test_SOURCES = intr_test.cc
legal_data_test_SOURCES = legal_data_test.cc
long_data_test_SOURCES = long_data_test.cc
long_strm_test_SOURCES = long_strm_test.cc
random_data_test_SOURCES = random_data_test.cc
soft_zero_data_test_SOURCES = soft_zero_data_test.cc
syn2_test_SOURCES = syn2_test.cc
vogon_test_SOURCES = vogon_test.cc
shutdown_test_SOURCES = shutdown_test.cc
