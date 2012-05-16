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

#include <json/json.h>
#include "fwts.h"

#define MAX_JSON_STACK	(64)

typedef struct {
	json_object	*obj;
	json_object	*log;
} fwts_log_json_stack_t;

static fwts_log_json_stack_t json_stack[MAX_JSON_STACK];
static int json_stack_index = 0;

/*
 *  fwts_log_vprintf_json()
 *	vprintf to a log
 */
static int fwts_log_vprintf_json(fwts_log *log,
	const fwts_log_field field,
	const fwts_log_level level,
	const char *status,
	const char *label,
	const char *prefix,
	const char *fmt,
	va_list args)
{
	char buffer[4096];
	struct tm tm;
	time_t now;
	json_object *header;
	json_object *json_log = (json_object*)json_stack[json_stack_index-1].log;
	char *str;

	if (!((field & LOG_FIELD_MASK) & fwts_log_filter))
		return 0;

	if (field & (LOG_NEWLINE | LOG_SEPARATOR | LOG_DEBUG))
		return 0;

	time(&now);
	localtime_r(&now, &tm);

	header = json_object_new_object();
	json_object_object_add(header, "line_num", json_object_new_int(log->line_number));
	snprintf(buffer, sizeof(buffer), "%2.2d/%2.2d/%-2.2d",
		tm.tm_mday, tm.tm_mon + 1, (tm.tm_year+1900) % 100);
	json_object_object_add(header, "date", json_object_new_string(buffer));
	snprintf(buffer, sizeof(buffer), "%2.2d:%2.2d:%2.2d",
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	json_object_object_add(header, "time", json_object_new_string(buffer));
	json_object_object_add(header, "field_type",
		json_object_new_string(fwts_log_field_to_str_full(field)));

	str = fwts_log_level_to_str(level);
	if (!strcmp(str, " "))
		str = "None";
	json_object_object_add(header, "level",
		json_object_new_string(str));

	json_object_object_add(header, "status", json_object_new_string(*status ? status : "None"));
	json_object_object_add(header, "failure_label", json_object_new_string(label && *label ? label : "None"));

	/* Redundant really
	json_object_object_add(header, "owner",
		json_object_new_string(log->owner));
	*/
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	json_object_object_add(header, "log_text", json_object_new_string(buffer));

	json_object_array_add(json_log, header);

	log->line_number++;

	return 0;
}

/*
 *  fwts_log_underline_json()
 *	write an underline across log, using character ch as the underline
 */
static void fwts_log_underline_json(fwts_log *log, const int ch)
{
	/* No-op for json */
}

/*
 *  fwts_log_newline()
 *	write newline to log
 */
static void fwts_log_newline_json(fwts_log *log)
{
	/* No-op for json */
}

static void fwts_log_section_begin_json(fwts_log *log, const char *name)
{
	json_object *json_obj;
	json_object *json_log;

	json_obj = json_object_new_object();
	json_log = json_object_new_array();
	json_object_object_add(json_obj, name, json_log);

	json_stack[json_stack_index].obj = json_obj;
	json_stack[json_stack_index].log = json_log;

	if (json_stack_index > 0) 
		json_object_array_add(json_stack[json_stack_index-1].log, json_obj);

	if (json_stack_index < MAX_JSON_STACK)
		json_stack_index++;
	else  {
		fprintf(stderr, "json log stack overflow pushing section %s.\n", name);
		exit(EXIT_FAILURE);
	}
}

static void fwts_log_section_end_json(fwts_log *log)
{
	if (json_stack_index > 0)
		json_stack_index--;
	else {
		fprintf(stderr, "json log stack underflow.\n");
		exit(EXIT_FAILURE);
	}
}

static void fwts_log_open_json(fwts_log *log)
{
	fwts_log_section_begin_json(log, "fwts");
}

static void fwts_log_close_json(fwts_log *log)
{
	const char *str;
	size_t len;

	fwts_log_section_end_json(log);

	str = json_object_to_json_string(json_stack[0].obj);
	len = strlen(str);

	fwrite(str, 1, len, log->fp);
	fwrite("\n", 1, 1, log->fp);
	fflush(log->fp);
	json_object_put(json_stack[0].obj);
}

fwts_log_ops fwts_log_json_ops = {
	.vprintf = 	 fwts_log_vprintf_json,
	.underline =	 fwts_log_underline_json,
	.newline =	 fwts_log_newline_json,
	.section_begin = fwts_log_section_begin_json,
	.section_end   = fwts_log_section_end_json,
	.open          = fwts_log_open_json,
	.close	       = fwts_log_close_json
};
