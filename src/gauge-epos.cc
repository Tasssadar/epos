/*
 *	(c) 1998-01 Jirka Hanika <geo@cuni.cz>
 *
 *	This single source file src/gauge-epos.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precedence.
 *
 *	This file implements a simple TTSCP client. See doc/english/ttscp.sgml
 *	for a preliminary technical specification.
 *
 *	This file is almost a plain C source file.  We compile it with a C++
 *	compiler just to avoid additional configure complexity.
 */

#define THIS_IS_A_TTSCP_CLIENT

#include "common.h"

#ifdef HAVE_WINSVC_H
	bool start_service();
	bool stop_service();
#else
	bool start_service() { return true; }
	bool stop_service() { return true; }
#endif

#ifndef HAVE_TERMINATE

void terminate(void)
{
	abort();
}

#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#define   TTSCP_PORT	8778

const char *COMMENT_LINES = "#;\r\n";
const char *WHITESPACE = " \t\r";

const char *output_file = NULL;

const char *charset = "8859-2";

bool chunking = false;
bool show_segments = false;
bool wavfile = false;
bool wavstdout = false;

#define STDIN_BUFF_SIZE  550000

int ctrld, datad;		/* file descriptors for the control and data connections */
char *data = NULL;

char *ch;
char *dh;

void shriek(char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
//	system("killall gauge-epos");
	exit(1);
}

void shriek(int, char *txt)
{
	shriek(txt);
}


#include "client.cc"

int get_result(int sd)
{
	while (sgets(scratch, scfg->scratch_size, sd)) {
		scratch[scfg->scratch_size] = 0;
//		printf("Received: %s\n", scratch);
		switch(*scratch) {
			case '1': continue;
			case '2': return 2;
			case '3': break;
			case '4': printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return 4;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
					exit(0);
				  } /* else fall through */
			case '8': printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  exit(2);

			case '5':
			case '7':
			case '9':
			case '0': printf("%s\n", scratch); shriek("Unhandled response code");
			default : ;
		}
		char *o = scratch+strspn(scratch, "0123456789 ");
		if (*scratch && *o) printf("Instead of result #%s#\n", o);
	}
	return 8;	/* guessing */
}

int size;

char *get_line()
{
	#undef SCRATCH_SPACE
	#define SCRATCH_SPACE	200
	char scratch[SCRATCH_SPACE + 2];

	char *b;
	size = 0;
	while (sgets(scratch, SCRATCH_SPACE, ctrld)) {
		scratch[SCRATCH_SPACE] = 0;
		if (strchr("2468", *scratch)) { 	/* all done, write result */
			if (*scratch != '2') shriek(scratch);
			if (!size) shriek("No processed data received");
			b[size] = 0;
			return b;
		}
		if (!strncmp(scratch, "123 ", 4)) {
			int count;
			sgets(scratch, SCRATCH_SPACE, ctrld);
			scratch[SCRATCH_SPACE] = 0;
			if (!sscanf(scratch, "%d", &count)) shriek("Matchfail!");
			b = size ? (char *)realloc(b, size + count + 1) : (char *)malloc(count + 1);
			if (!b) shriek("No buffer");
			if (size && count) printf("size %d, count %d, buffer %s", size, count, b);
			int limit = size + count;
			while (size < limit) {
				int res = yread(datad, b + size, limit - size);
				if (res == -1) {
					printf("size=%d, limit=%d, errno=%d\n", size, limit, errno);
					shriek("Wow!\n");
				}
				size += res;
			}
		}
	}
	if (size) shriek("Disconnect during transmit");
	else shriek("Disconnect before transmit");
	return NULL;
}

char *line = (char *)calloc(1,1);
char *demux = (char *)calloc(1,1);

char *get_word()
{
	if (!*demux) {
		free(line);
		line = get_line();
		demux = line;
	}

	char *ret = demux;
	demux = strchr(demux, ',');
	*demux++ = 0;
	return ret;
}

void send_line(char *line)
{
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", (int)strlen(line));
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	sputs(line, datad);
}

char multiplex[512] = { 0, };

void send_word(char *word)
{
	strcat (multiplex, word);
	strcat (multiplex, ",");
	if (strlen(multiplex) < 400) return;

	send_line(multiplex);
	multiplex[0] = 0;
}

int sent_count = 0;
int received_count = 0;

char *in = "gauge.in";
char *nw = "gauge.new";
char *lst = "gauge.lst";
char *lst_format = "%-24s %-24s %-24s\n";

void send_requests()
{
	FILE *f = fopen(in, "r");
	char buffer[1024];
	char *result;
	while (f && !feof(f)) {
		fgets(buffer, 1023, f);
		if (data && !strstr(buffer, data))
			continue;
		result = strchr(buffer, ' ');
		if (!result) continue;
		*result++ = 0;
		send_word(buffer);
		printf("\r%d %s                                   ", sent_count, buffer);
		sent_count++;
	}
	fclose(f);
	if (*multiplex) send_line(multiplex);
	printf("\nThe last sent line was: \"%s\"\n", multiplex);
	printf("Sent word count: %d\n", sent_count);
	multiplex[0] = 0;
}

void gather_results()
{
	FILE *f;
	FILE *g = NULL;
	FILE *h;
	char buffer[1024];
	char *old;
	f = fopen(in, "r");
	if (data) remove(nw);
	else g = fopen(nw, "w");
	h = fopen(lst, "w");
	if (!f) shriek("Could not open the dictionary");
	if (!h) shriek("Could not create the output files");
	while (1) {
		fgets(buffer, 1023, f);
		if (feof(f))
			break;
		if (data && !strstr(buffer, data))
			continue;
		old = strchr(buffer, ' ');
		if (!old) shriek("Need two space-separated items");
		*old++ = 0;
		old += strspn(old, " ");
		char *received = get_word();
		if (strchr(received, '#')) *strchr(received, '#') = 0;
		if (strchr(old, '\n')) *strchr(old, '\n') = 0;
//		printf("rules: %s, thesaurus: %s\n", received, old);
		if (!data) fprintf(g, "%s %s\n", buffer, received);
		if (strcmp(received, old))
			fprintf(h, lst_format, buffer, old, received);
		received_count++;
	}
	fclose(f);
	if (!data) fclose(g);
	fclose(h);
	printf("Received word count: %d\n", received_count);
}

void gauge()
{
	sputs("strm $", ctrld);
	sputs(dh, ctrld);
	sputs(":raw:rules:print:$", ctrld);
	sputs(dh, ctrld);
	sputs("\r\n", ctrld);
	get_result(ctrld);

	if (!fork()) {
		send_requests();
//		sleep(5);
		sputs("done\r\n", ctrld);
		exit(0);
	}
	else gather_results();


}

void send_option(const char *name, const char *value)
{
	xmit_option(name, value, ctrld);
	get_result(ctrld);
}

void use_trusted()
{
	in = "gauge-trusted.in";
	nw = "gauge-trusted.new";
	lst = "gauge-trusted.lst";
}

void use_bad()
{
	in = "gauge-bad.in";
	nw = "gauge-bad.new";
	lst = "gauge-bad.lst";
}

#define CMD_LINE_OPT "-"
#define CMD_LINE_VAL '='

void dump_help()
{
	printf("usage: gauge-epos [options] ['substring']\n");
	printf(" -t  Use gauge-trusted.* instead of gauge.*\n");
	printf(" -b  Use gauge-bad.* instead of gauge.*\n");
	printf("\n");
	printf(" * gauge.in is used as the input\n");
	printf(" * gauge.new will be created unless you specify a substring\n");
	printf(" * gauge.lst will be created always and shows differences\n");
	printf("Any long options will be passed to the server.\n");
}

void send_cmd_line(int argc, char **argv)
{
	char *ar;
	char *j = NULL;
	register char *par;

	for(int i=1; i<argc; i++) {
		ar=argv[i];
		switch(strspn(ar, CMD_LINE_OPT)) {
		case 3:
			ar+=3;
			if (strchr(ar, CMD_LINE_VAL)) 
				shriek("Thrice dashed options have an implicit value");
			send_option(ar, "0");
			break;
		case 2:
			ar+=2;
			par=strchr(ar, CMD_LINE_VAL);
			if (par) {					//opt=val
				*par=0;
				send_option(ar, par+1);
				*par=CMD_LINE_VAL;
			} else	if (i+1==argc || strchr(CMD_LINE_OPT, *argv[i+1])) 
					send_option(ar, "");	//opt
				else send_option(ar, argv[++i]);	//opt val
			break;
		case 1:
			for (j=ar+1; *j; j++) switch (*j) {
				case 'H': send_option("long_help", "true");	/* fall through */
				case 'h': dump_help(); exit(0);
				case 'p': send_option("pausing", "true"); break;
				case 't': use_trusted(); break;
				case 'b': use_bad(); break;
				case 'v': send_option("version", "true"); break;
				case 'D':
					send_option("debug", "true");
					break;
				default : shriek("Unknown short option");
			}
			if (j==ar+1) {			//dash only
				if (data) free(data);
				data = (char *)malloc(STDIN_BUFF_SIZE);
				fread(data,1,STDIN_BUFF_SIZE,stdin);
			}
			break;
		case 0:
			if (data) {
				sprintf(scratch, "%s %s", data, ar);
				free(data);
				data = strdup(scratch);
			} else data = strdup(ar);
			
			break;
		default:
			shriek("Too many dashes ");
		}
	}
	if (data) for(char *p=data; *p; p++) if (*p=='\n') *p=' ';
}

void send_options()
{
	send_option("generate_segs", "false");
	send_option("diphthongs", "false");
	send_option("phr_break", "false");
	send_option("roman", "false");
	send_option("degeminate", "true");
	send_option("handle_vocalic_groups", "false");
	send_option("handle_prepositions", "false");
	send_option("form_syllables", "false");
	send_option("prosody", "false");
	send_option("a_joins_sents", "false");

	send_option("syslog", "false");
	send_option("trusted", "true");
}

/*
 *	main() implements what most TTSCP clients do: it opens two TTSCP connections,
 *	converts one of them to a data connection dependent on the other one.
 *	Then, commands in a file found using the TTSCP_USER environment variable
 *	are transmitted and synthesis and gauge procedures invoked.
 *	Last, general cleanup is done (the connections are gracefully closed.)
 *
 *	Note that the connection establishment code is less intuitive than
 *	it could be because of paralelism oriented code ordering.
 */

int main(int argc, char **argv)
{
#ifdef HAVE_WINSOCK
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
	charset = "cp1250";
#endif
	start_service();		/* Windows NT etc. only */

	ctrld = connect_socket(0, 0);
	datad = connect_socket(0, 0);
	sputs("data ", datad);
	ch = get_handle(ctrld);
	sputs(ch, datad);
	sputs("\r\n", datad);
	free(ch);
	sputs("setl charset ",ctrld);
	sputs(charset, ctrld);
	sputs("\r\n", ctrld);
	get_result(ctrld);
	send_options();
	send_cmd_line(argc, argv);
	dh = get_handle(datad);
	get_result(datad);

// #ifdef HAVE_GETENV
	FILE *f = NULL;
	char *ttscp_user_config = getenv("TTSCP_USER");
	if (ttscp_user_config) f = fopen(ttscp_user_config, "rt");
	if (f) {
		while (!feof(f)) {
			*scratch = 0;
			fgets(scratch, SCRATCH_SPACE, f);
			if (*scratch && !strchr(COMMENT_LINES, scratch[strspn(scratch, WHITESPACE)])) {
				sputs(scratch, ctrld);
				get_result(ctrld);
			}
		}
		fclose(f);
	}
/// #endif

	gauge();

	sputs("delh ", ctrld);
	sputs(dh, ctrld);
	sputs("\r\ndone\r\n", ctrld);
	get_result(ctrld);
	get_result(ctrld);
	close(datad);
	close(ctrld);
	return 0;
}



