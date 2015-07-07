/*
 *	epos/src/voice.h
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
 *	This class represents a single front end segment inventory.
 */

class rules;
class synth;

enum SYNTH_TYPE {
	S_NONE = 0,
	S_TCP = 1,
	S_LPC_FLOAT = 4,
	S_LPC_INT = 5,
	S_LPC_VQ = 6,
	S_FD = 7,
	S_KTD = 8,
	S_TDP = 9,
	S_PTD = 10,
	S_MBROLA = 12
};

#define STstr "none:internet:::lpc-float:lpc-int:lpc-vq:fd::tdp:::mbrola:"
#define ST_MAX (S_MBROLA+1)

enum CHANNEL_TYPE {CT_MONO, CT_LEFT, CT_RIGHT, CT_BOTH};
#define CHANNEL_TYPEstr "mono:left:right:both:"

struct cowabilium
{
	int cow;
	cowabilium *parent;
	cowabilium() {cow = 0; parent = this;};
};

struct voice;

struct lang : public cowabilium
{
   #define CONFIG_LANG_DECLARE
   #include "options.lst"

	rules *ruleset;
	hash_table<char, epos_option> *soft_opts;
	void *soft_defaults;
	unsigned char *char_level;
	int   n_voices;
	voice **voicetab;
	int   default_voice;

   public:
	lang(const char *filename, const char *dirname);
	~lang();
	void add_soft_option(const char  *option_name);
	void add_soft_opts(const char *option_names);
	void add_voice(const char  *voice_name);
	void add_voices(const char *voice_names);
	void compile_rules();		// could be made delayed....

//	void *operator new(size_t size);
//	void operator delete(void *ptr);
};

struct sound_label
{
	int16_t pos;
	char labl;
};

#define NO_SOUND_LABEL -1

struct voice : public cowabilium
{
   #define CONFIG_VOICE_DECLARE
   #include "options.lst"

   public:
	file *segment_names;
	sound_label *sl;
	
	synth *syn;
	lang *parent_lang;
	
	voice(const char *filename, const char *dirname, lang *parent_lang);
	~voice();

	void claim_all();
	synth *setup_synth();
	bool load_synth();	// returns true iff successful

	void *operator new(size_t, lang *parent_lang);
	void  operator delete(void *p, lang *);
	void  operator delete(void *);
};

#define this_lang  (cfg->langs[cfg->default_lang])
#define this_voice (this_lang->voicetab[this_lang->default_voice])

struct epos_option;

extern epos_option langoptlist[];
extern epos_option voiceoptlist[];

