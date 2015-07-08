#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>

class bang_ios {
public:
  bang_ios(): state(0) { }
	virtual ~bang_ios() { }

	enum _Iostate {
		goodbit = 0x0,
		eofbit = 0x1,
		failbit = 0x2,
		badbit = 0x4,
	};

	enum _Openmode {
		in = 0x01,
		out = 0x02,
		ate = 0x04,
		app = 0x08,
		trunc = 0x10,
		binary = 0x20
	};

  virtual bool eof() const { return (state & eofbit) != 0; }
	virtual bool fail() const { return (state & failbit) != 0; }
	virtual bool bad() const { return (state & badbit) != 0; }
  virtual void clear() { state = state & !failbit; }

protected:
	int state;
};


extern const char *bang_endl;


//-----------------------------------------------------------------------------

class bang_ostream: virtual public bang_ios{
public:
	virtual bang_ostream &operator<<(int i) { return *this; }
	virtual bang_ostream &operator<<(unsigned u) { return *this; }
//	virtual bang_ostream &operator<<(long l) { return *this; }
//	virtual bang_ostream &operator<<(unsigned long ul) { return *this; }
	virtual bang_ostream &operator<<(double d) { return *this; }
	virtual bang_ostream &operator<<(char c) { return *this; }
	virtual bang_ostream &operator<<(const char *s) { return *this; }
};


class bang_istream: virtual public bang_ios{
public:
	virtual bang_istream &operator>>(int &i) = 0;
	virtual bang_istream &operator>>(char &c) = 0;
	virtual bang_istream &operator>>(char *s) = 0;
	virtual bang_istream &operator>>(double &d) = 0;

  virtual bang_istream &get(char &c);
  virtual bang_istream &get(char *buff, int count, char delim = '\n');
  virtual bang_istream &putback(char &c) = 0;
	virtual bang_istream &getline(char *s, size_t n, int delim = EOF) = 0;
	virtual bang_istream &ignore(size_t n = 1, int delim = EOF) = 0;
};


class bang_iostream: public bang_istream, public bang_ostream {
public:
  bang_iostream() { };
};

class bang_ifstream: public bang_istream {
public:
	bang_ifstream() : bang_istream()	{ file = NULL; }
	bang_ifstream(const char *name, int mode = in);
	bang_ifstream(FILE *file, int mode = in);
	virtual ~bang_ifstream();
  void close();
  void open (const char *name, int mode = in);

  operator bool() { return (file!=NULL); }

	virtual bang_istream &operator>>(int &i);
	virtual bang_istream &operator>>(char &c);
	virtual bang_istream &operator>>(char *s);
	virtual bang_istream &operator>>(double &d);

  virtual bang_istream &putback(char &c);
	virtual bang_istream &getline(char *s, size_t n, int delim = EOF);
	virtual bang_istream &ignore(size_t n = 1, int delim = EOF);

private:
	FILE *file;
};


//-----------------------------------------------------------------------------

class bang_ofstream: public bang_ostream {
public:
	bang_ofstream() : bang_ostream()	{ file = NULL; }
	bang_ofstream(const char *name, int mode = out);
	bang_ofstream(FILE *file, int mode = out);
	virtual ~bang_ofstream();
  void close();
  void open (const char *name, int mode = out);

  operator bool() { return (file!=NULL); }

	virtual bang_ostream &operator<<(int i);
	virtual bang_ostream &operator<<(unsigned u);
//	virtual bang_ostream &operator<<(long li);
//	virtual bang_ostream &operator<<(unsigned long lu);
	virtual bang_ostream &operator<<(double g);
	virtual bang_ostream &operator<<(char c);
	virtual bang_ostream &operator<<(const char *s);

private:
	FILE *file;
};

extern bang_ifstream bang_cin;
extern bang_ofstream bang_cout;
extern bang_ofstream bang_cerr;

#endif
