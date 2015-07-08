#ifndef __PAIR_H__
#define __PAIR_H__

template<class T1, class T2> class TPair {
	T1 data1;
	T2 data2;
public:
	TPair () { }
	T1 & first () { return data1; }
	T2 & second () { return data2; }
	bool operator < (const TPair &y) const		{ return data1 < y.data1; }
	bool operator > (const TPair &y) const		{ return data1 > y.data1; }
	bool operator <= (const TPair &y) const		{ return data1 <= y.data1; }
	bool operator >= (const TPair &y) const		{ return data1 >= y.data1; }

};

#endif
