/*
 *	epos/src/nonblock.cc
 *	(c) 2002 geo@cuni.cz
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

#include "epos.h"
#include "client.h"
#include "agent.h"

/*
 *	class a_replier handles control connection congestion.
 *	It is constructed at the moment of the first partial
 *	or EWOULDBLOCK/EAGAIN write in async_sputs().
 *
 *	As soon as the a_replier is attached to a control connection,
 *	it is never released and imposes its overhead over it forever.
 *
 *	There is no real flow control with repliers, so that their
 *	activity can become a memory hog.
 */

class a_replier : public agent
{
	int sd;
	char *buffer;
	int len;
	virtual const char *name() { return "replier"; };
	virtual void run();
   public:
	a_replier(socky int s);
	virtual ~a_replier();
	void write(const char *buffer, int len);
};

a_replier **replier_table = (a_replier **)xmalloc(1);
socky int n_repliers = 0;

a_replier::a_replier(socky int s) : agent(T_NONE, T_NONE)
{
	buffer = NULL;
	len = 0;
	sd = s;
}

a_replier::~a_replier()
{
	if (buffer) free(buffer);
	buffer = NULL;
	replier_table[sd] = NULL;
}

void
a_replier::run()
{
	int result = ywrite(sd, buffer, len);
	if (result == len || result == -1 && errno == EPIPE) {
		len = 0;
		free(buffer);
		buffer = NULL;
		return;
	}
	if (result == -1) {
		push(sd);
		return;
	}
	char *tmp = (char *)xmalloc(len - result);
	memcpy(tmp, buffer + result, len - result);
	free(buffer);
	buffer = tmp;
	len -= result;
	push(sd);
}

void
a_replier::write(const char *str, int l)
{
	if (!buffer) buffer = (char *)xmalloc(l);
	else buffer = (char *)xrealloc(buffer, len + l);

	memcpy(buffer + len, str, l);
	len += l;
	D_PRINT(0, "Replier stretches the buffer to %d bytes\n", len);

	if (len == l) push(sd);
}



int async_sputs(socky int sd, const char *buffer, int len)
{
	if (sd < 0) return -1;
	if (sd < n_repliers && replier_table[sd]) {
		replier_table[sd]->write(buffer, len);
		return len;
	}
	int result = ywrite(sd, buffer, len);
	if (result == len) return len;
	if (result == -1 && errno == EAGAIN)
		result = 0;
	if (result != -1) {
		if (sd >= n_repliers) {
			replier_table = (a_replier **)xrealloc(replier_table, (sd + 1) * sizeof(a_replier *));
			for (socky int i = n_repliers; i < sd; i++)
				replier_table[sd] = NULL;
			n_repliers = sd + 1;
		}
		replier_table[sd] = new a_replier(sd);
		replier_table[sd]->write(buffer + result, len - result);
		return len;
	}
#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
        if (errno == EPIPE || errno == WSAECONNRESET) {
#else
	if (errno == EPIPE || errno == ECONNRESET) {
#endif
		return -1;
	}
	shriek(861, "sputs failed in an unknown way");
	return -1;
}

void use_async_sputs()
{
	sputs_replacement = &async_sputs;
}

void free_replier_table()
{
	for (socky int i = 0; i < n_repliers; i++)
		delete replier_table[i];
	free(replier_table);
}
