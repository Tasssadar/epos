// Author: Jakub

#ifndef _BANG_ITERATOR_H_
#define _BANG_ITERATOR_H_

#ifdef _MSC_VER
	// return type for 'viterator<int>::operator ->' is 'int *' 
	//(ie; not a UDT or reference to a UDT.  Will produce errors if applied using infix notation)
	#pragma warning(disable:4284) 
#endif

/** @brief vector iterator - assumes all data are in a vector, in a continuous part of memory */

template<class T> class const_viterator {
private:
	T *d;
public:
	const_viterator ()					{ d = 0; }
	const_viterator (const const_viterator&src)		{ d = src.d; }
	const_viterator &operator =(const const_viterator&src)	{ d = src.d; return *this; }
	const_viterator (T *src)				{ d = src; }
	const T &operator * () const			{ return *d; }
	const T *operator -> ()	const			{ return d; }
	operator const T *() const				{ return d; }
	const_viterator &operator ++ () 			{ ++d; return *this; }
	const_viterator &operator -- ()			{ --d; return *this; }
	const_viterator &operator += (int i)			{ d+=i; return *this; }
	const_viterator &operator -= (int i)			{ d-=i; return *this; }
	const_viterator operator + (int i) const		{ return d+i; }
	const_viterator operator - (int i) const		{ return d-i; }
	bool operator == (const const_viterator&y) const	{ return d == y.d; }
	bool operator != (const const_viterator&y) const	{ return d != y.d; }
	bool valid () const				{ return d != 0; }
};

template<class T> class viterator {
private:
	T *d;
public:
	viterator ()					{ d = 0; }
	viterator (const viterator&src)			{ d = src.d; }
	viterator &operator =(const viterator&src)	{ d = src.d; return *this; }
	viterator (T *src)				{ d = src; }
	operator const_viterator<T>()	{ return const_viterator<T> (d); }
	operator T *() const			{ return d; }
	T &operator * ()				{ return *d; }
	T *operator -> ()				{ return d; }
	const T &operator * () const			{ return *d; }
	const T *operator -> ()	const			{ return d; }
	viterator &operator ++ () 			{ ++d; return *this; }
	viterator &operator -- ()			{ --d; return *this; }
	viterator &operator += (int i)			{ d+=i; return *this; }
	viterator &operator -= (int i)			{ d-=i; return *this; }
	viterator operator + (int i) const		{ return d+i; }
	viterator operator - (int i) const		{ return d-i; }
	bool operator == (const viterator&y) const	{ return d == y.d; }
	bool operator != (const viterator&y) const	{ return d != y.d; }
	bool valid () const				{ return d != 0; }
};
 
#endif

