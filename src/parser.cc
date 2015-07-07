/*
 *	epos/src/parser.cc
 *	(c) 1996-98 geo@cuni.cz
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

SLABIFY(parser, parser_slab, 64, shutdown_parser);

unsigned char parser::transl_input[PARSER_MODES][CHARSET_SIZE];

/*
 *	If PARSER_MODE_FILE, string is a filename.
 *	Otherwise the string is the content, NOT a filename.
 */

parser::parser(const char *string, int requested_mode)
{
	bool is_file = false;
	mode = requested_mode;

	if (requested_mode == PARSER_MODE_FILE) {
		is_file = true;
		mode = PARSER_MODE_INPUT;
	}

	char_level = this_lang->char_level + mode * CHARSET_SIZE;
	downgradables = this_lang->downgradables;
	depth = scfg->_text_level;
	if (!downgradables) downgradables = "";

	if (is_file) {
		if (!string || !*string) {
			register signed char c;
			int i = 0;
			text = (unsigned char *)xmalloc(cfg->dev_text_len+1);
			if(!text) shriek(422, "Parser: Out of memory");
			do text[i++] = c = getchar(); 
				while(c != -1 && c != scfg->end_of_file && i < cfg->dev_text_len);
			text[--i] = 0;
			txtlen = i;
		} else { 
			file *f = claim(string, this_lang->input_dir, scfg->lang_base_dir, "rt", "input text", NULL);
			text = (unsigned char *)strdup(f->data);
			unclaim(f);
			txtlen = strlen((char *)text);
		}
	} else {
		text = (unsigned char *)strdup(string);
		txtlen = strlen((char *)text);
	}
	D_PRINT(2, "Allocated %u bytes for the main parser\n", txtlen);
	init();
}

void
parser::init()
{
	unsigned int i;

	for(i = 0; i < txtlen; i++)
		text[i] = transl_input[mode][text[i]];
	D_PRINT(1, "Parser: has set up with %s\n", text);
	current = text; 
//	current--;
//	do level = chrlev(*++current); while (level > scfg->_phone_level && level < scfg->_text_level);
		//We had to skip any garbage before the first phone

	token = '0'; level = scfg->_text_level; gettoken();
	t = 1;

	D_PRINT(0, "Parser: initial level is %u.\n", level);
}

parser::~parser()
{
	free(text);
}

#define IS_DIGIT(x)	((x)>='0' && (x)<='9')
#define IS_ASCII_LOWER_ALPHA(x) ((x)>='a' && (x)<='z')

inline unsigned char
parser::identify_token()
{
	switch (token = *current) {
		case '.':
			if (IS_DIGIT(current[1]))
				return DECPOINT;
			if (IS_ASCII_LOWER_ALPHA(current[1]))
				return URLDOT;
			if (current[1] == '.' && current[2] == '.') {
				current += 2;
				return DOTS;
			}
			break;
		case '-':
			if (IS_DIGIT(current[1]))
				return current > text && IS_DIGIT(current[-1]) ? RANGE : MINUS;
			while (current[1] == '-') current++;
			break;
//		case '<': if (cfg->stml) shriek(462, "STML not implemented"); else break;
//		case '&': if (cfg->stml) shriek(462, "STML not implemented"); else break;
		default : ;
	}
	return *current;
}

inline bool
parser::is_garbage(UNIT level, UNIT last_level, UNIT toomuch)
{
	return (level >= toomuch || level <= last_level && level > scfg->_phone_level) && level < scfg->_text_level;
}

unsigned char
parser::gettoken()
{
	if (!token) return NO_CONT;

	UNIT lastlev = level;
	unsigned char ret = token;
	do {
		token = identify_token();
		if (char_level[token] == U_ILL && cfg->relax_input)
			token = cfg->default_char;
		level = chrlev(token);
		if (is_garbage(level, lastlev, depth) && strchr(downgradables, token)) {
			level = scfg->_phone_level; 
			D_PRINT(0, "Parser downgrading %c\n", token);
		} else D_PRINT(0, "Parser not downgrading %c\n", token);
		t = 1;
		current++;
	} while (is_garbage(level, lastlev, depth));
		// (We are skipping any empty units, except for phones.)
	D_PRINT(0, "Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n", ret, lastlev, *current, level);

	return ret;
}

void
parser::done()
{
	if (current < text + txtlen) 
		shriek(463, "Too high level symbol in a dictionary, parser contains %s", (char *)text);
}

UNIT
parser::chrlev(unsigned char c)
{
	if (current > text + txtlen + 1)
		return U_VOID;
	if (char_level[c] == U_ILL)
	{
		if (cfg->relax_input && char_level[cfg->default_char] != U_ILL)
			return char_level[cfg->default_char];
		DBG(3, fprintf(cfg->stdshriek, "Parser unhappily dumps core.\n%s\n", (char *)current - 2);)
		shriek(431, "Parsing an unhandled character  '%c' - ASCII code %d", (unsigned int) c, (unsigned int) c);
	}
	return char_level[c];
}

void
parser::regist(int mode, unsigned char *cl, UNIT u, const char *list)
{
	unsigned char *s;
	if (!list) return;
	for(s = (unsigned char *)list; *s != 0; s++)
	{
		if (cl[*s] != U_ILL && cl[*s] != u)
			shriek(812, "Ambiguous syntactic function of %c", *s);
		cl[*s] = u;
		unsigned char r = transl_input[mode][*s];
		if (r != (unsigned char)*s)
			if (r == transl_input[mode][r])
				shriek(812, "No direct access to %c, use the equivalent %c", *s, r);
			else shriek(862, "Character translation too complex for %c", *s);
	}
}

void
parser::alias(int mode, const char *canonicus, const char *alius)
{
	int i;
	if (!canonicus || !alius) shriek(861, "Parser configuration: Aliasing NULL");
	for (i = 0; canonicus[i] && alius[i]; i++)
		transl_input[mode][((unsigned char *)alius)[i]] = ((unsigned char *)canonicus)[i];
	if (canonicus[i] || alius[i]) 
		shriek(861, "Parser configuration: Can't match aliases");
}


void
parser::init_tables(lang *l)
{
	int c;
	UNIT u = scfg->_phone_level;

	l->char_level = (unsigned char *)xmalloc(PARSER_MODES * 256);

	for (int t = 0; t < PARSER_MODES; t++) {
		for (c = 0; c < CHARSET_SIZE; c++) {
			transl_input[t][c] = (unsigned char)c;
			l->char_level[t * CHARSET_SIZE + c] = U_ILL;
		}
		l->char_level[t * CHARSET_SIZE] = scfg->_text_level;
		for (u = scfg->_phone_level; u < scfg->_text_level; u = (UNIT)(u+1))
			regist(t, l->char_level + t * CHARSET_SIZE, u, l->perm[u]);
			regist(t, l->char_level + t * CHARSET_SIZE, u,
				t == PARSER_MODE_INPUT  ? l->perm_input[u]
							: l->perm_working[u]);
	}

	alias(PARSER_MODE_INPUT, " \n", "\t\r");
}
