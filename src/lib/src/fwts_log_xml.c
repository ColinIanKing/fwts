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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>

#include "fwts.h"

#define MAX_XML_STACK	(64)
#define XML_INDENT	(4)

typedef struct {
	const char *name;
} fwts_log_xml_stack_t;

static fwts_log_xml_stack_t xml_stack[MAX_XML_STACK];
static int xml_stack_index = 0;

/*
 *  fwts_log_vprintf_xml()
 *	vprintf to a log
 */
static int fwts_log_vprintf_xml(fwts_log *log,
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
	char *str;

	if (!((field & LOG_FIELD_MASK) & fwts_log_filter))
		return 0;

	if (field & (LOG_NEWLINE | LOG_SEPARATOR | LOG_DEBUG))
		return 0;

	time(&now);
	localtime_r(&now, &tm);

	fprintf(log->fp, "%*s<logentry>\n", xml_stack_index * XML_INDENT, "");

	fprintf(log->fp, "%*s<line_num>%d</line_num>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", log->line_number);

	fprintf(log->fp, "%*s<date>%2.2d/%2.2d/%-2.2d</date>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", tm.tm_mday, tm.tm_mon + 1, (tm.tm_year+1900) % 100);

	fprintf(log->fp, "%*s<time>%2.2d:%2.2d:%2.2d</time>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", tm.tm_hour, tm.tm_min, tm.tm_sec);

	fprintf(log->fp, "%*s<field_type>%s</field_type>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", fwts_log_field_to_str_full(field));

	str = fwts_log_level_to_str(level);
	if (!strcmp(str, " "))
		str = "None";

	fprintf(log->fp, "%*s<level>%s</level>\n",
		(xml_stack_index + 1) * XML_INDENT, "", str);

	fprintf(log->fp, "%*s<status>%s</status>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", *status ? status : "None");

	fprintf(log->fp, "%*s<failure_label>%s</failure_label>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", label && *label ? label : "None");

	vsnprintf(buffer, sizeof(buffer), fmt, args);
	fprintf(log->fp, "%*s<log_text>%s</log_text>\n",
		(xml_stack_index + 1) * XML_INDENT,
		"", buffer);

	fprintf(log->fp, "%*s</logentry>\n", xml_stack_index * XML_INDENT, "");
	fflush(log->fp);

	log->line_number++;

	return 0;
}

/*
 *  fwts_log_underline_xml()
 *	write an underline across log, using character ch as the underline
 */
static void fwts_log_underline_xml(fwts_log *log, const int ch)
{
	/* No-op for xml */
}

/*
 *  fwts_log_newline()
 *	write newline to log
 */
static void fwts_log_newline_xml(fwts_log *log)
{
	/* No-op for xml */
}

static void fwts_log_section_begin_xml(fwts_log *log, const char *name)
{
	xml_stack[xml_stack_index].name = name;

	fprintf(log->fp, "%*s<%s>\n", xml_stack_index * XML_INDENT, "", name);
	fflush(log->fp);

	if (xml_stack_index < MAX_XML_STACK)
		xml_stack_index++;
	else  {
		fprintf(stderr, "xml log stack overflow pushing section %s.\n", name);
		exit(EXIT_FAILURE);
	}
}

static void fwts_log_section_end_xml(fwts_log *log)
{
	if (xml_stack_index > 0) {
		xml_stack_index--;
		fprintf(log->fp, "%*s</%s>\n", xml_stack_index * XML_INDENT,
			"", xml_stack[xml_stack_index].name);
		fflush(log->fp);
	} else {
		fprintf(stderr, "xml log stack underflow.\n");
		exit(EXIT_FAILURE);
	}

}

static void fwts_log_open_xml(fwts_log *log)
{
	char *xml_header = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";

	fwrite(xml_header, 1, strlen(xml_header), log->fp);
	fflush(log->fp);

	fwts_log_section_begin_xml(log, "fwts");
}

static void fwts_log_close_xml(fwts_log *log)
{
	fwts_log_section_end_xml(log);

	fwrite("\n", 1, 1, log->fp);
	fflush(log->fp);
}

fwts_log_ops fwts_log_xml_ops = {
	.vprintf = 	 fwts_log_vprintf_xml,
	.underline =	 fwts_log_underline_xml,
	.newline =	 fwts_log_newline_xml,
	.section_begin = fwts_log_section_begin_xml,
	.section_end   = fwts_log_section_end_xml,
	.open          = fwts_log_open_xml,
	.close	       = fwts_log_close_xml
};
