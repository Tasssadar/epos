/*
 *	epos/src/tcpsyn.h
 *	(c) 1998-99 Jirka Hanika, geo@cuni.cz
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
 *	tcpsyn is a special synthesis which connects to another TTSCP server
 *	and uses it to synthesize the segments. This incarnation
 *	is unfortunately synchronous (waits for completion).
 */

#ifndef EPOS_TCPSYN_H
#define EPOS_TCPSYN_H

class tcpsyn : public synth
{
	int cd;		/* control TTSCP connection	*/
	int dd;		/* data TTSCP connection	*/
	char *handle;	/* data connection handle	*/
   public:
	tcpsyn(voice *v);
	virtual ~tcpsyn(void);
	virtual void synseg(voice *v, segment d, wavefm *w);
	virtual void synsegs(voice *v, segment *d, int count, wavefm *w);
};

#endif		// EPOS_TCPSYN_H

