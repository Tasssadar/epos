#ifndef __MAP_H__
#define __MAP_H__

#include "pair.h"
#include "set.h"

template<class T1, class T2> class TMap : public TSet< TPair<T1,T2> > {
	typedef TPair<T1,T2> TData;
	typedef T1 TFirst;
	typedef T2 TSecond;
public:
	TMap () { this->d = 0; this->capacity = 0; }
	T2 &operator [] (const T1 &key);
	typedef typename TVector<TPair<T1,T2> >::iterator iterator;
	virtual iterator find (const T1& key) const;
};

#define MAP(x,y,z) typedef TMap<x,y> z;
 
template<class T1,class T2> 
T2 & TMap<T1,T2>::operator [] (const T1 &key) {
	iterator found = find (key);
	if (found != this->end())
		return found->second();
	else {
		TData x;
		x.first() = key;
		return insert (x)->second();
	}
}

template<class T1,class T2> 
typename TMap<T1,T2>::iterator TMap<T1,T2>::find (const T1 &key) const {
	TData x;
	x.first() = key;
	return TSet<TData>::find (x);
}

#endif
