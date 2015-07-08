#ifndef __SET_H__
#define __SET_H__

#include "vector.h"

template<class T> class TSet: public TVector<T> {
public:
		TSet() : TVector<T>()		{};

		typedef viterator<T> iterator;			 
		virtual iterator insert (const T &x);
		virtual iterator find(const T& key) const;

private:
		// do not use!
		virtual iterator push_back (const T & x) { return TVector<T>::push_back (x); }	
};

#define SET(x,y) typedef TSet<x> y;

template<class T> typename TSet<T>::iterator TSet<T>::insert (const T &x)
{
	if (this->capacity == 0) this->Realloc (1);
	else this->Realloc (this->capacity + 1);
	int i;
	for (i=0; i < this->capacity-1 && this->d[i] < x; ++i); 
	int i2;
	for (i2=this->capacity-1; i2 > i; --i2)
		this->d[i2] = this->d[i2-1];
	this->d[i] = x;

	return this->d+i;
}

template<class T> typename TSet<T>::iterator TSet<T>::find(const T& x) const
{
	viterator<T> iter;
	for (iter = this->begin(); iter != this->end() && x > *iter; ++iter);
	if (iter == this->end()) return this->end();
	if (*iter > x) return this->end();
	else return iter;		
}

#endif
