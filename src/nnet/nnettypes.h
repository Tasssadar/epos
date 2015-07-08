/*  Author: Jakub Adámek
	Purpose: Data types used in various places of my code */

#ifndef _BANG_TYPES_H_
#define _BANG_TYPES_H_

#ifdef _MSC_VER
	/** switch off some warnings */
	#pragma warning(disable:4355) // this used in base member initializer list
	#pragma warning(disable: 4250) // inherits via dominance
	#pragma warning(disable: 4786) // identifier was truncated
#endif

#include "stream.h"
#include "slowstring.h"
#include "vector.h"
#include "map.h"

class CXml;
#define CRox CXml

class CDummy {
public:
	virtual void Init () {}
};

typedef int CInt;
typedef double CFloat;
#define STRUCT_BEGIN(x) class x : public CDummy \
	{ public: x(){Init();} CXml *print () const; CString read (CRox *xml);
#define STRUCT_END(x) };

#define STRUCT_BEGIN_FROM(x,y) class x : public y { public: x(){Init();} CXml *print () const; CString read (CRox *xml);
#define STRUCT_END_FROM(x,y) };

#define SMALLVECTOR(dummy,x,y) VECTOR(x,y)

typedef TVector<CString> TVectorString;

typedef int Mutex;

/*
// G++ and MSVC cannot instanciate templates unless the whole source is included
// MSVC is so stupid that it doesn't recognize the .cc extension
//

#ifndef _MSC_VER
	#include "../dict/array.cc"
	#include "../dict/vector.cc"
	#include "../dict/svector.cc"
	#include "../dict/set.cc"
	#include "../dict/pair.cc"	
	#include "../dict/map.cc"
	#include "../dict/matrix.cc"
#else
	#include "../dict/array.cpp"
	#include "../dict/vector.cpp"
	#include "../dict/svector.cpp"
	#include "../dict/set.cpp"
	#include "../dict/pair.cpp"
	#include "../dict/map.cpp"
	#include "../dict/matrix.cpp"
#endif
*/

#include "matrix.h"
#include "enumstring.h"

#endif
