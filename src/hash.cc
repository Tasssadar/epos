/*
 *	epos/src/hash.cc
 *	(c) geo@cuni.cz (Jirka Hanika)
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
 *
 */

#include "epos.h"
#include "hashtmpl.h"

int _hash_sp = 0;
hsearchtree <void, void> **_hash_stk[_HASH_DEPTH+1];

/****************************************************************************
 This constructor will construct a hash table out of a text file "filename". 
 Its potential capacity will be adjusted to the percentage_full specified.
 The parameters are explained in hash.h
 ****************************************************************************/

hash::hash(const char *filename, const char *dirname, const char *treename,
		const char *description,
		int perc_full, int perc_downsize, int perc_upsize,
		int max_tree_depth, bool allow_id, bool multi_data)
			: hash_table<char, char>(0)
{
	text *hashfile;
	char *buff;
	char *key;
	const char *value;
	int l = 0;
	char *tmp;
	free(ht);
	ht = NULL;

	dupkey = dupdata = true;
	buff = get_text_line_buffer();

	D_PRINT(0, "hash::hash: using file %s\n", filename);

	maxdep = longest = items = 0;
	if (max_tree_depth == -1) max_tree_depth=_HASH_DEPTH;
	perc_optimal = perc_full;
	
	capacity = 0;
	hashfile = new text(filename, dirname, treename, description, true);
	if (!description && !hashfile->exists()) {
		shriek(811, "%s:   Description required with hash tables", filename);
	}
	while (hashfile->get_line(buff)) l++;

	capacity = l*100/perc_full | 1;
	cfg_rehash(perc_downsize, perc_upsize, max_tree_depth);
	ht = (hsearchtree<char, char> **)xcalloc(capacity,sizeof(hsearchtree<char, char>*));
//	fseek(hashfile, 0, SEEK_SET);
	hashfile->rewind(false);
	l = 0;
	while (l++, hashfile->get_line(buff)) try {
		tmp = buff + strlen(buff);
		while (strchr(WHITESPACE, *--tmp) && tmp>=buff);
		tmp[1]=0;			//Strip out trailing whitespace

		tmp = key = buff + strspn(buff, WHITESPACE);
	        if (!*key) continue;   		//Nothing but a comment
	        tmp += strcspn(key, WHITESPACE);
		if (*tmp) *tmp++ = 0;		//terminate the key and go on
		value = tmp += strspn(tmp, WHITESPACE);
		if (!*value) switch (allow_id) {
			case true: value = key; break;
			case false: shriek(811, "%s:%d No value specified",
				hashfile->current_file, hashfile->current_line);
//			default: value = no_data;
		}
		else if (!multi_data && tmp[strcspn(tmp,WHITESPACE)]) 
			shriek(811, "%s:%d Multiple values specified", hashfile->current_file, hashfile->current_line);
		add(key, value);
	} catch (any_exception *e) {
		if (e->code / 10 != 81) throw e;
		delete e;
	}
//	fclose(hashfile);
	delete hashfile;
//   fail:
	free(buff);

	D_PRINT(0, "hash::hash: successfully returning, file %s\n", filename);
	if (_hash_sp) shriek(862, "Hash stack dirty! %d", _hash_sp);
}

void hash::add_int(const char *key, int value)
{
	char buff[MAX_DIGITS+1];
	char *i;
	buff[MAX_DIGITS]=0;
	i=buff+MAX_DIGITS-1;
	char negative=value<0;
	if (negative) value *=-1;
	do {
		*i=(char)((unsigned int)value%10 + '0');
		value = (unsigned int)value / 10;
		i--;
	} while (value);
	if (negative) *i='-'; else i++;
	D_PRINT(0, "hash::add_int %s to '%s'\n",key,i);
	add(key, (char *)i); 
}

int
hash::translate_int(const char *key)
{
	char *result;
	char *i;
	signed char sign=1;
	unsigned int to_return=0;
	if(!(result=(char *)translate(key))) return INT_NOT_FOUND;
	if (*result=='-') result++,sign=-1;
	if (*result=='+') result++;
	for(i=result;*i;i++) to_return=to_return*10+*i-'0';
	return (int)to_return*sign;
}

void
hash::debug()
{
	int i;
	D_PRINT(1, "hash::debug dump follows\n");
	for(i=0;i<capacity;i++) listtree(ht[i], 0);
	D_PRINT(1, "hash::debug done.\n");
}


//	forall(_hdump);


/****************************************************************************
 Various little nothings
 ****************************************************************************/

void
hash::listtree(hsearchtree <char, char> *tree, int indent)
{
	if (tree!=NULL) {
		for(int i=0; i<indent; i++) D_PRINT(1, "   ");
		D_PRINT(1, "%s %s %d\n",(char *)tree->key,(char *)tree->data, tree->height);
		if (tree->l && tree->r && tree->height!=tree->l->height+1 &&
		        tree->height!=tree->r->height+1 ||
			!tree->r && tree->l && tree->l->height+1!=tree->height ||
			!tree->l && tree->r && tree->r->height+1!=tree->height )
//			shriek(862, "Bad height: %d", tree->height);
		listtree(tree->r, indent+1);
		listtree(tree->l, indent+1);
//		if (tree->l && tree->r && abs(tree->l->height-tree->r->height)>1)
//			shriek(862, "Both children, but bad: %d", tree->height);
	}
}

slab <hsearchtree_size> *hash_tree_slab = 0;

void shutdown_hashing()
{
#ifdef HASH_IS_SLABBING
	if (hash_tree_slab) delete hash_tree_slab;
#endif
}
