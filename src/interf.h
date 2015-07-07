/*
 *	epos/src/interf.h
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
 *	This file contains the few things that don't fit elsewhere, 
 *	including nearly everything that should be ever printf'ed.
 */

#ifndef EPOS_INTERF_H
#define EPOS_INTERF_H

#define CFG_FILE_ENVIR_VAR	"EPOSCFGFILE"

void epos_init(int argc, char**argv);
void epos_init();
void epos_reinit();
void epos_done();		// No real need to call, ever. Just to be 100% dmalloc correct.

#define color(stream, seq) if (scfg->colored && seq) fprintf(stream, seq)
void colorize(int level, FILE *handle);  // See this function in interf.cc for various #defines

#define MAX_ERR_LINE 320	// No error nor warning message may be longer
				// than that. We need to have it on the stack,
				// as we dare not allocate it if in trouble

// void check_lib_version(const char *s);

void user_pause();

extern int fatal_bugs;		// normally zero

void shriek(int code, const char *msg, ...)
#ifdef __GNUC__
	__attribute__((__noreturn__))
#endif
					;
char *split_string(char *string);	// 0-terminate the first word, return the rest

FILE *fopen(const char *filename, const char *flags, const char *reason);

void call_abort();

#ifndef HAVE_STRDUP
char *strdup(const char *src);   //Ultrix lacks it. Otherwise, we're just superfluous.
#endif

extern char *scratch;

char *get_text_buffer(int chars);		// alloc with extra space for char encoding/decoding
char *get_text_buffer(const char *string);	// realloc to current length plus extra space as above
char *get_text_line_buffer();
char *get_text_cmd_buffer();

inline const char *never_null(const char *string) { return string ? string : ""; };

FIT_IDX fit(char c);		 // converts 'f', 'i' or 't' to 0, 1 or 2, respectively
UNIT str2enum(const char *item, const char *list, int dflt);
const char *enum2str(int item, const char *list);
void list_of_calls(const char *list, void call(int, const char *));
// hash *str2hash(const char *list, unsigned int max_item_len);
unit *str2units(const char *text);
//char *fntab(const char *s, const char *t); //will calloc and return 256 bytes not freeing s,t
                                       //if len(s)!=len(t), ignore the rest if not cfg.paranoid
//bool *booltab(const char *s);          //will calloc and return 256 bytes not freeing s


char *compose_pathname(const char *filename, const char *dirname, const char *treename);
char *compose_pathname(const char *filename, const char *dirname);
char *limit_pathname(const char *filename, const char *dirname);

struct file
{
	char *data;
	char *filename;
	int ref_count;
	int timestamp;
	~file();
};

file *claim(const char *filename, const char *dirname, const char *treename, const char *flags, const char *description, void oven(char *buff, int len));
// bool reclaim(file *);	// false...unchanged, true...changed
void unclaim(file *);

// void list_languages();
// void list_voices();

class unit;
void process_segments(unit *root);

#define DQUOT		'"'            //used when parsing the .ini file
#define ESCAPE		'\\'
#define EXCLAM		'!'
#define PSEUDOSPACE	'\377'


extern charxlat *esctab;
extern const char* WHITESPACE;       // i.e. space and tab

// extern FILE *stdshriek;
// extern FILE *stdwarn;
// extern FILE *stddbg;

// extern int session_uid;

#define	UID_ANON	-1
#define	UID_ROOT	 0
#define UID_SERVER	 1

#ifndef HAVE_FORK
	int fork();
#endif

// bool privileged_exec();		// true if suid or sgid

// #ifdef DEBUGGING

extern char *current_debug_tag;
// bool debug_wanted(int lev, /*_DEBUG_AREA_*/ int area);
// void debug_prefix(int lev, int area);

// #define DBG(xxx,yyy,zzz) {if(debug_wanted(xxx,yyy)) {debug_prefix(xxx,yyy);zzz;fflush(STDDBG);};}
// #define STDDBG  cfg->stddbg

// #else       // ifndef DEBUGGING
// #define DBG(xxx,yyy,zzz) ; 
// #endif      // ifdef DEBUGGING

/*
 *	For debugging output, please use D_PRINT at all times.
 *	The first parameter to D_PRINT is the debugging message
 *	level, and should be one of:
 *		0  detailed non-systematic message
 *		1  verbose message
 *		2  informational message
 *		3  warning, stage transition or other important message
 *	Other parameters are a format string and optionally arguments
 *	as for printf().
 *
 *	DO_PRINT is equivalent to D_PRINT except that the message
 *	is printed unconditionally.  Use this for temporary ad hoc
 *	selection of requested debugging messages and don't forget
 *	to retreat to the original macro name.
 *
 *	DBG is a more general purpose macro.  It is equivalent to
 *	D_PRINT except that other code than a call to fprintf
 *	can be executed.  The code should however stay very simple
 *	and be limited to debugging output of some kind, e.g.
 *	dumping of arrays with reliable range checking, so as
 *	not to disturb the behavior of the program accidentally.
 *
 *	If DEBUGGING is not defined, all of these macros are
 *	completely ignored.  This may speed up the code and conserve
 *	some space.
 */

#define DEBUGGING

#ifdef DEBUGGING
	#define STDDBG stdout
		#define DBG(x,z) if (x >= scfg->debug_level) { z }
		#define D_PRINT dprint_normal
	#define DO_PRINT dprint_always
#else
	#define STDDBG stdout
	#define DBG(x,z)

	#define D_PRINT dprint_never
	#define DO_PRINT dprint_always
#endif

#ifdef __GNUC__
	#define PRINTF_STYLE(x, y) __attribute__ ((format (printf, x, y)))
#else
	#define PRINTF_STYLE(x, y) /* format will not be checked */
#endif
		
char *fmt(const char *s, ...) PRINTF_STYLE(1,2);

void dprint_always    (int level, const char *s, ...) PRINTF_STYLE(2,3);
void dprint_normal(int level, const char *s, ...) PRINTF_STYLE(2,3);
inline void dprint_never     (int level, const char *s, ...) {}

void tag_vfprintf(const char *fmt, va_list ap);
inline void dprint_normal(int level, const char *fmt, ...)
{
	if (scfg->debug_level <= level) {
		va_list ap;
		va_start(ap, fmt);
		tag_vfprintf(fmt, ap);
		va_end(ap);
	}
}


extern void *xmall_ptr_holder;

#if 0
	inline void log_alloc(size_t x)
	{
		if (x > 1024) {
			D_PRINT(1, "Alloc %d\n", x);
		}
	}
	inline void log_realloc(size_t x)
	{
		D_PRINT(1, "Realloc %d\n", x);
	}
#else
	inline void log_alloc(size_t) {};
	inline void log_realloc(size_t) {};
#endif

#define OOM_HANDLER	(shriek(422, "Out of memory"), (void *)NULL)
#define xmalloc(x)	((xmall_ptr_holder = malloc((x))) ? (log_alloc(x), xmall_ptr_holder) : OOM_HANDLER)
#define xrealloc(x,y)	((((xmall_ptr_holder = realloc((x),(y))))) && (x) ? (log_realloc(y), xmall_ptr_holder) : OOM_HANDLER)
#define xcalloc(x,y)	(((xmall_ptr_holder = calloc((x),(y)))) ? log_alloc(x*y), xmall_ptr_holder : OOM_HANDLER)



#ifdef WANT_DMALLOC

char *forever(void *heapptr);
void end_of_eternity();
#define FOREVER(allocated) forever(allocated)
#define ETERNAL_ALLOCS	1024
#define END_OF_ETERNITY  end_of_eternity()

#else       // ifndef WANT_DMALLOC
#define FOREVER(allocated) (allocated)
#define END_OF_ETERNITY  /**/
#endif      // ifdef WANT_DMALLOC


#endif      // ifndef EPOS_INTERF_H
