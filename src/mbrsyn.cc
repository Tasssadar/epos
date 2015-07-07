/*
 *	epos/src/mbrsyn.cc
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
 *
 */

#include "epos.h"
#include "mbrsyn.h"

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif

#ifdef HAVE_PROCESS_H
	#include <process.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifndef HAVE_PIPE
	#ifdef HAVE__PIPE
		inline int pipe(int arg[2]) { return _pipe(arg, 1000000, 0); };
	#else
		#error Apparently you have neither pipe() nor _pipe()
	#endif
#endif


#define there there_pipe[1]
#define back  back_pipe[0]

void
mbrsyn::restart_mbrola(voice *v)
{
	int p[2];
	int q[2];
	if (pipe(p)) shriek(463, "Failed to pipe mbrola");
	if (pipe(q)) shriek(463, "Failed to pipe mbrola");
	
	int tmp = fork();
	switch (tmp) {
	case -1: shriek(463, "Failed to fork mbrola");
	case 0:	
		if (p[0]) dup2(p[0], 0);
		dup2(q[1], 1);
		close(p[1]);
		close(q[0]);
		char *inv_pathname;
		char *mbrola_binary;
		inv_pathname = compose_pathname(v->models, v->location, scfg->inv_base_dir);
		mbrola_binary = compose_pathname(cfg->mbrola_binary, v->location, scfg->inv_base_dir);
		if (execl(mbrola_binary, mbrola_binary, "-e", inv_pathname, "-", "-.wav", NULL))
			shriek(463, "Failed to exec mbrola %s with voice %s", mbrola_binary, inv_pathname);
		break;
	default:
		there = p[1];
		back = q[0];
		close(p[0]);
		close(q[1]);
		pid = tmp;
		return;
	}
}

mbrsyn::mbrsyn(voice *v)
{
	restart_mbrola(v);
}

mbrsyn::~mbrsyn()
{
#ifdef HAVE_KILL
	kill(pid, SIGQUIT);
#endif
	close(there_pipe[0]);
	close(there_pipe[1]);
	close(back_pipe[0]);
	close(back_pipe[1]);
}

void
mbrsyn::synssif(voice *v, char *b, wavefm *w)
{
	char wb[1024 * 256];
	int l = 0;
	int offset = 0;

	do {
		offset += l;
		l = write(there, b + offset, strlen(b + offset));
//		if (l == -1)
//			shriek(461, "Apparently MBROLA didn't start\n");
		D_PRINT(1, "Sent %d bytes to MBROLA\n", l);
	} while (l < (int)strlen(b + offset));

	close(there);

	offset = l = 0;
	do {
		if (l == -1) l = 0;
		offset += l;
		l = read(back, wb + offset, 1024 * 256 - offset);
		D_PRINT(1, "Received %d bytes from MBROLA\n", l);
	} while (l > 0);

	close(back);

	restart_mbrola(v);

	w->become(wb, offset);
}

void
mbrsyn::synsegs(voice *v, segment *d, int n, wavefm *w)
{
	shriek(462, "mbrsyn cannot render the legacy SSIF.  Hint: 'say-epos -s'.");
}
