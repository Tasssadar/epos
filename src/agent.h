/*
 *	epos/src/agent.h
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
 */

#ifdef HAVE_UNISTD_H			// fork() only
	#include <unistd.h>	// in DOS, fork(){return -1;} is supplied in interf.cc
#endif

#ifdef HAVE_SYS_SELECT_H
	#include <sys/select.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_LINUX_IN_H
	#include <linux/in.h>
#endif

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

/*
 *	A context is a set of values for a few global variables. It is possible
 *	to switch the contexts, thus maintaining more simultaneous configurations
 *	(e.g. for unrelated active connections). Every agent has a context;
 *	a stream of connected agents may share a context.
 */

class context
{
   public:
	int uid;

	configuration *config;
//	lang *this_lang;
//	voice *this_voice;

	char *sgets_buff;	/* not touched upon context switch */

	context(socky int std_in, socky int std_out);
	~context();
	void enter();
	void leave();
};


/*
 *	An agent is "something that should work on something". Synonyms: task, process.
 *	The methods are in agent.cc; the agents are scheduled in server() in daemon.cc.
 */

class stream;

enum DATA_TYPE {T_NONE, T_INPUT, T_TEXT, T_STML, T_UNITS, T_SEGS, T_SSIF, T_WAVEFM};

struct pend_ll
{
	void *d;
	pend_ll *next;
	pend_ll(void *id, pend_ll *inext) { d = id; next = inext; };
};

// void server();	// friend decl

class agent
{
	friend class stream;
	friend void server();

	virtual void run() = 0;	/* run until out of input			*/
   protected:
	void *inb;
//	void *outb;
	pend_ll *pendout;
	pend_ll *pendin;
	int pendcount;
	agent *next;
	agent *prev;
	agent *dep;
	virtual bool mktask(int size);	/* process <size> data from input to output	*/
	virtual void finis(bool err);	/* tell the stream apply() has finished	*/
	void schedule();	/* add this agent to the run queue	*/
	void block(socky int fd);/* schedule other agents until fd has more data */
	void push(socky int fd);/* schedule other agents until fd can absorb more data */
	void unquench();	/* no more queue overfull */
	void relax();		/* free inb and outb properly		*/
	void pass(void *);	/* called with data to be passed along	*/
   public:
	virtual const char * name() = 0;
	context *c;
	DATA_TYPE in;
	DATA_TYPE out;
	agent(DATA_TYPE typein, DATA_TYPE typeout);
	virtual ~agent();
	virtual bool brk();	/* cancel your work to do, forget inb/outb, return whether brk did something	*/
	void timeslice();	/* switch context and agent::run()	*/
};

/*
 *	A stream is a linked list of agents, one of them being the
 *	stream agent itself. stream->head is an input agent.
 *	stream->next links the agents together; a linked list
 *	starting with next->pendin and ending with this->pendout
 *	may contain pending data to be processed by next.
 *	The length of this linked list must correspond to 
 *	next->pendcount, which is used for flow control.
 */

class stream : public agent
{
	agent *callbk;	/* variable */
	agent *head;	/* fixed    */
	virtual void run();
	virtual const char* name() { return "stream"; };
	virtual void finis(bool err);
   public:
	stream(char *, context *);
	virtual ~stream();
	virtual bool brk();
	void apply(agent *ref, int bytes);
	void release_agents();
	bool foreground() {return callbk ? true : false; };
};

class a_protocol : public agent
{
	char *sgets_buff;
	char *buffer;
	virtual void run();
	virtual int run_command(char *) = 0;	// returns: reschedule, terminate or ignore
   public:
	a_protocol();
	virtual ~a_protocol();
	virtual void disconnect() = 0;	// destructor, executes delayed. Also cleanup.
};

class a_ttscp : public a_protocol
{
	virtual int run_command(char *);
	virtual const char *name() { return "ttscp"; };
   public:
	a_ttscp *ctrl;
	hash_table<char, a_ttscp> *deps;
	char *handle;
	a_ttscp(socky int sd_in, socky int sd_out);
	virtual ~a_ttscp();
	virtual bool brk();
	virtual void disconnect();
};

class a_accept : public agent
{
	virtual void run();
	virtual const char *name() { return "accept"; };
	sockaddr_in ia;
	socky int listening;
   public:
	a_accept();
	virtual ~a_accept();
};

extern int runnable_agents;

agent *sched_sel();

extern agent **block_table;
extern agent **push_table;
extern fd_set block_set;
extern fd_set push_set;
extern socky int select_fd_max;

inline int my_fork()
{
	if (!scfg->forking) return -1;
	else return fork();
}

void reply(const char *message);
void reply(int code, const char *message);

/* non-blocking sgets: */
int sgets(char *buffer, int space, int sd, char *partbuff);

extern char server_passwd[];

/*
 *	make_rnd_passwd() generates a random password or handle consisting
 *	of lowercase and uppercase characters, digits, dashes and underscores.
 */

void make_rnd_passwd(char *buffer, int size);

void server_shutdown();

#define PA_NEXT		0
#define PA_DONE		1
#define PA_WAIT		2

enum PAR_SYNTAX {PAR_REQ, PAR_FORBIDDEN, PAR_OPTIONAL};

struct ttscp_cmd
{
//	char name[4];
	int name;		/* tried const, Visual C++ then rejects initialization */
	int(*impl)(char *param, a_ttscp *a);
	char *short_help;	/* was const, as above */
	PAR_SYNTAX param;
};

extern ttscp_cmd ttscp_cmd_set[];
int cmd_bad(char *);

extern hash_table<char, a_ttscp> *data_conns;
extern hash_table<char, a_ttscp> *ctrl_conns;
extern a_accept *accept_conn;
extern context *master_context;
extern context *this_context;

//extern int session_uid;

