/*
 *	epos/src/hash.h
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
 *      The hash tables are no longer independent from Epos, esp.
 *	because they now use the standard text file preprocessor.
 */

#ifndef __hash_tables
#define __hash_tables

//#define POWER_OF_TWO		// will avoid division in hash::fn()
				// by forcing the table size to be 2^n.
				// Not recommended for most processors.

#define MAX_DIGITS 10            //how many decimal digits the biggest int stored may have
                                 //  no performance penalty if much bigger
#define KEY_NOT_FOUND NULL
#define INT_NOT_FOUND      -32768   //guaranted to be negative, but that's all

#define _HASH_DEPTH	       64   //allows up to cca ten trillion elements

#define key_t hash_key_t
#define data_t hash_data_t

template <class key_t, class data_t>
struct hsearchtree;

extern hsearchtree<void, void> ** _hash_stk [_HASH_DEPTH+1];
extern int _hash_sp;


class hash;

template <class key_t, class data_t>
class hash_table
{
	
	inline int  fn(const key_t *key);
	int min_items;
	int max_items;
	int tree_too_deep;
#ifdef POWER_OF_TWO
	int hash_fn_mask;
#endif
   protected:
	int  maxdep;
	hsearchtree<key_t, data_t> **ht;
	int  capacity;
   public:
	int items;	// items inside
	int perc_optimal;
	int perc_too_sparse;
	int perc_too_dense;
	int dupkey;
	int dupdata;
	int longest;                     //the longest "key" ever inside (meaningless if key_t is not char)
	     hash_table(int size);
	     hash_table(hash_table<key_t, data_t> *);	// moderately slow copy constructor
	    ~hash_table();
	void 	add(const key_t *key, const data_t *value); //Will strcpy the arguments
	data_t*	remove(const key_t *key);       //Its "data" will be returned (just free() it)
	void 	forall(void usefn(key_t *key, data_t *value, void *parm), void *parm);
	inline void forall(void usefn(key_t *key, data_t *value, int parm), int parm) {
		forall((void(*)(key_t*, data_t*, void *))usefn, (void *)parm);
	}
	data_t*	translate(const key_t *key);    //Will NOT strdup the result
	key_t*	get_random();
	void 	rehash(int new_capacity);     //Adjust the capacity. Boring.
	void 	rehash();		     //Adjust the capacity as needed
	void 	cfg_rehash(int min_perc, int max_perc, int max_tree);
					     //When will the rehash occur?
 private:
	void 	rehash_tree(hsearchtree<key_t, data_t> *t);    //internal, called by rehash()
	void 	fortree(hsearchtree<key_t, data_t> *tree,
				void usefn(key_t *key, data_t *value, void *parm), void *parm);
	void 	dissolvetree(hsearchtree<key_t, data_t> *tree);
};

//void _hdump(key_t *s1, data_t *s2);        //See hash::debug
//void _hfree(key_t *s1, data_t *s2);      //See the destructor

class hash: public hash_table<char, char>
{
   public:
	hash (int i): hash_table<char, char> (i) {};
	hash (hash *h): hash_table<char, char> (h) {};
	     hash(const char *filename, const char *dirname, const char *treename,
	     	const char *description,
		int perc_full,		// initial and optimal ratio items/capacity
		int perc_downsize,	// if under that percentage full, call rehash()
		int perc_upsize,	// if over that percentage full, rehash()
		int max_tree_depth,	// if an AVL tree reaches this height, rehash()
		bool allow_id,		// true, if we accept single word input
					// 	as both key and data
					// false, if we reject single word input
					// anything else - default data for this case
		bool multi_data);	// true, if the data may consist of multiple words
	void 	add_int(const char *key, 
			int value);     //The integer is stored human-readable (decimal) :-)
	int 	translate_int(const char *key);//returns -1 if not found, garbage if not int

	void 	debug();                     //currently "dump thyself"
	void	listtree(hsearchtree<char, char> *tree, int indent);//See hash::debug
};

#undef key_t		// a stupid name collision with linux kernel 2.1.?? include/types.h
#undef data_t		//   requires this creeping attitude

#ifdef WANT_DEFAULT_HASH_FN

inline unsigned int
contiguous_hash_fn(void *ptr, int size)		// a hash function for contiguous keys
{
	const char *p;
	unsigned int j = 0;

	if (size % sizeof(int)) for (p=(char *)ptr;p<(char *)ptr + size;p++) j=173*j+*p;
	else for (p=(char *)ptr; p<(char *)ptr + size; p += sizeof(int))
		j = 233*j + *(unsigned int *)p;
	return j;
}

#endif

void shutdown_hashing();

#endif                     //__hash_tables
