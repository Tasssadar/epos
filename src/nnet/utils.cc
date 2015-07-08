/** @file utils.cpp

  @brief Miscellaneous utilities

  @author Jakub Adámek
  Last modified $Id: utils.cc,v 1.14 2002/04/20 12:11:10 jakubadamek Exp $
*/

#include "utils.h"
#include "stream.h"
#include <stdio.h>

#ifdef _MSC_VER
	int vsnprintf( char *buffer, size_t count, const char *format, va_list argptr )
	{ return _vsnprintf( buffer, count, format, argptr ); }
#endif

void Log (const CString &str)
{
	bang_ofstream log ("log.txt",bang_ofstream::out | bang_ofstream::app);
	log << str << bang_endl;
	if (logToConsole) bang_cout << str << bang_endl;
}

void LogError (const CString &str) { Log (CString("Error: ") + str); }

#if 0
CString toString (int x, int, bool)
{
	char tmp[100];
	sprintf(tmp,"%li",x); // itoa not found on Irix, paja
	return tmp;
}
#endif

CString findFileName (CString mask)
{
	CString fileName, prep, postp;
	if (mask.length()==0) return "";
	char *pos = strchr (mask,'%');
	if (pos) {
		prep = mask.substr(0,pos-mask.c_str());
		postp = mask.substr(pos-mask.c_str()+1);
	}
	else {
		prep = mask;
		postp = "";
	}

	FILE *file;
	for (int num=0; ; num++) {
		fileName = prep + toString (num) + postp;
		file = fopen (fileName, "r");
		if (!file) {
			file = fopen (fileName, "a");
			if (!file) {
				LogError (CString("findFileName: Error creating file ")+fileName+". Does the folder exist?");
				fileName = "";
				break;
			}
			else {
				fclose (file);
				break;
			}
		}
		else fclose (file);
	}
	return fileName;
}

CString findFirstFileName (CString mask)
{
	CString fileName, prep, postp;
	if (mask.length()==0) return "";
	char *pos = strchr (mask,'%');
	if (pos) {
		prep = mask.substr(0,pos-mask.c_str());
		postp = mask.substr(pos-mask.c_str()+1);
	}
	else {
		prep = mask;
		postp = "";
	}

	for (int num=0; num <= 999; num++) {
		fileName = prep + toString (num) + postp;
		FILE *file = fopen (fileName, "r");
		if (file) {
			fclose (file);
			return fileName;
		}
	}
	return "";
}

CString getFilePath (const CString &fileNameWithPath) 
{
	CString s = fileNameWithPath;
	if (s.length() == 0) 
		return "";
	char *end;
	for (end = const_cast<char *>(s.c_str()) + s.length() - 1; 
		end > s.c_str() && !strchr ("/\\",*end); end --);
	if (end == s.c_str()) 
		return "";
	*++end = 0;
	return s;
}
 
TVectorString split (const CString s, const CString &delim, bool allowEmpty)
{
	char *pos = const_cast<char *>(s.c_str());
	char *oldpos;
	char buf[1000], *pbuf;
	TVectorString retVal;

	while (*pos) {
		oldpos = pos;
		pbuf = buf;
		while (!strchr (delim.c_str(),*pos) && *pos) *pbuf++ = *pos++;
		*pbuf = '\x0';
		if (pbuf > buf || allowEmpty) {
			retVal.push_back (CString (buf));
			//retVal.Realloc (retVal.size()+1);
			//retVal[retVal.size()-1] = CString(buf);
		}
		if (*pos) {
			pos++;
			if (!*pos && allowEmpty) {
				retVal.Realloc(retVal.size()+1);
				retVal[retVal.size()-1] = CString("");
			}
		}
	}
	return retVal;
} 
