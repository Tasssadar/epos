/*
 *	epos/src/monolith.cc
 *	(c) 1996-01 geo@cuni.cz
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
 *	Please note that the monolithic binary is not supported
 *	nor documented.  If you don't know what it is, then you
 *	don't want to use it.
 *
 *	The reason is that monolithic TTS systems are either slow
 *	or of limited configurability.  We are of unlimited
 *	configurability, so the monolithic Epos is slow   :-)
 */

#include "epos.h"
#include "client.h"

const bool is_monolith = true;

#ifdef HAVE_UNISTD_H		// fork() only
	#include <unistd.h>
#else
	int fork();
#endif

// int session_uid = 0;

void close_and_invalidate(socky int sd)
{
	async_close(sd);
}

void use_async_sputs()
{
};

void free_replier_table()
{
};

void shutdown_sched_aq()
{
};

int submain()
{
	unit *root;

	root = str2units(scfg->_input_text);
	this_lang->ruleset->apply(root);
	root->fout(NULL);


	if (scfg->show_segments | scfg->play_segments | scfg->immed_segments | scfg->show_phones) {
		if (scfg->play_segments) {
			if (scfg->forking) {
				switch (fork()) {
					case 0:	//ds_used_cnt++;
						play_segments(root, this_voice);
						return 0;
					case -1:play_segments(root, this_voice);
					default:;
				}
			} else play_segments(root, this_voice);
		}
		if (scfg->show_phones) root->show_phones();
		if (scfg->show_segments) show_segments(root);
	}

//	if (scfg->neuronet) root->nnet_out(scfg->nnet_file, scfg->matlab_dir);
	delete(root);
//	fprintf(stdout,"***** The End. ******************************\n");
	return 0;
}

int main(int argc, char **argv)
{
	fprintf(stdout,"***************************************************\n");
	fprintf(stdout,"*  This binary \"eposm\" is an obsolete monolithic  *\n");
	fprintf(stdout,"*  incarnation of Epos and is NOT SUPPORTED.      *\n");
	fprintf(stdout,"*  Read the WELCOME file if you feel lost.        *\n");
	fprintf(stdout,"***************************************************\n");

	try {
		set_cmd_line(argc, argv);
		scfg->play_segments = true;
		epos_init();
		submain();
//		epos_done();
		return 0;
	} catch (any_exception *e) {
		delete(e);
		return 4;
	}
}

