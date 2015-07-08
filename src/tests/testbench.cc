/*
 *	epos/src/tests/testbench.cc
 *	(c) 1998-2005 Jirka Hanika <geo@cuni.cz>
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.

 *	This file implements a framework for single purpose TTSCP clients. See
 *	doc/english/ttscp.sgml for a preliminary technical specification.
 *	The Epos developers may use these clients to test a particular
 *	TTSCP implementation on a UNIX platform; using them for other
 *	purposes is discouraged: this client may deliberately violate
 *	the TTSCP specification in order to test the error recovery
 *	procedures of Epos or another TTSCP server.
 */

#include "testbench.h"

#define TTSCP_PORT	8789

const char *COMMENT_LINES = "#;\n\r";
const char *WHITESPACE = " \t\r";

// const char *output_file = "/dev/dsp";

int ctrld[10];
int datad[10];		/* file descriptors for the control and data connections */
char *data = NULL;

//#define SCRATCH_SPACE 16384
//char scratch[SCRATCH_SPACE + 2];

#define CONNECTION_PAIRS	10
#define UNUSED_PAIR		 9

char *chandle[CONNECTION_PAIRS];
char *dhandle[CONNECTION_PAIRS];
bool used[CONNECTION_PAIRS];

const char *testname = "";

const char* get_data_handle(int which)
{
	if (which < 0 || which >= CONNECTION_PAIRS)
		shriek("Invalid data handle index");
	return dhandle[which];
}



int connect_socket(unsigned int, int);
static void stops();

void shriek(char *txt)
{
	if (!*testname) {
		fprintf(stderr, "Client side error: %s\n", txt);
		exit(1);
	}
	fprintf(stderr, "Failed test %s test\n", testname);
	fprintf(stderr, "Reason: %s\n", txt);
	perror("Last error");

	sleep(1);
	testname = "";
	connect_socket(0, TTSCP_PORT);
	stops();
	exit(1);
}

void shriek(int, char *txt)
{
	shriek(txt);
}


#define EPOS_COMMON_H	// this is a lie
#include "client.cc"
//#define sputs(x,y) {printf("Sent to %d: %s\n", y, x); sputs(x,y);}

int get_result(int c)
{
	char *mess;

	while (sgets(scratch, scfg->scratch_size, ctrld[c])) {
		scratch[scfg->scratch_size] = 0;
//		printf("[%-20s] Received on %d: %s\n", testname, c, scratch);
		mess = scratch+strspn(scratch, "0123456789x ");
		switch(*scratch) {
			case '1': continue;
			case '2': return 2;
			case '3': break;
			case '4': if (*mess && strcmp(mess, "interrupted")) printf("%s\n", mess);
				  return 4;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
//					exit(0);
					return 2;
				  } /* else fall through */
			case '8': if (*mess) printf("%s\n", mess);
				  follow_server_down(false);	  // never returns
			case '5':
			case '7':
			case '9':
			case '0': printf("%s\n", scratch); shriek("Unhandled response code");
			default : ;
		}
		if (*mess) printf("%s\n", mess);
	}
	return 8;	/* guessing */
}

void generic_command(int c, char *cmd)
{
	if (c < 0 || c > CONNECTION_PAIRS)
		shriek("Invalid index");
	sputs(cmd, ctrld[c]);
	sputs("\r\n", ctrld[c]);
}

void generic_appl(int c, int d, const char *data, int data_len)
{
	sputs("appl ", ctrld[c]);
	sprintf(scratch, "%d", data_len);
	sputs(scratch, ctrld[c]);
	sputs("\r\n", ctrld[c]);
	ywrite(datad[d], data, data_len);
}

void spk_strm(int c, int d)
{
	sputs("strm $", ctrld[c]);
	sputs(dhandle[d], ctrld[c]);
	sputs(":raw:rules:diphs:synth:#localsound", ctrld[c]);
	sputs("\r\n", ctrld[c]);
}

void spk_appl(int c, int d, const char *data, int data_len)
{
	generic_appl(c, d, data, data_len);
}

void spk_appl(int c, int d, const char *data)
{
	spk_appl(c, d, data, (int)strlen(data));
}

void spk_intr(int c, int broken)
{
	sputs("intr ", ctrld[c]);
	sputs(chandle[broken], ctrld[c]);
	sputs("\r\n", ctrld[c]);
}


char *xscr_strm(int c, int d)
{
	int s = ctrld[c];
	sputs("strm $", s);
	sputs(dhandle[d], s);
	sputs(":raw:rules:print:$", s);
	sputs(dhandle[d], s);
	sputs("\r\n", s);
	if (get_result(c) > 2) shriek("Could not set up the stream");

}

char *get_data(int c, int d)
{
	char *b = NULL;
	int size = 0;
	while (sgets(scratch, scfg->scratch_size, ctrld[c])) {
		scratch[scfg->scratch_size] = 0;
		if (strchr("2468", *scratch)) { 	/* all done, write result */
			if (*scratch != '2') shriek(scratch);
			if (!size) shriek("No processed data received");
			b[size] = 0;
			return b;
		}
		if (!strncmp(scratch, "123 ", 4)) {
			int count;
			sgets(scratch, scfg->scratch_size, ctrld[c]);
			scratch[scfg->scratch_size] = 0;
			sscanf(scratch, "%d", &count);
			b = size ? (char *)realloc(b, size + count + 1) : (char *)malloc(count + 1);
			int limit = size + count;
			while (size < limit)
				size += yread(datad[d], b + size, limit - size);
		}
	}
	if (size) shriek("Disconnect during transmit");
	else shriek("Disconnect before transmit");
	return NULL;
}


char *xscr_appl(int c, int d, const char *data, int data_len)
{
	generic_appl(c, d, data, data_len);
	return get_data(c, d);
}

char *xscr_appl(int c, int d, const char *data)
{
	return xscr_appl(c, d, data, (int)strlen(data));
}



char *much_data()
{
	char *buffer = (char *)malloc(MUCH_SPACE + 1024);
	return buffer;
}

void setl(int c, const char *name, const char *value)
{
	xmit_option(name, value, ctrld[c]);
}

int just_connect_socket()
{
	return just_connect_socket(0, TTSCP_PORT);
}

void send_to_epos(char *what, int socket)
{
	sputs(what, socket);
}


void perform_test()
{
	testname = test_name;
	test_body();
	testname = "";
}

#define strfy(x) #x
#define stringify(x) strfy(x)

char * const exec_argv[] = {
	"eposd",
	"--forking=off",
	"--listen_port=" stringify(TTSCP_PORT),
	"--debug_password=make_check",
	"--base_dir=" stringify(SOURCEDIR) "/../../cfg",
	"--language=czech",
	"--voice=kubec-vq",
	NULL
};
char * const exec_envp[] = {
	NULL
};

int eposd_pid = 0;
int patience = 40;

void init_winsock()
{
#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
#endif
}

void init_server()
{
	if (just_connect_socket() == -1) {
		if ((eposd_pid = fork())) do usleep(250000); while (just_connect_socket() == -1 && --patience);
		else execve("../eposd", exec_argv, exec_envp);
	}
}

void init_connection_pair(int i)
{
	ctrld[i] = connect_socket(0, TTSCP_PORT);	// This will be the data connection...
	dhandle[i] = get_handle(ctrld[i]);
	sputs("data ", ctrld[i]);
	sputs(chandle[UNUSED_PAIR], ctrld[i]);
	sputs("\r\n", ctrld[i]);
	if (get_result(i) > 2) shriek("Couldn't data");
	datad[i] = ctrld[i];				// ...no sooner, as get_result() doesn't work on data conns
	ctrld[i] = connect_socket(0, TTSCP_PORT);	// This will be the control connection.
	chandle[i] = get_handle(ctrld[i]);
	used[i] = true;
}

void init()
{
	init_winsock();
	init_server();

	ctrld[UNUSED_PAIR] = connect_socket(0, TTSCP_PORT);
	chandle[UNUSED_PAIR] = get_handle(ctrld[UNUSED_PAIR]);

	for (int i = 0; i < CONNECTION_PAIRS; i++) {
		used[i] = false;
	}
	init_connection_pair(0);
}

void cleanup()
{
	testname = "closing everything";

	for (int i = 0; i < UNUSED_PAIR; i++) {
		if (used[i]) {
			sputs("delh ", ctrld[i]);
			sputs(dhandle[i], ctrld[i]);
			sputs("\r\ndone\r\n", ctrld[i]);
			if (get_result(i) > 2) shriek("Could not delete a data connection handle");
			if (get_result(i) > 2) shriek("Could not shut down a control connection");
			close(datad[i]);
			close(ctrld[i]);
		}
	}
	close(ctrld[UNUSED_PAIR]);
}

#ifdef HAVE_GETTIMEOFDAY

#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif

static struct timeval start, stop;
static void starts()
{
	if (gettimeofday(&start, NULL)) shriek("profiler fails");
}

static void stops()
{
	if (gettimeofday(&stop, NULL)) shriek("profiler fails");
	int duration = stop.tv_sec - start.tv_sec;
	duration *= 1000000;
	duration += stop.tv_usec - start.tv_usec;
	printf("%8ldms  ", duration / 1000);
}

#else		// HAVE_GETTIMEOFDAY

static void starts() {};
static void stops() {};

#endif		// HAVE_GETTIMEOFDAY



int main(int argc, char **argv)
{
	if (argc != 1) shriek("No arguments allowed");

	starts();

	init();
	perform_test();
	cleanup();

	stops();

	return 0;
}

void testbench_exit(bool success = true)
{
	stops();
	exit(success ? 0 : 3);
}

void follow_server_down(bool success)
{
	int s;
	do {
		s = just_connect_socket();
		if (s == -1) {
			usleep(30 * 1000);
			testbench_exit(success);
		}
		close(s);
		usleep(100 * 1000);
	} while (1);
}




#ifndef HAVE_TERMINATE

void terminate(void)
{
	abort();
}

#endif

