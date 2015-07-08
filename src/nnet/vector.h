#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "iterator.h"
#include "base.h"

template<class T> inline T Max (const T &x, const T &y) { return x > y ? x : y; }
template<class T> inline T Min (const T &x, const T &y) { return x < y ? x : y; }


template<class T> class TVector {
protected:
	int capacity;
	T * d;
public:
	TVector () { d = 0; capacity = 0; }
	TVector(const TVector&x);
	TVector(int sz);
	~TVector () { delete[] d; }

	const T &operator[] (int x) const { ASSERT (x >= 0 && x < capacity && d); return d[x]; }
    T& operator[](int i) { ASSERT(i>=0 && i<capacity && d); return d[i];  }
	TVector & operator = (const TVector &x);

	void	Realloc(int nsz);
	void	DeleteAll();

	typedef viterator<T> iterator;
	typedef const_viterator<T> const_iterator;

	virtual iterator begin () const		{ if (d) return &*d; else return 0; }
	virtual iterator end () const		{ if (d) return d+capacity; else return 0; }

	int size() const			{ return capacity; }
	virtual iterator push_back (T x);
};

#define VECTOR(x,y) typedef TVector<x> y;
#define VECTORC(x,y) VECTOR(x,y)

template<class T> void TVector<T>::Realloc (int nsz) {
	T *newd = new T [nsz];
	for (int i = 0; i < Min (this->capacity, nsz); i ++)
		newd[i] = d[i];
	delete[] d;
	d = newd;
	capacity = nsz;
}

template<class T> void TVector<T>::DeleteAll () {
	delete[] d;
	capacity = 0;
	d = 0;
}

template<class T> typename TVector<T>::iterator TVector<T>::push_back (T x)
{
	Realloc (capacity + 1);
	d[capacity-1] = x;
	return d+capacity-1;
}


template<class T> TVector<T>::TVector(const TVector&x)
{
	capacity = x.capacity;
	d= new T [capacity];
	for(int i=0; i<capacity; i++) d[i] = x[i];
}

template<class T> TVector<T> & TVector<T>::operator = (const TVector &x)
{
	delete[] d;
	capacity = x.capacity;
	d= new T [capacity];
	for(int i=0; i<capacity; i++) d[i] = x[i];
	return *this;
}

template<class T> TVector<T>::TVector(int sz)
{
	capacity = sz;
	d= new T [sz];
}

#endif
