#include "testbench.h"

const char *test_name = "vogon";

struct test_case
{
	char *input;
	char *output;
};

test_case tests[] = {
	{ "o", "oD~" },
	{ "o ok o", "oD~okCoD~" },
	{ "o ok ok o", "oDokC~okCoD~" },
	{ "uL|uB uL", "uAuB uA" },
	{ "finol", "finolC~" },
	{ NULL, NULL }
};



void test_body()
{
	bool error = false;

	setl(0, "language", "vogon");
	if (get_result(0) > 2) {
		shriek("Test language not configured");
	}

	xscr_strm(0, 0);

	for (int i = 0; tests[i].input; i++) {
		char * input = tests[i].input;
		char * expected = tests[i].output;
		char * output = xscr_appl(0, 0, input);
		if (strcmp(expected, output)) {
			printf("Received \"%s\" while expecting \"%s\"\n", output, expected);
			error = true;
		}
	}
	if (error) shriek("Got unexpected outputs");
}

