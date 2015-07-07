/*
 *	epos/src/function.cc
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
 *	This source file implements small sets and unary functions
 *	(operators) using perfect hashing techniques.
 */

#include "epos.h"

#define EXCL '!'
#define MAX_SIZE 256	// please change to scfg after Epos goes unicode
#define NOTHING 0

template<class T> void unexcl(T *s)
{
	T * last = strrchr(s, EXCL);
	if (!last) return;
	if (last != strchr(s, EXCL)) shriek(462, "Too many excls");
	*last = 0;
	int i, j;
	for (last++; *last; last++) {
		for (i = 0, j = 0; s[j]; j++) {
			s[i] = s[j];
			if (s[i] != *last) i++;
		}
		s[i] = 0;
	}
}

#if 0

template<class T> bool
booltab<T>::ismember(const T x)
{
	return (t[(unsigned int)x * mult % size] == x) ^ neg;
}

#endif

template<class T>
booltab<T>::booltab(T *s)
{
	always = !strcmp(s, "!");

	neg = false;
	while (s[0] == EXCL) neg = !neg, s++;
	unexcl<T>(s);
	
	size = strlen(s);
	int all = size << 1;
	if (!size) size = 1, all = 1;
	t = (T *)xmalloc(all * sizeof(T));
	
	for (; size < MAX_SIZE; size++) {
		if (size > all) {
			free(t);
			all <<= 1;
			t = (T *)xmalloc(all * sizeof(T));
		}
		mult = 0;
next:		if (++mult > scfg->hash_search)
			continue;
		for (int i = 0; i < size; i++) t[i] = NOTHING;
		for (int j = 0; s[j]; j++) {
			T *p = t + (unsigned int)s[j] * mult % size;
			if (*p != NOTHING && *p != s[j]) goto next;
			*p = s[j];
		}
		D_PRINT(1, "%set of %d characters sized %d items, mult is %d\n",
				neg ? "Antis" : "S", strlen(s), size, mult);
		t = (T *)xrealloc(t, size * sizeof(T));
		return;
	}
	shriek(461, "booltab maxsize reached");
}

template<class T>
booltab<T>::~booltab()
{
	free(t);
}

template class booltab<wchar>;



/*
 *	Functions can either be compressed or uncompressed
 *	at creation time.  Compressed functions do not store
 *	any identity mappings even if requested to, whereas
 *	uncompressed do.  Both types behave identically with
 *	respect to the xlat method.  In addition, uncompressed
 *	functions also support the ismember() method correctly
 *	at the expense of a small amount of extra memory.
 */

template<class T> T
function<T>::xlat(const T x)
{
	return t[(unsigned int)x * mult % size].x == x ?
			t[(unsigned int)x * mult % size].y : x;
}

template<class T> bool
function<T>::ismember(const T x)
{
	return t[(unsigned int)x * mult % size].x == x;
}

template<class T>
function<T>::function(const T *s, const T *r, bool compress)
{
	int k = strlen(r) == 1 ? 0 : 1; 
	if (k && (strlen(s) != strlen(r))) shriek(861, "Oh!");

	size = strlen(s);
	int allocated = size << 1;
	if (!size) size = 1, allocated = 1;
	t = (couple<T> *)xmalloc(allocated * sizeof(couple<T>));
	
	for (; size < MAX_SIZE; size++) {
		if (size > allocated) {
			free(t);
			allocated <<= 1;
			t = (couple<T> *)xmalloc(allocated * sizeof(couple<T>));
		}
		mult = 0;
next:		if (++mult > scfg->hash_search)
			continue;
		for (int i = 0; i < size; i++) t[i].x = NOTHING;
		for (int j = 0; s[j]; j++) {
			if (s[j] == r[j * k] && compress) continue;
			couple<T> *p = t + (unsigned int)s[j] * mult % size;
			if (p->x != NOTHING && p->x != s[j]) goto next;
			p->x = s[j]; 
			p->y = r[j * k];
		}
		D_PRINT(1, "Function for %d characters sized %d items, mult is %d\n",
				strlen(s), size, mult);
		t = (couple<T> *)xrealloc(t, size * sizeof(couple<T>));
		return;
	}
	shriek(461, "booltab maxsize reached");
}

template<class T>
function<T>::~function()
{
	free(t);
}

template class function<wchar>;


