/*
 * Copyright (C) 2010 Canonical
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "log.h"

#define LOG_UNKOWN_FIELD	"???"

static fwts_log_field fwts_log_filter = ~0;

static char fwts_log_format[256] = "%date %time [%field] %owner ";

static char *fwts_log_field_to_str(fwts_log_field field)
{
	switch (field) {
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
	default:
		return LOG_UNKOWN_FIELD;
	}
}

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


static fwts_log_field fwts_log_str_to_field(const char *text)
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
		{ "ALL", ~0 },
		{ NULL, 0 }
	};

	for (i=0; mappings[i].text != NULL; i++) {
		if (strcmp(mappings[i].text, text) == 0) {
			return mappings[i].field;
		}
	}
	return 0;
}

void fwts_log_filter_set_field(const fwts_log_field filter)
{
	fwts_log_filter |= filter;
}

void fwts_log_filter_unset_field(const fwts_log_field filter)
{
	fwts_log_filter &= ~filter;
}

void fwts_log_set_field_filter(char *str)
{	
	char *token;
	char *saveptr;
	fwts_log_field field;

	for (;; str=NULL) {
		if ((token = strtok_r(str, ",|", &saveptr)) == NULL)
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

void fwts_log_set_format(char *str)
{
	strncpy(fwts_log_format, str, sizeof(fwts_log_format)-1);	
	fwts_log_format[sizeof(fwts_log_format)-1]='\0';
}

static void fwts_log_handle_newlines(fwts_log *log, char *str, int posn)
{
	char *ptr1,*ptr2;

	for (ptr1=ptr2=str+posn; *ptr1; ptr1++) {
		if (*ptr1=='\n') {
			fwrite(str,  1, posn, log->fp);
			fwrite(ptr2, 1, ptr1-ptr2+1, log->fp);
			fflush(log->fp);
			ptr2 = ptr1+1;
		}
	}

	if (ptr1-ptr2) {
		fwrite(str,  1, posn, log->fp);
		fwrite(ptr2, 1, ptr1-ptr2, log->fp);
		fwrite("\n", 1, 1, log->fp);
		fflush(log->fp);
	}
}

int fwts_log_printf(fwts_log *log, fwts_log_field field, const char *fmt, ...)
{
	char buffer[1024];
	int n = 0;
	struct tm tm;
	time_t now;
	va_list ap;
	char *ptr;

	if ((!log) || (log && log->magic != LOG_MAGIC))
		return 0;

	if (!(field & fwts_log_filter))
		return 0;

	va_start(ap, fmt);

	time(&now);
	localtime_r(&now, &tm);

	for (ptr = fwts_log_format; *ptr; ) {
		if (*ptr == '%') {
			ptr++;
			if (strncmp(ptr,"date",4)==0) {
				n += snprintf(buffer+n, sizeof(buffer)-n, 
					"%2.2d/%2.2d/%-2.2d", 		
					tm.tm_mday, tm.tm_mon, (tm.tm_year+1900) % 100);
				ptr+=4;
			}
			if (strncmp(ptr,"time",4)==0) {
				n += snprintf(buffer+n, sizeof(buffer)-n, 
					"%2.2d:%2.2d:%2.2d", 		
					tm.tm_hour, tm.tm_min, tm.tm_sec);
				ptr+=4;
			}
			if (strncmp(ptr,"field",5)==0) {
				n += snprintf(buffer+n, sizeof(buffer)-n, "%s",
					fwts_log_field_to_str(field));
				ptr+=5;
			}
			if (strncmp(ptr,"owner",5)==0 && log->owner) {
				n += snprintf(buffer+n, sizeof(buffer)-n, "%-15.15s", log->owner);
				ptr+=5;
			}
		}
		else {
			n += snprintf(buffer+n, sizeof(buffer)-n, "%c", *ptr);
			ptr++;
		}
	}
	vsnprintf(buffer+n, sizeof(buffer)-n, fmt, ap);
	fwts_log_handle_newlines(log, buffer, n);


	va_end(ap);

	return n;
}

void fwts_log_underline(fwts_log *log, int ch)
{
	int i;

	char buffer[60];

	for (i=0;i<59;i++) {	
		buffer[i] = ch;
	}
	buffer[i] = '\0';

	fwts_log_printf(log, LOG_SEPARATOR, buffer);
}

void fwts_log_newline(fwts_log *log)
{
	if (log && (log->magic == LOG_MAGIC)) {
		fwrite("\n", 1, 1, log->fp);
		fflush(log->fp);
	}
}

int fwts_log_set_owner(fwts_log *log, const char *owner)
{
	if (log && (log->magic == LOG_MAGIC)) {
		char *newowner = strdup(owner);
		if (newowner) {
			if (log->owner)
				free(log->owner);
			log->owner = newowner;
			return 0;
		}
	}
	return 1;
}

fwts_log *fwts_log_open(const char *owner, const char *name, const char *mode)
{
	fwts_log *newlog;

	if ((newlog = malloc(sizeof(fwts_log))) == NULL) {
		return NULL;
	}

	newlog->magic = LOG_MAGIC;

	if (owner) {
		if ((newlog->owner = malloc(strlen(owner)+1)) == NULL) {		
			free(newlog);
			return NULL;
		}
		strcpy(newlog->owner, owner);
	}

	if (strcmp("stderr", name) == 0) {
		newlog->fp = stderr;
	} else if (strcmp("stdout", name) == 0) {
		newlog->fp = stdout;
	} else if ((newlog->fp = fopen(name, mode)) == NULL) {
		free(newlog);
		return NULL;
	}

	return newlog;
}

int fwts_log_close(fwts_log *log)
{
	if (log && (log->magic == LOG_MAGIC)) {
		if (log->fp && (log->fp != stdout && log->fp != stderr))
			fclose(log->fp);
		if (log->owner)
			free(log->owner);
		free(log);
	}
	return 0;
}
