#ifndef _STRING_
#define _STRING_

#include <stdlib.h>
#include <string.h>


// template<class T> class CBasicString;
// template<class T> CBasicString<T> operator+(const CBasicString<T> &a, const CBasicString<T> &b);

template<class T> class CBasicString {
	T * data;
public:
	CBasicString ();
	~CBasicString() { free (data); }

		CBasicString(const T*d) { if(d) data = strdup(d); else data = strdup(""); }
		CBasicString(const CBasicString& src) { if(src.data) data = strdup(src.data); else data = 0; }
  
		virtual	operator T*() const	{ return data; }
		virtual	operator T* &()	{ return data; }
		operator bool ()		{ return data && strlen(data); }

		virtual const T *c_str() const { return data; }

		//virtual	RStr	Tag()	const	{ return RStr("CBasicString",STATIC); }

		int length() const		{ if (data) return strlen(data); else return 0; }

		virtual CBasicString substr(int pos=0, int n=-1) const;
		/// replaces the occurences of source with target
		void replace (const CBasicString &source, const CBasicString &target);

		CBasicString	operator+(const CBasicString& b) const;
		CBasicString	operator+(const T	 b) const;
		CBasicString	operator+(const T*    b) const;
		CBasicString&operator =(const T    b);
		CBasicString&operator =(const T*    b);
		CBasicString&operator =(const CBasicString& b);
		CBasicString&operator+=(const CBasicString& b);
		CBasicString&operator+=(const T* b);
		CBasicString&operator+=(const T b);

		int	operator==(const T*)		const;
		int	operator==(const T)		const;
		int	operator==(const CBasicString&)	const;
		int	operator!=(const T*)		const;
		int	operator!=(const T)		const;
		int	operator!=(const CBasicString&)	const;

		bool operator < (const CBasicString &b) const { return strcmp (data,b.data) < 0; }
		bool operator > (const CBasicString &b) const { return strcmp (data,b.data) > 0; }

		//		friend CBasicString<T> operator+<T>(const CBasicString<T> &a, const CBasicString<T> &b);
};

typedef CBasicString<char> CString;
typedef CString RStr;

template<class T>
CBasicString<T>::CBasicString ()
{
	data = new T;
	data[0] = 0;
}

template<class T>
int	CBasicString<T>::operator==(const T* b)	const
{
	if (!data) return b == 0;
	return	!strcmp( data, b );
}
template<class T>
int	CBasicString<T>::operator==(const T b)	const
{
	T bb[2] = { b, 0 };
	if (!data) return false;
	return	!strcmp( data, bb );
}
template<class T>
int	CBasicString<T>::operator==(const CBasicString<T>& b)	const
{
	if (!data) return b.data == 0;
	return	!strcmp( data, b.data );
}
template<class T>
int	CBasicString<T>::operator!=(const T* b)	const
{
	if (!data) return b != 0;
	return	strcmp( data, b );
}
template<class T>
int	CBasicString<T>::operator!=(const T b)	const
{
	T bb[2] = { b, 0 };
	if (!data) return true;
	return	strcmp( data, bb );
}
template<class T>
int	CBasicString<T>::operator!=(const CBasicString<T>& b)	const
{
	if (!data || !b.data) return data || b.data;
	return	strcmp( data, b.data );
}

template<class T>
CBasicString<T>	CBasicString<T>::operator+(const CBasicString<T>& b) const		
{ 
	CBasicString<T> r;
	if (!b.data) {
		if (data) r.data = strdup (data);
		return r;
	}
	if (!data) {
		r.data = strdup (b.data);
		return r;
	}
	r.data = (T*)malloc(strlen(data)+strlen(b.data)+1);
	strcpy(r.data,data);
	strcat(r.data,b.data);
	return r;
}
template<class T>
CBasicString<T>	CBasicString<T>::operator+(const T*    b) const	
{ 
	CBasicString<T> r;
	if (!data) {
		r.data = strdup (b);
		return r;
	}
	r.data = (T*)malloc(strlen(data)+strlen(b)+1);
	strcpy(r.data,data);
	strcat(r.data,b);
	return r;
}
template<class T> inline
CBasicString<T> CBasicString<T>::operator+(const T    b) const 	
{ 
	CBasicString<T> r;
	T bb[2] = { b, 0 };
	if (!data) {
		r.data = strdup (bb);
		return r;
	}
	r.data = (T*)malloc(strlen(data)+2);
	strcpy(r.data,data);
	strcat(r.data,bb);
	return r;
}
template<class T>
CBasicString<T>&CBasicString<T>::operator =(const T*    b)
{
	if( data ) free( data );
	data = strdup( b );
	return *this;
}
template<class T>
CBasicString<T>&CBasicString<T>::operator =(const T    b)
{
	if( data ) free( data );
	T bb[2] = { b, 0 };
	data = strdup( bb );
	return *this;
}
template<class T>
CBasicString<T> &CBasicString<T>::operator =(const CBasicString<T>& b)
{
	if( data ) free( data );
	if (!b.data) data = 0; else data = strdup( b.data );
	return *this;
}
template<class T>
CBasicString<T>&CBasicString<T>::operator+=(const CBasicString<T>& b)
{ 
	if (!b.data) return *this;
	if (!data) {
		data = strdup (b.data);
		return *this;
	}
	data = (T*)realloc( data, strlen(data)+strlen(b.data)+1 );
	strcat(data,b.data);
	return *this;
}
template<class T>
CBasicString<T>&CBasicString<T>::operator+=(const T* b)
{ 
	if (!b) return *this;
	if (!data) {
		data = strdup (b);
		return *this;
	}
	data = (T*)realloc( data, strlen(data)+strlen(b)+1 );
	strcat(data,b);
	return *this;
}
template<class T>
CBasicString<T>&CBasicString<T>::operator+=(const T b)
{ 
	T bb[2];
	if (!data) {
		data = strdup (bb);
		return *this;
	}
	data = (T*)realloc( data, strlen(data)+2 );
	bb[0]=b; bb[1]=0;
	strcat(data,bb);
	return *this;
}
template<class T>
CBasicString<T> operator+(const CBasicString<T> &a, const CBasicString<T> &b)
{
	CBasicString<T> r;
	if (!a.data) return b;
	if (!b.data) return a;
	r.data = (T*)malloc(strlen(a.data)+strlen(b.data)+1);
	strcpy(r.data,a.data);
	strcat(r.data,b.data);
	return r;
}

template<class T>
CBasicString<T> CBasicString<T>::substr(int pos, int n) const
{
	if (!data || pos > signed(strlen(data))) return CBasicString();
	if (n == -1) n = strlen (data);
	else if (n < 0) return CBasicString();
	char *tmp = strdup (data + pos);
	if (signed(strlen (tmp)) > n) tmp[n] = '\x0';
	CBasicString retVal = tmp;
	free (tmp);
	return retVal;
}
 
template<class T>
void CBasicString<T>::replace (const CBasicString<T> &source, const CBasicString<T> &target)
{
	if (!data) return;
	CString rest = *this, result = "";
	char *pos;
	while (pos = strstr (rest.data, source)) {
		result += rest.substr (0,pos-rest.data) + target;
		rest = rest.substr (pos-rest.data+strlen(source));
	}
	result += rest;
	*this = result;
} 


#endif
