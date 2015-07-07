/*
 *	epos/src/encoding.cc
 *	(c) 2001 geo@cuni.cz
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
 
#ifndef FORGET_CHARSETS
  
#include "epos.h"

#define UNUSED 0
#define UNDEFINED 0

char *charset_list = NULL;
int charset_list_len = 0;

wchar_t epos_charset[256];	/* Epos to unicode */
wchar_t **charsets = 0;		/* charset to unicode */

unsigned char **encoders = 0;	/* charset to Epos */
unsigned char **decoders = 0;	/* Epos to charset */

int max_sampa_alts = 0;		// means: not even sampa-std.txt
#define MAX_SAMPA_ENC	4	// max ASCII chars per SAMPA char
#define MAX_SAMPA_CAND  3	// max Epos chars which map to similar SAMPA chars

char (*sampa)[256][MAX_SAMPA_ENC] = NULL;		/* Epos to SAMPA */
char (*sampa_encoder)[256][MAX_SAMPA_CAND] = NULL;	/* SAMPA to Epos */

bool sampa_updated = false;


void init_enc()
{
	cfg->charset = 0;

	encoders = (unsigned char **)xmalloc(sizeof(unsigned char *));
	decoders = (unsigned char **)xmalloc(sizeof(unsigned char *));
	charsets = (wchar_t **)xmalloc(sizeof(wchar_t *));
	for (int i = 0; i < 256; i++) epos_charset[i] = UNUSED;
}

void shutdown_enc()
{
	for (int i = 0; i < charset_list_len; i++) {
		free(encoders[i]);
		free(decoders[i]);
		free(charsets[i]);
	}
	free(encoders);
	free(decoders);
	free(charsets);
	free(charset_list);
	
	encoders = NULL;
	decoders = NULL;
	charsets = NULL;
	charset_list = NULL;
	charset_list_len = 0;
}

/*
 *	internal_code()  translates a wide character to an internal charcode;
 *		it allocates a new charcode as necessary.
 *		The hint is based on the original encoding and it is used as
 *		the preferred allocation point for the character.
 */

static inline int internal_code(wchar_t c, unsigned char hint)
{
	if (epos_charset[hint] == c) return hint;
	for (int i = 255; i > -1; i--) {
		if (epos_charset[i] == c) return i;
	}
	return UNDEFINED;
}

static inline unsigned char do_alloc_code(wchar_t c, unsigned char hint)
{
	int x = internal_code(c, hint);
	if (x != UNDEFINED) return x;

	if (epos_charset[hint] == UNUSED) return hint;
	for (int i = 255; i ; i--) {
		if (epos_charset[i] == UNUSED) {
			D_PRINT(2, "Allocating charcode unicode %d, original enc %d, internal enc %d\n", c, hint, i);
			return i;
		}
	}
	D_PRINT(2, "Want to allocate charcode unicode %d, original enc %d\n", c, hint);
	shriek(447, "Epos internal character table overflow");
	return 0;

}

static inline unsigned char alloc_code(wchar_t c, unsigned char hint, int cs)
{
	int orig = hint;
	hint = do_alloc_code(c, hint);

	epos_charset[hint] = c;
	encoders[cs][orig] = hint;
	decoders[cs][hint] = orig;

	sampa_updated = false;

	return hint;
}

/*
 *	non_unicode_alloc_code() is generally usable for handling any necessary extensions
 *	beyond unicode.  We however use it only to supplement any charset which leaves
 *	character codes 0 to 31 unspecified, with an identical mapping of these characters.
 *	This conforms both to the unicode standard and the way these charsets are used.
 */

static inline wchar_t non_unicode_alloc_code(unsigned char request)
{
	D_PRINT(2, "Allocating charcode, non-unicode character %d\n", request);
	if (request < ' ' && epos_charset[request] == UNUSED || epos_charset[request] == request) return request;
	else return UNDEFINED;
}

int get_count_allocated()
{
	int result = 0;
	for (int i = 0; i < 256; i++) if (epos_charset[i] != UNUSED) result++;
	return result;
}

/*
 *	The following function encodes strings into the internal Epos charset.
 *
 *	s	input string
 *	t	output string (same buffer as s, as long as possible)
 *	cs	character set
 *	alloc	whether allocating previously unknown characters is allowed
 *	c	input character under processing
 *	u	unicode value of c
 *	res	internal value of c (result of successful encoding)
 */

static void encode_from_8bit(unsigned char *s, int cs, bool alloc)
{
	unsigned char *t = s;
	unsigned char c;
	do {
		c = *s;
		int res = encoders[cs][c];
		if (res == UNDEFINED && c) {
			if (alloc) {
				wchar_t u = charsets[cs][c];
				if (u == UNDEFINED) u = non_unicode_alloc_code(c);
				if (u == UNDEFINED)
					shriek(418, cs ? "Illegal character '%c' in int '%d'" : "Unspecified charset for character %c in int %d", c, (unsigned int) c);  // FIXME - filename
				alloc_code(u, c, cs);
				continue;
			} else if (cfg->relax_input) {
				if (encoders[cs][(unsigned char)cfg->default_char] == UNDEFINED)
					shriek(431, "You specified relax_input but default_char is undefined");
				else res = encoders[cs][(unsigned char)cfg->default_char];
			} else shriek(431, "Parsing an unhandled character  '%c' - code %d", (unsigned int) c, (unsigned int) c);
		}
		*t++ = res;
		s++;
	} while (c);
}

void encode_string(unsigned char *s, int cs, bool alloc)
{
	if (cs >= 0) {
		encode_from_8bit(s, cs, alloc);
	} else if (cs == CHARSET_UTF8) {
//		encode_from_utf8(s, alloc);
	} else {
		encode_from_sampa(s, (-cs) - 5);	/* no possibility to alloc */
	}
}

static void decode_to_8bit(unsigned char *s, int cs)
{
	do {
		int t = decoders[cs][*s];
		if (t == UNDEFINED && *s) shriek(461, "Character '%c', code %d infiltrated internal encoding", *s);
		else *s = t;
	} while (*s++);
}

void decode_string(unsigned char *s, int cs)
{
	if (cs >= 0) {
		decode_to_8bit(s, cs);
	} else if (cs == CHARSET_UTF8) {
//		decode_to_utf8(s);
	} else {
		decode_to_sampa(s, (-cs) - 5);
	}
}


void alloc_charset(const char *name)
{
	char *p;
	if (charset_list) {
		p = (char *)xrealloc(charset_list, strlen(charset_list) + strlen(name) + 2);
		p[strlen(p) + 1] = 0;
		p[strlen(p)] = LIST_DELIM;
	} else {
		p = (char *)xmalloc(strlen(name) + 1);
		*p = 0;
	}
	strcat(p, name);
	charset_list = p;
	int idx = charset_list_len;
	charset_list_len++;
	
	encoders = (unsigned char **)xrealloc(encoders, charset_list_len * sizeof(unsigned char *));
	decoders = (unsigned char **)xrealloc(decoders, charset_list_len * sizeof(unsigned char *));
	charsets = (wchar_t **)xrealloc(charsets, charset_list_len * sizeof(wchar *));

	encoders[idx] = (unsigned char *)xmalloc(256);
	decoders[idx] = (unsigned char *)xmalloc(256);
	charsets[idx] = (wchar_t *)xmalloc(256 * sizeof(wchar_t));
	memset(encoders[idx], UNDEFINED, 256);
	memset(decoders[idx], UNDEFINED, 256);
	for (int i = 0; i < 256; i++) charsets[idx][i] = UNDEFINED;
}

int load_charset(const char *name)
{
	UNIT u = str2enum(name, charset_list, U_ILL);
	if (u != U_ILL) return (int)u;
	
	D_PRINT(3, "Loading charset %s\n", name);
	char *p = (char *)xmalloc(strlen(name) + 5);
	strcpy(p, name);
	strcat(p, ".txt");
	text *t = new text(p, scfg->unimap_dir, "", NULL, true);
	free(p);
	if (!t->exists()) {
		delete t;
		if (!charsets) shriek(844, "Couldn't find unicode map for initial charset %s", name);
		return CHARSET_NOT_AVAILABLE;
	}
	char *line = get_text_line_buffer();
	alloc_charset(name);
	unsigned char *c = encoders[charset_list_len - 1];
	unsigned char *d = decoders[charset_list_len - 1];
	wchar_t *w = charsets[charset_list_len - 1];
	while(t->get_line(line)) {
		int b, x, dummy;
		if (sscanf(line, "%i %i %i", &b, &x, & dummy) != 2) continue;
		if (b < 0 || b >= 256) shriek(463, "unicode map file %s maps out of range characters", name);
		int e = internal_code(x, b);
		w[b] = x;
		if (e == UNDEFINED) continue;
		c[b] = e;
		d[e] = b;
	}
	delete t;
	free(line);

//	if (cfg->paranoid) for (int i = 1; i < 256; i++) if (w[i] == UNDEFINED)	FIXME
//		shriek(462, "Character code %d left undefined for charset %s", i, name);
	return charset_list_len - 1;
}

void load_default_charset()
{
	init_enc();
	alloc_charset("default");
	for (int i = 0; i < 128; i++) {		/* all ASCII chars automatically allocated */
		encoders[0][i] = i;
		decoders[0][i] = i;
		charsets[0][i] = i;
	}
}

#define MAX_EXPANSION_FACTOR	3

char *get_text_buffer(int chars)
{
	return (char *)xmalloc(chars * MAX_EXPANSION_FACTOR + 2);
}

char *get_text_buffer(const char *string)
{
	char *ret = get_text_buffer(strlen(string));
	strcpy(ret, string);
	return ret;
}

char *get_text_line_buffer()
{
	// CHECKME change to call get_text_buffer when internal chars stop being 8bit:
	return (char *)xmalloc(scfg->max_line_len + 2);
}

char *get_text_cmd_buffer()
{
	return get_text_buffer(cfg->max_net_cmd);
}

static unsigned char extract_sampa_token(char *sampa_char)
{
	bool escaped = sampa_char[1] == '\\';
	return sampa_char[0] | (escaped && sampa_char[0] ? 0x80 : 0);
}


static void load_sampa_decoder(int alt, const char *filename)
{
	text *t = new text(filename, scfg->unimap_dir, "", NULL, true);
	if (!t->exists()) {
		delete t;
		shriek(844, "Couldn't find the SAMPA map %s", filename);
	}
//	t->raw = true;
	char *line = get_text_line_buffer();
	while(t->get_line(line)) {
		int u;
		char x[4 + MAX_SAMPA_ENC], dummy;
		memset(x, 0, sizeof(x));
		int n = sscanf(line, "%i %3s %2s", &u, x, &dummy);
		if (n ==1 || n == 3) shriek(463, "Weird entry in file %s line %d", t->current_file, t->current_line);
		for (int i = 0; i < 256; i++) {
			if (epos_charset[i] == u) {
				strncpy(sampa[alt][i], x, MAX_SAMPA_ENC);
			}
		}
	}
	delete t;
	free(line);
}

static void load_sampa(int alt, const char *filename)
{
	D_PRINT(3, "Loading %sSAMPA mappings\n", alt ? "alternate " : "");

	if (alt >= max_sampa_alts) {
		max_sampa_alts = alt + 1;
		if (!sampa) sampa = (char (*)[256][MAX_SAMPA_ENC])xmalloc(256 * MAX_SAMPA_ENC * max_sampa_alts);
		else sampa = (char (*)[256][MAX_SAMPA_ENC])xrealloc(sampa,256 * MAX_SAMPA_ENC * max_sampa_alts);
		if (!sampa_encoder) sampa_encoder = (char (*)[256][MAX_SAMPA_CAND])xmalloc(MAX_SAMPA_CAND * 256 * max_sampa_alts);
		else sampa_encoder = (char (*)[256][MAX_SAMPA_CAND])xrealloc(sampa_encoder,MAX_SAMPA_CAND * 256 * max_sampa_alts);
	}
	memset(sampa[alt], 0, 256 * MAX_SAMPA_ENC);
	load_sampa_decoder(alt, filename);
	D_PRINT(2, "Decoder into SAMPA, alt %d loaded\n", alt);
	
	memset(sampa_encoder[alt], 0, 256 * MAX_SAMPA_CAND);
	for (int i = 0; i < 256; i++) {
		unsigned char sampa_token = extract_sampa_token(sampa[alt][i]);
		if (!sampa_token) {
			continue;
		}
		char *cands = sampa_encoder[alt][sampa_token];
		if (cands[MAX_SAMPA_CAND - 1]) {
			shriek(463, "Too many SAMPA chars (alt %d) begin with token %c(code %d), examples: %c(%d), %c(%d), %c(%d)", alt, sampa_token & 0x7f, sampa_token, cands[0], (unsigned char)cands[0], cands[MAX_SAMPA_CAND - 1], (unsigned char)cands[MAX_SAMPA_CAND - 1], i, i);
		}
		D_PRINT(1, "Storing %c(%d) as a candidate for SAMPA fragment %c(%d)\n", i, i, sampa_token & 0x7f, sampa_token);
		cands[strlen(cands)] = i;
	}
}

int load_named_sampa(const char *charset)
{
	if (!strcmp(charset, NAME_SAMPA_STD)) {
		return CHARSET_SAMPA_STD;
	}

	if (strncmp(charset, NAME_SAMPA_ALT, strlen(NAME_SAMPA_ALT))) {
		shriek(447, "Ill-formed SAMPA charset name %s", charset);
	}

	const char *name = charset + strlen(NAME_SAMPA_ALT);
	D_PRINT(2, "Switching to SAMPA alternate named %s\n", name);

	UNIT u = str2enum(name, scfg->sampa_alts, U_ILL);
	int alt = (int)u + 1;
	D_PRINT(1, "That alternate is numbered %d\n", alt);
	if (u != U_ILL) return CHARSET_SAMPA_ALT(alt);

	shriek(447, "Dynamic loading of SAMPA mappings (%s) unsupported", charset);
}

static void add_alt_sampa(int alt, const char *name)
{
	char filename[300];
	if (strlen(name) > 256)
		return;
	sprintf(filename, "sampa-alt-%s.txt", name);
	load_sampa(alt + 1, filename);
}

void update_sampa()
{
	load_sampa(0, "sampa-std.txt");
	if (scfg->sampa_alts) list_of_calls(scfg->sampa_alts, add_alt_sampa);
	sampa_updated = true;
}

void release_sampa()
{
	free(sampa);
	sampa = NULL;
	max_sampa_alts = 0;
}

const char *nothing = "_";

const char *decode_to_sampa(unsigned char c, int sampa_alt)
{

	if (!sampa_updated) update_sampa();
	if (sampa_alt < 0 || sampa_alt >= max_sampa_alts) return nothing;

	const char *ret = sampa[sampa_alt][c];
	if (*ret) {
		return ret;
	} else {
		if (!cfg->paranoid || 1) return nothing;	// FIXME
		else shriek(462, "Character code %d has no SAMPA representation\n", c);
	}
}

char *decoder_swap = NULL;
int decoder_swap_size = 0;

void decode_to_sampa(unsigned char *s, int sampa_alt)
{
	if (!sampa_updated) update_sampa();
	if (sampa_alt < 0 || sampa_alt >= max_sampa_alts) {
		shriek(461, "Invalid SAMPA alternate");
	}

	D_PRINT(1, "Decoding %s, SAMPA alternate index is %d\n", s, sampa_alt);

	if (!decoder_swap) decoder_swap_size = 512, decoder_swap = (char *)xmalloc(512);
	int needed = strlen((char *)s) * MAX_EXPANSION_FACTOR;
	if (needed > decoder_swap_size) {
		do {
			decoder_swap_size <<= 1;
		} while (needed > decoder_swap_size);
		decoder_swap = (char *)xrealloc(decoder_swap, decoder_swap_size);
	}
	char *target = decoder_swap;
	for (int i = 0; i < strlen((char *)s); i++) {
		const char *sampa_char = decode_to_sampa(s[i], sampa_alt);
		strcpy(target, sampa_char);
		target += strlen(sampa_char);
	}
	strcpy((char *)s, decoder_swap);
}

int match(char *pattern, char *string)
{
	int ret = 0;
	int i;
	for (i = 0; pattern[i] && pattern[i] == string[i]; i++) {
		ret++;
	}
	ret =  pattern[i] ? 0 : ret;
	D_PRINT(0, "Evaluated candidate %10s against %10s, its weight is %d\n", pattern, string, ret);
	return ret;
}

void encode_from_sampa(char *source, unsigned char *target, int alt)
{
	D_PRINT(1, "Encoding from SAMPA: %s, SAMPA alternate index is %d\n", source, alt);
	while (*source) {
		unsigned char token = extract_sampa_token(source);
		char *cands = sampa_encoder[alt][token];
		D_PRINT(0, "Candidates for token %c(%d) are: %3s\n", token & 0x7f, token, cands);
		int max = 0, best = 0;
		for (int i = 0; cands[i] && i < MAX_SAMPA_CAND; i++) {
			int w = match(sampa[alt][(unsigned char)cands[i]], source);
			if (w && w == max) {
				D_PRINT(3, "Ambiguous SAMPA to decode: %10s, chars %d and %d\n", source, cands[best], cands[i]);
			}
			if (w > max) {
				D_PRINT(0, "%s SAMPA candidate: %s has weight %d and position %d\n", (max ? "Better" : "First"), sampa[alt][cands[i]], w, i);
				best = i;
				max = w;
			}
		}
		if (max == 0) {
			D_PRINT(3, "Unrecognized SAMPA: %10s\n", source);
			*target++ = *source++;
		} else {
			D_PRINT(0, "Recognized SAMPA as: %c\n", cands[best]);
			*target++ = cands[best];
			source += max;
		}
	}
	*target = 0;
}

void encode_from_sampa(unsigned char *s, int sampa_alt)
{
	encode_from_sampa((char *)s, s, sampa_alt);
}

const char *get_charset_name(int code)
{
	if (code >= 0) {
		return enum2str(code, charset_list);
	} else if (code == CHARSET_UTF8) {
		return NAME_UTF8;
	} else if (code == CHARSET_SAMPA_STD) {
		return NAME_SAMPA_STD;
	} else if (code < CHARSET_SAMPA_STD) {
		return enum2str(code, scfg->sampa_alts);
	} else {
		return "invalid";
	}
}


#endif	// FORGET_CHARSETS
