/*
 *	epos/src/hashd.cc
 *	(c) geo@cuni.cz (Jirka Hanika)
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
 *
 */

#include "epos.h"
#include "hash.h"
#include "hashtmpl.h"
#include "agent.h"	/* class a_ttscp */

/*
 *	This is a good place for hash table instantiation which is
 *	needed by the daemon but not the monolith code.
 */

#ifdef HAVE_TEMPL_INST

template class hash_table<char, a_ttscp>;

#else	// else it is WatcomC

extern hash_table<char, a_ttscp> *_dummy_a_ttscp_hash_tmp_inst;

#endif

