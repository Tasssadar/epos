#include "stream.h"

//-----------------------------------------------------------------------------

bang_ifstream::bang_ifstream(const char *name, int mode)
:	bang_istream()
{
	file = NULL;
	open (name, mode);
}

void bang_ifstream::open (const char *name, int mode)
{
	char *modeStr;

	modeStr = "r";

	if (file != NULL) fclose (file);
	if(name) {
		file = fopen(name, modeStr);
	} else {
		file = NULL;
	}
	if (file == NULL) {
		state |= badbit | eofbit;
	}
}

bang_ifstream::bang_ifstream(FILE *file, int mode)
:	bang_istream()
{
	this->file = file;
	if (file == NULL) {
		state |= badbit | eofbit;
	}
}

//-----------------------------------------------------------------------------

bang_ifstream::~bang_ifstream()
{
	if (file != NULL)
		fclose(file);
}


//-----------------------------------------------------------------------------

void bang_ifstream::close()
{
	if (file != NULL)
		fclose(file);
  file = NULL;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::operator>>(int &i)
{
	int retVal;

	if (state) {
		state |= failbit;
		return *this;
	}

	retVal = fscanf(file, "%d", &i);
	if (retVal != 1)
		state |= failbit;
	if (retVal == EOF)
		state |= eofbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::operator>>(char &c)
{
	int retVal;

	if (state) {
		state |= failbit;
		return *this;
	}

	retVal = fscanf(file, "%c", &c);
	if (retVal != 1)
		state |= failbit;
	if (retVal == EOF)
		state |= eofbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::operator>>(char *s)
{
	int retVal;

	if (state) {
		state |= failbit;
		return *this;
	}

	retVal = fscanf(file, "%s", s);
	if (retVal != 1)
		state |= failbit;
	if (retVal == EOF)
		state |= eofbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::operator>>(double &d)
{
	int retVal;

	if (state) {
		state |= failbit;
		return *this;
	}

	retVal = fscanf(file, "%lg", &d);
	if (retVal != 1)
		state |= failbit;
	if (retVal == EOF)
		state |= eofbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::putback(char &c)
{
	int retVal;

	if (state) {
		state |= failbit;
		return *this;
	}

	retVal = ungetc(c, file);
	if (retVal == EOF)
		state |= failbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::getline(char *s, size_t n, int delim)
{
	if (state) {
		state |= failbit;
		return *this;
	}

	fgets(s, n,file);
	if (feof(file))
		state |= eofbit;
	if (ferror(file))
		state |= failbit;

	if(!state) {
		int c;
		c = fgetc(file);
		if(c == EOF) {
			if (feof(file))
				state |= eofbit;
			if (ferror(file))
				state |= failbit;
		} else {
			int retVal;
			retVal = ungetc(c, file);
			if (retVal == EOF)
				state |= failbit;
		}
	}

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_ifstream::ignore(size_t n, int delim)
{
	size_t i;
	char c;

	if (state) {
		state |= failbit;
		return *this;
	}

	for(i = 0; i < n; i++) {
		*this >> c;
		if (state) break;
		if (c == delim) break;
	}

	return *this;
}


//-----------------------------------------------------------------------------

bang_ofstream::bang_ofstream(const char *name, int mode)
:	bang_ostream()
{
	file = NULL;
	open (name, mode);
}

void bang_ofstream::open(const char *name, int mode)
{
	char *modeStr;

	switch (mode) {
	case out:
		modeStr = "w";
		break;

  case out | app:
		modeStr = "a";
		break;
	}

	if (file != NULL) fclose (file);
	if(name) {
		file = fopen(name, modeStr);
	} else {
		file = NULL;
	}
	if (file == NULL) {
		state |= badbit | eofbit;
	}
}

bang_ofstream::bang_ofstream(FILE *file, int mode)
:	bang_ostream()
{
	this->file = file;
	if (file == NULL) {
		state |= badbit | eofbit;
	}
}

//-----------------------------------------------------------------------------

bang_ofstream::~bang_ofstream()
{
	if (file != NULL)
		fclose(file);
}


//-----------------------------------------------------------------------------

void bang_ofstream::close()
{
	if (file != NULL)
		fclose(file);
  file = NULL;
}


//-----------------------------------------------------------------------------

bang_ostream &bang_ofstream::operator<<(int i)
{
	if (state) {
		state |= failbit;
		return *this;
	}

	fprintf(file, "%d", i);
	if (ferror(file))
		state |= failbit | badbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_ostream &bang_ofstream::operator<<(unsigned u)
{
	if (state) {
		state |= failbit;
		return *this;
	}

	fprintf(file, "%u", u);
	if (ferror(file))
		state |= failbit | badbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_ostream &bang_ofstream::operator<<(char c)
{
	if (state) {
		state |= failbit;
		return *this;
	}

	fputc(c, file);
	if (ferror(file))
		state |= failbit | badbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_ostream &bang_ofstream::operator<<(const char *s)
{
	if (state) {
		state |= failbit;
		return *this;
	}

	if (s) fputs(s, file);
	if (ferror(file))
		state |= failbit | badbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_ostream &bang_ofstream::operator<<(double g)
{
	if (state) {
		state |= failbit;
		return *this;
	}

	fprintf(file, "%lg", g);
	if (ferror(file))
		state |= failbit | badbit;

	return *this;
}


//-----------------------------------------------------------------------------

bang_istream &bang_istream::get(char &c)
{
  return (*this >> c);
}


//-----------------------------------------------------------------------------

bang_istream &bang_istream::get(char *buff, int count, char delim)
{
  char c;

  if (state) {
    state |= failbit;
    return *this;
  }

  for (int i = 1; i < count; buff++, i++) {
    *this >> c;
	if (eof()) {
		break;
	}
	if (c == delim) {
		putback(c);
		break;
	}
	*buff = c;
  }
  *buff = 0;
  return *this;
}

 

//-----------------------------------------------------------------------------

const char *bang_endl = "\n";


//-----------------------------------------------------------------------------

bang_ifstream bang_cin (stdin);
bang_ofstream bang_cout (stdout);
bang_ofstream bang_cerr (stderr);

//-----------------------------------------------------------------------------
