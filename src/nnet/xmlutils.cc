/** @file utils.cpp

  @brief Miscellaneous utilities

  @author Jakub Adámek
  Last modified $Id: xmlutils.cc,v 1.2 2002/03/26 15:45:12 jakubadamek Exp $
*/

#include "xml.h"
#include "xmlutils.h"
#include <stdio.h>

CString xml_read_enum (const CRox *xml, CInt & x, const TEnumString &enums, const CString &tag, bool req)
{
	if (!(*xml)[tag].Exists()) 
		if (req) return CString(tag)+" required ";
		else return "";
	return (*xml)[tag].GetAttr ("value", x, enums);
}

