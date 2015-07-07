/*
 *	epos/src/exc.cc
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
 *	The most boring file around.
 */

inline any_exception::any_exception(int n, const char *message)
{
	code = n;
	msg = message;
}

#define DERIVE_EXCEPTION(base, derived) \
derived::derived() : base(MUTE_EXCEPTION, "") {}\
derived::derived(int n, const char *message) : base(n, message) {}

#include "exc.h"
