#include "testbench.h"

const char *test_name = "hard zero data test";

void test_body()
{
	char *buffer = much_data();
	memset(buffer, 0, MUCH_SPACE);
	buffer[MUCH_SPACE] = 0;

	xscr_strm(0, 0);
	xscr_appl(0, 0, buffer, MUCH_SPACE);
}

