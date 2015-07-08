#include "testbench.h"

const char *test_name = "long data";

#define EVEN_MORE_SPACE 20000		// Well, not that much data.

#define TEXT "Slovo jako slon."
#define SIZE strlen(TEXT)

void test_body()
{
	char *buffer = (char *)malloc(EVEN_MORE_SPACE + 1024);

	memset(buffer, 'A', EVEN_MORE_SPACE);
	for (int i = 0; i < EVEN_MORE_SPACE + SIZE; i += SIZE) {
		strcpy(buffer + i, TEXT);
	}
	buffer[EVEN_MORE_SPACE] = 0;

	xscr_strm(0, 0);
	xscr_appl(0, 0, buffer);
}

