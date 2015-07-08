/** @file xmltempl.cpp

  @brief XML input / output

  @author Jakub Adámek
  Last modified $Id: xmltempl.cc,v 1.3 2002/04/19 17:44:55 jakubadamek Exp $
*/

#include "xmlutils.h"

REGVECTORC (TrainingProcess, TBatch, TBatches)
REGSTRUCTC (TrainingProcess, TBatch)

#define XMLIZE(x,tag) \
\
CXml * x ::print () const { \
	CXml *retval = new CXml (#tag); \
	retval->SetFF(DODELETE);

#define R(x) 
#define P(x) x
#define RP(x) x

#define child(x,tag) retval->AddChild (*xml_print (x,tag));
#define container(x,tag) retval->AddChild (*xml_print_container (x,tag));
#define child_opt(x,tag) if(x) child(x,tag)
#define child_enum(x,tag,enumS) retval->AddChild (*xml_print (enumS[x],tag));
#define container_opt(x,tag) if(x.size()) container(x,tag)
#define structure(x,tag) retval->AddChild (*xml_print_str (x,tag));
#define structure_opt(x,tag,write_cond) if (write_cond) structure(x,tag)

#define attr(x,tag) retval->AddAttr (tag,toString(x));
#define attr_enum(x,tag,enumS) retval->AddAttr (tag, x >= 0 ? enumS[x].c_str() : "?");
#define attr_opt(x,tag) if(x) retval->AddAttr (tag,toString(x));

//#define attr(x,tag) child(x,tag)
//#define attr_enum(x,tag,enumS) child_enum(x,tag,enumS)
//#define attr_opt(x,tag) child_opt(x,tag)

#define CHILD(x)		child(x,#x)
#define CHILD_OPT(x)		child_opt(x,#x)
#define CHILD_ENUM(x,enumS)	child_enum(x,#x,enumS)
#define CONTAINER(x)		container(x,#x)
#define CONTAINER_OPT(x)	container_opt (x,#x)
#define STRUCTURE(x)		structure(x,#x)
#define STRUCTURE_OPT(x,cond)	structure_opt(x,#x,cond)

#define ATTR(x)			attr(x,#x)
#define ATTR_ENUM(x,enumS)	attr_enum (x,#x,enumS)
#define ATTR_OPT(x)		attr_opt (x,#x)

#define END_XMLIZE \
return retval; }

#include "xmltempl.h"

#undef XMLIZE
#undef R
#undef P
#undef child
#undef child_opt
#undef child_enum
#undef container
#undef container_opt
#undef structure
#undef structure_opt
#undef attr
#undef attr_enum
#undef attr_opt
#undef END_XMLIZE

#define XMLIZE(x,tag) \
\
CString x ::read (CRox *xml) { \
	if (xml == 0) return "read called with NULL, perhaps a result of wrong XML parsing"; \
	CString err; 
	//*this = x (); // set defaults

#define R(x) x
#define P(x) 

#define child(x,tag) err += xml_read (xml,x,tag);
#define child_opt(x,tag) err += xml_read(xml,x,tag,false);
#define child_enum(x,tag,enumS) err += xml_read_enum(xml,x,enumS,tag);
#define container(x,tag) err += xml_read_container (xml,x,tag);
#define container_opt(x,tag) err += xml_read_container (xml,x,tag,false);
#define structure(x,tag) err += xml_read_str (xml,x,tag);
#define structure_opt(x,tag,write_cond) err += xml_read_str (xml,x,tag,false);

#define attr(x,tag) err += xml->GetAttr (tag,x);
#define attr_enum(x,tag,enumS) err += xml->GetAttr (tag,x,enumS);
#define attr_opt(x,tag) err += xml->GetAttr(tag,x,false);

//#define attr(x,tag) child(x,tag)
//#define attr_enum(x,tag,enumS) child_enum(x,tag,enumS)
//#define attr_opt(x,tag) child_opt(x,tag)

#define END_XMLIZE \
if (err.length()) return CString(xml->Tag())+":"+err; \
else return ""; }

#include "xmltempl.h"		
/*
#undef XMLIZE
#undef child
#undef child_opt
#undef child_enum
#undef container
#undef container_opt
#undef structure
#undef structure_opt
#undef attr
#undef attr_enum
#undef attr_opt
#undef END_XMLIZE
#undef R
#undef P
#undef RP

#define XMLIZE(x,tag) x tmp##tag; retval->AddChild (*tmp##tag.printTemplate ());
#define child(x,tag) 
#define container(x,tag) 
#define child_opt(x,tag) 
#define child_enum(x,tag,enumS) 
#define container_opt(x,tag) 
#define structure(x,tag) 
#define structure_opt(x,tag,write_cond) 
#define attr(x,tag) 
#define attr_enum(x,tag,enumS) 
#define attr_opt(x,tag) 
#define END_XMLIZE 
#define R(x)
#define P(x)
#define RP(x)

CRox *xmltempl ()
{
	CRox *retval = new CXml ("XML_template");
	retval->SetFF (DODELETE);
#include "xmltempl.h"
	return retval;
}

#undef XMLIZE
#undef child
#undef child_opt
#undef child_enum
#undef container
#undef container_opt
#undef structure
#undef structure_opt
#undef attr
#undef attr_enum
#undef attr_opt
#undef END_XMLIZE
#undef CHILD
#undef CHILD_OPT
#undef CHILD_ENUM
#undef CONTAINER
#undef CONTAINER_OPT
#undef STRUCTURE
#undef STRUCTURE_OPT

#undef ATTR
#undef ATTR_ENUM
#undef ATTR_OPT

#define XMLIZE(x,tag) \
\
CXml * x ::printTemplate () const { \
	CXml *retval = new CXml (#tag,1,0,"comment","structure " #x); \
	retval->SetFF(DODELETE);
#define child(x,tag) retval->AddChild (*(new CXml(tag,1,0,"comment",#x))->SetFF(DODELETE));
#define container(x,tag) 
#define child_opt(x,tag) 
#define child_enum(x,tag,enumS) 
#define container_opt(x,tag) 
#define structure(x,tag) 
#define structure_opt(x,tag,write_cond) 
#define attr(x,tag) 
#define attr_enum(x,tag,enumS) 
#define attr_opt(x,tag) 
#define END_XMLIZE return retval; }

#define CHILD(x) retval->AddChild (*(new CXml(#x,1,0,"comment",toString(x.GetTypeID()).c_str()))->SetFF(DODELETE));
#define CHILD_OPT(x)		
#define CHILD_ENUM(x,enumS)	
#define CONTAINER(x)		
#define CONTAINER_OPT(x)	
#define STRUCTURE(x)		
#define STRUCTURE_OPT(x,cond)	

#define ATTR(x)			
#define ATTR_ENUM(x,enumS)	
#define ATTR_OPT(x)		

#include "xmltempl.h"*/

