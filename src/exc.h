/*
 *	epos/src/exc.h
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
 *	This file contains a few class decls for the exception hierarchy:
 *
 *	exception	- the base for exception inheritance, not used directly
 *	command_failed	- a TTSCP command fails, no problem
 *	connection_lost - a connection related problem, a whole TTSCP
 *				session is terminated for sanity
 *	fatal_error	- all connections should be terminated, the server aborts
 */

#ifndef EXCEPTION_ALREADY_DECLARED
#define EXCEPTION_ALREADY_DECLARED
struct any_exception
{
	int code;
	const char *msg;
	any_exception(int code, const char *msg);
};

#define MUTE_EXCEPTION	0	/* code of MUTE_EXCEPTION invalidates the message */

#endif

#ifndef DERIVE_EXCEPTION
#define DERIVE_EXCEPTION(base, derived) \
struct derived : public base\
{\
	derived();\
	derived(int n, const char *message);\
};
#endif


DERIVE_EXCEPTION(any_exception, command_failed)
DERIVE_EXCEPTION(any_exception, connection_lost)
DERIVE_EXCEPTION(any_exception, fatal_error)


#undef DERIVE_EXCEPTION
