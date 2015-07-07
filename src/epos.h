/*
 *	epos/src/epos.h
 *	(c) Jirka Hanika, geo@cuni.cz
 *	(c) Petr Horak, horak@ure.cas.cz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

 *
 *	The GNU General Public License can be found in file doc/COPYING.
 *
 *	This is the main header file for the server side (service, daemon,
 *	or monolith incarnation of Epos).  It includes other header
 *	files generally considered to defined the internal interfaces
 *	of Epos, inlined templates etc.
 *
 */

#include "common.h"

#ifndef FORGET_PORTAUDIO
	#define WANT_PORTAUDIO_PABLIO
#endif

#ifdef WANT_DMALLOC
	#include <dmalloc.h>
#endif			      // new and delete are overloaded in interf.cc !

#ifndef  IGNORE_REGEX_RULES_OR_THE_FASTMATCH_OPTIMIZATION
	#define WANT_REGEX    // About always, we want to use the regex code
#endif

#ifdef WANT_REGEX
   extern "C" {
	#ifdef HAVE_RX_H
		#include <rx.h>
	#else
	    #ifdef HAVE_REGEX_H
		#include <regex.h>
	    #else
		#include "rx.h"
	    #endif
	#endif
   }
#else
	typedef void regex_t;
#endif

#ifndef HAVE_STRCASECMP
#ifdef  HAVE_STRICMP
	#define strcasecmp stricmp	// if only stricmp is defined
	#define strncasecmp strnicmp
#endif
#endif

#ifdef HAVE_GETCWD
#else
	#ifdef HAVE_DIRECT_H
		#include <direct.h>
		#define getcwd _getcwd
	#else
		#error No getcwd and no replacement for getcwd
	#endif
#endif


enum SUBST_METHOD {M_EXACT=0, M_SUBSTR=4, M_PROPER=7, M_LEFT=8, M_RIGHT=16, M_ONCE=32, M_NEGATED=64};
enum REPARENT {M_DELETE, M_RIGHTWARDS, M_LEFTWARDS};
enum FIT_IDX {Q_FREQ, Q_INTENS, Q_TIME};
enum OUT_ML { ML_NONE, ML_ANSI, ML_RTF};
#define OUT_MLstr "none:ansi:rtf:"
#define FITstr	"f:i:t:"
#define BOOLstr "false:true:off:on:no:yes:disabled:enabled:-:+:n:y:0:1:non::"
#define LIST_DELIM	 ':'

typedef char UNIT;
#define UNIT_MAX 12
#define U_ILL		127
#define U_DEFAULT	126
#define U_INHERIT	125
#define U_VOID		120

extern int unused_variable;
#define unuse(x) (unused_variable = (long int)(x));

extern const bool is_monolith;

struct file;
struct epos_option;
class  unit;

class stream;

typedef char wchar;

#define MAX_PATHNAME       256	  // only load_language uses this

#ifdef HAVE_UNISTD_H
	#define SLASH              '/'
	#define NULL_FILE	   "/dev/null"
	#define MODE_MASK	   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
	#define O_BINARY	   0
#else
	#define SLASH              '\\'
	#define NULL_FILE	   "NUL"
	#define MODE_MASK	   (S_IREAD | S_IWRITE)
#endif

#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
	#define HAVE_WINSOCK
	#define socky unsigned
#else
	#define socky signed
#endif


#include "hash.h"
#include "text.h"
#include "voice.h"
#include "function.h"
#include "options.h"
#include "interf.h"
#include "parser.h"
#include "unit.h"
#include "rule.h"              //See rules.h for additional #defines and enums
#include "waveform.h"
#include "synth.h"
#include "encoding.h"




