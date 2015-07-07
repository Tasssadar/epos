/*
 *	epos/src/qnxipc.cc
 *	(c) 1999 geo@cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.

	This implements a QNX IPC proxy.  It is impossible to
	encapsulate the connection-oriented TTSCP in the
	datagram-oriented model of QNX interprocess communication,
	but it is possible to use TTSCP internally to synthesize
	any text received over the IPC.  Unless you use the QNX
	operating system with the right header files in the
	right place, this file will not compile and link in at all.
 *
 */

#ifndef WITHOUT_EPOS_INCLUDES
	#include "epos.h"
	#include "client.h"
	#include "agent.h"
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_SYS_KERNEL_H
	#include <sys/kernel.h>
#endif

#ifdef HAVE_SYS_NAME_H
	#include <sys/name.h>
#endif

#define QIPC_PROXY_BUFFER_SIZE	65536


void qipc_proxy_crashed(char *reason)
{
	FILE *h = fopen("epos.err","a");
	fprintf(h, "qipc_proxy_crashed: ");
	fprintf(h, reason);
	fprintf(h, "\n");
	fclose(h);

	call_abort();
}

static inline void sync_finish_command(int fd, char *reason)
{
	if (sync_finish_command(fd)) qipc_proxy_crashed(reason);
}

void qipc_proxy_setup(int commands, int cmd_back, int data, int data_back)
{
	char *ctrlh = get_handle(cmd_back);
	char *datah = get_handle(data_back);

	sprintf(scratch, "data %s\n", ctrlh);
	sputs(scratch, data);
	sync_finish_command(data_back, "data cmd");

	sprintf(scratch, "strm $%s:raw:rules:diphs:synth:#localsound\n", datah);
	sputs(scratch, commands);
	sync_finish_command(cmd_back, "strm cmd");

	free(ctrlh);
	free(datah);
}

void qipc_proxy_run(int cmds, int back, int data)
{
	int client;
	char buff[QIPC_PROXY_BUFFER_SIZE];

	if (qnx_name_attach(0, "epos-spk-proxy") == -1)
		qipc_proxy_crashed("qnx_name_attach");
	while (1) {
		client = Receive(0, buff, QIPC_PROXY_BUFFER_SIZE);
		if (client == -1)
			qipc_proxy_crashed("receive");
		write(data, buff, strlen(buff));	// really strlen?
		sputs("appl ", cmds);
		sprintf(scratch, "%d\r\n", strlen(buff));
		sputs(scratch, cmds);
		sync_finish_command(back, "appl cmd");
		strcpy(buff, "OK");
		if (Reply(client, buff, strlen(buff)) == -1)
			qipc_proxy_crashed("reply");
	}
}

void qipc_proxy_init()
{
	int cmds[2];
	int bcmd[2];
	int data[2];
	int bdat[2];

	pipe(cmds);
	pipe(bcmd);
	pipe(data);
	pipe(bdat);
	switch(fork()) {		/* even if --forking is off */
		case -1: qipc_proxy_crashed("fork");
		case  0: close(cmds[0]); close(bcmd[1]); close(data[0]); close(bdat[1]);
			 qipc_proxy_setup(cmds[1], bcmd[0], data[1], bdat[0]);
			 qipc_proxy_run(cmds[1], bcmd[0], data[1]);
			 qipc_proxy_crashed("strange");
	}
	close(cmds[1]);
	close(bcmd[0]);
	close(data[1]);
	close(bdat[0]);
	unuse(new a_ttscp(cmds[0], bcmd[1]));
	unuse(new a_ttscp(data[0], bdat[1]));
}

