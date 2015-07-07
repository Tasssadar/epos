/*
 *	epos/src/ttscp.cc
 *	(c) 1998-99 geo@cuni.cz
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

#include "epos.h"
#include "agent.h"
#include "client.h"


#ifdef SIGNAL_H
	#include <signal.h>
#endif

#ifdef HAVE_SYSLOG_H
	#include <syslog.h>
	
	int severity(int code);
#endif

hash_table<char, a_ttscp> *data_conns = new hash_table<char, a_ttscp> (30);
hash_table<char, a_ttscp> *ctrl_conns = new hash_table<char, a_ttscp> (30);
a_accept *accept_conn = NULL;

static inline void sendstring(const char *text)
{
	sputs(text, cfg->get__sd_out());
	sputs("\r\n", cfg->get__sd_out());
}

void reply(const char *text)
{
#ifdef HAVE_SYSLOG_H
	if (scfg->syslog && (scfg->full_syslog || text[0] > '2')) {
		if (text[3] != ' ' || text[0] > '9')
			shriek(461, "Ill-formed TTSCP error code supplied");
		syslog(severity(text[0]*100 + text[1]*10 + text[2] - '0'*111),
			scfg->log_codes ? text : text+4);
	}
#endif
	sendstring(text);
}

void reply(int code, const char *text)
{
	if (code == MUTE_EXCEPTION) return;
	if (0) fflush(NULL);
	char c[5];
	c[0] = code / 100 + '0';
	c[1] = code / 10 % 10 + '0';
	c[2] = code % 10 + '0';
	c[3] = ' ';
	c[4] = 0;
	sputs(c, cfg->get__sd_out());
	sendstring(text);
}

static inline void reply(const char *text, context *real_context)
{
	if (real_context) real_context->enter();
	reply(text);
	if (real_context) real_context->leave();
}

inline ACCESS access_level(int uid)
{
	if (uid == UID_ROOT) return A_ROOT;
	if (uid < 0) return A_PUBLIC;
	return A_AUTH;
}

int cmd_bad(char *)
{
	reply("411 He?");
	return PA_NEXT;
}

int cmd_user(char *param, a_ttscp *)
{
	if (!strcmp(param, "anonymous")) {
		reply("212 anonymous access allowed");
		return PA_NEXT;
	}
	reply("452 no such user");
//	reply("211 go ahead");			// 211 grant access
	return PA_NEXT;
}

int cmd_pass(char *param, a_ttscp *)
{
	if (this_context->uid == UID_ANON) {
		if (!strcmp(param, server_passwd) || scfg->debug_password
				&& !strcmp(param, scfg->debug_password)) {
			D_PRINT(2, "It's me!\n");
			this_context->uid = UID_SERVER;
			reply("200 OK");
			return PA_NEXT;
		}
	}
	reply("452 bad password");
//	reply("211 go ahead");			// 211 anonymous access
	return PA_NEXT;
}

int cmd_done(char *, a_ttscp *)
{
	reply("600 OK");
	return PA_DONE;
}

int cmd_intr(char *param, a_ttscp *a)
{
	/*
	 *	the 401 reply is sent by the stream
	 */

	a_ttscp *ctrl = ctrl_conns->translate(param);
	if (!ctrl) shriek(444, "ctrl connection handle bad");

	a->c->leave();
	ctrl->c->enter();
	bool result = ctrl->brk();
	ctrl->c->leave();
	a->c->enter();

	if (result) reply("200 OK");
	else reply("423 nothing to interrupt");
	return PA_NEXT;
}

static void strip(char *val)
{
	char *brutto;
	char *netto;
	for (netto=val, brutto = val; *brutto; brutto++, netto++) {
		*netto = *brutto;
		if (*brutto == ESCAPE) *netto = esctab->xlat(*++brutto);
	}				//resolve escape sequences
	*netto = 0;
}

static inline int do_set(char *param, context *real)
{
	char *value = split_string(param);
	epos_option *o = option_struct(param, this_lang->soft_opts);
	strip(value);

	if (o) {
		if (access_level(this_context->uid) >= o->writable) {
			if (set_option(o, value)) reply ("200 OK", real);
			else reply ("412 illegal value", real);
		} else reply ("451 Access denied", real);
	} else {
		reply("442 No such option", real);
	}
	return PA_NEXT;
}


int cmd_setl(char *param, a_ttscp *)
{
	stream *cs = cfg->current_stream;
	if (cs) {
		int result = do_set(param, NULL);
		return result;
	}
	return do_set(param, NULL);
}

int cmd_setg(char *param, a_ttscp *a)
{
	int failed = 0;
	const char *msg = NULL;
	int result = PA_NEXT;

	if (this_context->uid != UID_SERVER) shriek(451,  "Access denied.");
	a->c->leave();
	try {
		result = do_set(param, a->c);
	} catch (any_exception *e) {
		failed = e->code;
		msg = e->msg;
		delete e;
	}
	a->c->enter();
	if (failed) shriek(failed, msg);
	return result;
}


int cmd_help(char *param, a_ttscp *)
{
	if (param) {
		FILE *f;
		char *pathname;

		ttscp_cmd *cmd = ttscp_cmd_set;
		while (cmd->name && strncmp((char *)&cmd->name, param, 4)) cmd++;
		if (cmd->name) {
			sprintf(scratch, "%.4s %s", (char *)&cmd->name, cmd->short_help);
			sendstring(scratch);
		}

		pathname = compose_pathname(param, cfg->ttscp_help_dir);
		f = fopen(pathname, "rt");
		free(pathname);
		if (!f) {
			reply("441 No help on that");
			return PA_NEXT;
		}
		*scratch = ' ';
		while (fgets(scratch + 1, scfg->scratch_size - 3, f)) {
			int l = strlen(scratch);
			scratch[l-1] = '\r'; scratch[l] = '\n'; scratch[l+1] = 0;
			sputs(scratch, cfg->get__sd_out());
		}
	} else
		for (ttscp_cmd *cmd = ttscp_cmd_set; cmd->name; cmd++) {
			sprintf(scratch, "%.4s %s", (char *)&cmd->name, cmd->short_help);
			sendstring(scratch);
		}
	reply("200 OK");
	return PA_NEXT;
}

// void free_sleep_table();

int cmd_shutdown(char *, a_ttscp *a)
{
	if (this_context->uid != UID_SERVER) shriek(451,  "Access denied.");
	reply("800 shutdown OK");
	a->c->leave();
	ctrl_conns->remove(a->handle);
	delete a;		/* I'm not sure this is OK */
	server_shutdown();
	return PA_DONE;		/* naive compilers */
}

/************ ain't work (see the next line in server() just after the dispatcher call)
void cmd_restart(char *param)
{
	if (param) shriek(416, "shutdown should have no param");
	reply("800 will restart");	// may be mad
	register_child(0);		// kill all children, release memory
	leave_context(cfg->sd);
	close(cfg->sd);
	for (int i=0; i<n_contexts; i++)
		if (context_table[i]) forget_context(i);
	epos_reinit();
}
**************/


#define SHOW_SPACE " "

int do_show(char *param)
{
	int i;
	epos_option *o = option_struct(param, this_lang->soft_opts);

	if (o) {
		if (access_level(this_context->uid) >= o->readable) {
			sputs(SHOW_SPACE, cfg->get__sd_out());
			char *value = get_text_buffer(never_null(format_option(o)));
			decode_string(value, this_lang->charset);
			sendstring(value);
			reply("200 OK");
			free(value);
		} else reply("451 Access denied");
	} else {
//		if (!strcmp("language", param)) {
//			sendstring(this_lang->name);
//			reply("200 OK");
//			return PA_NEXT;
//		}
//		if (!strcmp("voice", param)) {
//			sendstring(this_voice->name);
//			reply("200 OK");
//			return PA_NEXT;
//		}
		if (!strcmp("languages", param)) {
			sputs(SHOW_SPACE, cfg->get__sd_out());
			int bufflen = 0;
			for (i=0; i < cfg->n_langs; i++) bufflen += strlen(cfg->langs[i]->name) + strlen(scfg->comma);
			char *result = (char *)xmalloc(bufflen + 1);
			strcpy(result, cfg->n_langs ? cfg->langs[0]->name : "(empty list)");
			for (i=1; i < cfg->n_langs; i++) {
				strcat(result, scfg->comma);
				strcat(result, cfg->langs[i]->name);
			}
			sendstring(result);
			free(result);
			reply("200 OK");
			return PA_NEXT;
		}
		if (!strcmp("voices", param)) {
			sputs(SHOW_SPACE, cfg->get__sd_out());
			int bufflen = 0;
			for (i=0; i < this_lang->n_voices; i++)
				bufflen += strlen(this_lang->voicetab[i]->name) + strlen(scfg->comma);
			char *result = (char *)xmalloc(bufflen + 1);
			strcpy(result, this_lang->n_voices ? this_lang->voicetab[0]->name : "(empty list)");
			for (i=1; i < this_lang->n_voices; i++) {
				strcat(result, scfg->comma);
				strcat(result, this_lang->voicetab[i]->name);
			}
			sendstring(result);
			free(result);
			reply("200 OK");
			return PA_NEXT;
		}
		reply("442 No such option");
	}
	return PA_NEXT;
}


int cmd_show(char *param, a_ttscp *a)
{
	stream *cs = cfg->current_stream;
	if (cs) {
		a->c->leave();
		cs->c->enter();
		int result = do_show(param);
		cs->c->leave();
		a->c->enter();
		return result;
	}
	return do_show(param);
}

int cmd_stream(char *param, a_ttscp *a)
{
	D_PRINT(1, "current_stream %p\n", cfg->current_stream);
	if (cfg->current_stream) delete cfg->current_stream;
	cfg->current_stream = NULL;
	cfg->current_stream = new stream(param, a->c);
	reply("200 OK");
	return PA_NEXT;
}

int cmd_apply(char *param, a_ttscp *a)
{
	int n;
	if (!sscanf(param, "%d", &n) || n <= 0) {
		reply("414 Bad size");
		return PA_NEXT;
	}
	D_PRINT(2, "cmd_apply calls %p\n", cfg->current_stream);
	if (!cfg->current_stream) {
		reply("415 strm command must be issued first");
		return PA_NEXT;
	}
	cfg->current_stream->apply(a, n);
	reply("112 started");
	return PA_WAIT;
}

int cmd_data(char *param, a_ttscp *a)
{
	a_ttscp *ctrl = ctrl_conns->translate(param);
	if (!ctrl) {
		reply("444 invalid ctrl connection handle");
		return PA_NEXT;
	}
	if (ctrl == a) {
		reply("444 cannot control myself");
		return PA_NEXT;
	}

	ctrl_conns->remove(a->handle);
	a->ctrl = ctrl;
	ctrl->deps->add(a->handle, a);
	data_conns->add(a->handle, a);

	reply("200 OK");
	return PA_WAIT;
}

int cmd_delhandle(char *param, a_ttscp *a)
{
	a_ttscp *da = data_conns->remove(param);
	if (!da) shriek(444, "invalid data connection handle");
	else if (da->ctrl) da->ctrl->deps->remove(param);
	a->c->leave();
	delete da;
	a->c->enter();
	reply("200 OK");
	return PA_NEXT;
}

/*
 *	The commands below are ordered by their estimated relative frequency
 *	in real world TTSCP sessions, for efficiency reasons.
 */

#define TTSCP_COMMAND(x,y,s,p) {(*(const int *)x), (&y), (s), (p)},

ttscp_cmd ttscp_cmd_set[] = {
	TTSCP_COMMAND("appl", cmd_apply, "n",		   PAR_REQ)
	TTSCP_COMMAND("setl", cmd_setl, "parameter value", PAR_REQ)
	TTSCP_COMMAND("show", cmd_show, "parameter",	   PAR_REQ)
	TTSCP_COMMAND("strm", cmd_stream, "stream",	   PAR_REQ)
	TTSCP_COMMAND("data", cmd_data, "handle",	   PAR_REQ)
	TTSCP_COMMAND("delh", cmd_delhandle, "handle",	   PAR_REQ)
	TTSCP_COMMAND("pass", cmd_pass, "password",	   PAR_REQ)
	TTSCP_COMMAND("user", cmd_user, "username",	   PAR_REQ)
	TTSCP_COMMAND("done", cmd_done, "",		   PAR_FORBIDDEN)
	TTSCP_COMMAND("intr", cmd_intr, "handle",	   PAR_REQ)
	TTSCP_COMMAND("help", cmd_help, "[command]",	   PAR_OPTIONAL)
	TTSCP_COMMAND("setg", cmd_setg, "parameter value", PAR_REQ)
	TTSCP_COMMAND("down", cmd_shutdown, "",		   PAR_FORBIDDEN)
	{0, NULL, ""}
};
