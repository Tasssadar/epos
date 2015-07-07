/*
 *	epos/src/interf.cc
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
#include "exc.cc"

#ifdef HAVE_TIME_H
	#include <time.h>	// used to initialize the rand number generator
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_WINSOCK2_H
	#include <winsock2.h>
#else
	#ifdef HAVE_WINSOCK_H
		#include <winsock.h>
	#endif
#endif

#ifndef HAVE_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = vsprintf(str, format, ap);
	va_end(ap);
	return ret;
}
#endif	// ifdef HAVE_VSNPRINTF

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif


#ifdef HAVE_SYSLOG_H
	#include <syslog.h>
#endif

#if (('a'-'A')!=32 || '9'-'0'!=9)	//Well, we rely on these a few times I think
#error If this machine doesn't believe in ASCII, I wouldn't port it hither.
#error Hmmm... but you will manage it.
#endif

char *scratch = NULL;

charxlat *esctab = NULL;
const char* WHITESPACE    = " \t\r";     //i.e. space and tab

int unused_variable;

void *xmall_ptr_holder;

#ifdef HAVE_SYSLOG_H
int severity(int code)
{
	if (code > 899 || code < 100)	return LOG_ERR;		/* that is, meta-error */
	if (code / 10 == 80) return LOG_NOTICE;
	if (code >= 610) return LOG_CRIT;
	if (code / 10 == 45 || code == 444 || code == 445) {
		if (scfg->authpriv) return LOG_ERR | LOG_AUTHPRIV;
		return LOG_WARNING;
	}
	if (code / 10 == 46) return LOG_ERR;
	if (code / 10 == 47) return LOG_WARNING;
	if (code < 210) return LOG_DEBUG;
	if (code == 600) return LOG_DEBUG;
	return LOG_INFO;
}
#endif

char error_fmt_scratch[MAX_ERR_LINE];
int errors = 0;
int fatal_bugs = 0;

void shriek(int code, const char *fs, ...) 
{
	int l_errno = errno;
	/* Art (c) David Miller */
	if (scfg->shriek_art == 1) fprintf(cfg->stdshriek,
"              \\|/ ____ \\|/\n"
"              \"@'/ ,. \\`@\"\n"
"              /_| \\__/ |_\\\n"
"                 \\__U_/\n");
	if (scfg->shriek_art == 2) fprintf(cfg->stdshriek, "\nSuddenly, something went wrong.\n\n");

	const char *message;
	if (strchr(fs, '%')) {
		va_list ap;
		va_start(ap, fs);
		vsnprintf(error_fmt_scratch, MAX_ERR_LINE, fs, ap);
		error_fmt_scratch[MAX_ERR_LINE - 1] = 0;
		message = error_fmt_scratch;
		va_end(ap);
	} else message = fs;

	color(cfg->stdshriek, scfg->fatal_color);
	fprintf(cfg->stdshriek, "Error: %s (%d)\n", message, code); 
	color(cfg->stdshriek, scfg->normal_color);

#ifdef HAVE_SYSLOG_H
	if (scfg->syslog)
		if (scfg->log_codes) syslog(LOG_DAEMON | severity(code), "%3d %s", code, message);
		else syslog(LOG_DAEMON | severity(code), "%s", message);
	unuse(l_errno);
#else
	FILE *h = fopen("epos.err","w");
	if (h) {
		fprintf(h, "%s\nerrno=%d (%s)\n", message, l_errno, strerror(l_errno));
		fclose(h);
	}
#endif
	command_failed *xcf;

	switch (code / 100) {
		case 4 :
			xcf = new command_failed (code, message);
			throw xcf;

		case 6 :
			throw new connection_lost (code, message);

		case 8 :
			errors++;
			if (code / 10 == 86) fatal_bugs++;
			if (l_errno && EAGAIN) printf("Current errno value: %d (%s)\n", l_errno, strerror(l_errno));
			throw new fatal_error (code, message);

			printf("Your compiler does NOT support exception handling, aborting\n");
			fflush(NULL);
			abort();
		default:  shriek(869, "Bad error class %d", code);
	}
}

char *fmt(const char *s, ...) 
{
	va_list ap;
	va_start(ap, s);
	vsnprintf(scratch, scfg->scratch_size, s, ap);
	va_end(ap);
	return scratch;
}

void tag_vfprintf(const char *fmt, va_list ap)
{
	if (!fmt || !*fmt) return;
//	if (cfg->debug_tag) fprintf(STDDBG, "%-9s ", module_debug_tag);
	vfprintf(STDDBG, fmt, ap);
}

void dprint_always(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	tag_vfprintf(fmt, ap);
	va_end(ap);
}

void user_pause()
{
	printf("Press Enter\n");
	getchar();
}


char *split_string(char *param)
{
	char *value;
	param += strspn(param, WHITESPACE);
	value = param + strcspn(param, WHITESPACE);
	if (*value) *value++ = 0;
	value += strspn(value, WHITESPACE);
	return value;
}

FILE *
fopen(const char *filename, const char *flags, const char *reason)
{
	FILE *f;
	const char *message;

	if (!filename || !*filename)
//		return *flags == 'r' ? stdin : stdout;		// has to be dupped
		return NULL;
	switch (*flags) {
		case 'r': message = "Failed to read %s from %s: %s"; break;
		case 'w': message = "Failed to write %s to %s: %s"; break;
		default : shriek(861, "Bad flags for %s", reason); message = NULL;
	}
	f = fopen(filename, flags);
	if (!f && errno == ENOMEM)
		OOM_HANDLER;
	if (!f && reason) shriek(445, message, reason, filename, strerror(errno));
	if (cfg->paranoid && f && *flags == 'r') {
		if (reason && !fread(&message, 1, 1, f))
			shriek(445, message, reason, filename, "maybe a directory");
		fseek(f, 0, SEEK_SET);
	}
	return f;
}

void colorize(int level, FILE *handle)
{
	if (!scfg->colored) return; 
	if (level==-1) {
		if (scfg->normal_color) fputs(scfg->normal_color, handle);
		return;
	}
	if (scfg->color[level]) fputs(scfg->color[level],handle);
}

FIT_IDX fit(char c)
{
	switch(c) {
		case 'f':
		case 'F': return Q_FREQ;
		case 'i':
		case 'I': return Q_INTENS;
		case 't':
		case 'T': return Q_TIME;
		default: shriek(811, "Expected f,i or t, found %c",c);
			return Q_FREQ;
	}
}

#define LOWERCASED(x) (((x)<'A' || (x)>'Z') ? (x) : (x)|('a'-'A'))

		//list is colon-separated, case insensitive.  
UNIT str2enum(const char *item, const char *list, int dflt) 
{
	const char *i = list;
	const char *j = item;
	int k = 0;
	if (!item)  return (UNIT)dflt;
	for(; i && *i; j++,i++) {
		if (!*j && (*i==LIST_DELIM)) return UNIT(k);
		if (LOWERCASED(*i) != LOWERCASED(*j)) {
			k++, j=item-1, i=strchr(i,LIST_DELIM);
			if (!i) break;
		}
//		D_PRINT(0, "str2enum %s %s %d\n",i,j,k);
	}
	if(i && j && !*j) return (UNIT)k;
	D_PRINT(2, "str2enum returns ILL for %s %s\n",item, list);
	return U_ILL;
}

#define MAX_SYMBOLIC 32

char _enum2str_buff[MAX_SYMBOLIC];

const char *enum2str(int item, const char *list)
{
	const char *i;
//	char *b = _enum2str_buff;
	int j = 0;
	for(i=list; *i && item+1; i++) {
		if (*i==LIST_DELIM) {
			item--;
			_enum2str_buff[j] = 0;
			j = 0;
		} else {
			_enum2str_buff[j++] = *i;
			if (j >= MAX_SYMBOLIC)
				shriek(461, "Symbolic %.20s... too long", _enum2str_buff);
		}
	}

	if (*i)	return _enum2str_buff;
	if (item == 0) {
		_enum2str_buff[j] = 0;
		return _enum2str_buff;
	}
	return NULL;	// shriek(446, "enum2str should stringize %d",item);
}

void list_of_calls(const char *list, void call(int, const char *))
{
	int i;
	int j = 0;
	int k = 0;
	char *tmp = (char *)xmalloc(strlen(list)+1);


	for (i=0; (tmp[j] = list[i]); i++) {
		if (tmp[j] == LIST_DELIM ) {
			tmp[j] = 0;
			call(k, tmp);
			j = 0;
			k++;
		} else j++;
	}
	call(k, tmp);
	free(tmp);
}

/*************

hash *str2hash(const char *list, unsigned int max_item_len)
{
	const char *item;
	int i,d;
	hash * h;

	for (i=0,d=0; list[i]; i++) if (list[i]==LIST_DELIM) d++;
	h = new hash(d*2);
	for (; d; d--) {
		item=enum2str(d-1, list);	// sllooww
		h->add_int(item,d-1);
		if (max_item_len && strlen(item) > max_item_len)
			shriek(446, "Directive %s too long", item);
	}
	return h;
}

***********/

/*
 *  In a sense, str2units is the main routine responsible for the whole
 *  conversion of input text to the textual structure of the text just
 *  before the rules are to be applied.
 */

unit *str2units(const char *text)
{
	unit *root;
	parser *parsie;

	if (text && (signed)strlen(text) > scfg->max_text_size) shriek(456, "input too long");

	if (text && *text) parsie = new parser(text, PARSER_MODE_INPUT);
	else parsie = new parser(this_lang->input_file, PARSER_MODE_FILE);
	root = new unit(scfg->_text_level, parsie);
	delete parsie;
	return root;
}

#ifdef CLASSIC_BOOLTAB

char *fntab(const char *s, const char *t)
{
	char *tab;
	int tmp;
	tab=(char *)xmalloc(256); for(tmp=0; tmp<256; tmp++) tab[tmp] = (unsigned char)tmp;  //identity mapping
	if(cfg->paranoid && (tmp=strlen(s)-strlen(t)) && t[1]) 
		shriek(811, tmp>0 ? "Not enough (%d) resultant elements"
				  : "Too many (%d) resultant elements",abs(tmp));
	if(!t[1]) for (tmp=0; s[tmp]; tmp++) tab[(unsigned char)s[tmp]]=*t;
	else for (tmp=0;s[tmp]&&t[tmp];tmp++) tab[(unsigned char)(s[tmp])]=t[tmp];
	
	int ids = 0;
	DBG(1,for (tmp = 0; tmp < 256; tmp++) if ((unsigned char)tab[tmp] == tmp) ids++;\
	int globs = t[1] ? 0 : strlen(s);\
	int rest = 256 - ids - globs;\
	fprintf(STDDBG, "Adding a function, %d ids, %d being mapped to %c, and %d nontrivials\n", ids, globs, t[1] ? '#' : t[0], rest);)
	return(tab);
}


bool *booltab(const char *s)
{
	bool *tab;
	const char *tmp;
	bool mode = true;
	tab = (bool *)xcalloc(sizeof(bool), 256);         //should always return zero-filled array
	
	D_PRINT(0, "gonna booltab out of %s\n", s);
	if (*s==EXCLAM) memset(tab, true, 256*sizeof(bool));
	for(tmp=s; *tmp; tmp++) switch (*tmp) {
		case EXCLAM: mode = !mode; break;
		case ESCAPE: if (!*++tmp) tmp--;	// and fall through
		default:  tab[(unsigned char)(*tmp)]=mode;
	}
	DBG(2, int yes = 0; for(int i = 0; i < 256; i++) if (tab[i]) yes++;\
	fprintf(STDDBG, "Adding a booltab, %d yes, %d no\n", yes, 256 - yes);)
	return(tab);
}

#endif

/*
 * be slow and robust
 * if filename is absolute, ignore the dirname
 * if filename begins with "./", ignore the dirname
 * non-absolute dirnames start from scfg->base_dir
 * convert any slashes to backslashes in DOS
 * the compiler will notice if '/'==SLASH and test only once
 */
 
#define IS_NOT_SLASH(x) (x!=SLASH && x!='/')

char *compose_pathname(const char *filename, const char *dirname, const char *treename)
{
	register int tmp=0;
	char *pathname;

	if (!filename) filename = "";
	if (!dirname || !*dirname) dirname = ".";
	if (!treename) treename = "";
	pathname = (char *)xmalloc(strlen(filename) + strlen(dirname) + strlen(treename) + strlen(scfg->base_dir) + 4);
	if (IS_NOT_SLASH(*filename) && (filename[0]!='.' || IS_NOT_SLASH(filename[1]))) {
		if (IS_NOT_SLASH(*dirname)) {
			if (IS_NOT_SLASH(*treename)) {
				strcpy(pathname, scfg->base_dir);
				tmp = strlen(pathname);
				if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++] = SLASH;
			}
			if (*treename) {	// just optimizing away the non-typical branch
				strcpy(pathname+tmp, treename);
				tmp = strlen(pathname);
				if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++] = SLASH;
			}
		}
		strcpy(pathname+tmp, dirname);
		tmp = strlen(pathname);
		if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++] = SLASH;
	}
	strcpy(pathname+tmp, filename);

#if SLASH!='/'
	for (tmp = 0; pathname[tmp]; tmp++)
		if (pathname[tmp]=='/') pathname[tmp] = SLASH;
#endif
	return pathname;
}

char *compose_pathname(const char *filename, const char *dirname)
{
	return compose_pathname(filename, dirname, "");
}

char *limit_pathname(const char *filename, const char *dirname)
{
	char *p;
	char *r;
	if ((int)strlen(filename) >= scfg->scratch_size) shriek(864, "File name too long");
	strcpy(scratch, filename);
	for (p = scratch; !IS_NOT_SLASH(*p); p++);
	for (r = scratch; (*r = *p); p++, r++) {
		if (IS_NOT_SLASH(*p) || p[1] != '.' || p[2] != '.'
				|| p[3] && IS_NOT_SLASH(p[3]))  continue;
		while (IS_NOT_SLASH(*--r) && scratch < r) ;
		p += 3;
	}
	return compose_pathname(scratch, dirname);
}

#undef IS_NOT_SLASH

/*
 *	If file does not exist or stat() is broken, return 0 for a timestamp
 */

int get_timestamp(char *filename)
{
#ifdef HAVE_SYS_STAT_H
	struct stat buff;
	if (!stat(filename, &buff)) return buff.st_ctime;
#endif
	return 0;
}


file::~file()
{
	if (ref_count) shriek(862, "File %s not unclaimed", filename);
	free(data);
	free(filename);
}

static hash_table<char, file> *file_cache = NULL;

inline bool reclaim(file *ff, const char *flags, const char *description, void oven(char *, int))
{
	FILE *f;
	int tmp;
	int ts = get_timestamp(ff->filename);
	int len;

	if (ts == ff->timestamp)
		return false;
	ff->timestamp = ts;
	f = fopen(ff->filename, flags, description);
	fseek(f, 0, SEEK_END);
	tmp = ftell(f) + 1;
	ff->data = (char *)xrealloc(ff->data, tmp);
	fseek(f, 0, SEEK_SET);
	len = fread(ff->data, 1, tmp, f);
	if (len < 0) {
		if (!description) description = "an unspecified stuff";
		shriek(445, "Failed to read %s from %s", description, ff->filename);
	}
	ff->data[len] = 0;
	if (oven != NULL) oven(ff->data, len);
	fclose(f);
	D_PRINT(1, "cache update for %s\n", ff->filename);
	return true;
}

file *claim(const char *filename, const char *dirname, const char *treename, const char *flags, const char *description, void oven(char *, int))
{
	FILE *f;
	file *ff;
	char *pathname;
	char *data;
	int tmp;
	
	if (!file_cache) {
		file_cache = new hash_table<char, file>(30);
		file_cache->dupkey = file_cache->dupdata = false;
	}

	if (!filename) shriek(445, "Failed to read %s (no file name)", description ? description : "??");

	pathname = compose_pathname(filename, dirname, treename);
	ff = file_cache->translate(pathname);
	if (ff) {
		reclaim(ff, flags, description, oven);
		D_PRINT(1, "cache hit on %s\n", pathname);
		free(pathname);
		ff->ref_count++;
		return ff;
	}

	f = fopen(pathname, flags, description);
	if (!f && !description) return NULL;
	if (fseek(f, 0, SEEK_END)) tmp = cfg->dev_text_len;
	else tmp = ftell(f) + 1;
	data = (char *)xmalloc(tmp);
	fseek(f, 0, SEEK_SET);
	tmp = fread(data, 1, tmp, f);
	if (tmp == -1) {
		if (!description) return NULL;
		shriek(445, "Failed to read %s from %s", description, pathname);
	}
	data[tmp] = 0;	  // remember DOS crlfs. tmp suits here.
	if (oven != NULL) oven(data, tmp);
	fclose(f);

	ff = new file;
	ff->ref_count = 1;
	ff->data = data;
	ff->filename = pathname;
	ff->timestamp = get_timestamp(pathname);
	file_cache->add(pathname, ff);
	D_PRINT(1, "cache miss on %s\n", pathname);
	return ff;
}

void uncache_file(char *, file *ff, void *)	// the first argument is ignored
{
	delete file_cache->remove(ff->filename);
}

void unclaim(file *ff)
{
	if (!ff) {
		D_PRINT(3, "Ignoring a bogus unclaim request\n");
		return;
	}
	if (ff->ref_count <= 0) shriek(461, "Forgot to unclaim()");
	
	if (!--ff->ref_count) {
		if (scfg->memory_low) uncache_file(NULL, ff, NULL);
	}
}

void shutdown_file_cache()
{
	if (!file_cache) return; // shriek("No file cache to shutdown");
	if (file_cache->items)
		file_cache->forall(uncache_file, NULL);
	delete file_cache;
	file_cache = NULL;
}

static inline void release(char **buffer)
{
	if (*buffer) free(*buffer);
	*buffer = NULL;
}


static inline void compile_rules()
{
	int tmp = cfg->default_lang;
	_next_rule_line = get_text_line_buffer();
	for (int i=0; i<cfg->n_langs; i++) {
		cfg->default_lang = i;
		if (cfg->langs[i]->n_voices) try {
			cfg->langs[i]->compile_rules();
		} catch (any_exception *e) {
			errors++;
			delete e;
		}
		if (errors > 0) shriek(811, "Rules for %s cannot be compiled", cfg->langs[i]->name);
	}
	free(_next_rule_line); _next_rule_line = NULL;
	cfg->default_lang = tmp;
}


/*
 *	epos_init(): to bring up everything (from a "main() {" state)
 *	epos_done(): to release everything just before exit() (little need to
 *		call this one except for debugging)
 *	epos_reinit(): to reread all files, adjust all structures
		Takes nearly as much time as killing and restarting
 *	epos_catharsis(): to release as much as possible, but leave a way back
 */
 
void use_async_sputs();

void epos_init()	 //Some global sanity checks made here
{
#ifdef HAVE_SYSLOG_H
	openlog("epos", LOG_CONS, LOG_DAEMON);
#endif
	if (!scfg->_loaded)cfg->stddbg = stdout,
				cfg->stdshriek = stderr;
	if (sizeof(int)<4*sizeof(char) || sizeof(int *)<4*sizeof(char)) 
		shriek (862, "I need at least 32 bit arithmetic & pointery [%d]", sizeof(int));
	if (sizeof(int) > sizeof(void *)) shriek(862, "Your integers are longer than pointers!\n"
				"Turn hash_table::forall() fns in hash.h the other way round.");
	if ((unsigned char)-1 != 255) shriek(862, "Your chars are not 8-bit? Funny.");
	if (*(int16_t *)"wxyz" == 256 * 'w' + 'x') scfg->_big_endian = true;
	if (*(int16_t *)"wxyz" != 256 * 'x' + 'w' && !scfg->_big_endian) shriek(862,
				"Not little-endian nor big-endian. Whew!");

#ifdef HAVE_TIME_H
	srand(time(NULL));	// randomize
#endif
	load_default_charset();
	if (!esctab) esctab = new charxlat(scfg->_token_esc, scfg->_value_esc, false);

	config_init();

#ifdef DEBUGGING
	if (scfg->debug && scfg->stddbg_file && *scfg->stddbg_file)
		cfg->stddbg = fopen(scfg->stddbg_file,"w","debugging messages");
#else
	if (scfg->debug) shriek(813, "Either disable debugging in config file, or #define it in interf.h");
#endif
	cfg->stdshriek = stderr;
	if (scfg->stdshriek_file && *scfg->stdshriek_file)
		cfg->stdshriek = fopen(scfg->stdshriek_file, "w", "error messages");
		
//	if (!_subst_buff) _subst_buff = (char *)xmalloc(MAX_GATHER+2);
//	if (!_gather_buff) _gather_buff = (char *)xmalloc(MAX_GATHER+2);
	if (!_resolve_vars_buff) _resolve_vars_buff = get_text_line_buffer(); 
	if (!scratch) {
		scratch = (char *)xmalloc(scfg->scratch_size+1);
		scratch[scfg->scratch_size] = 0;
	}
	use_async_sputs();
	select_local_soundcard();
	
	compile_rules();
	update_sampa();

	D_PRINT(1, "struct unit is %d bytes\n", (int)sizeof(unit));
	D_PRINT(1, "struct static_configuration is %d bytes\n", (int)sizeof(static_configuration));
	D_PRINT(1, "struct configuration is %d bytes\n", (int)sizeof(configuration));
	D_PRINT(1, "struct lang is %d bytes\n", (int)sizeof(lang));
	D_PRINT(1, "struct voice is %d bytes\n", (int)sizeof(voice));
	D_PRINT(2, "allocated %d chars, %d chars free\n", get_count_allocated(), 256 - get_count_allocated());
	D_PRINT(1, "charsets already in use are %s\n", charset_list);
}

void end_of_eternity();
void shutdown_sched_aq();

void epos_catharsis()
{
	if (_unit_just_unlinked) {  // to avoid a memory leak in unlink()
		_unit_just_unlinked->next=NULL;
		delete _unit_just_unlinked;
		_unit_just_unlinked=NULL;
	}

	// one a_protocol may be lost here, see agent.cc

	cow_catharsis();

	release_sampa();

	cfg->shutdown();	//...FIXME: might need proto_cfg->shutdown; othrws shutdown never called
	scfg->shutdown();
	free_extra_options();
	delete esctab; esctab = NULL;

	shutdown_file_cache();
	
	shutdown_enc();
#ifdef HAVE_SYSLOG_H
	closelog();
#endif
}

void free_replier_table();
void shutdown_parser();
void shutdown_partbuffs();

void epos_done()
{
	epos_catharsis();
	free_replier_table();
	free_cmd_line();
	config_release();
	unit::done();
	shutdown_hashing();
	shutdown_cfgs();
	shutdown_sched_aq();
	shutdown_parser();

	release(&_resolve_vars_buff);
	release(&scratch);
	shutdown_partbuffs();
	
	END_OF_ETERNITY;
	cow_catharsis();
}

void epos_reinit()
{
	epos_catharsis();
	reinitialize_configuration();
	epos_init();
}

/*

bool privileged_exec()
{
#ifdef HAVE_GETEGID
	if (getuid() != geteuid() || getgid() != getegid())
		return true;
#endif
	return false;
}

*/


#ifdef DEBUGGING

char *current_debug_tag = NULL;

void debug_prefix(int lev, int area)
{
	unuse(lev); unuse(area);
	color(STDDBG, scfg->normal_color);
	if (current_debug_tag) fprintf(STDDBG, "[%s] ", current_debug_tag);
}

#endif   // ifdef DEBUGGING


#ifdef WANT_DMALLOC

static int forever_count = 0;
static void *forever_ptr_list[ETERNAL_ALLOCS];

char *forever(void *heapptr)
{
	if (forever_count >= ETERNAL_ALLOCS) {
		shriek(863, "Too many eternal allocations (memory leak?)");
		return (char *)heapptr;
	}
	forever_ptr_list[forever_count++] = heapptr;
	return (char *)heapptr;
}

void end_of_eternity()
{
	D_PRINT(3, "Freeing %d permanent heap buffers.\n", forever_count);
	while (forever_count-- > 0) {
		D_PRINT(0, "pointer number %d was %p\n", forever_count, forever_ptr_list[forever_count]);
		free(forever_ptr_list[forever_count]);
	}
}

void *
operator new(size_t n)
{
	void *ret = xmalloc(n);
	return ret;
}

void
operator delete(void * cp)
{
	free(cp);
}

#endif   // ifdef WANT_DMALLOC




void call_abort()
{
#ifdef HAVE_ABORT
	abort();
#else
	while (1) ;	/* as far as I know, only used on Windows CE */
#endif
}

#ifndef HAVE_TERMINATE

void terminate(void)
{
	call_abort();
}

#endif

#ifndef HAVE_FORK
int fork()
{
	return -1;
}
#endif   // ifdef HAVE_FORK



