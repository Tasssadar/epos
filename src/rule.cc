/*
 *	epos/src/rule.cc
 *	(c) 1996-99 geo@cuni.cz
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
 *	This file includes block.cc at the very last line.
 *
 */

#include "epos.h"

#ifdef HAVE_WINDOWS_H
	#include <windows.h>
#endif

#define SEG_BUFF_SIZE  1000 //unimportant

#define OPCODEstr "nnet:subst:regex:postp:prep:segments:absolutize:prosody:contour:progress:regress:insert:syll:analyze:smooth:raise:debug:if:inside:near:with:fail:{:}:[:]:<:>:nothing:error:"
enum OPCODE {OP_NNET, OP_SUBST, OP_REGEX, OP_POSTP, OP_PREP, OP_SEG, OP_ABSOL, OP_PROSODY, OP_CONTOUR, OP_PROGRESS, OP_REGRESS, 
	OP_INSERT, OP_SYLL, OP_ANALYZE, OP_SMOOTH, OP_RAISE, OP_DEBUG, OP_IF, OP_INSIDE, OP_NEAR, OP_WITH,
	OP_FAIL,
	OP_BEGIN, OP_END, OP_CHOICE, OP_CHOICEND, OP_SWITCH, OP_SWEND, OP_NOTHING, OP_ERROR};
		/* OP_BEGIN, OP_END and other OP's without parameters should come last
		   OP_FAIL  compiles, and only reports an error after having been "applied"
		   OP_ERROR would abort the compilation (never used)			*/
enum RULE_STATUS {RULE_OK, RULE_IGNORED, RULE_BAD=-1};



rule *next_rule(text *file, hash *vars, int *count);	// count is an out-parameter
extern char * _rules_buff;



// #define DEFAULT_SCOPE    U_WORD
// #define DEFAULT_TARGET   U_PHONE

/*
 * Syntax of an assimilation rule proper: "ptk>bdg(aeiou_aeiou)" is defined here
 * The delimiters need not be different, but they shouldn't collide with the delimitees
 */
 
#define ASSIM_DELIM1    '>'
#define ASSIM_DELIM2    '('
#define ASSIM_DELIM3    '_'
#define ASSIM_DELIM4    ')'
#define RAISE_DELIM	':'

#define LESS_THAN       '<'	// to delimit the sonority groups in parse_syll() 

#define COMMA		','	// item,value
#define SPACE		' '	// to delimit lists in literal hash tables
#define TILDE		'~'

#define UNSEGMENT "~"			//OP_SEG only, means "delete segments if any"

class rule
{
   private:
	virtual void apply(unit *root) = 0;
   protected:
	const char *debug_tag();
   public:
	virtual OPCODE code() = 0;
	UNIT   scope;
	UNIT   target;
	char  *raw;

	     rule(char *param);
	virtual    ~rule();
	virtual void set_level(UNIT scope, UNIT target);
	virtual void set_dbg_tag(text *file);
	virtual void check_child(rule *r);
	virtual void check_children();
	virtual void verify() {};
	virtual void debug();

	void cook(unit *root);
#ifdef DEBUGGING
	char *dbg_tag;
#endif
};

class hashing_rule: public rule
{
   protected:
	bool allow_id;
	bool use_fastmatch;
	char negated;
	hash *dict;
	regex_t *fastmatch;
   public:
		hashing_rule(char *param);
	virtual ~hashing_rule();
	virtual void verify();
	void load_hash();
};

rule::rule(char *param)
{
	raw = param ? strdup(param) : (char *)NULL;
#ifdef DEBUGGING
	dbg_tag = NULL;
#endif
}

rule::~rule()
{
	D_PRINT(0, "rule::~rule, raw=%s, dbg_tag=%s\n", raw, dbg_tag);
	if (raw) free(raw);
#ifdef DEBUGGING
	if (dbg_tag) free(dbg_tag);
#endif
}

void
rule::set_level(UNIT scp, UNIT trg)
{
	scope =  scp == U_DEFAULT ? scfg->default_scope  : scp;
	target = trg == U_DEFAULT ? scfg->default_target : trg;
	if (scope < target) shriek (811, "%s Scope must not be more narrow than target", debug_tag());
			// was scope <= target -  consider returning back
}

void
rule::cook(unit *root)
{
	bool tmpscope;
	unit *u;
#ifdef DEBUGGING
		if (scfg->debug) current_debug_tag = dbg_tag;
		if (scfg->show_rule) fprintf(STDDBG, "[%s] %s %s\n",
			dbg_tag,
			enum2str(code(), OPCODEstr), raw);
#endif
	for (u = root->LeftMost(scope); u != &EMPTY;) {
		unit *tmp_next = u->Next(scope);
		tmpscope = u->scope;
		u->scope = true;
		apply(u);
		u->scope = tmpscope;
		u = tmp_next;
	}
#ifdef DEBUGGING
		current_debug_tag = NULL;
#endif
}

void
rule::set_dbg_tag(text *file)
{
#ifdef DEBUGGING
//	D_PRINT(0, "Debugging tag %s\n", file->current_file);
	sprintf(scratch, "%s:%d", file->current_file, file->current_line);
	dbg_tag=strdup(scratch);
#else
	unuse(file);
#endif
}

void
rule::check_child(rule *r)
{
	if (this->scope < r->scope) {
		if (r->scope != U_INHERIT) {
			printf("this %d child %d\n",scope, r->scope);
			debug();
			shriek(811, "%s Scope of rule exceeds scope of its block", debug_tag());
		}
		r->scope = this->scope;
	}
}

void
rule::check_children()
{
}

const char *
rule::debug_tag()
{
	static char *wholetag=(char *)FOREVER(xmalloc(scfg->max_line_len));

#ifdef DEBUGGING
	char *tmp;
	if (dbg_tag) {
		tmp=strchr(dbg_tag, ':');
		if (!tmp) {
			sprintf(wholetag, "in unnumbered place %s", dbg_tag);
		} else {
			*tmp++=0;
			sprintf(wholetag, "%s:%s", dbg_tag, tmp);
			tmp[-1]=':';
		}
	} else
#endif
	{
		if (!global_current_file) return ". Debug tag is corrupted as well.";
		sprintf(wholetag, "%s:%d", global_current_file, global_current_line);
	}
	return wholetag;
}


/* Default value = key */

hash *literal_hash(char *s)
{
	char *p;
	char *last;
	char *comma = NULL;

	hash *dict = new hash((strlen(s) >> 4) + 4);

	p = s;
	last = s + 1;
	while (1) {
		switch(*++p) {
		case COMMA:
			if (comma) shriek(811, "too many commae");
			comma = p;
			*comma = 0;
			break;
		case SPACE:
		case TILDE:
			*p = 0;
			dict->add(last, comma ? comma+1 : last);
			if (comma) *comma = COMMA;
			*p = SPACE;
			last = NULL;
			comma = NULL;
			for (last = p+1; *last == SPACE; last++) ;
			break;
		case DQUOT:
			*p = 0;
			if (last) dict->add(last, comma ? comma+1 : last);
			if (comma) *comma = COMMA;
			*p = DQUOT;
			return dict;
		case 0: return NULL;	// error - unterminated string
		}
	}
}

#if 0
static char esc(char x)
{
	return esctab->xlat(x);
}
#endif

hashing_rule::hashing_rule(char *param) : rule(NULL)
{
	negated = 0;
	if (*param == '!') param++, negated = 1;
	raw = strdup(param);
//	if (*param == DQUOT) raw = strdup(param);
//	else raw = compose_pathname(param, this_lang->hash_dir, scfg->lang_base_dir);
	dict = NULL;
	fastmatch = NULL;
	allow_id = false;
	use_fastmatch = false;
}


hashing_rule::~hashing_rule()
{
	if (dict) delete dict;
#ifdef WANT_REGEX
	if (fastmatch) {
		regfree(fastmatch);
		free(fastmatch);
	}
#endif
}

void
hashing_rule::verify()
{
	if (negated > 1)
		shriek(811, "%s Unexpected negated dictionary", debug_tag());
	if (cfg->paranoid) {
		load_hash();

		if (scfg->memory_low) {
			D_PRINT(2, "Hash table caching is disabled.\n"); //hashtabscache[rulist[i].param]->debug();
			delete dict;
			dict = NULL;
		}
	}
}

#ifdef WANT_REGEX

#define REGEX_SPECIALS	".[*\\"	// contains no ^ nor $ nor { nor ( ) + = |

static bool begins_with_caret(char *key) { return key[0] == '^'; }
static bool ends_with_dollar(char *key) { return *key && key[strlen(key) - 1] == '$'; }
static int special_chars(char *key)
{
	int r = 0;
	for (char *s = key; *s; s++) if (strchr(REGEX_SPECIALS, *s)) r++;
	return r;
}

void strcpy_escapeful(char *where, const char *from)
{
	do {
		if (*from && strchr(REGEX_SPECIALS, *from)) *where++ = '\\';
	} while ((*where++ = *from++));
}

static void compute_size(char *key, char *value, void *total)
{
	*(int *)total += strlen(key) + 2 + special_chars(key)
		+ 2 * begins_with_caret(key) + 2 * ends_with_dollar(key);
}

struct buffie
{
	char *buffer;
	int offset;
};

static void append_to_regex(char *key, char *value, void *bp)
{
	buffie *b = (buffie *)bp;
	char *s = b->buffer;
	int l = b->offset;

	if (begins_with_caret(key)) {
		strcpy(s + l, "^\\");
		l += 2;
	}
	strcpy_escapeful(s + l, key);
	l += strlen(key) + special_chars(key);

	if (ends_with_dollar(key)) {
		strcpy(s + l - 1, "\\$$");
		l += 2;
	}
	strcpy(s + l, "\\|");
	l += 2;
	b->offset = l;
}

static regex_t *regex_from_hash(hash *h)
{
	int total = 3;	
	h->forall(compute_size, &total);
	char *buffer = (char *)xmalloc(total);
	*buffer = 0;
	buffie b;
	b.buffer = buffer;
	b.offset = 0;
	h->forall(append_to_regex, &b);
	if (b.offset > 2) b.buffer[b.offset - 2] = 0;
	D_PRINT(1, "Fastmatch regex: %s\n", b);
	regex_t *r = (regex_t *)xmalloc(sizeof(regex_t));
	int status = regcomp(r, b.buffer, 0);
	if (status) {
		D_PRINT(3, "Failed to create a fastmatch from %.20s..., code=%d\n", b.buffer, status);
		r = 0;
	}
	free(b.buffer);
	return r;
}

#endif


void
hashing_rule::load_hash()
{
	if (dict) shriek(862, "unwanted load_hash");

	dict = NULL;
	if (*raw != DQUOT) {
		dict = new hash(raw, this_lang->hash_dir, scfg->lang_base_dir, "dictionary",
			scfg->hashes_full, 0, 200, 5, allow_id, false);
	} else dict = literal_hash(raw);
#ifdef WANT_REGEX
	if (use_fastmatch) {
		fastmatch = regex_from_hash(dict);
		if (!fastmatch) {
			use_fastmatch = false;
		}
	}
#endif
	if (!dict) shriek(463, "%s Unterminated argument", debug_tag());	// or out of memory
}

void 
rule::debug()
{
	fprintf(STDDBG," rule: '%s' ",enum2str(code(),OPCODEstr));
	fprintf(STDDBG,"par %s scope '%s' ", raw, enum2str(scope, scfg->unit_levels));
	fprintf(STDDBG,"target '%s'\n", enum2str(target, scfg->unit_levels));
}




/************************************************
 r_subst  The following rule classes implement
	  substitutions and joining of units
 **	  by enumeration
 ************************************************/


class r_subst: public hashing_rule
{
   protected:
	SUBST_METHOD method;
	virtual OPCODE code() {return OP_SUBST;};
   public:
		r_subst(char *param);
	virtual void set_level(UNIT scope, UNIT target);
	virtual void apply(unit *root);
};

class r_prep: public r_subst
{
	virtual OPCODE code() {return OP_PREP;};
   public:
		r_prep(char *param);
};

class r_postp: public r_subst
{
	virtual OPCODE code() {return OP_POSTP;};
   public:
		r_postp(char *param);
};

r_subst::r_subst(char *param) : hashing_rule(param)
{
	method = M_SUBSTR;
	negated *= 2;
	use_fastmatch = scfg->fastmatch_substs;
}

r_prep::r_prep(char *param) : r_subst(param)
{
	method = M_RIGHT;
	if (negated) {
		method = (SUBST_METHOD)(method | M_NEGATED);
		negated = 1;
	}
	allow_id = true;
	use_fastmatch = false;
}

r_postp::r_postp(char *param) : r_subst(param)
{
	method = M_LEFT;
	if (negated) {
		method = (SUBST_METHOD)(method | M_NEGATED);
		negated = 1;
	}
	allow_id = true;
	use_fastmatch = false;
}

void
r_subst::set_level(UNIT scp, UNIT trg)
{
	rule::set_level(scp, trg);
}

/************************************************
 r_subst::apply
 ************************************************/

void
r_subst::apply(unit *root)
{
	if (!dict) load_hash();

//	if (target == U_PHONE) root->subst(dict, method);

	root->relabel(dict, fastmatch, method, target);

	if (scfg->memory_low) {
		D_PRINT(2, "Hash table caching is disabled.\n"); //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
}

/************************************************
 r_seg   The following rule class constructs
 	  the segment layer according to segment
 **	  numbers found in the dictionary
 ************************************************/


class r_seg: public hashing_rule
{
	virtual OPCODE code() {return OP_SEG;};
   public:
		r_seg(char *param);
	virtual void apply(unit *root);
};

r_seg::r_seg(char *param) : hashing_rule(param)
{
	if (!strcmp(param, UNSEGMENT)) {
		free(raw);
		raw=strdup(NULL_FILE);
	}
}

/************************************************
 r_seg::apply()
 ************************************************/

void
r_seg::apply(unit *root)
{
	if (!dict) load_hash();
	root->segs(target, dict);
	if (scfg->memory_low) {
		D_PRINT(2, "Hash table caching is disabled.\n"); //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict=NULL;
	}


	D_PRINT(1, "Segments w%s be dumped just after the segments rule\n", scfg->immed_segments?"ill":"on't");
	if (scfg->immed_segments) {
		static segment d[SEG_BUFF_SIZE];   //every item is 16 bytes long
		
		int i = SEG_BUFF_SIZE;
		for (int k=0; i == SEG_BUFF_SIZE; k += SEG_BUFF_SIZE) {
			i = root->write_segs(d, k, SEG_BUFF_SIZE);
			for (int j=0;j<i;j++) 
				fprintf(STDDBG,"segment number=%3d f=%d t=%d i=%d\n", d[j].code, d[j].f, d[j].t, d[j].e);
		}
	}
}










/************************************************
 r_absol  The following rule class computes
 	  the absolute timings for target units
 **	  using voice dependent data
 ************************************************/


class r_absol: public hashing_rule
{
	virtual OPCODE code() {return OP_ABSOL;};
   public:
		r_absol(char *param);
	virtual void apply(unit *root);
};

r_absol::r_absol(char *param) : hashing_rule(param)
{
}

/************************************************
 r_absol::apply()
 ************************************************/

void
r_absol::apply(unit *root)
{
	if (!dict) load_hash();

//	if (target == U_PHONE) root->subst(dict, method);

	root->absol(dict, target);

	if (scfg->memory_low) {
		D_PRINT(2, "Hash table caching is disabled.\n"); //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
}












/************************************************
 r_prosody The following rule class manipulates
	   suprasegmentalia according to subrules
 **	   in a dictionary
 ************************************************/


class r_prosody: public hashing_rule
{
	virtual OPCODE code() {return OP_PROSODY;};
   public:
		r_prosody(char *param);
	virtual void apply(unit *root);
};

r_prosody::r_prosody(char *param) : hashing_rule(param)
{
	negated *= 2;
}

/************************************************
 r_prosody::apply
 ************************************************/

void
r_prosody::apply(unit *root)
{
	if (!dict) load_hash();
	D_PRINT(1, "entering rules::sseg()\n");
//	root->sseg(target, dict);
	shriek(862, "no sseg");
	if (scfg->memory_low) {
		D_PRINT(2, "Hash table caching is disabled.\n"); //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
}
/************************************************
 r_contour The following rule class distributes
	  some prosodic contour over a linear
 **	  sequence of units
 ************************************************/

class r_contour: public rule
{
   protected:
	virtual OPCODE code() {return OP_CONTOUR;};
	int *contour;
	int l;
	int padd_start;
	FIT_IDX quantity;
   public:
		r_contour(char *param);
	virtual ~r_contour();
	virtual void apply(unit *root);
};

r_contour::r_contour(char *param) : rule(param)
{
	char *p;
	int tmp=0, sgn=1;
	
	contour = (int *)xmalloc(strlen(param)*sizeof(int));
	contour[0] = l = 0;
	padd_start = -1;
	for (p=param+1+(param[1]=='/'); *p; p++) {
		switch (*p) {
		case ':':  contour[l++] += tmp*sgn; tmp=0;
				sgn=1; contour[l] = 0;  break;
		case '*':  if (p[1] && p[1] != ':') shriek(811, "%s A ':' should follow '*'", debug_tag());
			   if (padd_start > -1) shriek(811, "%s Ambiguous padding", debug_tag());
				padd_start = l; break;
		case '-':
		case '+':  contour[l]+=tmp*sgn; sgn=(*p=='+' ?+1:-1); break;
		default:   if (*p<'0' || *p>'9') shriek(811, "%s Expected a number, found \"%s\"",
					debug_tag(), p);
			   else tmp = tmp*10 + *p-'0';
		}
	}
	if (tmp) contour[l++] += tmp*sgn;

	quantity = fit(*param);
}

r_contour::~r_contour()
{
	free(contour);
}

void r_contour::apply(unit *root)
{
	root->contour(target, contour, l, padd_start, quantity, false);
}


/************************************************
 r_smooth The following rule class smoothens
	  suprasegmentalia using a given weighted
 **	  averaging
 ************************************************/


class r_smooth: public rule
{
   protected:
	virtual OPCODE code() {return OP_SMOOTH;};
	int *list;
	int n;
	int l;
	FIT_IDX quantity;
   public:
		r_smooth(char *param);
	virtual ~r_smooth();
	virtual void apply(unit *root);
};

r_smooth::r_smooth(char *param) : rule(param)
{
	char *p;
	int tmp=0, sgn=1, total=0, max;
	
	list=(int *)xmalloc(strlen(param)*sizeof(int));
	list[0]=n=l=0;
	for (p=param+1+(param[1]=='/'); *p; p++) {
		switch (*p) {
		case '/':  n++;		/* and fall through */
		case '\\': list[l++] += tmp*sgn; total += tmp*sgn; tmp=0;
			   sgn=1; list[l]=0;  break;
		case '-':
		case '+':  list[l]+=tmp*sgn; sgn=(*p=='+' ?+1:-1); break;
		default:   if (*p<'0' || *p>'9') shriek(811, "%s Expected a number, found \"%s\"",
					debug_tag(), p);
			   else tmp = tmp*10 + *p-'0';
		}
	}
	if (tmp) list[l++]+=tmp*sgn, total+=tmp*sgn;
	if (total!=RATIO_TOTAL)
		shriek (811, "%s Smooth percentages don't add up to 100%% (%d%%)", debug_tag(), total);
	if (cfg->paranoid) {
		for (tmp=max=0; tmp<l; tmp++)
			if (list[tmp]>max) max=list[tmp];
		if (max!=list[n]) shriek(811, "Oversmooth, max weight is given elsewhere");
	}

	if (l>=SMOOTH_CQ_SIZE) shriek(864, "unit::smooth circular queue too small, increase SMOOTH_CQ_SIZE");
	
	quantity = fit(*param);
}

r_smooth::~r_smooth()
{
	free(list);
}


/************************************************
 r_smooth::apply()
 ************************************************/
 
void
r_smooth::apply(unit *root)
{
	
	D_PRINT(1, "entering rules::smooth()\n");
	root->project(target);
	root->smooth(target, list, n, l, quantity);
}

/************************************************
 r_*gress The following rule classes implement
	  the traditional "phonetic changes",
 **	  i.e. which phone is changing to which
 **	  one and in what environment
 ************************************************/

static void switch_zeroes(char *s)
{
	int i, j;
	for (i = 0, j = 0; s[i]; ) {
		if (s[i] == LITERAL_ZERO) {
			if (i && s[i - 1] == ESCAPE)
				j--;
			else {
				s[j++] = ABSENT_CHARACTER;
				i++;
				continue;
			}
		}
		s[j++] = s[i++];
	}
	s[j] = 0;
}

class r_regress: public rule
{
   protected:
	virtual OPCODE code() {return OP_REGRESS;};

	charclass *ltab;       //callocced by booltab()
	charclass *rtab;
	charxlat *fn;
	bool backwards;

   public:
		r_regress(char *param);
	virtual ~r_regress();
	virtual void apply(unit *root);
};

class r_progress: public r_regress
{
	virtual OPCODE code() {return OP_PROGRESS;};
   public:
		r_progress(char *param);
};


r_regress::r_regress(char *param) : rule(param)
{
	char *aff;
	char *eff;
	char *left;      
	char *right;

	eff = strchr(aff=strdup(param),ASSIM_DELIM1);
	if (!eff) shriek(811, "%s Bad param", debug_tag());
	*eff++ = 0;
	switch_zeroes(aff);

	left = strchr(eff,ASSIM_DELIM2);
	if(!left) shriek(811, "%s Bad param", debug_tag());
	*left++ = 0;
	switch_zeroes(eff);
	
	right = strchr(left,ASSIM_DELIM3);
	if (!right) shriek(811, "%s Bad param", debug_tag());
	*right++ = 0;
	switch_zeroes(left);


	char *tmp = strchr(right,ASSIM_DELIM4);
	if (!tmp) shriek(811, "%s Bad param", debug_tag());
	*tmp++ = 0;
	switch_zeroes(right);
	if (*tmp) shriek(811, "%s Strange appendix to param", debug_tag());
	
	D_PRINT(0, "Parsed assim param \"%s>%s(%s_%s)\"\n",aff,eff,left,right);

	if (strlen(aff) != strlen(eff) && strlen(eff) != 1)
		shriek(811, "%s Bad param", debug_tag());
	fn = new charxlat(aff,eff,true); ltab = new charclass(left); rtab = new charclass(right);
	free(aff);
	
	backwards = true;
}

r_regress::~r_regress()
{
	delete fn;
	delete ltab;
	delete rtab;
}

r_progress::r_progress(char *param) : r_regress(param)
{
	backwards = false;
}

/********************************************************
 r_*gress::apply
 ********************************************************/

void 
r_regress::apply(unit *root)
{
	root->assim(target,backwards,fn,ltab,rtab);

	if (fn->xlat(JUNCTURE) != JUNCTURE)
		root->insert(target, backwards, fn->xlat(JUNCTURE), ltab, rtab);
}

/************************************************
 r_syll   The following rule class splits words
 	  into syllables using sonority values
 **	  of phones
 ************************************************/


class r_syll: public rule
{
	virtual OPCODE code() {return OP_SYLL;};
	char *son;
   public:
		r_syll(char *param);
	virtual ~r_syll();
//	virtual void set_level(UNIT scope, UNIT target);
	virtual void apply(unit *root);
};

#define MIN_SONORITY	1
#define NO_SONORITY	1

r_syll::r_syll(char *param) : rule(param)
{
	int lv = MIN_SONORITY;
	char *tmp;
	int i;

	son=(char *) xmalloc(256);
	for (i = 0; i < 256; i++) son[i] = NO_SONORITY;
	for (tmp = param; *tmp && lv; tmp++) {
		switch (*tmp) {
			case LESS_THAN:
				lv++;
				continue;	
			case LITERAL_ZERO:
				*tmp = ABSENT_CHARACTER;
				// and fall through
			default:
				son[(unsigned char)(*tmp)] = lv;
				D_PRINT(1, "Giving to %c sonority %d\n", *tmp, lv);
		}
	}
	D_PRINT(0, "rules::parse_syll going to call syllabify()\n");
}

r_syll::~r_syll()
{
	free(son);
}

/********************************************************
 r_syll::apply
 ********************************************************/

void
r_syll::apply(unit *root)
{
	root->syllabify(target, son);
}

/************************************************
 r_analyze   The following rule class splits words
 	     into morphemes using a dictionary
 **	     of available morphemes
 ************************************************/


class r_analyze: public hashing_rule
{
	virtual OPCODE code() { return OP_ANALYZE; };
	int unanal_unit_penalty, unanal_part_penalty;
   public:
		r_analyze(char *param);
	virtual ~r_analyze();
	virtual void apply(unit *root);
	virtual void verify();
};

r_analyze::r_analyze(char *param) : hashing_rule(param)
{
}

r_analyze::~r_analyze()
{
}

void
r_analyze::verify()
{
	hashing_rule::verify();

	char *uup = dict->translate("!META_unanal_unit_penalty");
	char *upp = dict->translate("!META_unanal_part_penalty");
	if (!uup || !upp)
		shriek(811, "%s Must specify penalties in the dictionary", debug_tag());
	unanal_unit_penalty = atoi(uup);
	unanal_part_penalty = atoi(upp);
}

/********************************************************
 r_analyze::apply
 ********************************************************/

void
r_analyze::apply(unit *root)
{
	root->analyze(target, dict, unanal_unit_penalty, unanal_part_penalty);
}



/************************************************
 r_raise  The following rule class moves
 	  tokens between different levels
 **	  (phones to sentences et c.)
 ************************************************/


class r_raise: public rule
{
	virtual OPCODE code() {return OP_RAISE;};
	charclass *whattab;
	charclass *whentab;
   public:
		r_raise(char *param);
	virtual ~r_raise();
	virtual void apply(unit *root);
};

r_raise::r_raise(char *param) : rule(param)
{
	char *cond;
	if ((cond = strchr(raw,RAISE_DELIM))) *cond++=0; else cond=(char *)"!";
	whattab = new charclass(raw);
	whentab = new charclass(cond);
}

r_raise::~r_raise()
{
	delete whattab;
	delete whentab;
}

void
r_raise::apply(unit *root)
{
	root->raise(whattab, whentab, scope, target);
};

#ifdef WANT_REGEX

/************************************************
 r_regex  The following rule class can replace
 	  an arbitrary regular expression with
 **	  a replacement based on it
 ************************************************/


class r_regex: public rule
{
	regex_t regex;
	int parens;
	regmatch_t *matchbuff;
	char *repl;
	virtual OPCODE code() {return OP_REGEX;};
   public:
		r_regex(char *param);
		~r_regex();
	virtual void apply(unit *root);
};

#define PAREN_OPEN  '('
#define PAREN_CLOS  ')'

#define rshr(x) shriek(811, "%s Regex invalid: %s", debug_tag(), x);

r_regex::r_regex(char *param) : rule(param)
{
	char separator = *param++;
	char *tmp = strchr(param, separator);
	if (!tmp) shriek(811, "%s Regex param should be separated thus: /regex/replacement/", debug_tag());
	*tmp++=0;
	parens = 0;
	int result;
	
	for(int i=0, j=0; ; i++,j++) {
		if (param[i]==PAREN_OPEN || param[i]==PAREN_CLOS) {
			if (j && scratch[j-1]==ESCAPE) j--;
			else {
				scratch[j++] = ESCAPE;
				parens++;
			}
		}
		scratch[j] = param[i];
		if (!param[i]) break;
	}
	
	param = strdup(scratch);

	matchbuff = (regmatch_t *)xmalloc((parens+2)*sizeof(regmatch_t));
	D_PRINT(0, "Compiling regex %s\n", param);
	result = regcomp(&regex, param, 0);
	switch (result) {
		case 0: break;
		case REG_BADBR:
		case REG_EBRACE:
			rshr("braces should specify an interval");
		case REG_EBRACK:
		case REG_ERANGE:
		case REG_ECTYPE:
			rshr("brackets should enclose a list");
		case REG_EPAREN:
		case REG_ESUBREG:
			rshr("read the docs about () subexps");
		case REG_EESCAPE:
			rshr("badly escaped");
		case REG_BADRPT:
		case REG_BADPAT:
#ifdef HAVE_REG_EEND
		case REG_EEND:
#endif
			rshr("too ugly");
		case REG_ESPACE:
#ifdef HAVE_REG_EEND		// EEND and ESIZE are glibc specific
		case REG_ESIZE:
#endif
			rshr("too huge");
		default: shriek(811, "%s Bad regex, regcomp returns %d", debug_tag(), result);
	}
	free(param);
	repl = tmp;
	tmp = strchr(tmp, separator);
	if (!tmp) shriek(811, "%s Regex param should be separated thus: /regex/replacement/ ", debug_tag());
	if (tmp[1]) shriek(811, "%s garbage follows replacement", debug_tag());
	*tmp=0;
	repl=strdup(repl);
	D_PRINT(0, "%s Regex is okay\n", debug_tag());
}

#undef rshr

r_regex::~r_regex()
{
	free(matchbuff);
	regfree(&regex);
	free(repl);
}

void
r_regex::apply(unit *root)
{
	root->regex(&regex, parens, matchbuff, repl);
}

#endif


/************************************************
 r_debug  The following rule class can print
 	  various data at the moment the rule
 **	  is applied
 ************************************************/

//typedef UINT (CALLBACK* TSR_EVAL)(unit*);

typedef int (*TSR_EVAL)(unit*);

class r_debug: public rule
{
	virtual OPCODE code() {return OP_DEBUG;};
	TSR_EVAL tsr_eval;
   public:
		r_debug(char *param);
	virtual void apply(unit *root);
};

r_debug::r_debug(char *param) : rule(param)
{
}

void
r_debug::apply(unit *root)
{
	if(strstr(raw,"tsrtool")) {
#ifdef HAVE_WINDOWS_H		// The TSR debuging tool is up to now available only in the Win32 port
		HINSTANCE hDLL = LoadLibrary("tsrtool.dll");
		if (hDLL != NULL)
		{
			tsr_eval = (TSR_EVAL)GetProcAddress(hDLL,"_tsr_eval");
			if (!tsr_eval) shriek(445, "cannot get 'tsr_eval' function from tsrtool.dll");
			//root->fout(NULL);
			//printf("tsr_eval...\n");
			tsr_eval(root);
		}
#endif
	}
	if(strstr(raw,"elem")) root->fout(NULL);
//	if(strstr(raw,"rules")) ruleset->debug();
//	else if(strstr(raw,"rule") && ruleset->current_rule+1 < ruleset->n_rules)
//		ruleset->rulist[ruleset->current_rule+1]->debug();
	if(strstr(raw,"pause")) user_pause();
//	if(strstr(raw,"dumpunit")) root->filedump ((char*) cfg->dumpfilename);
}

/************************************************
 cond_rule (abstract class)
 ************************************************/

class cond_rule: public rule
{
   protected:
	rule *then;
   public:
	cond_rule(char *param, text *file, hash *vars);
	~cond_rule();
	virtual void check_children();
};

cond_rule::cond_rule(char *param, text *file, hash *vars) : rule(param)
{
	then = next_rule(file, vars, NULL);
}

cond_rule::~cond_rule()
{
	delete then;
}

void 
cond_rule::check_children()
{
	check_child(then);
	then->check_children();
}

/************************************************
 r_inside The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  inside the units whose contents is
 **	  listed in r_inside parameter
 ************************************************/


class r_inside: public cond_rule
{
	virtual OPCODE code() {return OP_INSIDE;};
	charclass *affected;
   public:
		r_inside(char *param, text *file, hash *vars);
		~r_inside();
	virtual void apply(unit *root);
};

r_inside::r_inside(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	affected = new charclass(raw);
}

r_inside::~r_inside()
{
	delete affected;
}

void
r_inside::apply(unit *root)
{
	if (affected->ismember((unsigned char)root->cont)) then->cook(root);
}

/************************************************
 r_near  The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  inside the units which contain a unit
 **	  whose content is listed in r_near parameter
 ************************************************/


class r_near: public cond_rule
{
	virtual OPCODE code() {return OP_NEAR;};
	bool universal;
	charclass *affected;
   public:
		r_near(char *param, text *file, hash *vars);
		~r_near();
	virtual void apply(unit *root);
};

#define ASTERISK   '*'
#define ASTERISK_REPLACEMENT	'!'

r_near::r_near(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	universal = false;
	if (*raw == ASTERISK)
	{
		universal = true;
		*raw = ASTERISK_REPLACEMENT;
	}
	affected = new charclass(raw);
	if (universal) *raw = ASTERISK;
}

r_near::~r_near()
{
	delete affected;
}

void
r_near::apply(unit *root)
{
	if (root->contains(target, affected) ^ universal) then->cook(root);
}

/************************************************
 r_with   The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  inside the units whose subordinates
 **	  form a string found in the parameter file
 ************************************************/


class r_with: public cond_rule
{
	virtual OPCODE code() {return OP_WITH;};
	hash *dict;
	bool negated;
   public:
		r_with(char *param, text *file, hash *vars);
		~r_with();
	virtual void apply(unit *root);
};

r_with::r_with(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	char *old = raw;

	negated = false;
	if (*raw == '!') raw++, negated = true;

	raw = strdup(raw);
//	if (*raw == DQUOT) raw = strdup(raw);
//	else raw = compose_pathname(raw, this_lang->hash_dir, scfg->lang_base_dir);

	free(old);
	dict = NULL;
}

r_with::~r_with()
{
	if (dict) delete dict;
}

void
r_with::apply(unit *root)
{
	if (!dict) {
		if (*raw == DQUOT) dict = literal_hash(raw);
		else dict = new hash(raw, this_lang->hash_dir, scfg->lang_base_dir, "dictionary",
			scfg->hashes_full, 0, 200, 3, true, false);
	}
	if (!dict) shriek(811, "%s Unterminated argument", debug_tag());	// or out of memory

	if (root->subst(dict, NULL, M_ONCE) ^ negated) then->cook(root);
}


/************************************************
 r_if     The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  if a global condition holds
 ************************************************/


class r_if: public cond_rule
{
	virtual OPCODE code() {return OP_IF;};
//	bool result;
	int flag_offs;	// offset for this_voice
 	virtual void set_level(UNIT scp, UNIT trg);
  public:
		r_if(char *param, text *file, hash *vars);
	virtual void apply(unit *root);
};

r_if::r_if(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	epos_option *o = option_struct(raw + (*raw == EXCLAM) , this_lang->soft_opts);
	if (!o) shriek(811, "%s Not an option: %s", debug_tag(), raw);
	if (o->opttype != O_BOOL) shriek(811, "%s Not a truth value option: %s", debug_tag(), raw);
	if (o->structype != OS_VOICE) shriek(811, "%s Not a voice option: %s", debug_tag(), raw);
	flag_offs = o->offset;
}

void
r_if::apply(unit *root)		// this_lang must correspond to these rules!
{
	if (this_voice && (*(bool *)((char *)this_voice + flag_offs)) ^ (*raw == EXCLAM))
		then->cook(root);
}

void
r_if::set_level(UNIT scp, UNIT trg)
{
	if (scp == U_DEFAULT) scp = U_INHERIT;
	rule::set_level(scp, trg);
}	


/************************************************
 r_nothing  The following rule class can serve
 	    as a void placeholder, where a rule
 **	    is syntactically required
 ************************************************/


class r_nothing: public rule
{
   protected:
	virtual OPCODE code() {return OP_NOTHING;};
   public:
	r_nothing() : rule(NULL) {};
	virtual void apply(unit *) {};
};


/************************************************
 r_fail  The following rule is used at points
            which shouldn't be reached to report
            an error of rule processing
 ************************************************/


class r_fail: public rule
{
   protected:
	virtual OPCODE code() { return OP_FAIL; };
   public:
	r_fail(char *param) : rule(param) {};
	virtual void apply(unit *);
};

void
r_fail::apply(unit *root)
{
	shriek(463, "Cfg error %s reported from file %s", raw, debug_tag());
}



/************************************************
 r_neural  Neuralnet created by Jakub Adamek
 	    look at neural.cc, neural.h, unit.cc
 **	 
 ************************************************/

#include "nnet/neural.h"

class r_neural: public rule
{
protected:
	virtual OPCODE code() {return OP_NNET;};
	CNeuralNet *neuralnet;
public:
	r_neural (char *param, hash *vars);
	virtual ~r_neural ();
	virtual void apply(unit *root);
};

void
r_neural::apply (unit *root)
{
	if (!neuralnet)
		shriek(861, "The --neuronet option is false");
	neuralnet->init();
	root->neural (target, neuralnet);
}

r_neural::r_neural(char *param, hash *vars) : rule(param)
{
	neuralnet = NULL;
	if (scfg->neuronet)
		neuralnet = new CNeuralNet(raw, vars);
}

r_neural::~r_neural ()
{
	if (neuralnet) delete neuralnet;
}

#include "block.cc"

