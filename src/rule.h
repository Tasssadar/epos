/*
 *	epos/src/rules.h
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
 *	This file defines how a rules file should be parsed. 
 *
 *	Note that you may receive quite random behavior if
 *	you call rules::apply() for any other rule set than that
 *	of this_lang->ruleset. this_voice must be set to a voice
 *	of the same language as well. This is because r_if may
 *	refer to a soft option and the set of available soft options
 *	is language dependent.
 */

extern char * _resolve_vars_buff;
extern char * _next_rule_line;

extern char * global_current_file;
extern int    global_current_line;

class rule;
class r_block;

class rules
{
    public:
	int  current_rule;      //currently processed rule (in apply())
	r_block *body;		//contains pointers to the individual rules
	
	rules(const char *filename, const char *dirname);
	~rules();
	void apply(unit *root);
	void debug();       //dumps all rules
};

