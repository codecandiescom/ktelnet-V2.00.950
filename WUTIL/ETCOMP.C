/*
 * Copyright (c) 1998 Thomas Nyström and Stacken Computer Club
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Kungliga Tekniska
 *      Högskolan and its contributors.
 * 
 * 4. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include "config.h"
#include "roken.h"

typedef struct ent {
	struct ent *next;
	int code;
	char *id;
	char *str;
	} entry;

char name[128] = "";
char prefix[128] = "";
char id[128] = "";
int code = 0;
entry *head = 0, *tail = 0;

void load_infile(const char *fn);
void write_header(const char *fn, const char *org);
void write_resource(const char *fn, const char *hdr, const char *org);
void set_name(char **cp);
void define_code(char **cp);
void set_index(char **cp);
void set_prefix(char **cp);
void set_id(char **cp);
void token(char **cp, char *result, int UPPER);
#define TOK_UPCASE	0x0001
#define TOK_SKIPCOMMA	0x0002
void OutOfMemory(void);

int
main(int argc, char *argv[])
{
	char fn[256];
	char hdr[256];
	char res[256];

	if (argc != 2) {
		fprintf(stderr, "etcomp fil\n");
		exit(-1);
		}
	
	strcpy_truncate(fn, argv[1], sizeof(fn));
	strcat_truncate(fn, ".et", sizeof(fn));

	load_infile(fn);

	strcpy_truncate(hdr, argv[1], sizeof(hdr));
	strcat_truncate(hdr, ".rh", sizeof(hdr));
	
	write_header(hdr, fn);

	strcpy_truncate(res, argv[1], sizeof(res));
	strcat_truncate(res, ".rc", sizeof(res));
	write_resource(res, hdr, fn);

	return 0;
}


void
write_header(const char *fn, const char *org)
{
	FILE *dst;
	int err;
	
	if ((dst = fopen(fn, "w")) == NULL) {
		perror("Can't create outfile");
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}
	
	fprintf(dst, "/* Automatically generated resource header */\n");
	fprintf(dst, "/* Original file: %s */\n\n", org);

	for (tail = head; tail; tail = tail->next) {
		fprintf(dst, "#define %-40s (IDS_PREF_%s+0x%04x)\n",
			tail->id, name, tail->code);
		}

	err = ferror(dst);
	if (!err)
		err = fclose(dst);

	if (err) {
		perror("Write fault");
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}
		
}

void
write_resource(const char *fn, const char *hdr, const char *org)
{
	FILE *dst;
	int err;
	
	if ((dst = fopen(fn, "w")) == NULL) {
		perror("Can't create outfile");
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}
	
	fprintf(dst, "/* Automatically generated resource file */\n");
	fprintf(dst, "/* Original file: %s */\n\n", org);
	fprintf(dst, "#include \"%s\"\n\n", hdr);
	
	fprintf(dst, "STRINGTABLE\n{\n");

	for (tail = head; tail; tail = tail->next) {
		fprintf(dst, "  IDS_PREF_%s+0x%04x,  %s\n",
			name, tail->code, tail->str);
		}

	fprintf(dst, "}\n\n");

	err = ferror(dst);
	if (!err)
		err = fclose(dst);

	if (err) {
		perror("Write fault");
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}
		
}

void
load_infile(const char *fn)
{
	FILE *src;
	char line[512], *cp;
	char tokstr[512];

	if ((src = fopen(fn, "r")) == NULL) {
		perror("Can't open infile");
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}

	while (fgets(line, sizeof(line), src) != NULL) {
		cp = line;

		token(&cp, tokstr, TOK_UPCASE);
		
		if (strcmp(tokstr, "END") == 0)
			break;

		if ((tokstr[0] == 0) &&
		    ((*cp == '#') || (*cp == 0)))
			continue;		/* Comment or blank line */

		if (strcmp(tokstr, "ET") == 0)
			set_name(&cp);
		else if (strcmp(tokstr, "EC") == 0)
			define_code(&cp);
		else if (strcmp(tokstr, "INDEX") == 0)
			set_index(&cp);
		else if (strcmp(tokstr, "PREFIX") == 0)
			set_prefix(&cp);
		else if (strcmp(tokstr, "ID") == 0)
			set_id(&cp);
		else {
			fprintf(stderr, "Unknown keyword: <%s>\n", tokstr);
			fprintf(stderr, "File: %s\n", fn);
			exit(1);
			}

		if ((*cp == '#') || (*cp == 0))
			continue;		/* Comment or blank line */

		fprintf(stderr, "Unexpected data after line:\n%s\n", line);
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}

	if (ferror(src))
		perror("Read error from infile");
	else if (feof(src))
		fprintf(stderr, "Unexpected end-of-file\n");
	if (ferror(src) || feof(src)) {
		fprintf(stderr, "File: %s\n", fn);
		exit(1);
		}

	fclose(src);
}

void set_name(char **cp)
{
	char tokstr[512];
	
	token(cp, tokstr, TOK_UPCASE);
	strcpy_truncate(name, tokstr, sizeof(name));
}

void define_code(char **cp)
{
	char id[512], str[512];
	entry *e;
	
	token(cp, id, TOK_SKIPCOMMA);
	if (**cp == ',')
		(*cp)++;
	token(cp, str, 0);

	if ((e = malloc(sizeof(e))) == NULL)
		OutOfMemory();

	e->next = 0;
	e->code = code++;
	if ((e->id = malloc(strlen(prefix)+strlen(id)+1)) == NULL)
		OutOfMemory();
	strcpy(e->id, prefix);
	strcat(e->id, id);
	e->str = strdup(str);
	if (tail)
		tail->next = e;
	else
		head = e;
	tail = e;
}

void set_index(char **cp)
{
	code = strtol(*cp, cp, 0);
	while (isspace(**cp))
		(*cp)++;
}

void set_prefix(char **cp)
{
	char pref[512];
	
	token(cp, pref, 0);

	strcpy_truncate(prefix, pref, sizeof(prefix));
	if (prefix[0])
		strcat_truncate(prefix, "_", sizeof(prefix));
}

void set_id(char **cp)
{
	char ids[512];
	
	token(cp, ids, 0);

	strcpy_truncate(prefix, ids, sizeof(prefix));
}

void token(char **cp, char *result, int flag)
{
	int i, str;
	register int ch;

	while (isspace(**cp))
		(*cp)++;
	
	str = 0;
	for (i = 0; i < 510; i++) {
		if ((ch = **cp) == 0)
			break;

		if ((str == 0) && (ch == ','))
			break;

		if ((ch == '"') && (i == 0))
			str = ch;

		if ((str == 0) && (isspace(ch) || (ch == '#')))
			break;
		
		if (flag & TOK_UPCASE)
			result[i] = (char)toupper(ch);
		else
			result[i] = (char)ch;
		(*cp)++;
		
		if ((str == ch) && (i != 0)) {
			i++;
			break;
			}
		}

	result[i] = 0;

	while (isspace(**cp))
		(*cp)++;
}

void OutOfMemory()
{
	fprintf(stderr, "Out of memory.\n");
	exit(1);
}
