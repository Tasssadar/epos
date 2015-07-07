/*
 *	epos/src/block.cc
 *	(c) 1996-00 geo@cuni.cz
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
 *	This file is never compiled on its own, it gets included
 *	at the end of rule.cc
 *
 */
 
#define END_OF_BLOCK	0
#define END_OF_CHOICE	1
#define END_OF_SWITCH	2	   // by unit length
#define END_OF_RULES	3
#define LAST_SPECIAL_RULE	END_OF_RULES
#define ORDINARY_RULE(x) (((unsigned long)(x)) > LAST_SPECIAL_RULE)
#define MAX_WORDS_PER_LINE 64

#define DOLLAR             '$'             //These symbols are used to represent 
#define EQUALSIGN          '='             // variable assignments and evaluation
#define CURLY              '{'
#define UNCURLY            '}'
#define VAR_TERM " \t$()_><%*?:\n\\&/!" //variable identifier terminators, incl. <Tab>
#define VAR_TERM_ONLY    "&:}"             // these are skipped if used to terminate v.i.
                                           //UNCURLY should be included in VAR_TERM_ONLY,
                                           // but not in VAR_TERM

char *global_current_file = NULL;
int   global_current_line = 0;

extern unit EMPTY;

#define diatax(x) shriek(811, "%s:%d %s", file->current_file, file->current_line, (x))


class block_rule : public rule
{
   protected:
	rule ** rulist;
	int current_rule, n_rules;

	void apply_current(unit *root);
	void load_rules(int terminator, text *file, hash *inherited_vars);
   public:
	block_rule();
	~block_rule();

	virtual void set_level(UNIT scp, UNIT trg);
	virtual void check_children();
	void debug(void);
};


class r_block : public block_rule
{
   protected:
	virtual OPCODE code() {return OP_BEGIN;};
   public:
	r_block(text *file, hash *inherited_vars);
	~r_block();
	void apply(unit *root);
};

class r_choice : public block_rule
{
   protected:
	virtual OPCODE code() {return OP_CHOICE;};
   public:
	r_choice(text *file, hash *inherited_vars);
	~r_choice();
	void apply(unit *root);
};

class r_switch : public block_rule
{
   protected:
	virtual OPCODE code() {return OP_SWITCH;};
   public:
	r_switch(text *file, hash *inherited_vars);
	~r_switch();
	void apply(unit *root);
};

block_rule::block_rule() : rule(NULL)
{
	rulist = NULL;
}


block_rule::~block_rule()
{
	for(current_rule = 1; current_rule < n_rules; current_rule++) {
		if (rulist[current_rule - 1] != rulist[current_rule])
			delete rulist[current_rule];
	}
	if (n_rules) delete rulist[0];
	if (rulist) free(rulist);
}

void
block_rule::load_rules(int terminator, text *file, hash *inherited_vars)
{
	int l, again;
	hash *vars = new hash(inherited_vars);
	char *began_in = strdup(file->current_file);
	int   began_at = file->current_line;

	l = scfg->rules_in_block;
	rulist = (rule **)xmalloc(sizeof(rule *) * l);
	n_rules = 0; again = 1;
	while(ORDINARY_RULE(rulist[n_rules] = --again ? rulist[n_rules-1]
					 : next_rule(file, vars, &again))) {
//		rulist[n_rules]->set_dbg_tag(file);
		if (++n_rules == l) {
			l+=scfg->rules_in_block;
			rulist = (rule **)xrealloc(rulist, sizeof(rule *) * l);
		}
	}
	if (again > 1) diatax("Badly placed count");
	long last_as_special = (long int)rulist[n_rules];
	if (last_as_special != terminator) switch (last_as_special) {
		case END_OF_BLOCK:  diatax("No block to terminate");
		case END_OF_CHOICE: diatax("No choice to terminate");
		case END_OF_SWITCH: diatax("No length-based switch to terminate");
		case END_OF_RULES: 
			if (!began_at) break;
			else shriek(811, "Unterminated block in file %s line %d", began_in, began_at);
		default: shriek(861, "next_rule() gone mad");
	}
	free(began_in);
	if (n_rules) rulist = (rule **)xrealloc(rulist, sizeof(rule *) * n_rules);
	else free(rulist), rulist = NULL;
	delete vars;
}

void
block_rule::set_level(UNIT scp, UNIT trg)
{
	if (scp == U_DEFAULT) scp = U_INHERIT;
	rule::set_level(scp, trg);
}	

void
block_rule::check_children()
{
	rule *r;
	
	for (current_rule=0; current_rule < n_rules; current_rule++) {
		r = rulist[current_rule];
		check_child(r);
		r->check_children();
	}
}

void
block_rule::apply_current(unit *root)
{
	rule *r;

	DBG(0, root->fout(NULL);)
	D_PRINT(0, "block_rule::apply current_rule %d\n", current_rule);
	r = rulist[current_rule];

	r->cook(root);
	
	if (scfg->pausing) {
		debug();
		root->fout(NULL);
		user_pause();
	}
}

void
block_rule::debug()
{
	int i;
	for(i=0;i<n_rules;i++) {
		if (i==current_rule) color(cfg->stddbg, scfg->curr_rule_color);
		if (i==current_rule || scfg->verbose) rulist[i]->debug();
		if (i==current_rule) color(cfg->stddbg, scfg->normal_color);
	}
}



r_block::r_block(text *file, hash *inherited_vars)
{
	load_rules(END_OF_BLOCK, file, inherited_vars);
}

r_block::~r_block()
{
}

void
r_block::apply(unit *root)
{
	for(current_rule=0; current_rule < n_rules; current_rule++) {
		apply_current(root);
	}
}



r_choice::r_choice(text *file, hash *inherited_vars)
{
	load_rules(END_OF_CHOICE, file, inherited_vars);
	if (!n_rules) diatax("No rules to choose from");
}

r_choice::~r_choice()
{
}

void
r_choice::apply(unit *root)
{
	current_rule = rand() % n_rules;	//random, less than n_rules
	D_PRINT(2, "Randomly chose rule [%s] %s %s\n",
			rulist[current_rule]->dbg_tag,
			enum2str(rulist[current_rule]->code(), OPCODEstr),
			rulist[current_rule]->raw);
	apply_current(root);
}



r_switch::r_switch(text *file, hash *inherited_vars)
{
	load_rules(END_OF_SWITCH, file, inherited_vars);
	if (!n_rules) diatax("Empty length dependencies");
}

r_switch::~r_switch()
{
}

void
r_switch::apply(unit *root)
{
	current_rule = root->count(target) - 1;
	if (current_rule == -1) shriek(461, "An empty unit strikes a switch");
	if (current_rule >= n_rules) current_rule = n_rules - 1;
	D_PRINT(1, "Length-determined rule [%s] %s %s\n",
			rulist[current_rule]->dbg_tag,
			enum2str(rulist[current_rule]->code(), OPCODEstr),
			rulist[current_rule]->raw);
	apply_current(root);
}



/************************************************************
 This constructor reads and precompiles the rules 
 from a text file. 
 ************************************************************/

rules::rules(const char *filename, const char *dirname)
{
	text *file;
	hash *vars;

	DBG(0, if (scfg->_loaded) fprintf(STDDBG,"Configuration already loaded when constructing rules\n");)

	if (!scfg->_loaded) /* epos_init() */ shriek(862, "compiling rules too soon");
	
	file = new text(filename, dirname, scfg->lang_base_dir, "rules", false);
	file->charset = this_lang->charset;
	vars = new hash(scfg->variables|1);
	D_PRINT(2, "Rules shall be taken from %s\n", filename);
	
	body = new r_block(file, vars);
	body->scope = scfg->_text_level;
	body->target = scfg->_phone_level;
#ifdef DEBUGGING
	body->dbg_tag = strdup("root block");
#endif
	body->check_children();
	
//	D_PRINT(2, "There're %d parsed rules in %s\n",n_rules,filename);
	D_PRINT(0, "rules will now release allocated memory\n");
	delete vars;
	delete file;
	D_PRINT(2, "Rules from %s parsed OK\n", filename);
}

rules::~rules()
{
	D_PRINT(2, "Deleting rules\n");
	delete body;
}

/****************************************************************
 get_words	Split a line into words. Returns how many words.
 		Whitespace gets replaced by hard zeroes,
 		char**words gets 0 to max pointers to the words
 ****************************************************************/

char *_next_rule_line;

int
get_words(char *line, char**words, int max)
{
	int n=0;
	while (*line) {
		while (*line && strchr(WHITESPACE, *line)) *line++=0;
		if (!*line || n==max) break;
		words[n++] = line;
		if (*line == DQUOT)
			line = strchr(line+1, DQUOT) + 1;
		else if (line[0] == EXCLAM && line[1] == DQUOT)
			line = strchr(line+2, DQUOT) + 1;
		else line += strcspn(line, WHITESPACE);
	}
	return n;
}

inline int
rule_weight(const char *word, text *file)
{
	int i=0;
	int result=0;
	
	if (!word) shriek(461, "Weight bug strikes");
	
	while (word[i] >= '0' && word[i] <= '9') {
		result = result * 10 + word[i++]-'0';
	}
	if ((word[i]|('a'-'A'))=='x' && !word[i+1] && i) {
		if (!result) diatax("Zero weight");
		if (result > scfg->max_rule_weight) diatax("Weight too large");
		return result;	// shriek if 0 ?
	}
	return 0;
}

/**************************************************************
 resolve_vars  Interpret the $variables on an input line
 **************************************************************/
 
char *_resolve_vars_buff = NULL;

void
resolve_vars(char *line, hash *vars, text *file)
{
	char *src; char *dest;
	char *eov; char *value;                     //eov...end of variable identifier ptr
	char takeout;
    
	for(src=line+1,dest=_resolve_vars_buff;*src;) {
		D_PRINT(0, "Cycling in rules::resolve_vars. Reading %s\n",src);
		if(*src==DOLLAR && --dest && src[-1]!=ESCAPE && dest++) {
			if (*++src==CURLY) eov=strchr(++src,UNCURLY); 
			else eov=src+strcspn(src,VAR_TERM);
			if (!eov) diatax("Missing right brace");
			takeout=*eov;
			*eov=0;
			D_PRINT(0, "Resolving \"%s\"\n",src);
			value = vars->translate(src);
			if (!value) diatax(fmt("Undefined identifier '%s'", src));
			strcpy(dest, value);
			while (*dest) dest++;
			if (strchr(VAR_TERM_ONLY,takeout)) eov++; else *eov=takeout;
			src = eov;
		} else *dest++ = *src++;
	}
	*dest=0;
	strncpy(line+1, _resolve_vars_buff,scfg->max_line_len);          //Copy it back
}


/****************************************************************
 next_rule	  converts a string to class rule
                  Returns: ptr to rule or a small integer
			(up to LAST_SPECIAL_RULE) typecast to rule*
                  
                  Caution! This function is not really reentrant,
                  as the word buffer is global. This implies we
                  need to be finished with this buffer before
                  we call individual rules constructors, some
                  of which call next_rule() again.
 ****************************************************************/

#define str _next_rule_line

rule *
parse_rule(text *file, hash *vars, int *count)
{
	int param = 0;
	static char **word=(char **)FOREVER(xmalloc(sizeof(char *) * MAX_WORDS_PER_LINE));
	int idx, weight;
	char *tmp;
	UNIT scope, target;
	OPCODE code;
	rule *result;

	next_line:
	
	if(!file->get_line(str)) return (rule *)END_OF_RULES;
	
	D_PRINT(0, "str2rule should parse: %s\n",str);
	if(!str[strspn(str,WHITESPACE)]) goto next_line;
	if (*str && strchr(str+1,DOLLAR))
		resolve_vars(str, vars, file);			   //Var reference found?

	if (*str==DOLLAR) {                                        //Variable assignment?
		tmp=strchr(str+2,EQUALSIGN);
		if (tmp) *tmp=' ';        //Yes. Wipe '=' if any.
		if (get_words(str+1, word, 4)!=2) diatax("Illegal group assignment");
		if (tmp) vars->add(word[0], word[1]);
		else if (strcmp(word[1], "external")) diatax("'=' probably forgotten");
			else vars->add(word[0], never_null(format_option(word[0])));
		goto next_line;
	}

	idx = get_words(str, word, MAX_WORDS_PER_LINE);
	
	weight = rule_weight(word[param], file);
	D_PRINT(0, "Rule weight: %d\n", weight);
	if (weight) {
		if (count) {
			*count = weight;
			param++;
			idx--;
		} else diatax("Badly placed count");
	} else if (count) *count = 1;
	
	code = (OPCODE)str2enum(word[param++], OPCODEstr, OP_ERROR);
	idx--;

	if (code >= OP_BEGIN) idx++, param--;	//faked param for rules that don't want one

	switch (idx) {
		case 0: diatax("Missing parameter");
		case 1: word[param+1] = NULL;	/* and fall through */
		case 2: word[param+2] = NULL; 	/* and fall through */
		case 3: break;

		default: if (code == U_ILL) diatax("Unknown rule type");
			 else   	    diatax("Extra stuff follows rule");
	}
		
	scope = str2enum(word[param+1], scfg->unit_levels, U_DEFAULT);	/* no later */
	target = str2enum(word[param+2], scfg->unit_levels, U_DEFAULT);

	if (word[param] && strchr(word[param], PSEUDOSPACE))
		for (char *p = word[param]; *p; p++)
			if (*p == PSEUDOSPACE) *p = ' ';

	switch(code) {

	case OP_SEG:     result = new r_seg(word[param]); break;
	case OP_ABSOL:	 result = new r_absol(word[param]); break;
	case OP_SUBST:   result = new r_subst(word[param]); break;
#ifdef WANT_REGEX
	case OP_REGEX:   result = new r_regex(word[param]); break;
#else
	case OP_REGEX:	 result = new r_debug("ignore"); break;
#endif
	case OP_PREP:    result = new r_prep(word[param]); break;
	case OP_POSTP:   result = new r_postp(word[param]); break;
	case OP_CONTOUR: result = new r_contour(word[param]); break;
	case OP_PROSODY: result = new r_prosody(word[param]); break;
	case OP_RAISE:   result = new r_raise(word[param]); break;
	case OP_SMOOTH:  result = new r_smooth(word[param]); break;
	case OP_REGRESS: result = new r_regress(word[param]); break;
	case OP_PROGRESS:result = new r_progress(word[param]); break;
	case OP_SYLL:    result = new r_syll(word[param]); break;
	case OP_ANALYZE: result = new r_analyze(word[param]); break;
	case OP_DEBUG:   result = new r_debug(word[param]); break;
	case OP_IF:	 result = new r_if(word[param], file, vars); break;
	case OP_INSIDE:	 result = new r_inside(word[param], file, vars); break;
	case OP_NEAR:	 result = new r_near(word[param], file, vars); break;
	case OP_WITH:	 result = new r_with(word[param], file, vars); break;
	case OP_FAIL:	 result = new r_fail(word[param]); break;
	case OP_BEGIN:	 result = new r_block(file, vars); break;
	case OP_END:	 return (rule *)END_OF_BLOCK;
	case OP_CHOICE:  result = new r_choice(file, vars); break;
	case OP_CHOICEND:return (rule *)END_OF_CHOICE;
	case OP_SWITCH:  result = new r_switch(file, vars); break;
	case OP_SWEND:	 return (rule *)END_OF_SWITCH;
	case OP_NOTHING: result = new r_nothing(); break;
	case OP_NNET:    result = new r_neural(word[param], vars); break;

	default:  diatax("Unknown rule type"); goto next_line;  // to fool the compiler

	}
    
	if (scope == U_ILL) diatax("Scope misspelled");
	if (target == U_ILL) diatax("Target misspelled");

	result->set_level(scope, target);
	result->set_dbg_tag(file);
	result->verify();
	
	D_PRINT(0, "str2rule about to return OK, %s\n", word[param]);

	return result;
}

rule *
next_rule(text *file, hash *vars, int *count)
{
	static int tmp = -1;
	if (tmp == -1) tmp = scfg->max_errors;
	for (; tmp > 0; tmp--) try {
		return parse_rule(file, vars, count);
	} catch (any_exception *e) {
		if (e->code / 10 != 81) throw e;
		delete e;
	}
	shriek(811, "Too many errors");
	return NULL;
}

rule *
next_real_rule(text *file, hash *vars, int *count)
{
	try {
		rule *r = parse_rule(file, vars, count);
		if (ORDINARY_RULE(r)) return r;
		if ((long int)r != END_OF_RULES) diatax("No rule follows a conditional rule");
		shriek(811, "No rule follows a conditional rule at the end of %s", file->current_file);
	} catch (any_exception *e) {
		if (e->code / 10 != 81) throw e;
		delete e;
		shriek(881, "Parse error, cannot continue");
	}
	return NULL;
}

#undef str
#undef diatax

/*******************************************************
 rules::apply   Applies the precompiled rules to a tree
 *******************************************************/

void
rules::apply(unit *root)
{
	if (this_lang->ruleset != this) shriek(862, "Cannot compile rules for other languages");
	body->apply(root);
}



void
rules::debug()
{
	body->debug();
}


