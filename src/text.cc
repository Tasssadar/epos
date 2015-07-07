/*
 *	epos/src/text.cc
 *	(c) 1997-00 geo@cuni.cz
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

//enum DIRECTIVE {DI_INCL, DI_WARN, DI_ERROR};

#define PP_ESCAPE	'@'
#define D_INCLUDE	"include"
#define D_CHARSET	"charset"
#define D_WARN		"warn"
#define D_ERROR		"error"

#define SPECIAL_CHARS "#;\n\\"

inline bool text::strip(char *s)
{
	char *r;
	char *t;
	char c;

	if (strip_whitespace) {
		r = s;
		t = s + strspn(s, WHITESPACE);
		if (r != t) do {
			*r++ = c = *t++;
		} while (c); else r = s + strlen(s) + 1;
		for (r -= 2; r >= s && strchr(WHITESPACE, *r); r--)
			*r = 0;
	}
	if (strip_specials) {
		for (r = t = s + strcspn(s, SPECIAL_CHARS); *t; t++, r++) switch (*r = *t) {
			case ';':
			case '#':
				if (s != r && !strchr(WHITESPACE, r[-1])) break;	// else fall through
			case '\n':
//				while (r > s && strchr(WHITESPACE, *--r));
				*r = 0;
				return false;
			case ESCAPE:
				if (!*++t || *t == '\n') {
					*r = 0;
					return true;
				}
				if (esctab->ismember(*t)) *r = esctab->xlat(*t);
//				if (strchr(scfg->token_esc, *t)) *r = esctab->xlat(*t);
				else t--;
				break;
			default:;
		}
		*r = 0;
	}
	return false;
}

struct textlink {
	FILE *f;
	textlink *prev;
	char *filename;
	int line;
};

text::text(const char *filename, const char *dirname, const char *treename,
				const char *description, bool warnings)
{
//	if (!_directive_prefices) 
//		_directive_prefices = str2hash(DIRECTIVEstr, MAX_DIRECTIVE_LEN);
	dir = dirname;
	tree = treename;
	tag = description;
	base = strdup(filename);
	warn = warnings;
	strip_specials = true;
	strip_whitespace = true;
	basefile(base, "opening");
}

void
text::basefile(const char *base, const char *action)
{
	embed = 0;
	current = NULL;
	current_file = NULL;
	current_line = 0;
	charset = cfg->charset;

	subfile(base, action);
}

void
text::subfile(const char *filename, const char *action)
{
	textlink *parent;

	parent = current;
	current = new textlink;
	if (!current) shriek(422, "text::subfile out of memory");
	if (!filename || !*filename) {
		current->f = stdin;
	} else { 
		char *pathname = compose_pathname(filename, dir, tree);
		D_PRINT(2, "Text preprocessor %s %s\n", action, pathname);
		current->f = fopen(pathname, "r", tag);
		free(pathname);
	}
	current->prev = parent;
	current->filename = current_file;
	current->line = current_line;
	current_file = strdup(filename);
	current_line = 0;
	if (++embed > scfg->max_nest) shriek(882, "infinite @include cycle");
}

void
text::superfile()
{
	free(current_file);
	if (current->filename) {
		current_file = current->filename;
		current_line = current->line;
		D_PRINT(2, "Text preprocessor returns to %s\n", current_file);
	} else {
		current_file = NULL;
		D_PRINT(1, "Text preprocessor reached the end of %s\n" , current_file);
	}
	fclose(current->f);
	textlink *tmp = current;
	current = tmp->prev;
	embed--;
	delete tmp;
}

bool
text::exists()
{
	return current && current->f;
}

inline bool begins(const char *buffer, const char *s)
{
	return !strncasecmp(buffer + strspn(buffer, WHITESPACE) + 1, s, strlen(s));
}

inline char *get_quoted(char *buffer)
{
	char *tmp1;
	char *tmp2;

	tmp1=strchr(buffer+1, DQUOT);
	if (!tmp1) shriek (812, "%s:%d Forgotten quotes", global_current_file, global_current_line);
	tmp2=strchr(++tmp1,DQUOT);
	if (!tmp2) shriek (812, "%s:%d Forgotten quotes", global_current_file, global_current_line);
	*tmp2=0;
	return tmp1;
}

void
text::handle_directive(char *buffer)
{
	if (begins(buffer, D_INCLUDE)) {
		subfile(get_quoted(buffer), "includes");
	} else if (begins(buffer, D_CHARSET)) {
		charset = load_charset(get_quoted(buffer));
		cfg->charset = charset;
		if (charset == CHARSET_NOT_AVAILABLE)
			shriek(812, "%s:%d Charset not available", current_file, current_line);
	} else if (begins(buffer, D_WARN)) {
		if (warn) fprintf(cfg->stdshriek,
			"%s\n",buffer+1+strcspn(buffer+1, WHITESPACE));
	} else if (begins(buffer, D_ERROR)) {
		shriek(801, get_quoted(buffer));
	} else shriek(812, "%s:%d Bad directive", current_file, current_line);
}
	
bool
text::get_line(char *buffer)
{
	int l = 0;

	if (!current) return false;	// EOF, again
	
	while (true) {
		while(!fgets(buffer + l, scfg->max_line_len - l, current->f)) {
			if (l) shriek(462, "Backslash at the end of %s", current_file);
			else superfile();
			if (!current) return false;
		}
		current_line++;
		global_current_line = current_line;
		global_current_file = current_file;
		D_PRINT(0, "text::get_line processing %s",buffer);
		if ((int)strlen(buffer) + 1 >= scfg->max_line_len)
			shriek(462, "Line too long in %s:%d", current_file, current_line);
		if (buffer[strspn(buffer, WHITESPACE)] == PP_ESCAPE) {
			handle_directive(buffer);
			continue;
		}		
		if (strip(buffer + l)) {
			l = strlen(buffer);
			continue;	/* continuation line */
		}
		if (!buffer[strspn(buffer,WHITESPACE)]) continue;
		
		encode_string(buffer, charset, true);
		return true;
	}
}

void
text::rewind()
{
	if (cfg->paranoid) done();
	basefile(base, "rewinds");
}

void
text::rewind(bool warnings)
{
	text::rewind();
	warn=warnings;
}

text::~text()
{
	done();
	free(base);
//	doneprefices();
};

void
text::done()
{
	if (current && exists()) shriek(812, "File %s was left prematurely at line %d", base, current_line);
}
