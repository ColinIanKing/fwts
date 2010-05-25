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

static log_field log_filter = ~0;

static char log_format[256] = "%date %time [%field] %owner %text";

static char *log_field_to_str(log_field field)
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

void log_print_fields(void)
{
	log_field field = 1;
	char *str;
	
	printf("Available fields: ");
	for (field=1; ; field <<= 1) {
		str = log_field_to_str(field);
		if (strcmp(str, LOG_UNKOWN_FIELD) == 0)
			break;
		printf("%s%s", field == 1 ? "" : ",", str);
	}
	printf("\n");
}


static log_field log_str_to_field(const char *text)
{
	int i;

	static struct mapping {
		char *text;
		log_field field;
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

void log_filter_set_field(const log_field filter)
{
	log_filter |= filter;
}

void log_filter_unset_field(const log_field filter)
{
	log_filter &= ~filter;
}

void log_set_field_filter(char *str)
{	
	char *token;
	char *saveptr;
	log_field field;

	for (;; str=NULL) {
		if ((token = strtok_r(str, ",|", &saveptr)) == NULL)
			break;
		if (*token == '^' || *token == '~') {
			field = log_str_to_field(token+1);
			if (field)
				log_filter_unset_field(field);
		}
		else {
			field = log_str_to_field(token);
			if (field)
				log_filter_set_field(field);
		}
	}
}

void log_set_format(char *str)
{
	strncpy(log_format, str, sizeof(log_format)-1);	
	log_format[sizeof(log_format)-1]='\0';
}

static void log_remove_newlines(char *str)
{
	char *ptr1,*ptr2;

	for (ptr1=ptr2=str; *ptr1; ) {
		if (*ptr1=='\n')
			ptr1++;
		else
			*ptr2++ = *ptr1++;
	}
	*ptr2 = '\0';
}

int log_printf(log *log, log_field field, const char *fmt, ...)
{
	char buffer[1024];
	int n = 0;
	struct tm tm;
	time_t now;
	va_list ap;
	char *ptr;

	if ((!log) || (log && log->magic != LOG_MAGIC))
		return 0;

	if (!(field & log_filter))
		return 0;

	va_start(ap, fmt);

	time(&now);
	localtime_r(&now, &tm);

	for (ptr = log_format; *ptr; ) {
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
					log_field_to_str(field));
				ptr+=5;
			}
			if (strncmp(ptr,"owner",5)==0 && log->owner) {
				n += snprintf(buffer+n, sizeof(buffer)-n, "%-15.15s", log->owner);
				ptr+=5;
			}
			if (strncmp(ptr,"text",4)==0) {	
				n+= vsnprintf(buffer+n, sizeof(buffer)-n, fmt, ap);
				ptr+=4;
			}	
		}
		else {
			n += snprintf(buffer+n, sizeof(buffer)-n, "%c", *ptr);
			ptr++;
		}
	}

	log_remove_newlines(buffer);
	fwrite(buffer, 1, strlen(buffer), log->fp);
	fwrite("\n", 1, 1, log->fp);
	fflush(log->fp);

	va_end(ap);

	return n;
}

void log_underline(log *log, int ch)
{
	int i;

	char buffer[60];

	for (i=0;i<59;i++) {	
		buffer[i] = ch;
	}
	buffer[i] = '\0';

	log_printf(log, LOG_SEPARATOR, buffer);
}

void log_newline(log *log)
{
	fwrite("\n", 1, 1, log->fp);
	fflush(log->fp);
}

int log_set_owner(log *log, const char *owner)
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

log *log_open(const char *owner, const char *name, const char *mode)
{
	log *newlog;

	if ((newlog = malloc(sizeof(log))) == NULL) {
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

int log_close(log *log)
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
