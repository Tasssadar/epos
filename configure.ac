
dnl Process this file with autoconf to produce a configure script

AC_INIT(src/unit.cc)
AM_INIT_AUTOMAKE(epos, 2.5.37)
AC_PREFIX_DEFAULT(/usr/local)

CCC="g++ c++ cc++ gcc cc"
AC_PROG_CC
AC_PROG_CXX
if test $GXX = yes; then 
	if $CXX -v 2>&1 | grep -q egcs
		then AM_CXXFLAGS="-Wall -Wunused -pipe -gstabs+ -fdefer-pop -O"
		else AM_CXXFLAGS="-Wall -Wunused -gstabs+ -O"
	fi
	else AM_CXXFLAGS=-O
fi

AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(CONFIGURED_CXXFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_ARG_WITH(dmalloc, AC_HELP_STRING([--with-dmalloc], [dmalloc debugging library (developers only)]),
	if test $withval = yes; then
		AM_CXXFLAGS="${AM_CXXFLAGS} -DWANT_DMALLOC"
		LIBS="$LIBS -ldmalloc -L/usr/local/lib"
	fi
)
dnl AC_ARG_WITH(regex, --with-regex       regular expression substitution, , )
AC_ARG_ENABLE(charsets, AC_HELP_STRING([--enable-charsets], [allow automatic conversions between character sets]),
	if test x$enableval = xno; then
		AM_CXXFLAGS="${AM_CXXFLAGS} -DFORGET_CHARSETS"
	fi,
)
AC_ARG_ENABLE(portaudio, AC_HELP_STRING([--enable-portaudio], [alternative sound output method]),
	[case "x$enableval" in
		xno)
			EXTRA_PORTAUDIO_DEFS="-DFORGET_PORTAUDIO"
			EXTRA_PORTAUDIO_LIBS=""
			;;
		xyes)
			EXTRA_PORTAUDIO_DEFS=""
			EXTRA_PORTAUDIO_LIBS="-lportaudio -lpablio -L../libs/portaudio/lib -lpthread"
			;;
		*)	AC_MSG_ERROR(bad value $enableval for --enable-portaudio) ;;
	esac],
	[EXTRA_PORTAUDIO_LIBS=""
	 EXTRA_PORTAUDIO_DEFS="-DFORGET_PORTAUDIO"]
)
AM_CONDITIONAL(PORTAUDIO, test "${EXTRA_PORTAUDIO_DEFS}" = "")

AC_LANG_CPLUSPLUS

AC_CHECK_PROGS(LNK, ld wlink, linker)
AC_CHECK_LIB(stdc++, main)
AC_SEARCH_LIBS(regcomp, c rx regex System, EXTRA_REGEX_OBJ="", EXTRA_REGEX_OBJ="../libs/regex/rx.$(OBJEXT)")
AC_SUBST([EXTRA_REGEX_OBJ])
AC_SUBST([EXTRA_PORTAUDIO_LIBS])
AC_SUBST([EXTRA_PORTAUDIO_DEFS])
AC_CHECK_LIB(socket, main)
AC_CHECK_LIB(lnsl, main)
AC_LANG_CPLUSPLUS
AC_PROG_INSTALL
AC_PROG_YACC
AM_PROG_LEX
AC_STDC_HEADERS
AC_HAVE_HEADERS(strings.h)
AC_HAVE_HEADERS(string.h)
AC_HAVE_HEADERS(stdint.h)
AC_HAVE_HEADERS(io.h)
AC_HAVE_HEADERS(process.h)
AC_HAVE_HEADERS(rx.h)
AC_HAVE_HEADERS(regex.h)
AC_HAVE_HEADERS(errno.h)
AC_HAVE_HEADERS(wait.h)
AC_HAVE_HEADERS(unistd.h)
AC_HAVE_HEADERS(unix.h)
AC_HAVE_HEADERS(signal.h)
AC_HAVE_HEADERS(syslog.h)
AC_HAVE_HEADERS(time.h)
AC_HAVE_HEADERS(sys/soundcard.h)
AC_HAVE_HEADERS(sys/audio.h)
AC_HAVE_HEADERS(sys/stat.h)
AC_HAVE_HEADERS(sys/ioctl.h)
AC_HAVE_HEADERS(sys/socket.h)
AC_HAVE_HEADERS(sys/select.h)
AC_HAVE_HEADERS(sys/time.h)
AC_HAVE_HEADERS(sys/types.h)
AC_HAVE_HEADERS(sys/termios.h)
AC_HAVE_HEADERS(netdb.h)
AC_HAVE_HEADERS(netinet/in.h)
AC_HAVE_HEADERS(linux/kd.h)
AC_HAVE_HEADERS(sys/kernel.h)
AC_HAVE_HEADERS(sys/name.h)
AC_HAVE_HEADERS(fcntl.h)
AC_HAVE_HEADERS(direct.h)
AC_HAVE_FUNCS(strerror vsnprintf strdup fork getegid strcasecmp stricmp getcwd gethostname gettimeofday qnx_name_attach abort kill pipe _pipe)

AC_CACHE_CHECK([for glibc regex error codes],
   epos_cv_symb_regeend,
   AC_TRY_COMPILE(
	#include <sys/types.h>
	#ifdef HAVE_RX_H
	#include <rx.h>
	#else
	#ifdef HAVE_REGEX_H
	#include <regex.h>
	#else
	#include "rx.h"
	#endif
	#endif
	,
	int result = REG_EEND;
	int result2 = REG_ESIZE;
	return 0;,
	epos_cv_symb_regeend=yes,
	epos_cv_symb_regeend=no)
)
if test "$epos_cv_symb_regeend" = yes; then
   AC_DEFINE([HAVE_REG_EEND], [], [ Define if your regex library defines the REG_EEND and REG_ESIZE error codes ])
fi

AC_CACHE_CHECK([for regmatch_t.rm_so],
   epos_cv_struct_rm_so,
   AC_TRY_COMPILE(
	#include <sys/types.h>
	#ifdef HAVE_RX_H
	#include <rx.h>
	#else
	#ifdef HAVE_REGEX_H
	#include <regex.h>
	#else
	#include "rx.h"
	#endif
	#endif
	,
	regmatch_t rt;
	rt.rm_so;
	return 0;,
	epos_cv_struct_rm_so=yes,
	epos_cv_struct_rm_so=no)
)
if test "$epos_cv_struct_rm_so" = yes; then
   AC_DEFINE([HAVE_RM_SO], [], [ Define if your regex library defines the REG_EEND and REG_ESIZE error codes ])
fi


AC_CACHE_CHECK([for terminate],
   epos_cv_fn_terminate,
   AC_TRY_COMPILE(
	[ #include <stdlib.h> ],
	[ throw new int; exit(1); terminate() ] ,
	[ epos_cv_fn_terminate=yes ],
	[ epos_cv_fn_terminate=no ],
	[ epos_cv_fn_terminate=yes ])
)
if test "$epos_cv_fn_terminate" = yes; then
   AC_DEFINE([HAVE_TERMINATE], [], [ Define if you have the built-in terminate function. ])
fi

AC_CACHE_CHECK([for explicit template instatiations],
   epos_cv_c_templ_inst,
   AC_TRY_COMPILE(
	[ template <class any_type> class some_type{}; template class some_type<int>; ]	,
	[],
	[ epos_cv_c_templ_inst=yes ],
	[ epos_cv_c_templ_inst=no ])
)
if test "$epos_cv_c_templ_inst" = yes; then
   AC_DEFINE([HAVE_TEMPL_INST], [], [ Define if your compiler understands explicit class template instantiations
   as in template class templ_type<param_type>; ]
)
fi

AC_CACHE_CHECK([for socklen_t],
   epos_cv_type_socklen,
   AC_TRY_COMPILE(
	#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
	#endif
	#include <sys/types.h>
	socklen_t x;,
	,
	epos_cv_type_socklen=yes,
	epos_cv_type_socklen=no)
)
if test "$epos_cv_type_socklen" = yes; then
   AC_DEFINE([HAVE_SOCKLEN_T], [], [ Define if you have socklen_t in sys/socket.h or sys/types.h. ])
fi

broken_enum_bitfields_test='\
	#include <stdlib.h>	\
	enum t { a, b, c };	\
	struct s		\
	{			\
		t m : 2;	\
		t n : 2;	\
		t o : 2;	\
	};			\
	s arr[[2]] = {{a, b, c},{c, c, c}};\
	int main()		\
	{			\
		exit (arr[[1]].n != 2);	\
		return 0;       \
	}\
	'

AC_CACHE_CHECK([for broken enum bitfields],
   epos_cv_c_broken_enum_bitfields,
   AC_TRY_RUN(
	$broken_enum_bitfields_test,
	epos_cv_c_broken_enum_bitfields=no,
	epos_cv_c_broken_enum_bitfields=yes,
	epos_cv_c_broken_enum_bitfields=yes)
)
if test "$epos_cv_c_broken_enum_bitfields" = yes; then
   AC_DEFINE([BROKEN_ENUM_BITFIELDS], [], [ Define if your compiler incorrectly treats enum bit fields as signed ])
fi

EPOS_CXX_OPTION(mafwks,
	[mac audio frameworks],
	["-framework CoreAudio -framework AudioToolbox"])
CONFIGURED_CXXFLAGS="$CONFIGURED_CXXFLAGS $mafwks"


AM_CONFIG_HEADER(config.h)
#AC_CONFIG_SUBDIRS(libs/portaudio)
AC_OUTPUT(Makefile src/Makefile src/nnet/Makefile src/tests/Makefile \
	libs/Makefile libs/regex/Makefile libs/portaudio/Makefile \
	cfg/Makefile arch/Makefile doc/Makefile)

