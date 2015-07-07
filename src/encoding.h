/*
 *	epos/src/encoding.h
 *	(c) geo@cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *
 *	This file provides the interface for character set support.
 *
 *	Epos internally uses an 8-bit encoding which corresponds to
 *	the union of the necessary subsets of all 8-bit encodings
 *	met in configuration files, rule files, TTSCP sessions etc.
 *	We need the unicode tables only to perform character unification
 *	across code sets.
 *
 *	Charsets are internally assigned numeric codes as follows:
 *	0 - initial system charset (ASCII)
 *	1, 2, 3, ... - 8-bit charsets in load order
 *	-1 - CHARSET_NOT_AVAILABLE (invalid code number)
 *	-2, -3 - reserved
 *	-4 - UTF-8
 *	-5 - standard SAMPA
 *	-6, -7, ... non-standard SAMPAs (in naming order: alt 1, alt 2, ...)
 *	
 */
 
#ifndef FORGET_CHARSETS

	extern char *charset_list;

	#define NAME_UTF8		"utf-8"
	#define NAME_SAMPA_STD		"sampa-std"
	#define NAME_SAMPA_ALT		"sampa-alt-"

	#define CHARSET_ASCII		 0
	#define CHARSET_NOT_AVAILABLE	-1
	#define CHARSET_UTF8		-4
	#define CHARSET_SAMPA_STD	-5
	#define CHARSET_SAMPA_ALT(id)	(-(id) - 5)

	void load_default_charset();
	int load_charset(const char *);		/* return positive charset index or CHARSET_NOT_AVAILABLE */
	int load_named_sampa(const char *);	/* return negative charset index or CHARSET_NOT_AVAILABLE */

	void encode_string(unsigned char *, int cs, bool alloc);
	void decode_string(unsigned char *, int cs);
	inline void encode_string(char *s, int cs, bool alloc) { encode_string((unsigned char *)s, cs,alloc); }
	inline void decode_string(char *s, int cs) { decode_string((unsigned char *)s, cs); }

	const char *decode_to_sampa(unsigned char c, int sampa_alt);
	void decode_to_sampa(unsigned char *s, int sampa_alt);
	void encode_from_sampa(unsigned char *s, int sampa_alt);
	void update_sampa();
	void release_sampa();

	int get_count_allocated();
	const char *get_charset_name(int code);

	void shutdown_enc();

#else		// FORGET_CHARSETS
	#define CHARSET_NOT_AVAILABLE	-1

	inline void load_default_charset()		{ return; };
	inline int load_charset(const char *)		{ return CHARSET_NOT_AVAILABLE; };
	int load_named_sampa(const char *);	/* return negative charset index or CHARSET_NOT_AVAILABLE */
	inline void encode_string(char *, int, bool)	{ return; };
	inline void decode_string(char *, int)		{ return; };
	inline int get_count_allocated()		{ return 256; };
	const char *get_charset_name(int code);
	inline void shutdown_enc()			{ return; };
	const char *decode_to_sampa(unsigned char c, int sampa_alt);
	void encode_from_sampa(unsigned char *s, int sampa_alt);
	void update_sampa()				{ return; };
	void release_sampa()				{ return; };
#endif
