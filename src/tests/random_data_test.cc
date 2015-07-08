#include "testbench.h"

const char *test_name = "random data";

void test_body()
{
	char *buffer = much_data();
	for (int i = 0; i < LITTLE_SPACE; i++)
		buffer[i] = rand() % 255 + 1;
	buffer[LITTLE_SPACE] = 0;

	xscr_strm(0, 0);
	xscr_appl(0, 0, buffer, LITTLE_SPACE);
}

