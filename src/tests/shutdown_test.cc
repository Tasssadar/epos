#include "testbench.h"

const char *test_name = "server shutdown";

void test_body()
{
	int s = just_connect_socket() == -1;
	if (s == -1) {
		shriek("Server not running");
	} else {
		generic_command(0, "pass make_check");
		generic_command(0, "down");
		if (get_result(0) > 2) {
			shriek("Failed to authenticate");
		}
		close(s);
	}
	follow_server_down();
}

