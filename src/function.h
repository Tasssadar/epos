/*
 *	epos/src/function.h
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
 */

template<class T> class booltab
{
	bool neg;
	int mult, size;
	T *t;
   public:
	bool always;	// the typical case is optimized
	booltab(T *s);
	~booltab();
	bool ismember(const T x) { return (t[(unsigned int)x * mult % size] == x) ^ neg; };
};

/*
 *	The following speed optimization must remain a macro to prevent
 *	y from expansion when x->always.  Never use fast_ismember() where
 *	you don't expect frequent alwayses!
 */

#define fast_ismember(x, y) ((x)->always || (x)->ismember(y))

template<class T> struct couple
{
	T x;
	T y;
};

template<class T> class function
{
	int mult, size;
	couple<T> *t;
   public:
	function(const T *s, const T *r, bool compress);
	~function();
	bool ismember(const T x);
	T xlat(const T x);
};

typedef booltab<char> charclass;
typedef function<char> charxlat;
