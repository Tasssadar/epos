/*
 *	epos/src/mbrsyn.h
 *	(c) 2000 Jirka Hanika, geo@cuni.cz
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
 */

#ifndef EPOS_MBR_H
#define EPOS_MBR_H

class mbrsyn : public synth
{
	int there_pipe[2];
	int back_pipe[2];
	int pid;

   public:
	mbrsyn(voice *v);
	~mbrsyn();
	virtual void synsegs(voice *v, segment *d, int count, wavefm *w);
	virtual void synssif(voice *v, char *, wavefm *w);
	void restart_mbrola(voice *);
};

#endif		// EPOS_MBR_H
