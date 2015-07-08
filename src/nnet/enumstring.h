// MT:Safe, AL:Safe, Version:Stable, Author:Paja

#ifndef _ENUMSTR_
#define _ENUMSTR_

#include "nnettypes.h"
#include "utils.h"

MAP (CString, CInt, TMapString2Int)

STRUCT_BEGIN (TEnumString)
	TVectorString data;
	mutable TMapString2Int map;
public:
	TEnumString (const char *x);	
	TEnumString & operator = (const char *x);
	CInt operator[] (const CString &x) const	{ return map.find(x) != map.end() ? map[x] : CInt(-1); }
	CString operator[] (int x) const		{ if (x >= 0) return data[x]; else return "?"; }
	int addString (const CString &x);
	int size() const				{ return map.size(); }
STRUCT_END (TEnumString)
#endif
