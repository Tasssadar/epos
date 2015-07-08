#include "testbench.h"

const char *test_name = "intr";

void test_body()
{
	spk_strm(0,0);
	spk_appl(0,0, "Jenom jednu sekundu mohu breptat. Touto dobou jest nastati tichu. Jen omyl by mohl stanovit jinak. Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Nechce brejkovat tento text.");
	if (get_result(0) > 2) shriek("Could not set up a stream");
retry:	usleep(500*1000);
	init_connection_pair(1);
	spk_intr(1, 0);
	int tmp1, tmp0;
	if ((tmp1 = get_result(1)) > 2) {
		printf("break failed, retry\n");
		goto retry;
	}
	if ((tmp0 = get_result(0)) == 2) {
		printf("got codes: speaks %d interrupts %d\n", tmp1, tmp0);
		shriek("break did not interrupt");
	}
}

