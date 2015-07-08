/*
 *	epos/src/tests/testbench.h
 *	(c) 2005 geo@cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *
 */

#define THIS_IS_A_TTSCP_CLIENT

#include "config.h"	/* You can usually remove this item */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>


#define LITTLE_SPACE	   320
#define MUCH_SPACE	 16000

void init_connection_pair(int i);
const char *get_data_handle(int which);
char *much_data();

void spk_appl(int c, int d, const char *data, int data_len);
void spk_appl(int c, int d, const char *data);
void spk_strm(int c, int d);
void spk_intr(int c, int broken);
char *xscr_strm(int c, int d);
char *xscr_appl(int c, int d, const char *data, int data_len);
char *xscr_appl(int c, int d, const char *data);		// please no unprocessed stuff in c and d.

void generic_command(int c, char *cmd);
void generic_appl(int c, int d, const char *data, int data_len);
int get_result(int c);

int just_connect_socket();
void send_to_epos(char *what, int socket);
void setl(int c, const char *name, const char *value);


/*
 *	These functions never return.  Use them to specify the result of the test.
 */

void shriek(char *txt);		// fail
void shriek(int, char *txt);	// fail
// void testbench_exit();	// success
void follow_server_down(bool whether_success = true);


/*
 * The following symbols should be defined by each test separately:
 */

extern void test_body();
extern const char *test_name;
