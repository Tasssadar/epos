/*
 *	epos/src/common.h
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
 *	This header file is shared by the server side and client side.
 *	It is the only file the clients need to include.
 *
 */


#ifndef EPOS_COMMON_H
#define EPOS_COMMON_H

#define MAINTAINER  "Jirka Hanika"
#define MAIL        "geo@cuni.cz"
#define TTSCP_PORT  8778

#include "config.h"

#ifndef VERSION
	#include "..\arch\version.h"
#endif

#include <stdio.h>
#include <stdlib.h>           // just exit() in shriek(), malloc &co...
#include <stdarg.h>

#ifdef HAVE_STDINT_H
	#include <stdint.h>
#else
	/* the following list is probably incomplete/buggy, please
	   report any ports which do need to actually use this     */
	typedef short int int16_t;
	typedef int int32_t;
	typedef unsigned short int uint16_t;
	typedef unsigned int uint32_t;
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_STRING_H
	#include <string.h>
#else
   #ifdef HAVE_STRINGS_H
	#include <strings.h>
   #else
	#error String library misconfigured. No.
   #endif
#endif

#ifndef HAVE_STRDUP
	inline char *strdup(const char *src) { return strcpy((char *)malloc(strlen(src)+1), src); };
#endif

#include "exc.h"

struct segment {	/* This structure is part of an obsolete interface. */
	int16_t code;
	char nothing;
	char ll;
	int f,e,t;
};


#endif   //#ifndef EPOS_COMMON_H
