#include "testbench.h"

const char *test_name = "speak";

void test_body()
{
	spk_strm(0,0);
	spk_appl(0,0, "Raz");
	spk_appl(0,0, "Dva");
	if (get_result(0) > 2) shriek("Could not set up a stream");
	if (get_result(0) > 2) shriek("Could not apply a stream to the first text");
	if (get_result(0) > 2) shriek("Could not apply a stream to the second text");
}
