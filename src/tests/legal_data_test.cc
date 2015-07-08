#include "testbench.h"

const char *test_name = "legal data";

void test_body()
{
	char *buffer = much_data();
	memset(buffer, 'a', LITTLE_SPACE);
	buffer[LITTLE_SPACE] = 0;


	xscr_strm(0, 0);
	xscr_appl(0, 0, buffer, MUCH_SPACE);
}

