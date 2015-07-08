#include "testbench.h"

const char *test_name = "long stream";

void test_body()
{
	char *buffer = (char *)malloc(MUCH_SPACE + 1024);
	strcpy(buffer, "strm $");
	strcat(buffer, get_data_handle(0));
	int b = strlen(buffer);
	for (int i = 0; i < MUCH_SPACE; i++) {
		buffer[b + i] = ":raw:print"[i % 10];
	}
	strcat (buffer + MUCH_SPACE, ":$");
	strcat (buffer + MUCH_SPACE, get_data_handle(0));
	generic_command(0, buffer);

	spk_appl(0,0,"Identita");

	if (get_result(0) > 2) shriek("Could not set up the long stream");
	if (get_result(0) > 2) shriek("Could not apply long stream to trivial data");
}

