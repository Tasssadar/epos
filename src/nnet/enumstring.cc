// MT:Safe, AL:Safe, Version:Stable, Author:Paja

//#include "../base/bang.h"
#include "enumstring.h"
#include "assert.h"
#include "xml.h"

REGMAP (CString, CInt, TMapString2Int)
REGSTRUCT (TEnumString)

TEnumString::TEnumString (const char *x)
{ 
	data = split(x,";"); 
	for (int i=0; i < data.size(); ++i)
		map[data[i]] = CInt(i);
}

TEnumString &TEnumString::operator = (const char *x)
{ 
	map.DeleteAll();
	data = split(x,";"); 
	for (int i=0; i < data.size(); ++i)
		map[data[i]] = CInt(i);
	return *this;
}

int TEnumString::addString (const CString &x)
{
	if (map.find(x) == map.end()) {
		map[x] = data.size();
		data.push_back (x);
	}
	return (int)map[x];
}


CXml *TEnumString::print () const
{
	CXml *retval = new CXml ("enumstring");
	retval->AddAttr ("value",join (data.begin(), data.end(), ";"));
	return retval;
}

CString TEnumString::read (CRox *xml)
{
	CString val;
	CString err = xml->GetAttr ("value", val);
	operator = (val);
	return err;
}

// this instantiation fits nowhere and perhaps shouldn't be necessary at all
// for compliant compilers

template class CBasicString<char>;
