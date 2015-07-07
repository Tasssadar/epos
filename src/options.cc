/*
 *	epos/src/options.cc
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
 */

#include "epos.h"

#include "slab.h"
SLABIFY(configuration, cfg_slab, 2, shutdown_cfgs)

#define CMD_LINE_OPT	"-"
#define CMD_LINE_VAL	'='

#define INDEXER		"@"

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

inline void *memdup(void *p, int size)
{
	if (!p) {
		return NULL;
	}
	void *r = xmalloc(size);
	memcpy(r,p,size);
	return r;
}

configuration proto_cfg;

configuration *cfg = &proto_cfg;

static_configuration proto_scfg;
static_configuration *scfg = &proto_scfg;


/*
 *	cow - this routine should detect whether a piece of memory used
 *		for global/language/voice configuration is shared with
 *		supposedly logical copies, and, if so, copy it physically
 *		before changing it. A nice space-saving technique.
 *
 *	cow_claim - claims all current configuration
 *	cow_unclaim - releases all current configuration; cowabilia without other
 *		previous claimers are freed
 *
 *	??Semantics: cow == number of claimers minus one
 *
 *	The implementation is terrible (though fast). If I knew how to design this
 *	correctly, I would do that. I have done it twice wrong and it grew moderately
 *	complex during the process of fixing.
 *
 */

void cow(cowabilium **p, int size, int extraoffset, int extrasize)
{
	cowabilium *src = *p;
	cowabilium **ptr = p;

	if (!*p) {
		shriek(861, "cow()ing a NULL");
	}

	if (src->cow) {
		*ptr = (cowabilium *)memdup(src, size);
		D_PRINT(0, "Copying %p(%d) to %p\n", src, src->cow, *ptr);
		(*ptr)->cow = 0;
		(*ptr)->parent = src;
		src->cow--;

		if (extraoffset) {
			*(void **)((char *)*ptr + extraoffset)
				= memdup(*(void **)((char *)src + extraoffset), extrasize);
		}
	}
}

#define  LANGS_OFFSET   ((long int)&((configuration *)NULL)->langs)
#define  LANGS_LENGTH   ((*cfg)->n_langs * sizeof(void *))

void cow_configuration(configuration **cfg)
{
	cow((cowabilium **)cfg, sizeof(configuration), LANGS_OFFSET, LANGS_LENGTH);
}

void cow_claim()
{
	D_PRINT(0, "Claiming %p, was %d\n", cfg, cfg->cow);
	for (int i=0; i < cfg->n_langs; i++) {
		lang *l = cfg->langs[i];
		D_PRINT(0, "   lang %s(%p) was %d\n", l->name, l, l->cow);
		l->cow++;
		for (int j=0; j < l->n_voices; j++) {
			D_PRINT(0, "      voice %s(%p) was %d\n", l->voicetab[j]->name, l->voicetab[j], l->voicetab[j]->cow);
			l->voicetab[j]->cow++;
		}
	}
	cfg->cow++;
}

void cow_unstring(cowabilium *p, epos_option *opts)
{
	for (epos_option *o = opts; o->optname; o++) {
		if (o->opttype != O_STRING && o->opttype != O_LIST || o->per_level)
			continue;
		char **to_free = (char **)((char *)p + o->offset);
		if (*to_free && (*(char **)((char *)p->parent + o->offset) != *to_free
				|| p->parent == p)) {
			D_PRINT(0, "Freeing %s\n", o->optname);
			free(*to_free);
			*to_free = NULL;
		}
	}
}

static void cow_unsynth(voice *v)
{
	voice *par = (voice *)v->parent;
	if (v->syn != par->syn) delete v->syn;
	if (v->segment_names != par->segment_names) unclaim(v->segment_names);
	if (v->sl != par->sl) free(v->sl);
}

static inline void cow_free(cowabilium *p, epos_option *opts, void *extra)
{
	cow_unstring(p, opts);
	if (opts == voiceoptlist) cow_unsynth((voice *)p);
	free(p);
	if (extra) free(extra);
}

extern epos_option optlist[];

void cow_unclaim(configuration *that_cfg)
{
	D_PRINT(0, "Unclaiming %p, was %d\n", that_cfg, that_cfg->cow);
	for (int i=0; i < that_cfg->n_langs; i++) {
		lang *l = that_cfg->langs[i];
		D_PRINT(0, "   lang %s(%p) was %d\n", l->name, l, l->cow);
		for (int j=0; j < l->n_voices; j++) {
			D_PRINT(0, "      voice %s(%p) was %d\n", l->voicetab[j]->name, l->voicetab[j],l->voicetab[j]->cow);
			if (!l->voicetab[j]->cow--) cow_free(l->voicetab[j], voiceoptlist, NULL);
			/* * * fortunately, soft options are never strings * * */
		}
		if (!l->cow--) cow_free(l, langoptlist, l->voicetab);
	}
	if (!that_cfg->cow--) cow_free(that_cfg, optlist, that_cfg->langs), that_cfg = NULL;
}

void cow_catharsis()
{
	cow_unclaim(cfg);
	cfg = &proto_cfg;
	cow_claim();
}

#define CONFIG_INITIALIZE
inline configuration::configuration() : cowabilium()
{
	#include "options.lst"

	n_langs = 0;
	langs = NULL;

	stdshriek = stderr;
	stddbg = stdout;

	current_stream = NULL;
}

configuration::~configuration()
{
//	cow_unstring(this, optlist);
}

#define EO {NULL, O_INT, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, -1},
#define TWENTY_EXTRA_OPTIONS	EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO 

// configuration master_cfg;

#define CONFIG_DESCRIBE
epos_option optlist[]={
        #include "options.lst"

	{"C:language" + 2, O_LANG, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, 0},
	{"", O_INT, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, -3},
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	{NULL, O_INT, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, -2}
};

#undef EO
#undef TWENTY_EXTRA_OPTIONS


#define CONFIG_STATIC_INITIALIZE
inline static_configuration::static_configuration() : cowabilium()
{
	#include "options.lst"
}

#define EO {NULL, O_INT, OS_STATIC, A_PUBLIC, A_PUBLIC, false, false, -1},
#define TWENTY_EXTRA_OPTIONS	EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO 

#define CONFIG_STATIC_DESCRIBE
epos_option staticoptlist[]={
	#include "options.lst"
	{"", O_INT, OS_STATIC, A_PUBLIC, A_PUBLIC, false, false, -3},
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	{NULL, O_INT, OS_STATIC, A_PUBLIC, A_PUBLIC, false, false, -2}
};

#undef EO
#undef TWENTY_EXTRA_OPTIONS


hash_table<char, epos_option> *option_set = NULL;

void configuration::shutdown()
{
	int i;
	for (i = 0; i < n_langs; i++) delete langs[i];
	free(langs);
	langs = NULL;
	n_langs = 0;

	cow_unstring(this, optlist);
}

void static_configuration::shutdown()
{
	cow_unstring(this, staticoptlist);
}

/*
 * Some string trickery below. option->name is initialised (in options.lst)
 *	to   "X:" "string"+2. In C, it means the same as "X:string"+2,
 *	which means "string" with "X:" immediately preceding it.
 * Now we add to our option_set both normal option names ("string")
 *	and their prefixed spellings ("X:string", for X being either
 *	S, C, L or V for static/global/language/voice configuration items,
 *	respectively), eating up minimum memory space only.
 */

inline void put_into_option_set(epos_option *o)
{
	if (*o->optname) {
		option_set->add(o->optname, o);
		option_set->add(o->optname - 2, o);
	}
}

void make_option_set()
{
	epos_option *o;

	if (option_set) return;

	option_set = new hash_table<char, epos_option>(300);
	option_set->dupkey = option_set->dupdata = 0;

	for (o = staticoptlist; o->optname; o++)put_into_option_set(o);
	for (o = optlist; o->optname; o++) 	put_into_option_set(o);
	for (o = langoptlist; o->optname; o++)	put_into_option_set(o);
	for (o = voiceoptlist; o->optname; o++)	put_into_option_set(o);

	option_set->remove("languages");	// FIXME
	option_set->remove("voices");
}

void restrict_options()
{
	epos_option *o;

	text *t = new text(scfg->restr_file, scfg->ini_dir, "", NULL, true);
	if (!t->exists()) {
		delete t;
		return;		/* if no restrictions file, ignore it */
	}
	char *line = get_text_line_buffer();
	while (t->get_line(line)) {
		char *status = split_string(line);
		o = option_set->translate(line);
		if (!o) {
			if (cfg->paranoid)
				shriek(812, "Typo in %s:%d\n", t->current_file, t->current_line);
			continue;
		}
		o->readable = o->writable = A_NOACCESS;
		ACCESS level = A_PUBLIC;
		for (char *p = status; *p; p++) {
			switch(*p) {
				case 'r': o->readable = level; break;
				case 'w': o->writable = level; break;
				case '#': level = A_ROOT; break;
				case '$': level = A_AUTH; break;
				default : shriek(812, "Invalid access rights in %s line %d",
						t->current_file, t->current_line);
			}
		}
	}
	free(line);
	delete t;
}

static void free_extra_options(epos_option *optlist, cowabilium *whence)
{
	epos_option *o;
	for (o = optlist; o->offset != -2; o++) ;
	for ( ; o->offset != -3; o--) ;
	for ( o++; o->offset != -2; o++) {
		if (o->offset == -1) continue;
		if ((o->opttype == O_STRING || o->opttype == O_LIST) && whence) {
			D_PRINT(1, "Clearing an extra option %s, whence=%p\n", o->optname - 2, whence);
			if (*(char **)((char *)whence + o->offset)) {
				free(*(char **)((char *)whence + o->offset));
				*(char **)((char *)whence + o->offset) = NULL;
			}
		}
		if (o->optname) {
			D_PRINT(1, "Freeing an extra option %s\n", o->optname - 2);
			option_set->remove(o->optname);
			option_set->remove(o->optname - 2);
			free((char *)o->optname - 2);
			o->optname = NULL;
		}
		o->offset = -1;
	}
}

void free_extra_options()
{
	free_extra_options(voiceoptlist, NULL);
	free_extra_options(langoptlist, NULL);
	free_extra_options(optlist, cfg);
	free_extra_options(staticoptlist, (cowabilium *)scfg);
}


static void free_all_options(epos_option *optlist, cowabilium *whence)
{
	epos_option *o;
	for (o = optlist; o->offset != -3; o++) {
		if (o->offset == -1) continue;
		if ((o->opttype == O_STRING || o->opttype == O_LIST) && whence) {
			if (*(char **)((char *)whence + o->offset)) {
				free(*(char **)((char *)whence + o->offset));
				*(char **)((char *)whence + o->offset) = NULL;
			}
		}
		o->offset = -1;
	}
}


epos_option *alloc_option(epos_option *optlist, OPT_STRUCT os)
{
	DBG(1, int j = 0; epos_option *p; for (p = optlist; p->offset != -2; p++) if (p->offset == -1) j++; fprintf(STDDBG, "Allocating an extra option, level %d out of %d free options\n", os, j);)

	epos_option *o;
	for (o = optlist; ; o++) {
		if (o->offset == -1) return o;
		if (o->offset == -2) shriek(864, "No more options to allocate, add extra options");
	}
	return NULL;	// to keep the compiler happy
}

#define MAX_TOTAL_OPTION_NAME 64

void alloc_level_options(epos_option *optlist, OPT_STRUCT os, void *base, int levnum, const char *levname)
{
	char b[MAX_TOTAL_OPTION_NAME];

	epos_option *o;
	for (o=optlist; o->optname; o++) {
		if (o->per_level) {
			epos_option *p = alloc_option(optlist, os);
			b[0] = OPT_STRUCT_PREFIX[os];
			b[1] = ':';
			strcpy(b+2, o->optname);
			strcat(b+2, INDEXER);
			strcat(b+2, levname);
			p->optname = strdup(b) + 2;
			p->opttype = o->opttype;
			p->readable = o->readable;
			p->writable = o->writable;
			option_set->add(p->optname - 2, p);
			option_set->add(p->optname, p);

			int size;
			switch (o->opttype) {
				case O_STRING:	size = sizeof(char *); break;
				case O_INT:	size = sizeof(int); break;
				case O_BOOL:	size = sizeof(bool); break;
				default: shriek(861, "strange type size_of-ed"); break;
			}
			p->offset = o->offset + levnum * size;
			if (base)
				if (o->opttype == O_STRING) *(char **)((char *)base + p->offset) = NULL;	// CHECKME should not rather copy the default from o->offset?
				else memcpy((char *)base + p->offset, (char *)base + o->offset, size);
		}
	}
}


#define LEVNAME_SEGMENT	"segment"
#define LEVNAME_PHONE	"phone"
#define LEVNAME_TEXT	"text"

static inline const char *pre_set_unit_levels(const char *value)
{
	if ((str2enum(LEVNAME_SEGMENT, value, U_DEFAULT) == U_ILL) ||
		(str2enum(LEVNAME_PHONE, value, U_DEFAULT) == U_ILL) ||
		(str2enum(LEVNAME_TEXT, value, U_DEFAULT) == U_ILL))
			return NULL;		// required level missing

	char b[MAX_TOTAL_OPTION_NAME];
	int n;

	for (n=0; enum2str(n, value); n++) ;
	if (n >= UNIT_MAX) return NULL;		// too many levels
	for (int i=0; i<n; i++) {
		strcpy(b, enum2str(i, value));
		for (int j=0; j<n; j++) {
			if (i!=j && !strcmp(b, enum2str(j, value))) 
				return NULL;	// two level names collide
		}
	}
	return value;
}

static inline void post_set_unit_levels(const char *)
{
	scfg->_segm_level = str2enum(LEVNAME_SEGMENT, scfg->unit_levels, U_DEFAULT);
	scfg->_phone_level = str2enum(LEVNAME_PHONE, scfg->unit_levels, U_DEFAULT);
	scfg->_text_level = str2enum(LEVNAME_TEXT, scfg->unit_levels, U_DEFAULT);
	scfg->default_scope = str2enum("word", scfg->unit_levels, U_DEFAULT);
	if (scfg->default_scope == U_ILL) scfg->default_scope = scfg->_text_level;
	scfg->default_target = scfg->_phone_level;

	int n;
	for (n=0; enum2str(n, scfg->unit_levels); n++) ;

	for (int i=0; i<n; i++) {
		const char *tmp = enum2str(i, scfg->unit_levels);
		alloc_level_options(staticoptlist, OS_STATIC, scfg, i, tmp);
		alloc_level_options(optlist, OS_CFG, cfg, i, tmp);
		alloc_level_options(langoptlist, OS_LANG, NULL, i, tmp);
	}
}

static inline char *get_startup_cwd(char *buff, size_t size)
{
	static char *startup_wd = NULL;
	if (startup_wd != NULL) {
		if (strlen(startup_wd) + 1 <= size) {
			strcpy(buff, startup_wd);
			return startup_wd;
		} else {
			return NULL;
		}
	}
	char *ret = getcwd(buff, size);
	if (ret == NULL) {
		return NULL;
	}
	startup_wd = FOREVER(strdup(ret));
	return ret;
}

static inline const char *pre_base_dir(const char *value)
{
	if (value && value[0] && value[0] != SLASH) {
		size_t size = scfg->scratch_size ? scfg->scratch_size : 64;
		char *abs = (char *)xmalloc(size);
		while (!get_startup_cwd(abs, size - 1))
			size *= 2, abs = (char *)xrealloc(abs, size);
		if (!strcmp(abs, "/")) {
			shriek(814, "base_dir relative to the root? Prepend the slash.");
		}
		while (strlen(abs) + strlen(value) + 1 >= size)
			size *= 2, abs = (char *)xrealloc(abs, size);
		char *tmp = abs + strlen(abs);
		*tmp++ = SLASH;
		strcpy(tmp, value);
		char *abs2 = FOREVER(strdup(abs));
		free(abs);
		return abs2;
	} else return value;
}

static inline const char *pre_init_f(const char *value)
{
	int v;
	if (!sscanf(value,"%d",&v)) return value;
	if (v < 1) return NULL;
	else return value;
}

/*
 *	invoke_set_action gets invoked twice for options whose action bit
 *		is set.  The pre-set call receives and returns a value,
 *		the post-set call receives a NULL and returns NULL.
 *
 *		If a pre-set call returns a NULL, the option will not
 *		be changed and no post-set call occurs.  Otherwise the
 *		value returned is used instead of the suggested one.
 *
 *		The value returned by invoke_set_action (if different
 *		from the argument) can and should be allocated using
 *		scratch.  invoke_set_action must not be called recursively.
 *
 *		All options have initially the action bit set, and
 *		invoke_set_action shall disable the action bit for
 *		options it doesn't handle specially.
 */

const char *invoke_set_action(epos_option *o, const char *value)
{
	const char *on = strchr(o->optname, ':') ? strchr(o->optname, ':') + 1 : o->optname;
	if (!strcmp(on, "base_dir")) {
		if (value) return pre_base_dir(value);
		else return NULL;
	}
	if (!strcmp(on, "init_f")) {
		if (value) return pre_init_f(value);
		else return NULL;
	}
	if (!strcmp(on, "unit_levels")) {
		if (value) {
			return pre_set_unit_levels(value);
		} else {
			post_set_unit_levels(NULL);
			return NULL;
		}
	}
#ifdef HAVE_WINSOCK
	if (!strcmp(on, "readfs")) {
		if (!value) return NULL;
		if (str2enum(value, BOOLstr, 0) & 1) return NULL;
		else return value;
	}
#endif
	o->action = false;
	return value;
}

epos_option *option_struct(const char *name, hash_table<char, epos_option> *softopts)
{
	epos_option *o;

	if (!option_set) shriek(862, "no config_init()");
	if (!name || !*name) return NULL;
	o = option_set->translate(name);
	if (!o && softopts) o = softopts->translate(name);
	return o;
}

void unknown_option(char *name, char *)
{
	shriek(442, "Never heard about \"%s\", option ignored", name);
}

void parse_cfg_str(char *val)
{
	char *brutto, *netto;
	bool tmp;
	
	tmp=0;
	if (*val == DQUOT && *(brutto = val + strlen(val)-1) == DQUOT) {
		*brutto = 0;       //If enclosed in double quotes, kill'em.
		for (netto = val; netto < brutto; netto++)
			netto[0] = netto[1];
	}
#if 0
	for (netto=val, brutto = val + tmp; *brutto; brutto++, netto++) {
		*netto = *brutto;
		if (*brutto == ESCAPE) *netto = esctab->xlat(*++brutto);
	}				//resolve escape sequences
	*netto = 0;
#endif
}

template<class T> inline void set_enum_option(epos_option *o, const char *val, const char *list, char *locus)
{
	parse_cfg_str(const_cast<char *>(val));
	T tmp = (T)str2enum(val, list, U_ILL);
	if (tmp == (T)U_ILL)
		shriek(447, "Can't set %s to %s", o->optname, val);
	*(T *)locus = tmp;
}

bool set_option(epos_option *o, const char *val, void *base)
{
	int tmp;
	if (!o) return false;
	if (o->action && !val) val = "";
	if (o->action && !(val = invoke_set_action(o, val))) return false;
	char *locus = (char *)base + o->offset;
	switch(o->opttype) {
		case O_BOOL:
			tmp = str2enum(val, BOOLstr, false);
			if (cfg->paranoid && (!val || tmp == U_ILL)) 
				shriek(447, "%s is used as a boolean value for %s", val, o->optname);
			*(bool *)locus = tmp & 1;
			D_PRINT(1, "Configuration option \"%s\" set to %s\n",
				o->optname,enum2str(*(bool*)locus, BOOLstr));
			break;
		case O_MARKUP:
			set_enum_option<OUT_ML>(o, val, OUT_MLstr, locus);
			D_PRINT(1, "Markup language option set to %i\n",*(int*)locus);
			break;
		case O_SALT:
			if (!strcmp(val, "SAMPA")) *(int *)locus = 0;
			else {
				set_enum_option<int>(o, val, scfg->sampa_alts, locus);
				(*(int *)locus)++;
			}
			D_PRINT(1, "Sampa alternate option set to %i\n",*(int*)locus);
			break;
		case O_SYNTH:
			set_enum_option<SYNTH_TYPE>(o, val, STstr, locus);
			D_PRINT(1, "Synthesis type option set to %i\n",*(int*)locus);
			break;
		case O_CHANNEL:
			set_enum_option<CHANNEL_TYPE>(o, val, CHANNEL_TYPEstr, locus);
			D_PRINT(1, "Channel type option set to %i\n",*(int*)locus);
			break;
		case O_UNIT:
//			if((*(UNIT *)locus=str2enum(val, scfg->unit_levels, U_ILL))==U_ILL) 
//				shriek(447, "Can't set %s to %s", o->optname, val);
			set_enum_option<UNIT>(o, val, scfg->unit_levels, locus);
			D_PRINT(1, "Configuration option \"%s\" set to %d\n",o->optname,*(int *)locus);
			break;
		case O_INT:
//			*(int *)locus=0;
			int v;
			if (!sscanf(val,"%d",&v)) shriek(447, "Unrecognized numeric parameter");
			*(int *)locus = v;
			D_PRINT(1, "Configuration option \"%s\" set to %d\n",o->optname,*(int *)locus);
			break;
		case O_STRING: 
			if (val && val[0]) parse_cfg_str(const_cast<char *>(val));
   process_as_string:
			D_PRINT(1, "Configuration option \"%s\" set to \"%s\"%s\n", o->optname, val, val && strchr(val, '\033') && scfg->normal_color ? scfg->normal_color : "");
			if (*(char **)locus) free(*(char**)locus);
			if (!val) *(char **)locus = NULL;
			else *(char**)locus = strdup(val);	// FIXME: should be forever if monolith etc. (maybe)
			break;
		case O_LIST:
			if (val && val[0]) parse_cfg_str(const_cast<char *>(val));
			else goto process_as_string;
			char *old;
			old = *(char **)locus;
			D_PRINT(1, "Configuration option \"%s\" adds \"%s\"%s to \"%s\"%s\n", o->optname,
					val, strchr(val, '\033') && scfg->normal_color ? scfg->normal_color : "",
					old ? old : "empty string", strchr(val, '\033') && scfg->normal_color ? scfg->normal_color : "");
			if (!old || !*old) {
				*(char **)locus = strdup(val);
				break;
			}
			char *cat;
			cat = (char *)xmalloc(strlen(val) + strlen(old) + 2);
			strcpy(cat, old);
			strcat(cat, ":");
			strcat(cat, val);
			*(char **)locus = cat;
			free(old);
			break;
		case O_CHAR:
			if (!val[0]) shriek(447, "Empty string given for a CHAR option %s", o->optname);
			parse_cfg_str(const_cast<char *>(val));
			if (val[1]) shriek(447, "Multiple chars given for a CHAR option %s", o->optname);
			else *(char *)locus=*val;
//			D_PRINT(1, "Configuration option \"%s\" set to \"%s\"\n",o->optname,val);
			break;
		case O_LANG:
			if (!lang_switch(val)) shriek(443, "unknown language");
			break;
		case O_VOICE:
			if (!voice_switch(val)) shriek(443, "unknown voice");
			break;
		case O_CHARSET:
			parse_cfg_str(const_cast<char *>(val));
			if (!strncmp(val, "sampa-", 6)) {
				tmp = load_named_sampa(val);
			} else {
				tmp = load_charset(val);
			}
			if (tmp == CHARSET_NOT_AVAILABLE)
				shriek(447, "Unknown charset %s", val);
			*(int *)locus = tmp;
			D_PRINT(2, "Charset set to %i\n",*(int*)locus);
			break;
		default: shriek(462, "Bad option type %d", (int)o->opttype);
	}
	if (o->action) invoke_set_action(o, NULL);

	return true;
}

bool set_option_to_default(epos_option *o, void *base)
{
	if (!o) return false;
	if (o->per_level) {
		return false;
	}
	char *locus = (char *)base + o->offset;
	switch(o->opttype) {
		case O_BOOL:	set_option(o, "false", base); break;
		case O_MARKUP:	set_option(o, "none", base); break;
		case O_SALT:	set_option(o, "SAMPA", base); break;
		case O_SYNTH:	set_option(o, "none", base); break;
		case O_CHANNEL:	set_option(o, "mono", base); break;
		case O_UNIT:	/* nothing */  break;
		case O_INT:	set_option(o, "0", base); break;
		case O_STRING:	set_option(o, NULL, base); break;
		case O_LIST:	set_option(o, NULL, base); break;
		case O_CHAR:	set_option(o, " ", base); break;
		case O_LANG:	/* nothing */  break;
		case O_VOICE:   /* nothing */  break;
		case O_CHARSET:	/* nothing */  break;
		default: shriek(462, "Bad option type %d", (int)o->opttype);
	}
	return true;
}

/*
 *	C++ is evil with respect to double indirection.
 *	The casts to cowabilium below are a braindamage,
 *	unless the idea of a typed language is a braindamage.
 *	C++ tries to force us to pass it by reference,
 *	but struct cowabilium should not have formed a basis for
 *	inheritance in the first place.
 *	
 *	The order of cowing cfg, lang and voice, is important.
 */

#define  VOICES_OFFSET  ((long int)&((lang *)NULL)->voicetab)
#define  VOICES_LENGTH  (this_lang->n_voices * sizeof(void *))

bool set_option(epos_option *o, const char *value)
{
	if (!o) return false;
	switch(o->structype) {
		case OS_STATIC: return set_option(o,value, scfg);
		case OS_CFG:	cow_configuration(&cfg);
				return set_option(o, value, cfg);
		case OS_LANG:	cow_configuration(&cfg);
				cow((cowabilium **)&this_lang, sizeof(lang),   VOICES_OFFSET, VOICES_LENGTH);
				return set_option(o, value, this_lang);
		case OS_VOICE:	cow_configuration(&cfg);
				cow((cowabilium **)&this_lang, sizeof(lang),   VOICES_OFFSET, VOICES_LENGTH);
				cow((cowabilium **)&this_voice, sizeof(voice) + (this_lang->soft_opts->items * sizeof(void *) >> 1), 0, 0);
				return set_option(o, value, this_voice);
	}
	return false;
}

bool set_option(char *name, const char *value)
{
	return set_option(option_struct(name, NULL), value);
}

static inline void set_option_or_die(char *name, const char *value)
{
	epos_option *o = option_struct(name, NULL);
	if (!o) shriek(814, "Unknown option %s", name);
	if (name[0] == '_') shriek(814, "Option %s is not user settable", name);
	if (!cfg->langs && (o->structype == OS_LANG || o->structype == OS_VOICE || o->opttype == O_LANG))
		return;

	if (set_option(o, value)) return;
	shriek(814, "Bad value %s for option %s", value, name);
}

/*
 *	For the following one, make sure that base is the correct type
 */

static inline bool set_option(char *name, char *value, void *base, hash_table<char, epos_option> *softopts)
{
	return set_option(option_struct(name, softopts), value, base);
}

bool lang_switch(const char *value)
{
	for (int i=0; i < cfg->n_langs; i++)
		if (!strcmp(cfg->langs[i]->name, value)) {
			if (!cfg->langs[i]->n_voices)
				shriek(462, "Switch to a mute language unimplemented");
			cfg->default_lang = i;
			return true;
		}
	return false;
}

bool voice_switch(const char *value)
{
	cow((cowabilium **)&(this_lang), sizeof(lang), VOICES_OFFSET, VOICES_LENGTH);	//new

	for (int i=0; i < this_lang->n_voices; i++)
		if (!strcmp(this_lang->voicetab[i]->name, value)) {
			this_lang->default_voice = i;
			return true;
		}
	return false;
}

static const char *format_option(epos_option *o, void *base)
{
	char *locus = (char *)base + o->offset;
	switch(o->opttype) {
		case O_BOOL:
			return *(bool *)locus ? "on" : "off";
		case O_MARKUP:
			return enum2str(*(int *)locus, OUT_MLstr);
		case O_SALT:
			if (!*(int *)locus)
				return "SAMPA";
			return enum2str(*(int *)locus - 1, scfg->sampa_alts);
		case O_SYNTH:
			return enum2str(*(int *)locus, STstr);
		case O_UNIT:
			return enum2str(*(UNIT *)locus, scfg->unit_levels);
		case O_INT:
			sprintf(scratch, "%d", *(int *)locus);
			return scratch;
		case O_STRING: 
		case O_LIST:
			return *(char **)locus;
		case O_CHAR:
			scratch[0] = *(char *)locus;
			scratch[1] = 0;
			return scratch;
		case O_LANG:
			return (char *)this_lang->name;
		case O_VOICE:
			return (char *)this_voice->name;
		case O_CHARSET:
			return get_charset_name(*(int *)locus);
		default: shriek(462, "Bad option type %d", (int)o->opttype);
	}
	return NULL; /* unreachable */
}

const char *format_option(epos_option *o)
{
	switch(o->structype) {
		case OS_STATIC:return format_option(o, scfg);
		case OS_CFG:   return format_option(o, cfg);
		case OS_LANG:  return format_option(o, this_lang);
		case OS_VOICE: return format_option(o, this_voice);
		default: shriek(861, "Bad option class");
	}
	return NULL; /* unreachable */
}

const char *format_option(const char *name)
{
	epos_option *o = option_struct(name, this_lang->soft_opts);
	if (!o) {
		shriek(442, "Nonexistent option %s", name);
		return NULL; /* unreachable */
	}
	return format_option(o);
}

void reinitialize_configuration(cowabilium *p, epos_option *optlist)
{
	for (epos_option *o = optlist; o->optname; o++) {
		set_option_to_default(o, p);
	}
	
}


void reinitialize_configuration()
{
	reinitialize_configuration(cfg, optlist);
	reinitialize_configuration(scfg, staticoptlist);

	#define strfy(x) #x
	#define stringify(x) strfy(x)
		set_option_or_die("base_dir", stringify(BASE_DIR));
	#undef stringify
	#undef strfy

	set_option_or_die("ini_dir", "cfg");
	set_option_or_die("fixed_ini_file", "fixed.ini");
	set_option_or_die("restr_file", "restr.ini");

	set_option("_token_esc", "nrt[eE\\ #;@~.d-mXYZWVU");		/* no ..._or_die just because of the leading underscore */
	set_option("_value_esc", "\n\r\t\033\033\033\\\377#;@\1\2\3\4\5\037\036\035\034\032\030");

	scfg->_slash_esc = '/';

	scfg->listen_port = TTSCP_PORT;
	scfg->syslog = true;
	scfg->debug_level = 4;
	scfg->hash_search = 60;
	scfg->max_nest = 60;
	scfg->max_line_len = 512;

	for (int i = 0; i < UNIT_MAX; i++)
		cfg->pros_weight[i] = 1;
}


int argc_copy = 0;		// these get filled by the args to main()
char **argv_copy = NULL;

void set_cmd_line(int argc, char **argv)
{
	argc_copy = argc;
	char **args = (char **)xmalloc(argc * (sizeof(char *) + 1));
	for (int i = 0; i < argc; i++) {
		args[i] = get_text_buffer(strlen(argv[i]));
		strcpy(args[i], argv[i]);
	}
	args[argc] = NULL;
	argv_copy = args;
}

void free_cmd_line()
{
	for (int i = 0; i < argc_copy; i++) {
		free(argv_copy[i]);
	}
	free(argv_copy);
}

void parse_cmd_line()
{
	char *ar;
	char *j;
	register char *par;
	int dees = 0;

	for(int i = 1; i < argc_copy; i++) {
		j = ar = argv_copy[i];
		encode_string(ar, cfg->charset, true);
		switch(strspn(ar, CMD_LINE_OPT)) {
		case 3:
			ar += 3;
			if (strchr(ar, CMD_LINE_VAL) && scfg->_warnings) 
				shriek(814, "Thrice dashed options have an implicit value");
			set_option_or_die(ar, "0");
			break;
		case 2:
			ar += 2;
			par = strchr(ar, CMD_LINE_VAL);
			if (par) {					//opt=val
				*par = 0;
				set_option_or_die(ar, par + 1);
				*par = CMD_LINE_VAL;
			} else	if (i + 1 == argc_copy || strchr(CMD_LINE_OPT, *argv_copy[i+1])) 
					set_option_or_die(ar, "");
				else set_option_or_die(ar, argv_copy[++i]);
			break;
		case 1:
			for (j = ar + 1; *j; j++) switch (*j) {
				case 'b': scfg->structured = false; break;
//				case 'd': scfg->show_seg = true; break;
//				case 'e': scfg->show_phones = true; break;
				case 'f': scfg->forking = false; break;
				case 'H': scfg->long_help = true;	/* fall through */
				case 'h': scfg->help = true; break;
				case 'p': scfg->pausing = true; break;
				case 's': scfg->play_segments = true; break;
				case 'v': scfg->version = true; break;
				case 'D':
					if (!scfg->debug) scfg->debug = true;
					dees++;
//					else if (scfg->_warnings)
//						scfg->always_dbg--;
					break;
				default : shriek(442, "Unknown option -%c, no action taken", *j);
			}
			if (!ar[1]) {
				cfg->input_file = "";   	//dash only
				if (this_lang) this_lang->input_file = "";
			}
			switch (dees) {
				case 0:
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					if (scfg->debug_level > 4 - dees) scfg->debug_level = 4 - dees;
					break;
				default:
					if (scfg->_warnings) shriek(814, "Too many dees");
					else break;
			}
			switch (dees) {
				case 0:
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					if (scfg->debug_level > 4 - dees) scfg->debug_level = 4 - dees;
					break;
				default:
					if (scfg->_warnings) shriek(814, "Too many dees");
					else break;
			}
			break;
		case 0:
			if (!is_monolith) shriek(814, "Only options allowed at Epos server command line\nUse a client (e.g. \"say-epos\") to specify text");
			if (scfg->_input_text && scfg->_input_text != ar) {
				if (!scfg->_warnings) break;
				if (cfg->paranoid) shriek(814, "Quotes forgotten on the command line?");
				scratch = (char *) xmalloc(strlen(ar)+strlen(scfg->_input_text)+2);
				sprintf(scratch, "%s %s", scfg->_input_text, ar);
				ar = FOREVER(strdup(scratch));
				free(scratch);
			}
			scfg->_input_text = ar;
			break;
		default:
			if (scfg->_warnings) shriek(814, "Too many dashes");
		}
	}
}

void load_config(const char *filename, const char *dirname, const char *what,
		OPT_STRUCT type, void *whither, lang *parent_lang)
{
	int i;

	if (!filename || !*filename) return;
	D_PRINT(3, "Loading %s from %s\n", what, filename);
	char *line = get_text_line_buffer() + 2;
	line[-2] = OPT_STRUCT_PREFIX[type];
	line[-1] = ':';
	text *t = new text(filename, dirname, "", what, true);
	while (t->get_line(line)) {
		char *value = split_string(line);
		if (value && *value) {
			for (i = strlen(value)-1; strchr(WHITESPACE, value[i]) && i; i--) ;
			if (value[i-1] == ESCAPE && value[i] && i) i++;
			if (value[i]) i++;
			value[i] = 0;		// clumsy: strip off trailing whitespace
		}
		if (line[0] == '_') shriek(814, "Option %s is not user settable in %s:%d", line, t->current_file, t->current_line);
		if (!set_option(line - 2, value, whither, parent_lang ? parent_lang->soft_opts : (hash_table<char, epos_option> *)NULL)) {
			if (whither == cfg) {		// ...try also static_cfg
				line[-2] = OPT_STRUCT_PREFIX[OS_STATIC];
				if (!set_option(line - 2, value, scfg, (hash_table<char, epos_option> *)NULL))
					shriek(812, "Bad option %s in %s:%d", line, t->current_file, t->current_line);
				line[-2] = OPT_STRUCT_PREFIX[OS_CFG];
			}
		}
	}
	free(line - 2);
	delete t;
}

void load_config(const char *filename)
{
	load_config(filename, scfg->ini_dir, "config", OS_CFG, cfg, NULL);
}

static inline void add_language(int, const char *lng_name)
{
	if (!*lng_name)
		return;

	char *filename = (char *)xmalloc(strlen(lng_name) + 6);
	char *dirname = (char *)xmalloc(strlen(lng_name) + 6);
	sprintf(filename, "%s.ini", lng_name);
	sprintf(dirname, "%s%c%s", scfg->lang_base_dir, SLASH, lng_name);

	D_PRINT(3, "Adding language %s\n", lng_name);
	if (!cfg->langs) cfg->langs = (lang **)xmalloc(8 * sizeof (void *));
	else if (!(cfg->n_langs-1 & cfg->n_langs) && cfg->n_langs > 4)	    // n_langs == 8, 16, 32...
		cfg->langs = (lang **)xrealloc(cfg->langs, (cfg->n_langs << 1) * sizeof(void *));
	cfg->langs[cfg->n_langs++] = new lang(filename, dirname);
	free(filename);
	free(dirname);
}

static inline void load_languages(const char *list)
{
	list_of_calls(list, add_language);
}

void set_base_dir(char *basedir)
{
	scfg->base_dir =  strdup(basedir);
}

static inline void version()
{
	fprintf(cfg->stdshriek, "This is Epos version %s, bug reports to \"%s\" <%s>\n", VERSION, MAINTAINER, MAIL);
}

static char opttype_letter(OPT_TYPE ot)
{
	switch (ot) {
		case O_STRING: return 's';
		case O_LIST: return 'r';
		case O_INT: return 'n';
		case O_CHAR: return 'c';
		case O_BOOL: return 'b';

		case O_UNIT:
		case O_LANG:
		case O_VOICE:
		case O_CHARSET:
		case O_SALT:
			return 'l';
		case O_MARKUP:
		case O_SYNTH:
		case O_CHANNEL:
			return 'f';
		default: return '?';
	}
}

static void dump_long_opts(char *label, epos_option *list)
{
	int i, j, k;
	k = 0;
	printf("\n%s:\n", label);
	for (i = 0; list[i].optname; i++) {
		if (*list[i].optname) {
			printf("--%s(%c)", list[i].optname, opttype_letter(list[i].opttype));
			for (j = -(signed int)strlen(list[i].optname) - 5; j <= 0; j += 26) k++;
			if (k >= 3) printf("\n"), k=0;
			else for (; j > 0; j--) printf(" ");
		}
	}
	printf("\n");
}

static inline void dump_help()
{

	if (is_monolith) {
		printf(" You probably don't want to use this monolithic binary.\n");
		printf(" Its usage resembles 'say-epos' and 'eposd',\n");
		printf(" but the details are not documented, maintained nor supported.\n");
		exit(0);
	}

	printf("usage: %s [options]\n", argv_copy[0]);
	printf(" -f  disable forking (detaching) the daemon\n");
	printf(" -p  pausing - show every intermediate state\n");
	printf(" -v  show version\n");
	printf(" -D  debugging info (more D's - more detailed)\n");
	printf(" --some_long_option    ...see src/options.lst or 'eposd -H' for these\n");
	printf("(use one of the clients (e.g. 'say-epos') to pass input to Epos)\n");
	if (!scfg->long_help) exit(0);

	printf("Long option types:\n");
	printf("        (b) boolean,  (c) character,  (n) integer,  (s) string,\n");
	printf("        (r) autoconcatenating string,\n");
	printf("        (f) fixed choice: source-level fixed,\n");
        printf("        (l) list of choices: configurable\n");
	dump_long_opts("Static options", staticoptlist);
	dump_long_opts("Global options", optlist);
	dump_long_opts("Language dependent options", langoptlist);
	dump_long_opts("Voice dependent options", voiceoptlist);
	exit(0);
}

void check_cfg_version(const char *filename)
{
	file *f = claim(filename, "", "", "r", NULL, NULL);
	if (!f) shriek(843, "Configuration files not installed or very old");
	if (strncmp(f->data, VERSION, strlen(VERSION)) && cfg->paranoid) {
		D_PRINT(3, "Expected version %s, found version %s\n",
			VERSION, f->data);
		shriek(843, "Configuration version bad");
	}
	unclaim(f);
}

void config_init()
{
	const char *mlinis[] = {"","ansi.ini","rtf.ini", NULL};

	make_option_set();
//	if (privileged_exec())	/* parse restr.ini before command line */
//		restrict_options(); 
	parse_cmd_line();
	restrict_options();
	D_PRINT(3, "Base directory is %s\n", scfg->base_dir);
	D_PRINT(2, "Using configuration file %s\n", scfg->cfg_file);

	load_config(scfg->fixed_ini_file);
	parse_cmd_line();

	check_cfg_version("version");

	load_config(mlinis[scfg->markup_language]);
	load_config(scfg->cfg_file);
	parse_cmd_line();
	load_languages(scfg->languages);

	if (!this_lang->voicetab || !this_voice) shriek(842, "No voices configured");

	scfg->_warnings = true;
	parse_cmd_line();
	scfg->_loaded=true;
	
	if (scfg->version) version();
	if (scfg->help || scfg->long_help) dump_help();
}

void config_release()
{
	delete option_set;
	option_set = NULL;

	free_all_options(optlist, cfg);
	free_all_options(staticoptlist, (cowabilium *)scfg);
}

