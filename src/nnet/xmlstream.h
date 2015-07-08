/** @file xmlstream.h

  @brief Reads XML stream

  @author Jakub Adámek
  Last modified $Id: xmlstream.h,v 1.7 2002/04/20 12:11:10 jakubadamek Exp $
*/

#ifndef __BANG_XMLSTREAM_H__
#define __BANG_XMLSTREAM_H__

#include "stream.h"
#include "slowstring.h"
#include "vector.h"
#include "map.h"
#include "enumstring.h"
#include "xml.h"

/// stream encoding
enum enumEncoding {
	/// ascii text
	SE_TEXT,
	/// base64
	SE_BASE64
};
static const TEnumString SEs = "text;base64";

/// stream type
enum enumType {
	/// in the xml message
	ST_LOCAL,
	/// in a file
	ST_REMOTE
};
static const TEnumString StreamTypes = "local;remote";

/** @brief Stream represents data in a simple format 
  
	Local or remote (file) stream represents data container.
	Stream tag look e.g.: <stream encoding="text" type="local" delimiter=";">.
	Write delimiter="tab" to set \t.

*/

STRUCT_BEGIN (TStream)
public:
	void Init()	{ encoding=SE_TEXT; delimiter="\t"; value=""; type=ST_REMOTE; }
	CInt encoding;
	CInt type;
	CString delimiter;
	CString value;
STRUCT_END (TStream)

/* These two functions cannot be methods of TStream because MSVC can't handle them */

/** Returns error description or empty string. The filename is used to find the path to the data. */
//template<class TContainer> CString readStream (TStream &str, CRox &stream, TContainer & data, const CString &filename);
template<class Titer> CXml *printStream (TStream &str, Titer begin, Titer end, const CString &filename);

template<class Titer> CXml *printStream (TStream &str, Titer begin, Titer end, const CString &filename)
{
	CXml *retVal = str.print();
	CString w;
	for (Titer i=begin; i != end; ++i) {
		if (w.length()) w += str.delimiter.c_str()[0];
		w += toString (*i);
	}
	if (str.type == ST_LOCAL)
		retVal->AddAttr ("value",w);
	else {
		bang_ofstream strf (getFilePath(filename)+str.value);
		if (strf) strf << ((const char *) w);
		else LogError (CString("printStream: Cannot open file ")+str.value);
	}

	return retVal;
}

template<class TContainer> CString readStream
	(TStream &str, CXml &stream, TContainer & data, const CString &filename)
{
	CString dataString;
	CString err = str.read (&stream);
	if (err.length()) return err;

	if (str.type == ST_LOCAL) 
		dataString = stream("value");
	else {
		bang_ifstream strf (getFilePath(filename) + str.value);
		if (!strf) return CString("Cannot open file ")+str.value;
		char tmp[1000];
		while (!strf.eof()) {
			strf.getline (tmp, 1000);
			dataString += tmp;
		}
	}

	if (str.encoding != SE_TEXT) return "Not yet implemented - encoding "+str.encoding;

	TVectorString dataV = split (dataString, str.delimiter);
	data.Realloc (dataV.size());
	typename TContainer::iterator dIter = data.begin();
	for (int i=0; i < dataV.size(); ++i) {
		fromString (dataV[i], *dIter);
		++dIter;
	}
	return "";
}

#endif

