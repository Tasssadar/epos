/*
 *	epos/src/agent.cc
 *	(c) 1998-01 geo@cuni.cz
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
#include "slab.h"

#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif

#ifdef HAVE_UNIX_H
	#include <unix.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifndef HAVE_SOCKLEN_T
	#define socklen_t int
#endif

#ifdef HAVE_WINSOCK2_H
	#include <winsock2.h>
#else
	#ifdef HAVE_WINSOCK_H
		#include <winsock.h>
	#endif
#endif

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif
	
#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifndef O_NONBLOCK
	#define O_NONBLOCK 0
#endif


#define DARK_ERRLOG 2	/* 2 == stderr; for global stdshriek and stddbg output */

#ifdef HAVE_GETTIMEOFDAY

inline void agent_profile(const char *s)
{
	if (!scfg->profile || !*scfg->profile) return;
	
	static FILE *log = NULL;
	if (!log) log = fopen(scfg->profile, "w", "profile");
	static struct timeval start, stop;
	if (!s) {
		if (gettimeofday(&start, NULL)) shriek(861, "profiler fails");
		int duration = start.tv_sec - stop.tv_sec;
		duration *= 1000000;
		duration += start.tv_usec - stop.tv_usec;
		fprintf(log, "%10ld", duration);
		fflush(log);
		if (gettimeofday(&start, NULL)) shriek(861, "profiler fails");
	} else {
		if (gettimeofday(&stop, NULL)) shriek(861, "profiler fails");
		int duration = stop.tv_sec - start.tv_sec;
		duration *= 1000000;
		duration += stop.tv_usec - start.tv_usec;
		fprintf(log, " %-13s%8ld\n", s, duration);
		fflush(log);
		if (gettimeofday(&stop, NULL)) shriek(861, "profiler fails");
	}
}

#else
	inline void agent_profile(const char *s) { return; }
#endif


agent::agent(DATA_TYPE typein, DATA_TYPE typeout)
{
	in = typein, out = typeout;
	next = prev = NULL, inb = NULL;
	pendin = pendout = NULL; pendcount = 0;
	c = NULL;
	dep = NULL;
	D_PRINT(1, "Creating a handler, intype %i, outtype %i\n", typein, typeout);
}

agent::~agent()
{
//	D_PRINT(1, "Handler deleted.\n");
}

inline void
agent::unquench()
{
	if (inb || pendin) schedule();
	/* can not unquench input agents */
}

void
agent::timeslice()
{
	c->enter();
	D_PRINT(1, "Timeslice for %s\n", name());
	D_PRINT(0, "pendcount %d, next->pendcount %d, pend_max %d\n", pendcount, next ? next->pendcount : -42, scfg->pend_max);
	if (next && next->pendcount > scfg->pend_max) {
		D_PRINT(1, "(satiated)\n");
		c->leave();
		return;
	}
	if (!inb && in != T_NONE) {
		if (!pendin) {
			/* We may take this path legally after an intr command */
			D_PRINT(2, "No input of type %i; shrugging off\n", in);
			c->leave();
			return;
		}
		D_PRINT(2, "Getting pending task, type %i\n", in);
		pend_ll *tmp = pendin;
		inb = tmp->d;
		pendin = tmp->next;
		delete tmp;
		if (pendcount-- == scfg->pend_min) prev->unquench();
		if (!pendin) {
			prev->pendout = NULL;
			if (pendcount) shriek(862, "pending count incorrect");
		}
	}
	try {
		agent_profile(NULL);
		run();
		agent_profile(name());
	} catch (command_failed *e) {
		if (!next) throw e;
		D_PRINT(2, "Processing failed, %d, %.60s\n", e->code, e->msg);
		reply(e->code, e->msg);
		delete e;
		finis(true);
	}
	if (pendcount && !inb) schedule();
	c->leave();

	agent *tmp;
	for (agent *a = dep; a; a = tmp) {
		D_PRINT(1, "Scheduling %s because of %s\n", a->name(), name());
		tmp = a->dep;
		a->schedule();
		a->dep = NULL;
	}
	dep = NULL;
}

bool
agent::mktask(int)
{
	return false;	// except for the input agent, agents can't start a task
}

inline void do_relax(void *ptr, DATA_TYPE type)
{
	if (ptr) switch(type) {
		case T_NONE:	break;
		case T_INPUT:	free(ptr); break;
		case T_STML:	
		case T_TEXT:	free(ptr); break;
		case T_UNITS:	delete ((unit *)ptr); break;
		case T_SEGS:	free(ptr); break;
		case T_SSIF:	free(ptr); break;
		case T_WAVEFM:	delete((wavefm *)ptr); break;
	}
}

void
agent::relax()
{
	if (inb) {
		do_relax(inb, in);	/* branch likely */
		inb = NULL;
	}
	if (!next) return;
	for (pend_ll *p = next->pendin; p != NULL; ) {
		do_relax(p->d, out);
		pend_ll *n = p->next;
		next->pendcount--;
		delete p;
		p = n;
	}
	pendout = next->pendin = NULL;
	if (next->pendcount) shriek(862, "pending count incorrect");
}

void
agent::finis(bool err)
{
	if (!next) shriek(861, "Non-module finished");
	agent *a = next;
	while (a->next) a = a->next;
	a->finis(err);
}

/*
 *	There is a slight difference between relax() and brk().  Both discard all
 *	pending input.  In addition, oa_wavefm::brk()  (only) discards any
 *	waveform already written to the sound card;
 *	relax() would call sync_soundcard() instead.
 */

bool agent::brk()
{
	relax();
	D_PRINT(1, "interrupting an agent, intype %i, outtype %i\n", in, out);
	return true;
}

void
agent::pass(void *ptr)
{
	if (!ptr) shriek(862, "Nothing to pass");
	if (!next) shriek(862, "Nowhere to pass to");
	if (pendout || next->inb) {
		pendout = (pendout ? pendout->next : next->pendin) = new pend_ll(ptr, NULL);
		next->pendcount++;
	} else {
		next->inb = ptr;
		next->schedule();
	}
}

class a_ascii : public agent
{
	virtual void run();
	virtual const char *name() { return "raw"; };
   public:
	a_ascii() : agent(T_TEXT, T_UNITS) {};
};

void
a_ascii::run()
{
	void *r = str2units((char *)inb);
	free(inb);
	inb = NULL;
	pass(r);
}


class a_stml : public agent
{
	virtual void run();
	virtual const char *name() { return "stml"; };
   public:
	a_stml() : agent(T_STML, T_UNITS) {};
};

void
a_stml::run()
{
	shriek(462, "STML parser not available");
}


class a_rules : public agent
{
	virtual void run();
	virtual const char *name() { return "rules"; };
   public:
	a_rules() : agent(T_UNITS, T_UNITS) {};
};

void
a_rules::run()
{
	void *r = inb;
	inb = NULL;
	this_lang->ruleset->apply((unit *)r);
	pass(r);
}


class a_print : public agent
{
	virtual void run();
	virtual const char *name() { return "print"; };
   public:
	a_print() : agent(T_UNITS, T_TEXT) {};
};

void
a_print::run()
{
	char *b;

	b = ((unit *)inb)->gather(false /* no ^$ */, true /* incl. ssegs */ );

	delete (unit *) inb;
	inb = NULL;
	pass(get_text_buffer(b));
}


class a_segs : public agent
{
	virtual void run();
	virtual const char *name() { return "segs"; };
	int position;
   public:
	a_segs() : agent(T_UNITS, T_SEGS) {position = 0;};
};

#define INIT_SEGS_BS	2048
#define INIT_SSIF_BS	4096

void
a_segs::run()
{
	int sbs = cfg->seg_buff_size ? cfg->seg_buff_size : INIT_SEGS_BS;
	segment *d = (segment *)xmalloc((sbs + 1) * sizeof(segment));
	segment *c = d + 1;
	int n;
	int items = 0;

	unit *root = *(unit **)&inb;
	root->project(scfg->_segm_level);
again:
	n = root->write_segs(c, position, sbs);
	D_PRINT(1, "Writing at most %d segs: wrote %d segs\n", sbs, n);
	position += n;
	items += n;
	if (n >= sbs) {
		if (cfg->seg_buff_size) {
			shriek(462, "Cannot combine nonzero seg_buff_size with the traditional SSIF");
		} else {
			d = (segment *)xrealloc(d, (sbs + 1 + position) * sizeof(segment));
			c = d + 1 + position;
			goto again;
		}
	} else {
		delete root;
		inb = NULL;
		position = 0;
	}
	d->code = items;
	d->nothing = d->ll = 0;
	D_PRINT(1, "agent segs generated %d segments\n", n);
	pass(d);
}

class a_ssif : public agent
{
	virtual void run();
	virtual const char *name() { return "to-mbrola"; };
	int position;
   public:
	a_ssif() : agent(T_UNITS, T_SSIF) {position = 0;};
};

void
a_ssif::run()
{
	int ssifbs = cfg->ssif_buff_size ? cfg->ssif_buff_size : INIT_SSIF_BS;
	char *d = (char *)xmalloc((ssifbs + 1));
	char *c = d /* + sizeof header */;
	int n;
	int items = 0;

	unit *root = *(unit **)&inb;
	root->project(scfg->_phone_level);
again:
	n = root->write_ssif(c, position, ssifbs);
	position += n;
	items += n;
	if (cfg->ssif_buff_size && n >= ssifbs) {
		d = (char *)xrealloc(d, (ssifbs + 1 + position));
		c = d /* + sizeof header */ + strlen(d);		/* FIXME: efficiency */
		goto again;
	} else {
		delete root;
		inb = NULL;
		position = 0;
	}
	D_PRINT(1, "agent ssif generated %d items\n", n);
	pass(d);
}

class a_chunk : public agent
{
	char *bm;
	virtual void run();
	virtual const char *name() { return "chunk"; };
   public:
	a_chunk() : agent(T_TEXT, T_TEXT) { bm = NULL; };
};

inline char *utt_break(char *t)		/* returns ptr past the last char */
{
	char *r = t;
	do {				/* split between . ? ... and whitespace */
		r += strcspn(r, ".?");
		r += strspn(r, ".?");
	} while (!strchr(WHITESPACE, r[0]));

	if (r - t > scfg->max_utterance) {
		r = t + strcspn(t, ".,?!:;");
		if (*r) r++;
	} else return r;

	if (r - t > scfg->max_utterance) {
		r = t + strcspn(t, ".,?!:;-=+_~@#$%^&*\\|/ \t\n");
		if (*r) r++;
	} else return r;

	if (r - t > scfg->max_utterance) {
		r = t + strcspn(t, "()<>{}[]'\"");
	} else return r;

	if (r - t > scfg->max_utterance) {
		if (scfg->split_utterance < (int)strlen(t))
			r = t + scfg->split_utterance;
	}

	return r;
}

void a_chunk::run()
{
	if (!bm) {
		bm = (char *)inb;
	}
	char *tmp = bm;
	bm = utt_break(bm);
	D_PRINT(2, "Utterance chunking about to split off %d bytes\n", bm - tmp);
	if (bm && *bm) {
		char h = bm[0];
		bm[0] = 0;
		pass(get_text_buffer(tmp));
		bm[0] = h;
		schedule();
	} else {
		pass(get_text_buffer(tmp));
		free(inb);
		inb = NULL;
		bm = NULL;
	}
}

class a_join : public agent
{
	char *heldout;
	virtual void run();
	virtual const char *name() { return "join"; };
   public:
	a_join() : agent(T_TEXT, T_TEXT) { heldout = NULL; };
};

void
a_join::run()
{
	char *b;
	char *last = (char *)inb;
	if (heldout) {
		b = (char *)xmalloc(strlen(heldout) + strlen(last) + 1);
		strcpy(b, heldout);
		strcat(b, last);
		free(last);
		free(heldout); heldout = NULL;
	} else b = last;

	if (*utt_break(b)) {
		char *decodable = get_text_buffer(b);
		free(b);
		pass(decodable);
	} else heldout = b;
}

class a_synth : public agent
{
	virtual void run();
	virtual const char *name() { return "synth"; };
   protected:
	void init_syn();
   public:
	a_synth(DATA_TYPE din, DATA_TYPE dout) : agent(din, dout) {};
	a_synth() : agent(T_SEGS, T_WAVEFM) {};
};

bool fallbackable_error(int code)
{
	switch (this_lang->fallback_mode) {
		case 0: return false;
		case 1: return true;
		case 4: return code == 445 || code / 10 == 47;
		case 7: return code / 10 == 47;
		default: return code == this_lang->fallback_mode;
	}
}

void
a_synth::init_syn()
{
	try {
		this_voice->syn = this_voice->setup_synth();
	} catch (command_failed *e) {
		if (fallbackable_error(e->code)
				&& this_lang->fallback_voice
				&& *this_lang->fallback_voice) {
			voice_switch(this_lang->fallback_voice);
			if (this_lang->permanent_fallbacks) {
				c->leave();
				voice_switch(this_lang->fallback_voice);
				c->enter();
			} 
			delete e;
			return;
		} else throw e;
	}
}

void
a_synth::run()
{
	if (!this_voice) shriek(861, "No current voice");
	while (!this_voice->syn) init_syn();	// at most twice
	wavefm *wfm = new wavefm(this_voice);
	this_voice->syn->synsegs(this_voice, (segment *)inb + 1, ((segment*)inb)->code, wfm);
	D_PRINT(1, "a_synth processes %d segments\n", ((segment *)inb)->code);

	free(inb);
	inb = NULL;
	pass(wfm);
}

class a_syn : public a_synth
{
	virtual void run();
   public:
	a_syn() : a_synth(T_SSIF, T_WAVEFM) {};
};

void
a_syn::run()
{
	if (!this_voice) shriek(861, "No current voice");
	while (!this_voice->syn) init_syn();	// at most twice
	wavefm *wfm = new wavefm(this_voice);
	this_voice->syn->synssif(this_voice, (char *)inb, wfm);
//	D_PRINT(1, "a_synth processes %d segments\n", ((segment *)inb)->code);

	free(inb);
	inb = NULL;
	pass(wfm);
}

template <DATA_TYPE TYPE> class a_type : public agent
{
	virtual void run();
	virtual const char *name() { return "type spec"; };
   public:
	a_type() : agent(TYPE, TYPE) {};
};

template <DATA_TYPE TYPE> void
a_type<TYPE>::run()
{
	void *outb = inb;
	inb = NULL;
	pass(outb);
}

class a_io : public agent
{
	virtual void run() = 0;
   protected:
	int socket;
	a_ttscp *dc;
	bool close_upon_exit;
   public:
	a_io(const char *, DATA_TYPE, DATA_TYPE);
	virtual ~a_io();
};


#define LOCALSOUNDAGENT "localsound"

extern int localsound;

socky int special_io(const char *name, DATA_TYPE intype)
{
	if (intype == T_INPUT || strcmp(name, LOCALSOUNDAGENT))
		shriek(415, "Bad stream component %s", name);
	if (!cfg->localsound) shriek(453, "Not allowed to use localsound");

	if (localsound != -1) return localsound;
	int r = open(scfg->local_sound_device, O_WRONLY | O_NONBLOCK);
	if (r == -1) shriek(462, "Could not open localsound device, error %d", errno);
	localsound = r;
	return r;
}

void stretch_sleep_tables(socky int);

a_io::a_io(const char *par, DATA_TYPE in, DATA_TYPE out) : agent(in, out)
{
	char *filename;

	close_upon_exit = false;
	dc = NULL;

	switch(*par) {
		case '$': dc = data_conns->translate(par + 1);
			  if (!dc) shriek(444, "Not a known data connection handle");
			  else socket = (in == T_INPUT ? dc->c->config->get__sd_in() : dc->c->config->get__sd_out());
			  break;
		case '/': if (in == T_INPUT && !cfg->readfs)
				shriek(454, "No filesystem inputs allowed");
			  if (out == T_NONE && !cfg->writefs)
			  	shriek(454, "No filesystem outputs allowed");
			  filename = limit_pathname(par, cfg->pseudo_root_dir);
			  socket = open(filename, in == T_INPUT ? O_RDONLY | O_NONBLOCK | O_BINARY
						: O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK | O_BINARY, MODE_MASK);
			  free(filename);
			  if (socket == -1) shriek(445, "Cannot open file %s", par);
			  else close_upon_exit = true;
			  break;
		case '#': socket = special_io(par + 1, in);
			  break;
		default:  shriek(462, "unimplemented i/o agent class");
			/* if ever adding classes, take care of closing/nonclosing
			 * the socket upon exit        */
	}
	if (socket >= 0) stretch_sleep_tables(socket);
	D_PRINT(0, "I/O agent is %p\n", this);
}

a_io::~a_io()
{
	D_PRINT(0, "~a_io\n");
	if (close_upon_exit) close_and_invalidate(socket);
}

class a_input : public a_io
{
	int toread;
	int offset;

	virtual void run();
	virtual const char *name() { return "input"; };
   protected:
	virtual bool mktask(int size);
   public:
	a_input(const char *);
};

a_input::a_input(const char *par) : a_io(par, T_INPUT, T_TEXT)
{
}


void a_input::run()
{
	int res;
	D_PRINT(0, "Entering input agent\n");
	if (block_table[socket]) {
		D_PRINT(2, "avoiding a nested input\n");
		block(socket);
		return;
	}
	res = yread(socket, (char *)inb + offset, toread - offset);
	if (res == -1 && errno == EAGAIN) {
		D_PRINT(2, "avoiding an EAGAIN on input\n");
		block(socket);
		return;
	}
	if (res <= 0) {
		if (!dc) {
			if (res == 0) shriek(438, "end of file");
			else shriek(437, "read error");
		}
		if (dc->ctrl) dc->ctrl->deps->remove(dc->handle);
		c->leave();
		delete data_conns->remove(dc->handle);
		c->enter();
		shriek(436, "data conn %d lost reading", socket);
	}
	offset += res;
	if (offset == toread) {
		void *dta = inb;
		((char *)dta)[offset] = 0;
		switch (out) {
			case T_SEGS:
				if ((((segment *)inb)->code + 1) * (int)sizeof(segment) != offset)
					shriek(432, "Received bad segments: %d segs, %d bytes",
						((segment *)inb)->code, offset);
				break;
			case T_WAVEFM:
				wavefm *w;
				w = new wavefm(this_voice);
				w->become(inb, offset);
				free(inb);
				inb = NULL; toread = offset = 0;
				pass(w);
				return;
			case T_TEXT:
				encode_string((char *)inb, this_lang->charset, false);
			default: ;	/* otherwise no problem */
		}
		D_PRINT(2, "Read and about to process %s\n", (char *)dta);
		inb = NULL;
		toread = 0;	// superfluous
		offset = 0;	// superfluous
		pass(dta);
	} else block(socket);
}

bool
a_input::mktask(int size)
{
	D_PRINT(2, "%d bytes to be read\n", size);
	if (inb) return false;	// busy
	toread = size;
	D_PRINT(0, "Alloc in a_input:\n");
	inb = get_text_buffer(size);
	offset = 0;
	block(socket);
	D_PRINT(1, "Apply task has been scheduled\n");
	return true;
}


class a_output : public a_io
{
	virtual int insize() = 0;
	virtual void decode() {};
	virtual void run();
	virtual const char *name() { return "output"; };
	bool foreground() {return ((stream *)next)->foreground(); };
	int written;
	bool decoded;
   protected:
	void report(bool total, int written);
   public:
	a_output(const char *par, DATA_TYPE i) : a_io(par, i, T_NONE) {written = 0; decoded = false;};
};

void
a_output::run()
{
	if (push_table[socket]) {
		push(socket);
		return;
	}
	int size = insize();
	if (!decoded) {
		decode();
		decoded = true;
	}
	
	if (written) {
		int now_written = ywrite(socket, (char *)inb + written, size - written);
		if (now_written == -1) {
			if (errno == EAGAIN) {
				push(socket);
				return;
			}
			shriek(436, "data connection %d lost during writing", socket);
		}
		report(false, now_written);
		written += now_written;
	} else {
		written = ywrite(socket, (char *)inb, size);
		if (written) {
			if (written == -1) {
				if (errno == EAGAIN) {
					push(socket);
					return;
				}
				shriek(436, "data connection %d lost before writing", socket);
			}
			report(true, size);
			report(false, written);
		}
	}
	if (written == size) {
		written = 0;
		decoded = false;
		relax();
		finis(false);
	} else push(socket);
}

void
a_output::report(bool total, int written)
{
//	inb = NULL;
	if (foreground()) {
		reply(total ? "122 total bytes" : "123 written bytes");
		sprintf(scratch, " %d", written);
		sputs(scratch, cfg->get__sd_out());
		sputs("\r\n", cfg->get__sd_out());
	}
}


template <DATA_TYPE type> class oa_ascii : public a_output
{
	virtual int insize() {
		return strlen((char *)inb);
	}
	virtual void decode() {
		decode_string((char *)inb, this_lang->charset);
	}
   public:
	oa_ascii(const char *s): a_output(s, type) {};
};

//class oa_stml : public a_output
//{
//	virtual int insize() {
//		return strlen((char *)inb);
//	}
//   public:
//	oa_stml(const char *s): a_output(s, T_STML) {};
//};

class oa_seg : public a_output
{
	virtual int insize() {
		D_PRINT(1, "Sending %d segments\n", ((segment *)inb)->code);
		return ((segment *)inb)->code * sizeof(segment);
	}
   public:
	oa_seg(const char *s): a_output(s, T_SEGS) {};
};

class oa_wavefm : public a_output
{
	virtual int insize() {
		shriek(462, "abstract oa_wavefm::insize"); return 0;
	}
	virtual void run();
	virtual bool brk();
	bool attached;
   public:
	oa_wavefm(const char *s): a_output(s, T_WAVEFM) {attached = false;};
};

void
oa_wavefm::run()
{
	wavefm *w = (wavefm *)inb;
	
	if (!attached && !push_table[socket]) {
		w->attach(socket);
		report(false, w->written);
		attached = true;
	}
	bool to_do;
	while ((to_do = w->flush()) && w->written > 0) {
		report(false, w->written);
	}

	if (to_do && w->written >= 0) push(socket);
	else {
		if (w->written == -1) {
			shriek(436, "data conn %d lost writing", socket);
		}
		if (w->written > 0) report(false, w->written);
		report(true, w->written_bytes());
		w->detach(socket);
		D_PRINT(1, "oa_wavefm wrote %d bytes\n", w->written_bytes());
		attached = false;
		delete w;
		inb = NULL;
		finis(false);
	}
}

bool
oa_wavefm::brk()
{
	if (inb) {

		wavefm *w = (wavefm *)inb;
		
		report(true, w->written_bytes());
		w->brk();
		if (attached) w->detach(socket);
		D_PRINT(1, "oa_wavefm wrote %d bytes\n", w->written_bytes());
		attached = false;
		relax();
	}
	finis(true);
	return true;
}


/*
 *	A stream is a linked list of agents, one of them being the
 *	stream agent itself. stream->head is an input agent.
 */

enum agent_type {AT_UNKNOWN, AT_CHUNK, AT_JOIN, AT_ASCII, AT_SSIF, AT_SEGS,
			AT_PRINT, AT_RULES, AT_STML, AT_SYN, AT_TRAD_SYNTH,
			AT_T_TEXT, AT_T_STML, AT_T_UNITS, AT_T_SSIF, AT_T_SEGS, AT_T_WAVEFM};
const char *agent_type_str = ":chunk:join:raw:dump:diphs:print:rules:stml:syn:synth:[t]:[s]:[i]:[p]:[d]:[w]:";

agent *make_agent(char *s, agent *preceding)
{
	if (strchr("@#/.$", *s)) {
		if (!preceding) return new a_input(s);
		switch (preceding->out) {
			case T_TEXT:   return new oa_ascii<T_TEXT>(s);
			case T_STML:   return new oa_ascii<T_STML>(s);
			case T_UNITS: shriek(448, "Units are hard to output");
			case T_SSIF:   return new oa_ascii<T_SSIF>(s);
			case T_SEGS:  return new oa_seg(s);
			case T_WAVEFM: return new oa_wavefm(s);
			default: shriek(462, "unimplmd oa");
		}
	}
	switch ((agent_type)str2enum(s, agent_type_str, AT_UNKNOWN))
	{
		case AT_UNKNOWN: shriek(861, "Agent type bug.");
		case AT_ASCII: return new a_ascii;
		case AT_CHUNK: return new a_chunk;
		case AT_SSIF:  return new a_ssif;
		case AT_SEGS:  return new a_segs;
		case AT_JOIN:  return new a_join;
		case AT_PRINT: return new a_print;
		case AT_RULES: return new a_rules;
		case AT_STML:  return new a_stml;
		case AT_SYN:   return new a_syn;
		case AT_TRAD_SYNTH: return new a_synth;

		case AT_T_TEXT:  return new a_type<T_TEXT>;
		case AT_T_STML:  return new a_type<T_STML>;
		case AT_T_UNITS: return new a_type<T_UNITS>;
		case AT_T_SSIF:  return new a_type<T_SSIF>;
		case AT_T_SEGS:  return new a_type<T_SEGS>;
		case AT_T_WAVEFM:return new a_type<T_WAVEFM>;

		default:       shriek(415, "Unknown agent type %s", s); return NULL;
	}
}


stream::stream(char *s, context *pc) : agent(T_NONE, T_NONE)
{
	char *tmp;
	agent *a;
	agent *l = head = NULL;

	callbk = NULL;
	c = pc;

	tmp = strchr(s, LIST_DELIM);
	if (!tmp) shriek(415, "Bad stream syntax");

	do {
		*tmp = 0;
		D_PRINT(1, "Making agent out of %s\n", s);
		try {
			a = make_agent(s, NULL); a->c = c;
		} catch (command_failed *e) {
			release_agents();
			throw e;
		}
		*tmp = LIST_DELIM;
		s = ++tmp;
		if (!l) head = a;
		else l->next = a;
		a->prev = l;
		l = a;
	} while((tmp = strchr(s, LIST_DELIM)));
	a = make_agent(s, a); a->c = c;
	l->next = a;
	a->prev = l;
	a->next = this;
	if (head->next != this) head->out = head->next->in;	/* adjust a_input type */
}

stream::~stream()
{
	release_agents();
//	delete c;
}

void
stream::release_agents()
{
	for (agent *a = head; a && a != this; ) {
		agent *b = a;
		a = a->next;
		delete b;
	}
}

void
stream::apply(agent *ref, int bytes)
{
	D_PRINT(2, "In stream::apply %p %p %d\n", head, ref, bytes);
	callbk = ref;
	head->mktask(bytes);
}

bool
stream::brk()
{
	if (!callbk) return false;	/* break only if running */
	for (agent *a = head; a && a != this; a = a->next)
		a->brk();
	reply("401 interrupted");
	return true;
}

void
stream::run()
{
	shriek(861, "scheduled a stream");
}

void
stream::finis(bool err)		// FIXME: simplify
{
	if (err) reply("191 finis recovery");
	D_PRINT(2, "submitted a subtask\n");
	for (agent *a = head; a != this; a = a->next) {
		if (a->inb || a->pendin) {
			D_PRINT(2, "more subtasks are pending\n");
			if (err) {
				a->relax();
				D_PRINT(2, "subtasks discarded\n");
				if  (callbk) callbk->schedule();
				else shriek(862, "double fault - no callback");
				callbk = NULL;
				return;
			}
			return;
		}
	}
	D_PRINT(2, "this has been the last subtask\n");
	if (!err) reply("200 output OK");
	if (callbk) callbk->schedule();
	else shriek(862, "no callback");
	callbk = NULL;
}

class a_disconnector : public agent
{
	virtual void run();
	virtual const char *name() { return "disconnector"; };
	a_protocol **to_delete;
	int last;
	int max;
   public:
	void disconnect(a_protocol *);
	a_disconnector();
	virtual ~a_disconnector();
};

a_disconnector::a_disconnector() : agent(T_NONE, T_NONE)
{
	to_delete = (a_protocol **)xmalloc(sizeof(void *));
	last = 0;
	max = 1;
}

a_disconnector::~a_disconnector()
{
	if (last) shriek(861, "Forgot to disconnect a protocol agent!");
	free(to_delete);
}

void a_disconnector::run()
{
	if (!last) shriek(861, "Spurious disconnect");
	delete to_delete[--last];
	to_delete[last] = NULL;
	D_PRINT(0, "Disconnect finished, last is %d\n", last);
}

void a_disconnector::disconnect(a_protocol *moriturus)
{
	if (last == max && ! (max & max - 1)) {
		max <<= 1;
		to_delete = (a_protocol **)xrealloc(to_delete, max * sizeof(void *));
	}
	to_delete[last++] = moriturus;
	D_PRINT(0, "Disconnect requested, last is %d\n", last);
	schedule();
}

a_disconnector disconnector;

a_protocol::a_protocol() : agent(T_NONE, T_NONE)
{
	sgets_buff = get_text_cmd_buffer();
	*sgets_buff = 0;
	buffer = get_text_cmd_buffer();
}

a_protocol::~a_protocol()
{
	free(sgets_buff);
	free(buffer);
}

void a_protocol::run()
{
	int res;
	res = sgets(buffer, cfg->max_net_cmd, cfg->get__sd_in(), sgets_buff);
	if (res < 0) {
		disconnect();
		return;
	}

	encode_string(buffer, this_lang->charset, false);	// FIXME (alloc->true)

	if ((int)strlen(buffer) >= cfg->max_net_cmd)
		shriek(413, "Received command is too long");
	if (res > 0 && *buffer) switch (run_command(buffer)) {
		case PA_NEXT:
			D_PRINT(0, "PA_NEXT\n");
			if (strchr(sgets_buff, '\n')) schedule();
			else block(cfg->get__sd_in());
			return;
		case PA_DONE:
			D_PRINT(0, "PA_DONE\n");
			disconnect();
			return;
		case PA_WAIT:
			D_PRINT(0, "PA_WAIT\n");
			return;
		default:
			shriek(861, "Bad protocol action\n");
	}
	block(cfg->get__sd_in());		/* partial line read */

//	leave_context(i);

//	non-blocking get_line etc.
}

a_ttscp::a_ttscp(socky int _sd_in, socky int _sd_out) : a_protocol()
{
	c = new context(_sd_in, _sd_out);
	c->enter();
	
	handle = (char *)malloc(cfg->handle_size + 1);
	do make_rnd_passwd(handle, cfg->handle_size);
		while (data_conns->translate(handle));
	ctrl_conns->add(handle, this);

	sputs(
		"TTSCP spoken here\r\n"
		"protocol: 0\r\n"
		"extensions:\r\n"
		"server: Epos\r\n"
		"release: " VERSION "\r\n"
		"handle: ", cfg->get__sd_out());
	sputs(		handle, cfg->get__sd_out());
	sputs(	"\r\n", cfg->get__sd_out());
	ctrl = NULL;
	deps = new hash_table<char, a_ttscp>(4);
	deps->dupdata = deps->dupkey = false;
	c->leave();
	block(_sd_in);
}

/*
 *	Warning: the following destructor runs in the master context;
 *	therefore, be careful with using cfg etc.
 */

a_ttscp::~a_ttscp()
{
	c->enter();
	if (cfg->current_stream) delete cfg->current_stream;
	cfg->current_stream = NULL;
	D_PRINT(2, "deleted context closes fd %d and %d\n", cfg->get__sd_in(), cfg->get__sd_out());
	c->leave();
	while (deps->items) {
		a_ttscp *tmp = deps->translate(deps->get_random());
		deps->remove(tmp->handle);
		delete data_conns->remove(tmp->handle);
	}
	delete deps;
	c->enter();
	if (cfg->get__sd_in() != -1)
		close_and_invalidate(cfg->get__sd_in());
	if (cfg->get__sd_out() != -1 && cfg->get__sd_out() != cfg->get__sd_in())
		close_and_invalidate(cfg->get__sd_out());
	if (data_conns->translate(handle) || ctrl_conns->translate(handle))
		shriek(862, "Forgot to forget a_ttscp");

	free(handle);
	c->leave();
	delete c;
	// close the descriptor? no, the ~context does that
}

bool
a_ttscp::brk()
{
	if (c->config->current_stream)
		return c->config->current_stream->brk();
	return false;
}

int
a_ttscp::run_command(char *cmd)
{
	char *keyword;
	char *param;

	D_PRINT(2, "[ cmd] %s\n", cmd);

	keyword = cmd + strspn (cmd, WHITESPACE);
	param = keyword + strcspn(keyword, WHITESPACE);
	if (param - keyword != 4) goto bad;	/* all cmds are 4 chars */

	if (!*param) param = NULL;
	else *param++ = 0;

	if (param) param += strspn(param, WHITESPACE);

	int i;
	for (i=0; ttscp_cmd_set[i].name &&
		(*(const int *)keyword != *(const int *)&ttscp_cmd_set[i].name);)
			i++;
	if (!ttscp_cmd_set[i].name) goto bad;
	
	if (!param && ttscp_cmd_set[i].param == PAR_REQ) {
		reply("417 parameter missing");
		return PA_NEXT;
	}
	if (param && ttscp_cmd_set[i].param == PAR_FORBIDDEN) {
		reply("416 parameter not allowed");
		return PA_NEXT;
	}

	try {
		return ttscp_cmd_set[i].impl(param, this);
	} catch (command_failed *e) {
		D_PRINT(2, "Command failed, %d, %.60s\n", e->code, e->msg);
		reply(e->code, e->msg);
		delete e;
		return PA_NEXT;
	} catch (connection_lost *d) {
		D_PRINT(2, "Releasing a TTSCP control connection, %d, %.60s\n", d->code, d->msg);
		reply(d->code, d->msg);		/* just in case */
		reply(201, fmt("debug %d", cfg->get__sd_in()));
		delete d;
		return PA_DONE;
	}

   bad:
	cmd_bad(cmd);
	return PA_NEXT;
}

void
a_ttscp::disconnect()
{
	D_PRINT(2, "ctrl conn %d lost\n", cfg->get__sd_in());
	if (this != ctrl_conns->remove(handle) /* && this != data_conns->remove(handle) */ )
		shriek(862, "Failed to disconnect a ctrl connection");
	disconnector.disconnect(this);
}

void make_nonblocking(int f)
{
#ifdef HAVE_WINSOCK
	ioctlsocket((unsigned long int)f, FIONBIO, (unsigned long int *)&make_nonblocking);	// &make_nonblocking is a dummy non-NULL pointer
#else
	fcntl(f, F_SETFL, O_NONBLOCK);
#endif
}

#ifndef HAVE_GETHOSTNAME
int gethostname(char *b, size_t)
{
	strcpy(b, "localhost");
	return 0;
}
#endif

a_accept::a_accept() : agent(T_NONE, T_NONE)
{
	static sockaddr_in sa;
	
	char one = 1;

//	c = new context(-1, /*** dark errors ***/ DARK_ERRLOG);	//FIXME
//	c->enter();

	listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&sa, 0, sizeof(sa));
	gethostname(scratch, scfg->scratch_size - 1);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(scfg->local_only ? INADDR_LOOPBACK : INADDR_ANY);
	sa.sin_port = htons(scfg->listen_port);
	setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	D_PRINT(3, "* Binding to the TTSCP port %d.\n", scfg->listen_port);
	if (bind(listening, (sockaddr *)&sa, sizeof (sa))) shriek(871, "Could not bind");
	if (listen(listening, 64)) shriek(871, "Could not listen");
	make_nonblocking(listening);

	ia.sin_family = AF_INET;
	ia.sin_addr.s_addr = htonl(INADDR_ANY);
	ia.sin_port = 0;

	block(listening);
//	c->leave();
}

a_accept::~a_accept()
{
	close (listening);
//	close (c->config->_sd_in);
//	delete c;
}

void
a_accept::run()
{
	static socklen_t sia = sizeof(sockaddr);	// Will __QNX__ complain?
	int f = accept(listening, (sockaddr *)&ia, &sia);
	if (f == -1) {
//		shriek(871, "Cannot accept() - network problem (errno %d)", errno);
		D_PRINT(3, "Cannot accept() - errno %d! Madly looping.\n", errno);
		if (errno != EAGAIN) schedule();
		return;
	}
	make_nonblocking(f);
	D_PRINT(2, "Accepted %d (on %d).\n", f, listening);
//	c->leave();
	unuse(new a_ttscp(f, f));
//	c->enter();
	block(listening);
}

struct sched_aq
{
	agent *ag;
	sched_aq *next;
	sched_aq *prev;

	void *operator new(size_t size);
	void operator delete(void *ptr);
};

SLABIFY(sched_aq, sched_aq_slab, 341, shutdown_sched_aq);

sched_aq *sched_head = NULL;
sched_aq *sched_tail = NULL;

int runnable_agents = 0;

void
agent::schedule()
{
	if (!this) shriek(862, "scheduling garbage");
	runnable_agents++;
	D_PRINT(1, "%d runnable agents\n", runnable_agents);
	sched_aq *tmp = new sched_aq;
	tmp->ag = this;
	tmp->prev = NULL;
	tmp->next = sched_head;
	if (sched_head) sched_head->prev = tmp;
	else sched_tail = tmp;
	sched_head = tmp;
}

agent *sched_sel()
{
	agent *r;
	sched_aq *tmp;
	if (!sched_tail) shriek(862, "agent queue empty");
	if (!sched_tail->prev) sched_head = NULL;
	else sched_tail->prev->next = NULL;
	r = sched_tail->ag;
	tmp = sched_tail;
	sched_tail = sched_tail->prev;
	delete tmp;
	runnable_agents--;
	D_PRINT(1, "Agent %s\n", r->name());
	return r;
}

void shutdown_agent_queue()
{
	for (sched_aq *tmp = sched_head; tmp; tmp = tmp->next)
		delete tmp;
}

agent **block_table = (agent **)xmalloc(1);
agent **push_table = (agent **)xmalloc(1);
fd_set block_set;
fd_set push_set;
socky int select_fd_max = 0;

void stretch_sleep_tables(socky int fd)
{
	if (select_fd_max <= fd) {
		block_table = (agent **)xrealloc(block_table, (fd + 1) * sizeof(agent *));
		push_table = (agent **)xrealloc(push_table, (fd + 1) * sizeof(agent *));
		for ( ; select_fd_max <= fd; select_fd_max++) {
			block_table[select_fd_max] = NULL;
			push_table[select_fd_max] = NULL;
		}
	}
}

/*
 *	agent::run() should return after calling block() or push()
 */

void
agent::block(socky int fd)
{
	D_PRINT(1, "Sleeping on %d\n", fd);
	stretch_sleep_tables(fd);
	if (block_table[fd]) {
		agent *a;

		if (this == block_table[fd])
			shriek(861, "Resleeping on %d", fd);
		if (!FD_ISSET(fd, &block_set))
			shriek(861, "Countersleeping on %d", fd);
		for (a = block_table[fd]; a->dep; a = a->dep) ;
		a->dep = this;
	} else {
		block_table[fd] = this;
		FD_SET(fd, &block_set);
	}
	return;
}

void
agent::push(socky int fd)
{
	D_PRINT(1, "Pushing on %d\n", fd);
	stretch_sleep_tables(fd);
	if (push_table[fd]) {
		agent *a;

		if (this == push_table[fd])
			shriek(861, "Resleeping on %d", fd);
		if (!FD_ISSET(fd, &push_set))
			shriek(861, "Countersleeping on %d", fd);
		for (a = push_table[fd]; a->dep; a = a->dep) ;
		a->dep = this;
	} else {
		push_table[fd] = this;
		FD_SET(fd, &push_set);
	}
	return;
}

