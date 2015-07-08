#ifndef _CXML_
#define _CXML_

#include "nnettypes.h"
#include "slowstring.h"
#include "vector.h"
#include "stream.h"

class TEnumString;

typedef enum { TAG, TEXT, COMMENT } Xmlsubtype;
typedef enum { DODELETE } dummy;

class CXml {
	public:
		class TAttr {
		public:
			TAttr () {}
			TAttr (CString name, CString value): name(name), value(value) {}
			CString name;
			CString value;
		};

	private:
		VECTOR(CXml,CXmls);
		CXmls	ch;

		VECTOR(TAttr, TAttrs);
		TAttrs at;

		CString		tag;
		Xmlsubtype	subt;
	
	public:
		CXml();
		CXml(const CString &tag);
		CXml(const CString &tag,int nattrs,int nchildren,...);
		CXml(const CXml& src);
		~CXml();

		operator CString () const;
		CString print (const CString &indent) const;

		void		SetSubtype(Xmlsubtype n) { subt = n; }
		Xmlsubtype	GetSubtype() { return subt; }

		void	AddAttr(const CString &name, const CString &val);
		void	AddChild(CXml&);

		CXml&	SetTag(const CString &tag);
		CXml&	Parse(const CString &xmlstring);

		CXml&	DelAttr(const CString &attrname);
		CXml&	DelChild(CXml&);

		CXml&	DelAttrs();
		CXml&	DelChildren();
		CXml&	DelAll();

		/// returns child with given tag or NullRox if not found
		CXml&	operator[](const CString &)	const;
		CXml&	Child(int) const;
		int	NAttrs()	const;
		int	NChildren()	const;

		CXml *SetFF (dummy)		{ return this; }

		/// returns attr value or empty string if not found
		RStr	operator()(const CString &) const;
		RStr	AttrV(int)	const;
		RStr	Tag()		const;
		RStr	AttrN(int)	const;

		/** You may use a special tag <default> which adds all its children and attribs
			to all tags on the same level. Defaults don't rewrite current ones, only
			new children and attribs are added. 
			
			You can copy children from another tag on the same level by 
			<... copyFrom="sourceName"> where the source has attrib <... name="sourceName">.
			
			You may use both, "copyFrom" is resolved before "default".
			Call it from root tag, it goes through all the tree. */
		void	AddDefaults();
		void	AddDefaults(CXml *dst);
		/// returns 1 on error, 0 on success 
		int AddIncludes(const CString &filepath);

		/// return values for GetAttr functions
		enum enumGetAttr { GA_OK, GA_NOTFOUND, GA_WRONGTYPE };

		/** these function proove that the attr exists and has the right type
			they return an error message or empty string
			if req=false adn not exists, no error message is sent and dst is not changed */
		RStr GetAttr (const CString &attrN, CInt &dst, bool req=true) const;
		RStr GetAttr (const CString &attrN, CString &dst, bool req=true) const;
		RStr GetAttr (const CString &attrN, CInt &dst, const TEnumString &enums, bool req=true) const;
		RStr GetAttr (const CString &attrN, CFloat &dst, bool req=true) const;

		int Exists() const		{ return tag != ""; } 
};

class XMLFile {
	CString filename;
public:
	void setfile (const CString &filename) { this->filename = filename; }
	CXml *parse ();
};

inline
CXml::CXml(): subt(TAG) { }

inline
CXml::CXml(const CString &ntag): tag(ntag),subt(TAG) { }

inline
CXml&	CXml::DelAll()
{
	DelAttrs();
	DelChildren();
	return *this;
}


inline
CXml::~CXml()
{
	DelAll(); 
}

inline
CXml&	CXml::SetTag(const CString& ntag)
{
	tag = ntag;
	return *this;
}
inline
RStr	CXml::Tag() const
{
	return	tag;
}

inline
CXml&  CXml::Child(int idx) const
{
	if(idx>=ch.size()) return *(new CXml());
	return const_cast<CXml &>(ch[idx]);
}

inline
RStr	CXml::operator()(const CString &name) const
{
	for (TAttrs::iterator attr = at.begin(); attr != at.end(); ++ attr)
		if ((*attr).name == name) return (*attr).value;
	return "";
}

inline
RStr	CXml::AttrV(int idx) const
{
	if(idx>=at.size()) return "";
	return at[idx].value;;
}

inline
RStr	CXml::AttrN(int idx) const
{
	if(idx>=at.size()) return "";
	return at[idx].name;
}

inline
int	CXml::NAttrs() const
{
	return	at.size();
}
inline
int	CXml::NChildren() const
{
	return	ch.size();
}

#endif
