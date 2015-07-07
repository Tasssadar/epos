/*
 *	epos/src/tcpsyn.cc
 *	(c) 1998-99 Jirka Hanika, geo@cuni.cz
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
 *
 */

#include "epos.h"
#include "tcpsyn.h"
#include "client.h"

#ifdef HAVE_IO_H
	#include <io.h>		/* open, write, (ioctl,) ... */
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif

// #include <netdb.h>

#define LANGNAMESEP	'.'
#define SERVERNAMESEP	'@'
#define TCPPORTSEP	':'

#define ZERO_DEADLK_TIMEOUT	100000		// in usec, if 0 seconds is specified

/*
 *	tcpsyn_appl() sends an apply command and blocks until it is processed.
 *	It reads and returns the data sent by the server.  The size argument
 *	is actually an output argument: the number of bytes written into the
 *	buffer returned.  The buffer should be freed with free() by the caller.
 *
 *	This function should be called only if the strm command uses datad
 *	as both the input and the output module.
 */

void *tcpsyn_appl(int bytes, int ctrld, int datad, int *size)
{
	char *rec = NULL;
	int offset = 0;
	int bs = 0;
	int sum = 0;

	sputs("appl ", ctrld);
	sprintf(scratch, "%d", bytes);
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	do {
		sgets(scratch, scfg->scratch_size, ctrld);
		if (!strncmp("122 ", scratch, 4)) {
			sgets(scratch, scfg->scratch_size, ctrld);
			sscanf(scratch, " %d", &bytes);
			if (bs != bytes) rec = rec ? (char *)xrealloc(rec, bytes)
						   : (char *)xmalloc(bytes);
		}
		if (!strncmp("123 ", scratch, 4)) {
			sgets(scratch, scfg->scratch_size, ctrld);
			sscanf(scratch, " %d", &bytes);
			sum += bytes;
			if (sum > bs) rec = rec ? (char *)xrealloc(rec, sum)
						: (char *)xmalloc(sum);
			bytes = yread(datad, rec + offset, sum - offset);
			if (bytes == -1) {
				if (errno == EAGAIN || errno == EINTR) bytes = 0;
				else shriek(473, "Connection lost");
			}
			offset += bytes;
		}
	} while (!strchr("2468", scratch[0]));
	if (scratch[0] != '2' && scratch[1] != '0') shriek(475, "Remote returned %.3s for appl", scratch);
	while (sum - offset) {
		bytes = yread(datad, rec + offset, sum - offset);
		if (bytes == -1) {
			if (errno == EAGAIN || errno == EINTR) bytes = 0;
			else shriek(473, "Connection lost");
		}
		offset += bytes;
	}
	*size = offset;
	return rec;

}

/*
 *	For tcpsyn, location denotes the remote server name and port
 */

static int tcpsyn_connect_socket(unsigned int ipaddr, int port)
{
	socky int sd = just_connect_socket(ipaddr, port);
	if (sd == -1) {
		shriek(473, "Server unreachable\n");
	}

	fd_set fds; FD_ZERO(&fds); FD_SET(sd, &fds);
	timeval tv; tv.tv_sec = cfg->deadlock_timeout; tv.tv_usec = ZERO_DEADLK_TIMEOUT;
	if (select(sd+1, &fds, NULL, NULL, &tv) < 1) shriek(476, "Timed out - tcpsyn deadlock");

	sgets(scratch, scfg->scratch_size, sd);
	if (strncmp(scratch, "TTSCP spoken here", 18)) {
		scratch[15] = 0;
		shriek(474, "Protocol not recognized");
	}
	return sd;
}

static inline void tcpsyn_chk_cmd(int cd, const char *tag, const char *par)
{
	int err = sync_finish_command(cd);
	if (err) shriek(475, "Remote returned %d for %s %s", err, tag, par);
}

static inline void tcpsyn_send_cmd(int cd, const char *tag, const char *par)
{
	sputs(tag, cd);
	sputs(" ", cd);
	sputs(par, cd);
	sputs("\r\n", cd);
	tcpsyn_chk_cmd(cd, tag, par);
}

tcpsyn::tcpsyn(voice *v)
{
	int port;
	char *langname;
	char *voicename;
	char *remote_server = strdup(v->location);
	if (remote_server[0] != SERVERNAMESEP && remote_server[0] != LANGNAMESEP)
		voicename = remote_server;
	else voicename = NULL;
	char *serv_id = strchr(remote_server, SERVERNAMESEP);
	if (serv_id) {
		*serv_id++ = 0;
		langname = strchr(remote_server, LANGNAMESEP);
		if (langname) *langname++ = 0;
		else langname = (char *)v->parent_lang->name;	// const cast
	} else {
		serv_id = remote_server;
		langname = (char *)v->parent_lang->name;		// const cast
	}
	char *port_id = strchr(serv_id, TCPPORTSEP);
	if (port_id) {
		*port_id++ = 0;
		sscanf(port_id, "%i", &port);
	} else port = TTSCP_PORT;

	unsigned int a = getaddrbyname(serv_id);
	if (a == -1) shriek(472, "Unknown remote tcpsyn server");
	
	cd = tcpsyn_connect_socket(a, port);
	dd = tcpsyn_connect_socket(a, port);
	D_PRINT(1, "tcpsyn uses port %d ctrl fd %d data fd %d\n", port, cd, dd);

	char *ctrl_handle = get_handle(cd);
	sputs("data ", dd);
	sputs(ctrl_handle, dd);
	sputs("\r\n", dd);
	free(ctrl_handle);
	handle = get_handle(dd);
	tcpsyn_chk_cmd(dd, "data", ctrl_handle);

	sputs("strm $", cd);
	sputs(handle, cd);
	sputs(":synth:$", cd);
	sputs(handle, cd);
	sputs("\r\n", cd);
	tcpsyn_chk_cmd(cd, "strm", "");

	tcpsyn_send_cmd(cd, "setl language", langname);

	if (voicename)
		tcpsyn_send_cmd(cd, "setl voice", voicename);
	free(remote_server);
	D_PRINT(1, "tcpsyn initialised\n");
}

tcpsyn::~tcpsyn()
{
	int err;

	sputs("delh ", cd);
	sputs(handle, cd);
	sputs("\r\ndone\r\n", cd);
	err = sync_finish_command(cd);
	if (err) shriek(475, "Remote returned %d for delh", err);
	err = sync_finish_command(cd);
	if (err) shriek(475, "Remote returned %d for done", err);
	close_and_invalidate(cd);
	close_and_invalidate(dd);
	free(handle);
}

void
tcpsyn::synseg(voice *, segment, wavefm *)
{
	shriek(861, "abstract tcpsyn::synseg");
}

void
tcpsyn::synsegs(voice *, segment *d, int count, wavefm *w)
{
	int size;
	void *b;

	ywrite(dd, d - 1,sizeof(segment) * ++count);
	b = tcpsyn_appl(sizeof(segment) * count, cd, dd, &size);
	if (!b) {
		shriek(471, "Remote server returned zero bytes to the local server");
	}
	w->become(b, size);
	free(b);
}


