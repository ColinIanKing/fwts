/*
 * Copyright (C) 2010-2012 Canonical
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
#define LOG_LINE_WIDTH 100

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>

#include "fwts.h"

#define LOG_UNKOWN_FIELD	"???"

static int log_line_width = 0;

fwts_log_field fwts_log_filter = ~0;

const char *fwts_log_format = "";

/*
 *  fwts_log_set_line_width()
 * 	set width of a log
 */
void fwts_log_set_line_width(int width)
{
	if ((width >= 80) && (width <= 1024))
		log_line_width = width;
}

/*
 *  fwts_log_line_number()
 * 	get current line number of log
 */
int fwts_log_line_number(fwts_log *log)
{
	return log->line_number;
}

/*
 *  fwts_log_field_to_str()
 *	return string name of log field
 */
char *fwts_log_field_to_str(const fwts_log_field field)
{
	switch (field & LOG_FIELD_MASK) {
	case LOG_RESULT:
		return "RES";
	case LOG_ERROR:
		return "ERR";
	case LOG_WARNING:
		return "WRN";
	case LOG_DEBUG:
		return "DBG";
	case LOG_INFO:
		return "INF";
	case LOG_SUMMARY:
		return "SUM";
	case LOG_SEPARATOR:
		return "SEP";
	case LOG_NEWLINE:
		return "NLN";
	case LOG_ADVICE:
		return "ADV";
	case LOG_HEADING:
		return "HED";
	case LOG_TAG:
		return "TAG";
	default:
		return LOG_UNKOWN_FIELD;
	}
}

/*
 *  fwts_log_field_to_str_full()
 *	return full string name of log field
 */
char *fwts_log_field_to_str_full(const fwts_log_field field)
{
	switch (field & LOG_FIELD_MASK) {
	case LOG_RESULT:
		return "Result";
	case LOG_ERROR:
		return "Error";
	case LOG_WARNING:
		return "Warning";
	case LOG_DEBUG:
		return "Debug";
	case LOG_INFO:
		return "Info";
	case LOG_SUMMARY:
		return "Summary";
	case LOG_SEPARATOR:
		return "Separator";
	case LOG_NEWLINE:
		return "Newline";
	case LOG_ADVICE:
		return "Advice";
	case LOG_HEADING:
		return "Heading";
	case LOG_TAG:
		return "Tag";
	default:
		return "Unknown";
	}
}

/*
 *  fwts_log_str_to_level()
 *	return log level mapped from the given string
 */
int fwts_log_str_to_level(const char *str)
{
	if (str) {
		if (strstr(str, "CRITICAL"))
			return LOG_LEVEL_CRITICAL;
		if (strstr(str, "HIGH"))
			return LOG_LEVEL_HIGH;
		if (strstr(str, "MEDIUM"))
			return LOG_LEVEL_MEDIUM;
		if (strstr(str, "LOW"))
			return LOG_LEVEL_LOW;
		if (strstr(str, "INFO"))
			return LOG_LEVEL_INFO;
	}

	return LOG_LEVEL_MEDIUM;	/* Default */
}

/*
 *  fwts_log_level_to_str()
 *	return string name of log level
 */
char *fwts_log_level_to_str(const fwts_log_level level)
{
	switch (level) {
	case LOG_LEVEL_CRITICAL:
		return "CRITICAL";
	case LOG_LEVEL_HIGH:
		return "HIGH";
	case LOG_LEVEL_MEDIUM:
		return "MEDIUM";
	case LOG_LEVEL_LOW:
		return "LOW";
	case LOG_LEVEL_INFO:
		return "INFO";
	case LOG_LEVEL_NONE:
	default:
		return " ";
	}
}

/*
 *  fwts_log_print_fields()
 *	dump out available log field names
 */
void fwts_log_print_fields(void)
{
	fwts_log_field field = 1;
	char *str;

	printf("Available fields: ");
	for (field=1; ; field <<= 1) {
		str = fwts_log_field_to_str(field);
		if (strcmp(str, LOG_UNKOWN_FIELD) == 0)
			break;
		printf("%s%s", field == 1 ? "" : ",", str);
	}
	printf("\n");
}


/*
 *  fwts_log_str_to_field()
 *	return log field of a given string, 0 if not matching
 */
fwts_log_field fwts_log_str_to_field(const char *text)
{
	int i;

	static struct mapping {
		char *text;
		fwts_log_field field;
	} mappings[] = {
		{ "RES", LOG_RESULT },
		{ "ERR", LOG_ERROR },
		{ "WRN", LOG_WARNING },
		{ "DBG", LOG_DEBUG },
		{ "INF", LOG_INFO },
		{ "SUM", LOG_SUMMARY },
		{ "SEP", LOG_SEPARATOR },
		{ "ADV", LOG_ADVICE },
		{ "HED", LOG_HEADING },
		{ "TAG", LOG_TAG },
		{ "ALL", ~0 },
		{ NULL, 0 }
	};

	for (i=0; mappings[i].text != NULL; i++)
		if (strcmp(mappings[i].text, text) == 0)
			return mappings[i].field;
	return 0;
}

/*
 *  fwts_log_filter_set_field
 *	set a filter bit map
 */
void fwts_log_filter_set_field(const fwts_log_field filter)
{
	fwts_log_filter |= filter;
}

/*
 *  fwts_log_filter_unset_field
 *	clear a filter bit map
 */
void fwts_log_filter_unset_field(const fwts_log_field filter)
{
	fwts_log_filter &= ~filter;
}

/*
 *  fwts_log_set_field_filter
 *	set/clear filter bit map based on a comma separated
 *	list of field names. ~ or ^ prefix to a field name
 *	clears a bit.
 */
void fwts_log_set_field_filter(const char *str)
{
	char *token;
	char *saveptr;
	fwts_log_field field;

	for (;; str=NULL) {
		if ((token = strtok_r((char*)str, ",|", &saveptr)) == NULL)
			break;
		if (*token == '^' || *token == '~') {
			field = fwts_log_str_to_field(token+1);
			if (field)
				fwts_log_filter_unset_field(field);
		}
		else {
			field = fwts_log_str_to_field(token);
			if (field)
				fwts_log_filter_set_field(field);
		}
	}
}

/*
 *  fwts_log_set_format()
 *	set the log format string
 */
void fwts_log_set_format(const char *str)
{
	fwts_log_format = str;
}

/*
 *  fwts_log_vprintf()
 *	printf to a log
 */
int fwts_log_printf(fwts_log *log,
	const fwts_log_field field,
	const fwts_log_level level,
	const char *status,
	const char *label,
	const char *prefix,
	const char *fmt, ...)
{
	va_list	args;
	int ret;

	va_start(args, fmt);
	ret = fwts_log_vprintf(log, field, level, status, label, prefix, fmt, args);
	va_end(args);

	return ret;
}

/*
 *  fwts_log_vprintf()
 *	vprintf to a log
 */
int fwts_log_vprintf(fwts_log *log,
	const fwts_log_field field,
	const fwts_log_level level,
	const char *status,
	const char *label,
	const char *prefix,
	const char *fmt,
	va_list args)
{
	if (!((field & LOG_FIELD_MASK) & fwts_log_filter))
		return 0;

	if (log && log->magic == LOG_MAGIC &&
	    log->ops && log->ops->underline)
		return log->ops->vprintf(log, field, level, status, label, prefix, fmt, args);
	else
		return 0;
}

/*
 *  fwts_log_underline()
 *	write an underline across log, using character ch as the underline
 */
void fwts_log_underline(fwts_log *log, const int ch)
{
	if (log && log->magic == LOG_MAGIC &&
	    log->ops && log->ops->underline)
		log->ops->underline(log, ch);
}

/*
 *  fwts_log_newline()
 *	write newline to log
 */
void fwts_log_newline(fwts_log *log)
{
	if (log && log->magic == LOG_MAGIC &&
	    log->ops && log->ops->underline)
		log->ops->newline(log);
}

int fwts_log_set_owner(fwts_log *log, const char *owner)
{
	if (log && (log->magic == LOG_MAGIC)) {
		char *newowner = strdup(owner);
		if (newowner) {
			if (log->owner)
				free(log->owner);
			log->owner = newowner;
			return FWTS_OK;
		}
	}
	return FWTS_ERROR;
}

void fwts_log_section_begin(fwts_log *log, const char *name)
{
	if (log && log->magic == LOG_MAGIC &&
	    log->ops && log->ops->section_begin)
		log->ops->section_begin(log, name);
}

void fwts_log_section_end(fwts_log *log)
{
	if (log && log->magic == LOG_MAGIC &&
	    log->ops && log->ops->section_end)
		log->ops->section_end(log);
}

/*
 *  fwts_log_open()
 *	open a log file. if name is stderr or stdout, then attach log to these
 *	streams.
 */
fwts_log *fwts_log_open(const char *owner, const char *name, const char *mode, fwts_log_type type)
{
	fwts_log *newlog;

	if ((newlog = calloc(1, sizeof(fwts_log))) == NULL)
		return NULL;

	newlog->magic = LOG_MAGIC;
	switch (type) {
	case LOG_TYPE_JSON:
		newlog->ops = &fwts_log_json_ops;
		break;
	case LOG_TYPE_PLAINTEXT:
		newlog->ops = &fwts_log_plaintext_ops;
		break;
	case LOG_TYPE_NONE:
	default:
		newlog->ops = &fwts_log_plaintext_ops;
		break;
	}

	if (owner) {
		if ((newlog->owner = calloc(1, strlen(owner)+1)) == NULL) {
			free(newlog);
			return NULL;
		}
		strcpy(newlog->owner, owner);
	}

	if (strcmp("stderr", name) == 0)
		newlog->fp = stderr;
	else if (strcmp("stdout", name) == 0)
		newlog->fp = stdout;
	else if ((newlog->fp = fopen(name, mode)) == NULL) {
		free(newlog);
		return NULL;
	}

	if (log_line_width) {
		/* User has specified width, so use it */
		newlog->line_width = log_line_width;
	} else {
		newlog->line_width = fwts_tty_width(fileno(newlog->fp), LOG_LINE_WIDTH);
	}

	if (newlog->ops && newlog->ops->open)
		newlog->ops->open(newlog);

	return newlog;
}

/*
 *  fwts_log_close()
 *	close a log file
 */
int fwts_log_close(fwts_log *log)
{
	if (log && (log->magic == LOG_MAGIC)) {
		if (log->ops && log->ops->close)
			log->ops->close(log);

		if (log->fp && (log->fp != stdout && log->fp != stderr))
			fclose(log->fp);
		if (log->owner)
			free(log->owner);
		free(log);
	}
	return FWTS_OK;
}
