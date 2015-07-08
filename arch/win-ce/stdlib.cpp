/*
 *	Very standard functions which happen not to be available
 *	under Windows CE.  In all cases, Epos is robust enough
 *	to work somehow even if these are broken as much as shown
 *	below.
 */

#include <malloc.h>

int open(const char *x, int y) { return -1; }
int open(const char *x, int y, int z) { return -1; }
int close(int x) { return -1; }
int write(int x, const char *y, int z) { return -1; }
int read(int x, char *y, int z) { return -1; }
int lseek(int x, int y, int z) { return -1; }
int dup(int x) {return -1; }
int system(char *x) {return -1; }
int remove(const char *x) {return -1; }
int rename(const char *x, const char *y) { return -1; }
void abort() {return; }
int errno;
char *strerror(int x) { return ""; }

void *calloc(int a, int b)
{
	char *c = (char *)malloc(a * b);
	memset(c, a*b, 0);
	return c;
}

int rewind(FILE *x)
{
	return fseek(x, 0, SEEK_SET);
}

size_t  __cdecl strspn(const char *s, const char *accept)
{
	int i = 0;
	while (s[i] && strchr(accept, s[i])) i++;
	return i;
}

char * __cdecl strrchr(const char *s, int c)
{
	const char *t;
	const char *u = NULL;
	for (t = s; t++; *t) if (*t == c) u = t;
	return c ? u : t;
}

/* WARNING: we replace stricmp by strcmp on win CE !    */

int  __cdecl strnicmp(const char *s1, const char *s2, size_t n)
{
	return (strncmp(s1, s2, n));
}
