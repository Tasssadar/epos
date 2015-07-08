/** @file utils.h

  @brief Miscellaneous utilities

  @author Jakub Adámek
  Last modified $Id: xmlutils.h,v 1.7 2002/04/20 12:12:47 jakubadamek Exp $
*/

#ifndef __BANG_XMLUTILS_H__
#define __BANG_XMLUTILS_H__

//#include "../base/bang.h"
/*
#include "../dict/float.h"
#include "../dict/vector.h"
#include "../dict/string.h"
#include "../dict/xml.h" */
#include "utils.h"
#include "xml.h"

// defined in xmltempl.cpp - prints info about all structures described in xmltempl.h
CRox *xmltempl ();

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * *                                                           * *
 * *                  XML PRINT and READ                       * *
 * *							       * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */	

#define XML_PRINT(x) xml_print (x, #x)
#define XML_READ(xml,x) xml_read (xml, x, #x)
#define XML_PRINT_CONTAINER(x) xml_print_container (x, #x)
#define XML_READ_STR(xml,x) xml_read_str (xml, x, #x)

template<class T> inline CRox * xml_print (T x, const char *tag);

// returns error description or empty string
//template<class T> CString xml_read (const CRox *xml, T & x, const char *tag, bool req);
CString xml_read_enum (const CRox *xml, CInt & x, const TEnumString &enums, const CString &tag, bool req=true);

/** Works on structures which have the "read" function defined.
	 Returns error description or empty string */
//template<class T> CString xml_read_str (const CRox *xml, T & x, const char *tag, bool req);
template<class T> CRox * xml_print_str (const T & x, const char *tag);
template<class TContainer> CRox * xml_print_container (TContainer x, const char *tag);
//template<class TContainer> CString xml_read_container (const CRox *xml, TContainer & x, const char *tag, bool req);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function bodies of template functions:			 */

template<class T> inline CRox * xml_print (T x, const char *tag)
{ 
	const CString &holder = toString ((const T)x);
	return (new CXml(tag,1,0,"value",holder.c_str()))->SetFF(DODELETE); 
}

template<class T> inline void setZero (T &x) { x = 0; }
template<> inline void setZero (CString &x) { x = ""; }

template<class T> CString xml_read (const CRox *xml, T & x, const char *tag, bool req=true)
{ 
	if (!(*xml)[tag].Exists()) {
		//setZero (x);
		if (req) return CString(tag)+" required ";
		else return "";
	}
	return (*xml)[tag].GetAttr ("value", x);
}

template<class T> CString xml_read_str (const CRox *xml, T & x, const char *tag, bool req=true)
{ 
	if (!(*xml)[tag].Exists()) 
		if (req) return CString(tag)+" required ";
		else return "";
	return x.read (&(*xml)[tag]);
}

template<class T> CRox * xml_print_str (const T & x, const char *tag)
{
	CRox *retval = x.print();
	retval->SetTag (tag);
	return retval;
}

template<class TContainer> CRox * xml_print_container (TContainer x, const char *tag)
{
	CRox *retval = new CXml(tag);
	retval->SetFF(DODELETE);
	typename TContainer::iterator istr;
	for (istr = x.begin(); istr != x.end(); ++istr)
		retval->AddChild (*istr->print());
	return retval;
}

template<class TContainer> CString xml_read_container (const CRox *xml, TContainer & x, const char *tag, bool req=true)
{
	const CRox &child = (*xml)[tag];
	if (!child.Exists())
		if (req) return CString(tag)+" required ";
		else return "";
	x.Realloc (1);
	CString childTag = x.begin()->print()->Tag();
	int size = 0;
	int i;
	for (i=0; i < child.NChildren(); ++i) {
		if (!strcmp (child.Child(i).Tag(), childTag.c_str())) 
			++size;
	}
	x.Realloc (size);
	CString err;
	typename TContainer::iterator iter = x.begin();
	for (i=0; i < child.NChildren(); ++i)
		if (!strcmp(child.Child(i).Tag(), childTag.c_str())) {
			err = iter->read (&child.Child(i));
			if (err != "") return err;
			++iter;
		}
	return err;
}

#endif

