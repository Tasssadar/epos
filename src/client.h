/*
 *	(c) 1998-01 Jirka Hanika <geo@cuni.cz>
 *
 *	This single source file src/client.h, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are covered strictly by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precendence.
 *
 *	This file implements supporting functions for a simple TTSCP client.
 *	See doc/english/ttscp.doc for a preliminary technical specification.
 *
 *	Note: the functions declared here often trash the scratch buffer.
 */

#ifndef EPOS_CLIENT_H
#define EPOS_CLIENT_H

#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
	#define HAVE_WINSOCK
	#define socky unsigned
#else
	#define socky signed
#endif

/*
 *	The just_connect_socket() routine returns -1 if it cannot return a connected
 *	socket. The connect_socket() routine additionally checks if the remote side
 *	announces the TTSCP protocol of an acceptable version and calls shriek(4xx)
 *	if it doesn't.  Use 0 for address for localhost, 0 for port to attempt to
 *	locate a public TTSCP server if no local one can be found.
 *
 *	Byte order: host byte order for the port number, network byte order for the addr.
 *	This is because the address has typically been acquired through gethostbyname(),
 *	while the port number is probably constant or directly specified by the user.
 */

int just_connect_socket(unsigned int ipaddr, int port);	// returns -1 if not connected
int connect_socket(unsigned int ipaddr, int port);	// as above, check protocol, shriek if bad
bool running_at_localhost();

/*
 *	getaddrbyname() converts an Internet host name (or address in the dotted
 *	format) to the host byte order IP address.
 */

int getaddrbyname(const char *inet_name);

/*
 *	xmit_option() send the "set" command appropriate for setting a named
 *	option to a specified value. 
 */
void xmit_option(const char *name, const char *value, int sd);

/*
 *	get_handle() should be called before any TTSCP command is issued.
 *	It returns strdup(connection handle) after having skipped protocol
 *	configuration sent by the server.
 */

char *get_handle(int sd);

/*
 *	sync_finish_command() can be used to wait for the completion code
 *	for a command. It will block until it is received.
 *	Returns zero if a successful reply has been received;
 *	otherwise the error code received is returned.
 *	The 649 error code (never issued in TTSCP) implies
 *	a broken connection due to end of file or error condition,
 *	such as a network disconnection.
 *
 *	If you are looking for a more complex client-side apply command
 *	handling example, one where the data produced by the server
 *	is also correctly received via a data connection, see
 *	tcpsyn_appl() in tcpsyn.cc
 */

int sync_finish_command(int ctrld);	// wait for the completion code

/*
 *	async_close() function is equivalent to close(), except that
 *	it returns immediately (and without a return value, which may
 *	be still unknown at the moment). Doing the close is left to
 *	a child process; as soon, as the child holds the only file
 *	descriptor copy in question, it is killed, which implies
 *	closing its file descriptors.
 *	
 *	yread() and write() are wrappers for read() and write(),
 *	respectively.  On non-UNIX systems (as guessed by the absence
 *	of the unistd.h file) these functions try calling recv or send
 *	before the read or write.  This behavior is necessary with
 *	the poor clumsy windows sockets, but unistd.h doesn't
 *	necessarily imply UNIX, Windows may not necessarily imply
 *	broken sockets, and the absence of unistd.h doesn't imply
 *	anything.  It would be nice to know whether one can
 *	read/write sockets under OS/2 or Hurd.
 */

#ifdef HAVE_UNISTD_H

	#include <unistd.h>

	#ifdef HAVE_SIGNAL_H
		#include <signal.h>
	#endif

	#ifdef HAVE_ERRNO_H
		#include <errno.h>
	#endif

	inline void async_close(int fd)
	{
		int pid = scfg->asyncing ? fork() : -1;
		switch(pid)
		{
			case -1:
				if(close(fd)) shriek(465,"Error on close()");
				return;
			case 0:
				sleep(1800);	/* will be killed by the parent soon */
				abort();	/* hopefully impossible */
			default:
				if(close(fd)) shriek(465, "Error on close()");
				kill(pid, SIGKILL);
		}
	}

	inline int ywrite(int fd, const void *buffer, int size)
	{
		return write(fd, (const char *)buffer, size);
	}

	inline int yread(int fd, void *buffer, int size)
	{
		return read(fd, (char *)buffer, size);
	}

	#if defined(HAVE_WINSOCK2_H) || defined(HAVE_WINSOCK_H)
		#error Funny - UNIX does not rhyme with winsock
	#endif

#else
	#ifdef HAVE_WINSOCK2_H
		#include <winsock2.h>
	#else
		#ifdef HAVE_WINSOCK_H
			#include <winsock.h>
		#endif
	#endif
	

	#ifdef HAVE_IO_H
		#include <io.h>
	#endif

	#ifdef HAVE_ERRNO_H
		#include <errno.h>
	#endif

	inline void async_close(int fd)
	{
		if (close(fd) && closesocket(fd)) shriek(465,"Error on close()");
		return;
	}

	inline int ywrite(int fd, const void *buffer, int size)
	{
		int result = send(fd, (const char *)buffer, size, 0);
		if (result == -1) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				errno = EAGAIN;
				return -1;
			}
			return write(fd, (const char *)buffer, size);
		}
		return result;
	}

	inline int yread(int fd, void *buffer, int size)
	{
		int result = recv(fd, (char *)buffer, size, 0);
		if (result == -1) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				errno = EAGAIN;
				return -1;
			}
			return read(fd, (char *)buffer, size);
		}
		return result;
	}

#endif	// HAVE_UNISTD_H

/*
 *	close_and_invalidate() is NOT supplied in client.cc and
 *	may only be used by server-side code, or the client side
 *	code can define it to call just async_close.  The purpose
 *	of close_and_invalidate() is to remove all references to
 *	the descriptor from the scheduler before closing it.
 */
 
void close_and_invalidate(socky int sd);

/*
 *	The sgets() and sputs() routines provide a slow get line and put line
 *	interface, especially on TTSCP control connections. They are suitable
 *	for the client. sgets blocks until a line is received, which is a problem for
 *	tcpsyn. sputs can be replaced by a callback fn assigned to sputs_replacement,
 *	which is what the server side does to make it non-blocking.
 */

extern int (*sputs_replacement)(socky int sd, const char *, int);

int sgets(char *buffer, int buffer_size, int sd);
int sputs(const char *buffer, int sd);


#endif		// EPOS_CLIENT_H

