/** @file utils.h

  @brief Miscellaneous utilities

  @author Jakub Adámek
  Last modified $Id: utils.h,v 1.14 2002/04/20 12:12:47 jakubadamek Exp $
*/

#ifndef __BANG_UTILS_H__
#define __BANG_UTILS_H__

#include "nnettypes.h"
#include <stdlib.h>

static const int logToConsole = 0;

void Log (const CString &str);
void LogError (const CString &str);

template<class T> inline T sqr (T x) {
	return x * x;
}

// CString toString (const long x, int dummy=0, bool dummy1=0);
/** You may specify the number of decimal places, -1 means all */
CString toString (const double x, int decimalPlaces=-1, bool forceDecimalPoint=false);
// inline CString toString (const int x, int dummy=0, bool dummy1=0) { return toString ((long int) x); }
inline CString toString (const CString &x, int dummy=0, bool dummy1=0) { return x; }

inline CString toString (double x, int decimalPlaces, bool forceDecimalPoint)
{
	char tmp[100];
	CString s = forceDecimalPoint ? "%#" : "%";
	if (decimalPlaces > -1) {
		s += ".*f";
		sprintf (tmp,s.c_str(),decimalPlaces,x);
	}
	else {
		s += "g";
		sprintf (tmp,s.c_str(),x);
	}
	return tmp;
}

template<class T> inline void swap (T & x, T & y) 
{
	T tmp = x;
	x = y;
	y = tmp;
}

#ifdef _MSC_VER
	int vsnprintf( char *buffer, size_t count, const char *format, va_list argptr );
#endif

/** mask contains one "%" char, e.g. "perceptron_%.txt",
	this function replaces it with smallest number so that the file name doesn't exist 
	
	If there is no "%", it adds the number at the end of mask */
CString findFileName (CString mask);

/** Works with % as well, finds first existing file name. Restricted to numbers <= 999 */
CString findFirstFileName (CString mask);

inline void fromString (CString x, double &y)	{ y = atof (x.c_str()); } 
inline void fromString (CString x, int &y)	{ y = atoi (x.c_str()); }

template<class Titer, class T> inline int findIndex (const Titer & begin, const Titer & end, const T & x)
{
	int retval = 0;
	for (Titer i = begin; i != end; ++i, ++retval)
		if (*i == x) return retval;
	return -1;
}

template<class Titer> CString join (const Titer & begin, const Titer & end, const CString & delimiter)
{
	CString retval;
	for (Titer i = begin; i != end; ++i) {
		if (i != begin) retval += delimiter;
		retval += toString (*i);
	}
	return retval;
}

/** Splits at any of delim, returns vector of strings, includes empty strings
	only if allowEmpty is true */
TVectorString split(const CString s, const CString &delim, bool allowEmpty=true); 

// negative number means size in percent
template<class T> void setPercent (T &x, int count)
{
	if (x < (T)0) x = x * count / -100;
}

CString getFilePath (const CString &fileNameWithPath);

#endif

