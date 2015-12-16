/*
 * Copyright (C) 2012-2015 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <regex.h>
#include <json.h>

#include "config.h"

#define PARSER_OK		0
#define PARSER_COMMENT_FOUND	1

#define __JSON_ERR_PTR__ ((json_object*) -1)
/*
 *  Older versions of json-c may return an error in an
 *  object as a ((json_object*)-1), where as newer
 *  versions return NULL, so check for these. Sigh.
 */
#define JSON_ERROR(ptr) \
	( (ptr == NULL) || ((json_object*)ptr == __JSON_ERR_PTR__) )

typedef enum {
        COMPARE_REGEX = 'r',
        COMPARE_STRING = 's',
        COMPARE_UNKNOWN = 'u',
} compare_mode;

typedef struct {
        char *pattern;		/* pattern that we compare to kernel messages */
	compare_mode cm;	/* 'r' regex or 's' string comparison */
        regex_t compiled;	/* regex from pattern */
	bool compiled_ok;	/* regex compiled OK */
} klog_pattern;

/*
 *  Subset of tokens that we need to intelligently parse the kernel C source
 */
typedef enum {
	TOKEN_UNKNOWN,		/* No idea what token it is */
	TOKEN_NUMBER,		/* Integer */
	TOKEN_LITERAL_STRING,	/* "string" */
	TOKEN_LITERAL_CHAR,	/* 'x' */
	TOKEN_IDENTIFIER,	/* identifier */
	TOKEN_PAREN_OPENED,	/* ( */
	TOKEN_PAREN_CLOSED,	/* ) */
	TOKEN_CPP,		/* # C pre-propressor */
	TOKEN_WHITE_SPACE,	/* ' ', '\t', '\r', '\n' white space */
	TOKEN_LESS_THAN,	/* < */
	TOKEN_GREATER_THAN,	/* > */
	TOKEN_COMMA,		/* , */
	TOKEN_ARROW,		/* -> */
	TOKEN_TERMINAL,		/* ; */
} token_type;

/*
 *  A token
 */
typedef struct {
	char *token;		/* The gathered string for this token */
	size_t len;		/* Length of the token buffer */
	char *ptr;		/* Current end of the token during the lexical analysis */
	token_type type;	/* The type of token we think it is */
} token;

/*
 *  Quick and dirty way to push input stream back, like ungetc()
 */
typedef struct get_stack {
	int ch;			/* Char pushed back */
	struct get_stack *next;	/* Next one in list */
} get_stack;

/*
 *  Parser context
 */
typedef struct {
	FILE *fp;		/* The file descriptor we are reading */
	bool skip_white_space;	/* Magic skip white space flag */
	get_stack *get_chars;	/* Ungot chars get pushed onto this */
} parser;

/*
 *  FWTS klog patterns, loaded from a json file
 */
static klog_pattern *patterns;

static int get_token(parser *p, token *t);

/*
 *  Initialise the parser
 */
static void parser_new(parser *p, FILE *fp, bool skip_white_space)
{
	p->get_chars = NULL;
	p->fp = fp;
	p->skip_white_space = skip_white_space;
}

/*
 *  Get next character from input stream
 */
static int get_next(parser *p)
{
	/*
	 * If we have chars pushed using unget_next
	 * then pop them off the list first
	 */
	if (p->get_chars) {
		get_stack *tmp = p->get_chars;
		int ch = tmp->ch;

		p->get_chars = tmp->next;
		free(tmp);

		return ch;
	}
	return fgetc(p->fp);
}

/*
 *  Push character back onto the input
 *  stream (in this case, it is a simple FIFO stack
 */
static void unget_next(parser *p, int ch)
{
	get_stack *new;

	if ((new = calloc(1, sizeof(get_stack))) == NULL) {
		fprintf(stderr, "unget_next: Out of memory!\n");
		exit(EXIT_FAILURE);
	}

	new->ch = ch;
	new->next = p->get_chars;
	p->get_chars = new;
}

/*
 *  Create a new token, give it plenty of slop so
 *  we don't need to keep on reallocating the token
 *  buffer as we append more characters to it during
 *  the lexing phase.
 */
static void token_new(token *t)
{
	if ((t->token = calloc(1024, 1)) == NULL) {
		fprintf(stderr, "token_new: Out of memory!\n");
		exit(EXIT_FAILURE);
	}
	t->len = 1024;
	t->ptr = t->token;
	t->type = TOKEN_UNKNOWN;
	*(t->ptr) = '\0';
}

/*
 *  Clear the token ready for re-use
 */
static void token_clear(token *t)
{
	t->ptr = t->token;
	t->type = TOKEN_UNKNOWN;
	*(t->ptr) = '\0';
}

/*
 *  Free the token
 */
static void token_free(token *t)
{
	free(t->token);
	t->token = NULL;
	t->ptr = NULL;
	t->len = 0;
	t->type = TOKEN_UNKNOWN;
}

/*
 *  Append a single character to the token,
 *  we may run out of space, so this occasionally
 *  adds an extra 1K of token space for long tokens
 */
static void token_append(token *t, int ch)
{
	if (t->ptr < t->token + t->len - 1) {
		/* Enough space, just add char */
		*(t->ptr) = ch;
		t->ptr++;
		*(t->ptr) = 0;
	} else {
		/* No more space, add 1K more space */
		ptrdiff_t diff = t->ptr - t->token;

		t->len += 1024;
		if ((t->token = realloc(t->token, t->len)) == NULL) {
			fprintf(stderr, "token_append: Out of memory!\n");
			exit(EXIT_FAILURE);
		}
		t->ptr = t->token + diff;
		*(t->ptr) = ch;
		t->ptr++;
		*(t->ptr) = 0;
	}
}

/*
 *  Figure out if a klog pattern is a regex or a plain text string
 */
static compare_mode klog_compare_mode_str_to_val(const char *str)
{
	if (strcmp(str, "regex") == 0)
		return COMPARE_REGEX;
	else if (strcmp(str, "string") == 0)
		return COMPARE_STRING;
	else
		return COMPARE_UNKNOWN;
}

/*
 *  Load FWTS klog messages from the json table
 */
static klog_pattern *klog_load(const char *table)
{
	int n;
	int i;
	json_object *klog_objs;
	json_object *klog_table;
	klog_pattern *patterns;

	klog_objs = json_object_from_file(DATAROOTDIR "/fwts/klog.json");
	if (JSON_ERROR(klog_objs)) {
		fprintf(stderr, "Cannot load klog data\n");
		exit(EXIT_FAILURE);
	}

#if JSON_HAS_GET_EX
	if (!json_object_object_get_ex(klog_objs, table, &klog_table)) {
		fprintf(stderr, "Cannot fetch klog table object from %s.\n", table);
		exit(EXIT_FAILURE);
	}
#else
	klog_table = json_object_object_get(klog_objs, table);
	if (JSON_ERROR(klog_table)) {
		fprintf(stderr, "Cannot fetch klog table object from %s.\n", table);
		exit(EXIT_FAILURE);
	}
#endif

	n = json_object_array_length(klog_table);

	/* Last entry is null to indicate end, so alloc n+1 items */
	if ((patterns = calloc(n+1, sizeof(klog_pattern))) == NULL) {
		fprintf(stderr, "Cannot allocate pattern table.\n");
		exit(EXIT_FAILURE);
	}

	/* Now fetch json objects and compile regex */
	for (i = 0; i < n; i++) {
		const char *str;
		json_object *obj;
#if JSON_HAS_GET_EX
		json_object *obj_str;
#endif

		obj = json_object_array_get_idx(klog_table, i);
		if (JSON_ERROR(obj)) {
			fprintf(stderr, "Cannot fetch %d item from table %s.\n", i, table);
			exit(EXIT_FAILURE);
		}

#if JSON_HAS_GET_EX
		if (!json_object_object_get_ex(obj, "compare_mode", &obj_str)) {
			fprintf(stderr, "Cannot fetch compare_mode object, item %d from table %s.\n", i, table);
			exit(EXIT_FAILURE);
		}
		str = json_object_get_string(obj_str);
#else
		str = (char*)json_object_get_string(json_object_object_get(obj, "compare_mode"));
#endif
		if (JSON_ERROR(str)) {
			fprintf(stderr, "Cannot fetch compare_mode string, item %d from table %s.\n", i, table);
			exit(EXIT_FAILURE);
		}
		patterns[i].cm = klog_compare_mode_str_to_val(str);

#if JSON_HAS_GET_EX
		if (!json_object_object_get_ex(obj, "pattern", &obj_str)) {
			fprintf(stderr, "Cannot fetch pattern object, item %d from table %s.\n", i, table);
			exit(EXIT_FAILURE);
		}
		str = json_object_get_string(obj_str);
#else
		str = (char*)json_object_get_string(json_object_object_get(obj, "pattern"));
#endif
		if (JSON_ERROR(str)) {
			fprintf(stderr, "Cannot fetch pattern string, item %d from table %s.\n", i, table);
			exit(EXIT_FAILURE);
		}
		patterns[i].pattern = strdup(str);
		if (patterns[i].pattern == NULL) {
			fprintf(stderr, "Failed to strdup regex pattern %d from table %s.\n", i, table);
			exit(EXIT_FAILURE);
		}

		/* Pre-compile regular expressions to make things run a bit faster */
		if (patterns[i].cm == COMPARE_REGEX) {
			int rc;

			rc = regcomp(&patterns[i].compiled, patterns[i].pattern, REG_EXTENDED);
			if (rc) {
				fprintf(stderr, "Regex %s failed to compile: %d.\n", patterns[i].pattern, rc);
				patterns[i].compiled_ok = false;
			} else {
				patterns[i].compiled_ok = true;
			}
		}
	}

	/* Discard the json table now we've parsed it into patterns */
	json_object_put(klog_objs);

	return patterns;
}

/*
 *  Does str match any of the patterns in the klog pattern table
 */
static bool klog_find(char *str, klog_pattern *patterns)
{
	int i;

	for (i = 0; patterns[i].pattern; i++) {
		if (patterns[i].cm == COMPARE_STRING) {
			if (strstr(str, patterns[i].pattern)) {
				return true;
			}
		} else if (patterns[i].cm == COMPARE_REGEX) {
			if (patterns[i].compiled_ok &&
			    (!regexec(&patterns[i].compiled, str, 0, NULL, 0)))
				return true;
		}
	}

	return false;
}

/*
 *  Free the klog patterns
 */
static void klog_free(klog_pattern *patterns)
{
	int i;

	for (i = 0; patterns[i].pattern; i++) {
		regfree(&patterns[i].compiled);
		free(patterns[i].pattern);
	}
	free(patterns);
}

/*
 *  Parse C comments and just throw them away
 */
static int skip_comments(parser *p)
{
	int ch;
	int nextch;

	nextch = get_next(p);
	if (nextch == EOF)
		return EOF;

	if (nextch == '/') {
		do {
			ch = get_next(p);
			if (ch == EOF)
				return EOF;
		}
		while (ch != '\n');

		return PARSER_COMMENT_FOUND;
	}

	if (nextch == '*') {
		for (;;) {
			ch = get_next(p);
			if (ch == EOF)
				return EOF;

			if (ch == '*') {
				ch = get_next(p);
				if (ch == EOF)
					return EOF;

				if (ch == '/')
					return PARSER_COMMENT_FOUND;

				unget_next(p, ch);
			}
		}
	}

	/* Not a comment, push back */
	unget_next(p, nextch);

	return PARSER_OK;
}

/*
 *  Parse an integer.  This is fairly minimal as the
 *  kernel doesn't have floats or doubles, so we
 *  can just parse decimal, octal or hex values.
 */
static int parse_number(parser *p, token *t, int ch)
{
	bool ishex = false;
	bool isoct = false;

	/*
	 *  Crude way to detect the kind of integer
	 */
	if (ch == '0') {
		int nextch1, nextch2;

		token_append(t, ch);

		nextch1 = get_next(p);
		if (nextch1 == EOF) {
			token_append(t, ch);
			return PARSER_OK;
		}

		if (nextch1 >= '0' && nextch1 <= '8') {
			/* Must be an octal value */
			ch = nextch1;
			isoct = true;
		} else if (nextch1 == 'x' || nextch1 == 'X') {
			/* Is it hexadecimal? */
			nextch2 = get_next(p);
			if (nextch2 == EOF) {
				unget_next(p, nextch1);
				return PARSER_OK;
			}

			if (isxdigit(nextch2)) {
				/* Hexadecimal */
				token_append(t, nextch1);
				ch = nextch2;
				ishex = true;
			} else {
				/* Nope */
				unget_next(p, nextch2);
				unget_next(p, nextch1);
				return PARSER_OK;
			}
		} else {
			unget_next(p, nextch1);
			return PARSER_OK;
		}
	}

	/*
	 * OK, we now know what type of integer we
	 * are processing, so just gather up the digits
	 */
	token_append(t, ch);

	for (;;) {
		ch = get_next(p);

		if (ch == EOF) {
			unget_next(p, ch);
			return PARSER_OK;
		}

		if (ishex) {
			if (isxdigit(ch)) {
				token_append(t, ch);
			} else {
				unget_next(p, ch);
				return PARSER_OK;
			}
		} else if (isoct) {
			if (ch >= '0' && ch <= '8') {
				token_append(t, ch);
			} else {
				unget_next(p, ch);
				return PARSER_OK;
			}
		} else {
			if (isdigit(ch)) {
				token_append(t, ch);
			} else {
				unget_next(p, ch);
				return PARSER_OK;
			}
		}
	}
}

/*
 *  Parse identifiers
 */
static int parse_identifier(parser *p, token *t, int ch)
{
	token_append(t, ch);

	t->type = TOKEN_IDENTIFIER;

	for (;;) {
		ch = get_next(p);
		if (ch == EOF) {
			break;
		}
		if (isalnum(ch) || ch == '_') {
			token_append(t, ch);
		} else {
			unget_next(p, ch);
			break;
		}
	}

	return PARSER_OK;
}

/*
 *  Parse literal strings
 */
static int parse_literal(parser *p, token *t, int literal, token_type type)
{
	bool escaped = false;

	t->type = type;

	token_append(t, literal);

	for (;;) {
		int ch = get_next(p);
		if (ch == EOF) {
			return PARSER_OK;
		}

		if (ch == '\\') {
			escaped = true;
			token_append(t, ch);
			continue;
		}

		if (!escaped && ch == literal) {
			token_append(t, ch);
			return PARSER_OK;
		}
		escaped = false;

		token_append(t, ch);
	}

	return PARSER_OK;
}

/*
 *  Parse operators such as +, - which can
 *  be + or ++ forms.
 */
static int parse_op(parser *p, token *t, int op)
{
	int ch;

	token_append(t, op);

	ch = get_next(p);
	if (ch == EOF) {
		return PARSER_OK;
	}

	if (ch == op) {
		token_append(t, op);
		return PARSER_OK;
	}

	unget_next(p, ch);
	return PARSER_OK;
}

/*
 *  Parse -, --, ->
 */
static int parse_minus(parser *p, token *t, int op)
{
	int ch;

	token_append(t, op);

	ch = get_next(p);
	if (ch == EOF) {
		return PARSER_OK;
	}

	if (ch == op) {
		token_append(t, ch);
		return PARSER_OK;
	}

	if (ch == '>') {
		token_append(t, ch);
		t->type = TOKEN_ARROW;
		return PARSER_OK;
	}

	unget_next(p, ch);
	return PARSER_OK;
}

/*
 *  Gather a token from input stream
 */
static int get_token(parser *p, token *t)
{
	int ret;

	for (;;) {
		int ch = get_next(p);

		switch (ch) {
		case EOF:
			return EOF;

		/* Skip comments */
		case '/':
			ret = skip_comments(p);
			if (ret == EOF)
				return EOF;
			if (ret == PARSER_COMMENT_FOUND)
				continue;
			token_append(t, ch);
			return PARSER_OK;
		case '#':
			token_append(t, ch);
			t->type = TOKEN_CPP;
			return PARSER_OK;
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\\':
			if (p->skip_white_space)
				continue;
			else {
				token_append(t, ch);
				t->type = TOKEN_WHITE_SPACE;
				return PARSER_OK;
			}
		case '(':
			token_append(t, ch);
			t->type = TOKEN_PAREN_OPENED;
			return PARSER_OK;
		case ')':
			token_append(t, ch);
			t->type = TOKEN_PAREN_CLOSED;
			return PARSER_OK;
		case '<':
			token_append(t, ch);
			t->type = TOKEN_LESS_THAN;
			return PARSER_OK;
		case '>':
			token_append(t, ch);
			t->type = TOKEN_GREATER_THAN;
			return PARSER_OK;
		case ',':
			token_append(t, ch);
			t->type = TOKEN_COMMA;
			return PARSER_OK;
		case ';':
			token_append(t, ch);
			t->type = TOKEN_TERMINAL;
			return PARSER_OK;
		case '{':
		case '}':
		case ':':
		case '~':
		case '?':
		case '*':
		case '%':
		case '!':
		case '.':
			token_append(t, ch);
			return PARSER_OK;
		case '0'...'9':
			return parse_number(p, t, ch);
			break;
		case 'a'...'z':
		case 'A'...'Z':
			return parse_identifier(p, t, ch);
			break;
		case '"':
			return parse_literal(p, t, ch, TOKEN_LITERAL_STRING);
		case '\'':
			return parse_literal(p, t, ch, TOKEN_LITERAL_CHAR);
		case '+':
		case '=':
		case '|':
		case '&':
			return parse_op(p, t, ch);
		case '-':
			return parse_minus(p, t, ch);
		}
	}

	return PARSER_OK;
}

/*
 *  Literals such as "foo" and 'f' sometimes
 *  need the quotes stripping off.
 */
static void literal_strip_quotes(token *t)
{
	size_t len = strlen(t->token);

	t->token[len-1] = 0;

	memmove(t->token, t->token + 1, len - 1);
}

/*
 *  Concatenate new string onto old. The old
 *  string can be NULL or an existing string
 *  on the heap.  This returns the newly
 *  concatenated string.
 */
static char *strdupcat(char *old, char *new)
{
	size_t len = strlen(new);
	char *tmp;

	if (old == NULL) {
		tmp = malloc(len + 1);
		if (tmp == NULL) {
			fprintf(stderr, "strdupcat(): Out of memory.\n");
			exit(EXIT_FAILURE);
		}
		strcpy(tmp, new);
	} else {
		size_t oldlen = strlen(old);
		tmp = realloc(old, oldlen + len + 1);
		if (tmp == NULL) {
			fprintf(stderr, "strdupcat(): Out of memory.\n");
			exit(EXIT_FAILURE);
		}
		strcat(tmp, new);
	}

	return tmp;
}

/*
 *  Parse a kernel message, like printk() or dev_err()
 */
static int parse_kernel_message(parser *p, token *t)
{
	bool got_string = false;
	bool emit = false;
	bool found = false;
	token_type prev_token_type = TOKEN_UNKNOWN;
	char *str = NULL;
	char *line = NULL;
	bool printk;

	printk = (strcmp(t->token, "printk") == 0);

	if ((strcmp(t->token, "dev_err") == 0) ||
	    (strcmp(t->token, "ACPI_ERROR") == 0) ||
	    (strcmp(t->token, "ACPI_BIOS_ERROR") == 0) ||
	    (strcmp(t->token, "ACPI_EXCEPTION") == 0) ||
	    (strcmp(t->token, "ACPI_ERROR_METHOD") == 0))
		emit = true;

	line = strdupcat(line, t->token);
	token_clear(t);

	for (;;) {
		int ret = get_token(p, t);
		if (ret == EOF) {
			free(line);
			free(str);
			return EOF;
		}

		/*
		 *  Hit ; so lets push out what we've parsed
		 */
		if (t->type == TOKEN_TERMINAL) {
			if (emit) {
				if (found) {
					printf("OK : %s\n", line);
				} else {
					printf("ADD: %s\n", line);
				}
			}
			free(line);
			free(str);
			return PARSER_OK;
		}

		/*
		 *  We are only interested in KERN_ERR
		 *  printk messages
		 */
		if (printk &&
		    (t->type == TOKEN_IDENTIFIER) &&
		    (prev_token_type == TOKEN_PAREN_OPENED) &&
		    ((strcmp(t->token, "KERN_ERR") == 0) ||
		     (strcmp(t->token, "KERN_CRIT") == 0) ||
		     (strcmp(t->token, "KERN_EMERG") == 0) ||
		     (strcmp(t->token, "KERN_WARNING") == 0))) {
			emit = true;
		}

		if (t->type == TOKEN_LITERAL_STRING) {
			literal_strip_quotes(t);
			str = strdupcat(str, t->token);

			if (!got_string)
				line = strdupcat(line, "\"");

			got_string = true;
		} else {
			if (got_string)
				line = strdupcat(line, "\"");

			got_string = false;

			if (str) {
				found |= klog_find(str, patterns);
				free(str);
				str = NULL;
			}
		}

		line = strdupcat(line, t->token);

		if (t->type == TOKEN_IDENTIFIER && prev_token_type != TOKEN_COMMA)
			line = strdupcat(line, " ");

		if (t->type == TOKEN_COMMA)
			line = strdupcat(line, " ");

		prev_token_type = t->type;

		token_clear(t);
	}
	free(line);
}

/*
 *  Parse input looking for printk or dev_err calls
 */
static void parse_kernel_messages(FILE *fp)
{
	token t;
	parser p;

	parser_new(&p, fp, true);
	p.fp = fp;
	p.skip_white_space = true;

	token_new(&t);

	while ((get_token(&p, &t)) != EOF) {
		if ((strcmp(t.token, "printk") == 0) ||
		    (strcmp(t.token, "dev_err") == 0) ||
		    (strcmp(t.token, "ACPI_ERROR") == 0) ||
		    (strcmp(t.token, "ACPI_BIOS_ERROR") == 0) ||
		    (strcmp(t.token, "ACPI_EXCEPTION") == 0) ||
		    (strcmp(t.token, "ACPI_ERROR_METHOD") == 0)) {
			parse_kernel_message(&p, &t);
		} else
			token_clear(&t);
	}

	token_free(&t);
}

/*
 *  This is evil.  We parse the input stream
 *  and throw away all #includes so we don't get
 *  gcc -E breaking on include files that we haven't
 *  got.  We don't really care at this level about
 *  macros being expanded as we want to see tokens
 *  such as KERN_ERR later on.
 */
static int parse_cpp_include(parser *p, token *t)
{
	/*
	 *  Gloop up #include "foo.h"
	 */
	do {
		token_clear(t);
		if (get_token(p, t) == EOF)
			return EOF;
		/* End of line, we're done! */
		if (strcmp(t->token, "\n") == 0)
			return PARSER_OK;
	} while (t->type == TOKEN_WHITE_SPACE);


	/*
	 *  Ah, we gobbled up white spaces and
	 *  now we should be at a '<' token
	 *  Parse #include <something/foo.h>
	 */
	if (t->type == TOKEN_LESS_THAN) {
		do {
			if (get_token(p, t) == EOF)
				return EOF;
		} while (t->type != TOKEN_GREATER_THAN);
	}

	token_clear(t);

	return PARSER_OK;
}

/*
 *  CPP phase, find and remove #includes
 */
static int parse_cpp_includes(FILE *fp)
{
	token t;
	parser p;

	parser_new(&p, fp, false);
	p.fp = fp;
	p.skip_white_space = false;

	token_new(&t);

	while ((get_token(&p, &t)) != EOF) {
		if (t.type == TOKEN_CPP) {
			for (;;) {
				token_clear(&t);
				if (get_token(&p, &t) == EOF) {
					token_free(&t);
					return EOF;
				}
				if (strcmp(t.token, "\n") == 0)
					break;
				if (t.type == TOKEN_WHITE_SPACE) {
					continue;
				}
				if (strcmp(t.token, "include") == 0) {
					if (parse_cpp_include(&p, &t) == EOF) {
						token_free(&t);
						return EOF;
					}
					break;
				}
				printf("#%s", t.token);
				break;
			}
		} else {
			printf("%s", t.token);
		}
		token_clear(&t);
	}
	token_free(&t);
	return EOF;
}

/*
 *  Scan kernel source for printk KERN_ERR and dev_err
 *  calls.
 *
 *  Usage:
 *	cat drivers/pnp/pnpacpi/rsparser.c | kernelscan -E | gcc  -E - | kernelscan -P
 *
 *  This prints out any kernel printk KERN_ERR calls
 *  or dev_err calls and checks to see if the error can be matched by
 *  any of the fwts klog messages.  It has some intelligence, it glues
 *  literal strings together such as "this is" "a message" into
 *  "this is a message" before it makes the klog comparison.
 */
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "%s: [-E] [-P]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/*
	 *  GCC -E preprocess phase
	 */
	if (strcmp(argv[1], "-E") == 0) {
		parse_cpp_includes(stdin);
		exit(EXIT_SUCCESS);
	}

	/*
	 *  Parse kernel printk and dev_err phase
	 */
	if (strcmp(argv[1], "-P") == 0) {
		patterns = klog_load("firmware_error_warning_patterns");
		parse_kernel_messages(stdin);
		klog_free(patterns);
	}

	exit(EXIT_SUCCESS);
}
