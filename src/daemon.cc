/*
 *	epos/src/daemon.cc
 *	(c) 1998-01 geo@cuni.cz
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
 *	We are running a kind of a cooperative multitasking. The tasks are
 *	represented by "agents"; they get their timeslices using agent::run().
 *	Every task is allowed to run as long as it needs, except if it had
 *	to perform some potentially (noticeably) slow i/o, such as a network
 *	commn or writing to limited sound card buffers. All file descriptors
 *	used for data transfer are marked as non-blocking and the agents
 *	yield control on -EAGAIN. The descriptor is then added to a select
 *	set and the agent gets another timeslice when new data arrives.
 *
 *	Occassionaly, there may be more than one agent with useful work to do.
 *	We queue them using agent::schedule() and select them using sched_sel().
 *	If more than SCHED_SATIATED agents are scheduled (for immediate execution),
 *	the scheduler will stop checking the file descriptors until the situation
 *	gets under control.
 */

#include "epos.h"
#include "client.h"
#include "agent.h"

#define __UNIX__

#ifdef HAVE_SYS_IOCTL_H
	#include <sys/ioctl.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_LINUX_IN_H
	#include <linux/in.h>
#endif

#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif

#ifdef HAVE_WAIT_H
	#include <wait.h>
#endif

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif

#ifdef HAVE_SYS_TERMIOS_H
	#include <sys/termios.h>
#endif

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_QNX_NAME_ATTACH
	#define WITHOUT_EPOS_INCLUDES
	#include "qnxipc.cc"
#else
	inline void qipc_proxy_init() {};
#endif


#ifdef HAVE_WINSOCK
class wsa_init			/* initialise winsock before main() is entered  */
{
   public:
	wsa_init()		/* global constructor */
	{
		char scratch[14227];
		if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch))
			shriek(464, "No winsock");
	};
} wsa_init_instance;

#endif

#ifdef HAVE_WINSVC_H
int start_nt_service();
#endif

const bool is_monolith = false;


#define DARK_ERRLOG 2	/* 2 == stderr; for global stdshriek and stddbg output */

// int session_uid = UID_SERVER;


context *master_context = NULL;
context *this_context = NULL;

context::context(socky int std_in, socky int std_out)
{
	config = cfg;

	if (/* already exists */ master_context) {
		cow_claim();
		cow_configuration(&config);
#ifdef DEBUG_TO_TTSCP
		config->stddbg =  fdopen(dup(std_out), "r+");
#else
		//	config->stddbg =
#endif
#ifdef SHRIEK_TO_TTSCP
		config->stdshriek = fdopen(dup(std_out), "r+");
#else
		//	config->stdshriek =
#endif
	} else {
		D_PRINT(2, "master context OK\n");
		cow_claim();
		this_context = this;
	}

	uid = UID_ANON;
	config->_sd_in =  (int)std_in;
	config->_sd_out = (int)std_out;
	D_PRINT(1, "new context uses fd %d and %d\n", std_in, std_out);

	sgets_buff = (char *)xmalloc(config->max_net_cmd);
	*sgets_buff = 0;
}

context::~context()
{
#ifdef DEBUG_TO_TTSCP
	if (!config->stddbg) shriek(862, "Reclosing in ~context");
	fclose(config->stddbg);
	config->stddbg = NULL;
#else
	// fclose(config->stddbg...
#endif
#ifdef SHRIEK_TO_TTSCP
	if (!config->stdshriek) shriek(862, "Reclosing in ~context");
	fclose(config->stdshriek);
	config->stdshriek = NULL;
#else
	// fclose(config->stdshriek...
#endif
	if (this == this_context) leave();  // shriek(862, "Deleting active context");
	if (this != master_context) cow_unclaim(config);
	free(sgets_buff);
}

void
context::enter()
{
	D_PRINT(1, "enter_context(%p)\n", this);
	if (!this) {
		D_PRINT(2, "(nothing to enter!)\n");
		return;
	}

	if (this_context != master_context) shriek(462, "nesting contexts");

	master_context->config = cfg;
	cfg = config;
	this_context = this;
}

void
context::leave()
{
	if (!this) {
		D_PRINT(2, "(nothing to leave!)\n");
		return;
	}
	if (this_context != this) shriek(462, "leaving unentered context");

	config = cfg;

	cfg = master_context->config;
	this_context = master_context;
	D_PRINT(1, "leave_context(%p)\n", this);
}

void make_rnd_passwd(char *buffer, int size)
{
	int i;
	for (i = 0; i < size; i++) buffer[i] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
			[rand() & 63];
	buffer[i] = 0;
}

#define   SERVER_PASSWD_LEN   14
char server_passwd[SERVER_PASSWD_LEN + 2];

// hash * ttscp_keywords = NULL;

#ifdef HAVE_UNISTD_H
	#define UNIX(x) x
#else
	#define UNIX(x)
#endif


static void detach()
{
	int i;
#ifdef TIOCNOTTY
	ioctl(0, TIOCNOTTY);          //Release the control terminal
#else
	#ifdef HAVE_SETPGRP
		setpgrp();
	#endif
#endif
	if (!scfg->daemon_log || !*scfg->daemon_log)
		return;
	for (i=0; i<3; i++) close(i);
	if (!open(scfg->daemon_log, O_RDWR|O_CREAT|O_APPEND  UNIX( |O_NOCTTY), MODE_MASK)
			|| !open(NULL_FILE, O_RDWR|O_APPEND, MODE_MASK)) {
		for (i=1; i<3; i++) dup(0);
		scfg->colored = false;
		D_PRINT(3, "\n\nEpos restarted at ");
		fflush(stdout);
		system("/bin/date");
	} else /* deep OS level trouble, contact authors */ call_abort();
}

static inline void make_server_passwd()
{
	make_rnd_passwd(server_passwd, SERVER_PASSWD_LEN);
//	D_PRINT(0, "Server internal password is %s\n", server_passwd);
	if (scfg->listen_port != TTSCP_PORT) return;

	FILE *f;
	char *filename = compose_pathname(scfg->server_pwd_file, "");
	if (filename && *filename && (f = fopen(filename, "w", NULL))) {
		UNIX(chmod(filename, S_IRUSR));
		fwrite(server_passwd, SERVER_PASSWD_LEN, 1, f);
		fwrite("\n", 1, 1, f);
		fclose(f);
		free((char *)scfg->server_pwd_file);
		scfg->server_pwd_file = filename;
	}
}

volatile bool server_shutting_down = false;
volatile bool server_reinit_req = false;

void shutdown_agent_queue();

void server_shutdown()
{
	while (ctrl_conns->items) {
		a_ttscp *tmp = ctrl_conns->translate(ctrl_conns->get_random());
		sputs("800 shutdown requested\r\n", tmp->c->config->get__sd_out());
		delete ctrl_conns->remove(tmp->handle);
	}
	if (scfg->server_pwd_file) remove(scfg->server_pwd_file);
	try {
		delete accept_conn;
		delete ctrl_conns;
		delete data_conns;
		delete master_context;
//		cfg->n_langs = 0;	// otherwise the langs point into no man's land
		free(block_table);
		free(push_table);
		shutdown_agent_queue();
		epos_done();
	} catch (any_exception *) {
		D_PRINT(3, "Shutdown didn't complete due to a fatal error.\n");
		abort();
	}
	exit(0);
}

static void server_reinit_check()
{
	if (server_reinit_req) {
		server_reinit_req = false;
		while (ctrl_conns->items) {
			a_ttscp *tmp = ctrl_conns->translate(ctrl_conns->get_random());
			sputs("601 reinit requested\r\n", tmp->c->config->get__sd_out());
			block_table[tmp->c->config->get__sd_out()] = NULL;
			push_table[tmp->c->config->get__sd_out()] = NULL;
			FD_CLR(tmp->c->config->get__sd_out(), &block_set);
			FD_CLR(tmp->c->config->get__sd_out(), &push_set);
			delete ctrl_conns->remove(tmp->handle);
		}
		while (data_conns->items) {
			a_ttscp *tmp = data_conns->translate(data_conns->get_random());
			block_table[tmp->c->config->get__sd_out()] = NULL;
			push_table[tmp->c->config->get__sd_out()] = NULL;
			FD_CLR(tmp->c->config->get__sd_out(), &block_set);
			FD_CLR(tmp->c->config->get__sd_out(), &push_set);
			delete data_conns->remove(tmp->handle);
		}
//		free_all_options();
		delete accept_conn;
		select_fd_max = 0;
		delete master_context;
		master_context = NULL;
		epos_reinit();
		FD_ZERO(&block_set);
		FD_ZERO(&push_set);
		master_context = new context(-1, DARK_ERRLOG);
		accept_conn = new a_accept();
	}
}

static void unix_sigterm(int)
{
	if (server_shutting_down) shriek(466, "forcing shutdown");
	server_shutting_down = true;
}

static void unix_sighup(int)
{
	server_reinit_req = true;
}

static void daemonize()
{
	UNIX(signal(SIGPIPE, SIG_IGN);)		// possibly dangerous
	UNIX(signal(SIGCHLD, SIG_IGN);)		// automatic child reaping
	UNIX(signal(SIGTERM, unix_sigterm);)
	UNIX(signal(SIGHUP, unix_sighup);)
//	UNIX(signal(SIGINT,...);)
//	UNIX(signal(SIGQUIT,...);)
//	UNIX(signal(SIGILL, unix_sigfatal);)
//	UNIX(signal(SIGFPE, unix_sigfatal);)
//	UNIX(signal(SIGSEGV, unix_sigfatal);)
	
	UNIX(chdir("/");)

//	dispatcher_pid = getpid();

	make_server_passwd();

	FD_ZERO(&block_set);
	FD_ZERO(&push_set);
	master_context = new context(-1, DARK_ERRLOG);
	accept_conn = new a_accept();
	qipc_proxy_init();

	data_conns->dupkey = data_conns->dupdata = ctrl_conns->dupkey
		= ctrl_conns->dupdata = false;
}


/*

fd_set data_conn_set;

void update_data_conn_set(char *, socky int *fd)
{
	FD_SET(*fd, &data_conn_set);
}

*/

static void idle()
{
//	FD_ZERO(&data_conn_set);
//	data_conns->forall(update_data_conn_set);

//	while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

static fd_set rd_set;
static fd_set wr_set;

static bool select_socket(bool sleep)
{
	rd_set = block_set;
	wr_set = push_set;
	timeval tv; tv.tv_sec = tv.tv_usec = 0;
	int n;

	n = select(select_fd_max, &rd_set, &wr_set, NULL, &tv);
	if (n > 0) return true;
	if (!sleep) return false;
	if (n < 0 && errno != EINTR) shriek(871, "select() failed");

   restart:
	if (server_shutting_down || server_reinit_req) {
		FD_ZERO(&rd_set); FD_ZERO(&wr_set); return false;
	}

	idle();

	rd_set = block_set;
	wr_set = push_set;

	n = select(select_fd_max, &rd_set, &wr_set, NULL, (timeval *)NULL);
	if (n <= 0) {
		if (n == -1 && errno == EINTR) goto restart;
		shriek(871, n ? "select() failed" : "select() failed to sleep");
	}
	return true;
}

#define SCHED_SATIATED	64
#define SCHED_WARN	30

void notify_invalidate(socky int fd)
{
	FD_CLR(fd, &block_set);
	FD_CLR(fd, &rd_set);
	block_table[fd] = NULL;
	FD_CLR(fd, &push_set);
	FD_CLR(fd, &wr_set);
	push_table[fd] = NULL;
}

void close_and_invalidate(socky int sd)
{
	notify_invalidate(sd);
	async_close(sd);
}

void wakeup(socky int fd, agent **table, fd_set *set)
{
	agent *a = table[fd];
	D_PRINT(2, a ? "Scheduling select(%d)ed agent\n"
			: "sche sche scheduler\n", fd);
	table[fd] = NULL;
	FD_CLR(fd, set);
	a->timeslice();
}


void server()
{
	socky int fd;
	daemonize();
	while (!server_shutting_down) {
		while (runnable_agents > SCHED_SATIATED
					|| runnable_agents && !select_socket(false)) {
			DBG(3, if (runnable_agents > SCHED_WARN) fprintf(STDDBG,"Busy! %d runnable agents\n", runnable_agents);)
			sched_sel()->timeslice();
		}
		select_socket(true);
		for (fd = 0; fd < select_fd_max; fd++) {
			if (FD_ISSET(fd, &wr_set))
				wakeup(fd, push_table, &push_set);
			if (FD_ISSET(fd, &rd_set))
				wakeup(fd, block_table, &block_set);
		}
		server_reinit_check();
	}
	server_shutdown();
}

void server_crashed(char *, a_ttscp *a, int why_we_crashed)
{
	int w = why_we_crashed;
	char code[]= "865";
	if (w / 100 == 8) code[1] = (w % 100) / 10 + '0', code[2] = w % 10 + '0';
	sputs(code, a->c->config->get__sd_out());
	sputs(" shutdown, not your problem\n", a->c->config->get__sd_out());
}

void lest_already_running()
{
	if (running_at_localhost()) {
		if (scfg->server_pwd_file) free((char *)scfg->server_pwd_file);	// FIXME: set_option()
		scfg->server_pwd_file = NULL;
		shriek(872, "Already running\n");
	}
#ifdef ENETUNREACH
	if (errno == ENETUNREACH)
		shriek(871, "Network unreachable\n");
#endif
}

void init_thrown_exception(int errcode)
{
	ctrl_conns->forall(server_crashed, errcode);

	if (scfg->server_pwd_file) remove(scfg->server_pwd_file);
}

int start_unix_daemon()
{
	try {
		epos_init();
		lest_already_running();
		switch (my_fork()) {
			case -1: server();
				 return 0;	/* foreground process */
			case 0:  detach();
				 server();
				 return 0;	/* child  */
			default: UNIX( while (!running_at_localhost() && scfg->init_time--) sleep(1);)
				 return 0;	/* parent */
		}

	} catch (any_exception *e) {
		/* handle all known uncatched exceptions here */
		init_thrown_exception(e->code);
		delete e;
		return 1;
#ifndef NO_CATCHALL_CATCHES
	} catch (...) {
		/* handle all unknown uncatched exceptions here */
		init_thrown_exception(869);
		return 2;
#endif
	}
}

int main(int argc, char **argv)
{
	set_cmd_line(argc, argv);
#ifdef HAVE_WINSVC_H
	int k = start_nt_service();
	if (!k) return 0;
	if (k != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) return k;
#endif
	return start_unix_daemon();
}
