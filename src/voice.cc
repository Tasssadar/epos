/*
 *	epos/src/voice.cc
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
//#include "ktdsyn.h"
//#include "ptdsyn.h"
#include "tdpsyn.h"
#include "lpcsyn.h"
#include "mbrsyn.h"
#include "tcpsyn.h"

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
	#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_AUDIO_H
	#include <sys/audio.h>
#endif

//#ifdef HAVE_SYS_SOUNDCARD_H
//#include <sys/soundcard.h>
//#endif

#ifdef HAVE_IO_H
	#include <io.h>		/* open, write, (ioctl,) ... */
#endif

//#pragma hdrstop

#ifdef KDGETLED		// Feel free to disable or delete the following stuff
inline void mark_voice(int a)
{
	static int voices_attached = 0;
	voices_attached += a;
	int kbd_flags = 0;
	ioctl(1, KDGETLED, kbd_flags);
	kbd_flags = kbd_flags & ~LED_SCR;
	if (voices_attached) kbd_flags |= LED_SCR;
	ioctl(1, KDSETLED, kbd_flags);
}
#else
inline void mark_voice(int) {};
#endif

#define   EQUALSIGN	'='
#define   OPENING	'('
#define   CLOSING	')'




// int n_langs = 0;
// int allocated_langs = 0;
// lang **langs = NULL;

// voice *this_voice = NULL;
// lang *this_lang = NULL;

#define EO  {NULL, O_INT, OS_LANG, A_PUBLIC, A_PUBLIC, false, false, -1},
#define TWENTY_EXTRA_OPTIONS EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO

#define CONFIG_LANG_DESCRIBE
epos_option langoptlist[] = {
	#include "options.lst"

	{"L:voice" + 2, O_VOICE, OS_LANG, A_PUBLIC, A_PUBLIC, false, false, 0},
	{"", O_INT, OS_LANG, A_PUBLIC, A_PUBLIC, false, false, -3},
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	{NULL, O_INT, OS_LANG, A_PUBLIC, A_PUBLIC, false, false, -2},
};

#define CONFIG_VOICE_DESCRIBE
epos_option voiceoptlist[] = {
	#include "options.lst"
	{"", O_INT, OS_VOICE, A_PUBLIC, A_PUBLIC, false, false, -3},
	{NULL, O_INT, OS_VOICE, A_PUBLIC, A_PUBLIC, false, false, -2},
};


// #include "slab.h"

// SLABIFY(lang, lang_slab, 2, shutdown_langs)

#define CONFIG_LANG_INITIALIZE
lang::lang(const char *filename, const char *dirname) : cowabilium()
{
	#include "options.lst"
//	name = "(unnamed)";
	ruleset = NULL;
	soft_opts = NULL;
	soft_defaults = NULL;
	n_voices = 0;
	voicetab = NULL;
	default_voice = 0;
	char_level = 0;
	load_config(filename, dirname, "language", OS_LANG, this, NULL);
	if (soft_options) add_soft_opts(soft_options);
	if (voices) add_voices(voices);
}

lang::~lang()
{
	for (int i=0; i<n_voices; i++) delete voicetab[i];
	if (voicetab) free(voicetab);
	if (ruleset) delete ruleset;
	if (char_level) free(char_level);
	if (soft_opts) {
		while (soft_opts->items) {
			char *tmp = soft_opts->get_random();
			if (tmp[0] != 'V' || tmp[1] != ':')
				tmp -= 2;
			delete soft_opts->remove(tmp);
			delete soft_opts->remove(tmp + 2);
			free(tmp);
		}
		delete soft_opts;
	}
	D_PRINT(3, "Disposed language %s\n", name);
	cow_unstring(this, langoptlist);
	if (soft_defaults) free(soft_defaults);
}

/*
 *	add_voice will horribly fail if cfg != master_context->config
 *	(the shadow voice lists cannot grow)
 */

void
lang::add_voice(const char *voice_name)
{
	char *filename = (char *)xmalloc(strlen(voice_name) + 6);
	char *dirname = (char *)xmalloc(strlen(name) + strlen(scfg->voice_base_dir) + 6);
	sprintf(filename, "%s.ini", voice_name);
	sprintf(dirname, "%s%c%s", scfg->voice_base_dir, SLASH, name);
	if (*voice_name) {
		if (!voicetab) voicetab = (voice **)xmalloc(8*sizeof(void *));
		else if (!(n_voices-1 & n_voices) && n_voices > 4)  // if n_voices==8,16,32...
			voicetab = (voice **)xrealloc(voicetab, (n_voices << 1) * sizeof(void *));
		voice *v = new(this) voice(filename, dirname, this);
		if (scfg->preload_voices) {
			if (!v->load_synth()) {
				D_PRINT(3, "Voice %s failed to initialize, disappearing\n", voice_name);
				delete v;
				return;
			}
		}
		voicetab[n_voices++] = v;
	}
	free(filename);
	free(dirname);
}

void
lang::add_voices(const char *voice_names)
{
	int i, j;
	char *tmp = (char *)xmalloc(strlen(voice_names)+1);

	for (i=0, j=0; voice_names[i]; ) {
		if ((tmp[j++] = voice_names[i++]) == ':' ) {
			tmp[j-1] = 0;
			add_voice(tmp);
			j = 0;
		}
	}
	tmp[j] = 0;
	if (j) add_voice(tmp);
	free(tmp);
}

void
lang::add_soft_option(const char *optname)
{
	char *dflt = (char *)strchr(optname, EQUALSIGN);
	if (dflt) *dflt++ = 0;
	else dflt = const_cast<char *>("");
	char *closing = (char *)strchr(optname, CLOSING);

	epos_option o;
	o.opttype = O_BOOL;		// default type
	o.structype = OS_VOICE;	// soft options can only be voice options
	o.readable = o.writable = A_PUBLIC;	// ...no access restrictions on them

	if (closing) {
		if (strchr(optname, OPENING) != closing - 2 || closing[1])
			shriek(812, "Syntax error in soft option %s in lang %s", optname, name);
		closing[-2] = 0;
		switch(closing[-1]|('a'-'A')) {
			case 'b': o.opttype = O_BOOL; break;
			case 's': o.opttype = O_STRING; break;
			case 'n': o.opttype = O_INT; break;
			case 'c': o.opttype = O_CHAR; shriek(812, "char typed soft options are tricky"); break;
//			case 'f': o.opttype = O_FILE; break;
			default : shriek(812, "Unknown option type in %s in lang %s", optname, name);
		}
	} else if (strchr(optname, OPENING))
		shriek(812, "Unterminated type spec in soft option %s in lang %s", optname, name);
	if (option_struct(optname, NULL))
		shriek(812, "Soft option name conflicts with a built-in option name %s in lang %s", optname, name);
	if (soft_opts) {
		if (soft_opts->translate(optname))
			shriek(812, "Soft option already exists in lang %s", name);
		soft_defaults = xrealloc(soft_defaults,
				(soft_opts->items + 2) * sizeof(void *) >> 1);
	} else {
		soft_opts = new hash_table<char, epos_option>(30);
		soft_opts->dupkey = 0;
		soft_defaults = xmalloc(sizeof(void *));
	}

	o.offset = sizeof(voice) + (soft_opts->items * sizeof(void *) >> 1);

	char *tmp = (char *)xmalloc(strlen(optname) + 3);
	strcpy(tmp + 2, optname);
	tmp[0] = 'V';
	tmp[1] = ':';
	o.optname = tmp + 2;

	soft_opts->add(o.optname - 2, &o);
	soft_opts->add(o.optname, &o);

	set_option(&o, dflt, (void *)((voice *)soft_defaults - 1));
}

void
lang::add_soft_opts(const char *names)
{
	int i, j;
	char *tmp = (char *)xmalloc(strlen(names)+1);

	for (i=0, j=0; names[i]; ) {
		if ((tmp[j++] = names[i++]) == ':' ) {
			tmp[j-1] = 0;
			add_soft_option(tmp);
			j = 0;
		}
	}
	tmp[j] = 0;
	if (j) add_soft_option(tmp);
	free(tmp);
}

void
lang::compile_rules()
{
	int tmp = cfg->default_lang;
	if (!lang_switch(name))
		shriek(862, "cannot lang_switch to myself");

	D_PRINT(3, "Compiling %s language rules\n", name);
	ruleset = new rules(rules_file, rules_dir);
	parser::init_tables(this);

	cfg->default_lang = tmp;
}


#define CONFIG_VOICE_INITIALIZE
voice::voice(const char *filename, const char *dirname, lang *parent_lang) : cowabilium()
{
	#include "options.lst"

	if (parent_lang->soft_defaults)
		memcpy(this + 1, parent_lang->soft_defaults,
			sizeof(void *) * parent_lang->soft_opts->items >> 1);
	
	load_config(filename, dirname, "voice", OS_VOICE, this, parent_lang);
	if (!parent_lang->default_voice) {
//		parent_lang->default_voice = this...;
//		if (!this_voice)
//			this_voice = this;
	}
	if (parent_lang->name == name) {	/* default the name to the stripped filename */
		if (strrchr(filename, SLASH))
			filename = strrchr(filename, SLASH) + 1;
		int l = strcspn(filename, ".");
		char *nname = (char *)xmalloc(l + 1);
		nname[l] = 0;
		strncpy(nname, filename, l);
		name = nname;
	}

	segment_names = NULL;
	sl = NULL;
	syn = NULL;
	this->parent_lang = parent_lang;
}

void
voice::claim_all()
{
	if (!segment_names && dpt_file && *dpt_file)
		segment_names = claim(dpt_file, location, scfg->inv_base_dir, "rt", "segment names", NULL);
	if (cfg->label_phones && !sl) {
		sl = (sound_label *)xmalloc(sizeof(sound_label) * n_segs);
		for (int i = 0; i < n_segs; i++) sl[i].pos = NO_SOUND_LABEL;
		text *t;
		if (!snl_file || !*snl_file) return;
		t = new text(snl_file, location, scfg->inv_base_dir, "sound labels", true);
		char * l = get_text_line_buffer();
		while (t->get_line(l)) {
			int a, b, d; char c;
			if (sscanf(l, "%d %d %c %d\n", &a, &b, &c, &d) == 3) {
				if (sl[a].pos != NO_SOUND_LABEL) shriek(861, "Multilabelled units unimplmd");
				sl[a].pos = b;
				sl[a].labl = c;
			}
		}
	}
}

synth *
voice::setup_synth()
{
	if (syn) shriek(862, "new v->syn - again");

	switch (type) {
		case S_NONE:	shriek(813, "This voice is mute");
		case S_TCP:	// shriek(462, "Network voices not implemented");
				return new tcpsyn(this);
		case S_LPC_FLOAT: return new lpcfloat(this);
		case S_LPC_INT: return new lpcint(this);
		case S_LPC_VQ:	return new lpcvq(this);
//		case S_KTD:	return new ktdsyn(this);
		case S_TDP:	return new tdpsyn(this);
//		case S_PTD:	return new ptdsyn(this);
		case S_MBROLA:  return new mbrsyn(this);
		default:	shriek(861, "Impossible synth type");
	}
	return NULL;
}

bool
voice::load_synth()
{
	if (!syn) {
		try {
			syn = setup_synth();
			D_PRINT(2, "Voice %s successfully initialized.\n", name);
			return true;
		} catch (command_failed *e) {
			D_PRINT(2, "Failed to initialize voice %s, error code %d\n", name, e->code);
			delete e;
			return false;
		}
	}
	D_PRINT(2, "Voice %s is already loaded.\n", name);
	return true;
}

voice::~voice()
{
	if (segment_names) unclaim(segment_names);
	if (sl) free(sl);
//	if (buffer) detach();
	if (syn) delete syn;
	D_PRINT(3, "Disposing voice %s\n", name);
	cow_unstring(this, voiceoptlist);
}

void *
voice::operator new(size_t size, lang *parent_lang)
{
	int nso;

#ifdef DEBUGGING
	if (size != sizeof(voice)) shriek(862, "I'm missing something");
#endif
	
	if (!parent_lang || !parent_lang->soft_opts) nso = 0;
	else nso = parent_lang->soft_opts->items >> 1;
	return xmalloc(size + nso * sizeof(void *));
}

void
voice::operator delete(void *ptr)
{
	free(ptr);
}

void
voice::operator delete(void *ptr, lang *l)
{
	free(ptr);
}
