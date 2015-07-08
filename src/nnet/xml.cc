// MT:Safe, AL:Safe, Version:WorkInProgress, Author:Paja

#include <stdarg.h>
#include "xml.h"
#include "utils.h"
#include "xml_parse.h"

#define REGTYPE(x)

REGTYPE( CXml );

CXml *bison_xml_result;
CString sxml;
int ixml; 

int xmlparse (void *);  //BISON

CXml *XMLFile::parse ()
{
	char temp[1000];
	bang_ifstream f (filename);
	sxml = "";
	while (!f.eof()) {
		f.getline (temp, 999);
		sxml += temp;
	}

	ixml = 0;
	xmlparse (NULL);
	sxml = "";
	return bison_xml_result;
}

CXml::CXml(const CString& ntag,int nattrs,int nchildren,...)
{
	int i;
	tag = strdup( ntag );
	subt = TAG;

	va_list list;
	va_start( list, nchildren );

	TAttr attr ("","");
	char* st;

	for(i=0; i<nattrs; i++) {
		st = va_arg( list, char* );
		if (st) attr.name = st;
		else attr.name = "";
		st = va_arg( list, char* );
		if (st) attr.name = st;
		else attr.name = "";
		at.push_back (attr);
	}

	CRox* chl;

	for(i=0; i<nchildren; i++) {
		chl = va_arg(list,CRox*);
		ch.push_back (*chl);
	}
	va_end( list );
}

CXml::CXml(const CXml& src)
{
	subt = TAG;

	ch = src.ch;
	at = src.at;
	tag	= src.tag;
}

void	CXml::AddAttr(const CString &name, const CString &val)
{
	at.push_back (TAttr (name,val));
}

void	CXml::AddChild(CRox& c)
{
	ch.push_back (c);
}

CRox&	CXml::operator[](const CString & tg) const
{
	for(CXmls::iterator child = ch.begin(); child != ch.end(); ++ child)
		if (child->Tag() == tg)
			return *child;
	return *new CXml();
}

void CXml::AddDefaults(CRox *dst)
{
	for(CXmls::iterator child = ch.begin(); child != ch.end(); ++ child) {
		if ((*dst)[child->Tag()].Exists())
			child->AddDefaults (&(*dst)[child->Tag()]);
		else
			dst->AddChild (*child);
	}
	for(TAttrs::iterator attr = at.begin(); attr != at.end(); ++ attr)
		if (!(*dst)(attr->name).length())
			dst->AddAttr (attr->name, attr->value);
}

void CXml::AddDefaults()
{
	for(CXmls::iterator child = ch.begin(); child != ch.end(); ++ child) {
		child->AddDefaults();
		if ((*child)("copyFrom").length()) {
			CString cf = (*child)("copyFrom"); 
			for(CXmls::iterator child2 = ch.begin(); child2 != ch.end(); ++ child2) 
				if ((*child2)("name") == cf)
					child2->AddDefaults (child);
		}
	}
			
	if ((*this)["default"].Exists()) 
		for(CXmls::iterator child = ch.begin(); child != ch.end(); ++ child) 
			(*this)["default"].AddDefaults (child);	
}

int CXml::AddIncludes (const CString &filename)
{
	XMLFile xmlfile;
	CRox *chi;
	CString include;
	for(CXmls::iterator child = ch.begin(); child != ch.end(); ++ child) {
		if (child->Tag() == "include" && (*child)("value").length()) {
			include = getFilePath (filename) + (*child)("value");
			xmlfile.setfile (include);
			chi = xmlfile.parse();
			if (chi) 
				*child = *chi;
			else return 1;
		}
		child->AddIncludes (filename);
	}
	return 0;
}

RStr CXml::GetAttr (const CString &attrN, CInt &dst, bool req) const
{
	if (!(*this)(attrN).length())
		if (req) return CString(attrN)+" required";
		else return "";
	char *end;
	const RStr &num = (*this)(attrN);
	dst = strtol (num, &end, 10);
	if (*end) return CString(attrN)+" should be integer number";
	return "";
}

RStr CXml::GetAttr (const CString &attrN, CString &dst, bool req) const
{
	if (!(*this)(attrN).length())
		if (req) return CString(attrN)+" required";
		else return "";
	dst = (*this)(attrN);
	return "";
}

RStr CXml::GetAttr (const CString &attrN, CInt &dst, const TEnumString &enums, bool req) const
{
	if (!(*this)(attrN).length())
		if (req) return CString(attrN)+" required";
		else return "";
	dst = enums[(*this)(attrN)];
	if ((int)dst == -1) {
		dst = 0;
		return CString(attrN)+" - value "+(*this)(attrN)+" not known.";
	}
	return "";
}

RStr CXml::GetAttr (const CString &attrN, CFloat &dst, bool req) const
{
	if (!(*this)(attrN).length())
		if (req) return CString(attrN)+" required";
		else return "";
	char *end;
	const RStr &num = (*this)(attrN);
	dst = strtod( num, &end );
	if (*end) return CString(attrN)+" should be a number";
	return "";
}


CXml &CXml::DelChildren()
{
	ch.DeleteAll();
	return *this;
}

CXml &CXml::DelAttrs()
{
	at.DeleteAll();
	return *this;
}

CXml::operator CString () const
{
	return print ("");
}

CString CXml::print (const CString &indent) const
{
	if (subt == COMMENT)
		return indent + CString("<!--") + tag + "-->\n"; 
	CString txt = indent + CString("<") + tag;
	CString val;
	for(TAttrs::iterator attr = at.begin(); attr != at.end(); ++ attr) {
		val = attr->value;
		val.replace ("\"", "\\\"");
		txt += CString(" ") + attr->name + "=\"" + val + "\"";
	}
	if (ch.size() == 1 && ch[0].subt == TEXT)
		return txt + ">" + ch[0].tag + "</" + tag + ">\n";
	if (ch.size() == 0) {
		txt += "/>\n";
		return txt;
	}
	txt += ">\n";
	for(CXmls::iterator child = ch.begin(); child != ch.end(); ++ child) 
		txt += child->print (indent + CString("\t"));
	txt += indent + CString("</") + tag + ">\n";
	return txt;
}
