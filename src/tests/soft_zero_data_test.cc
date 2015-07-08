#include "testbench.h"

const char *test_name = "soft zero data";

void test_body()
{
	char *buffer = much_data();
	memset(buffer, '0', LITTLE_SPACE);
	buffer[LITTLE_SPACE] = 0;

	xscr_strm(0, 0);
	xscr_appl(0, 0, buffer, LITTLE_SPACE);
}

