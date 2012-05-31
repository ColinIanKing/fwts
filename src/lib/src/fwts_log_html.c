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

#define MAX_HTML_STACK	(64)
#define HTML_INDENT	(2)

typedef struct {
	const char *name;
} fwts_log_html_stack_t;

static fwts_log_html_stack_t html_stack[MAX_HTML_STACK];
static int html_stack_index = 0;

static void fwts_log_html(fwts_log *log, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	fprintf(log->fp, "%*s", html_stack_index * HTML_INDENT, "");
	vfprintf(log->fp, fmt, args);

	va_end(args);
}


/*
 *  fwts_log_vprintf_html()
 *	vprintf to a log
 */
static int fwts_log_vprintf_html(fwts_log *log,
	const fwts_log_field field,
	const fwts_log_level level,
	const char *status,
	const char *label,
	const char *prefix,
	const char *fmt,
	va_list args)
{
	struct tm tm;
	time_t now;
	char *str;
	char *style;
	char *code_start;
	char *code_end;

	if (!((field & LOG_FIELD_MASK) & fwts_log_filter))
		return 0;

	if (field & (LOG_NEWLINE | LOG_SEPARATOR | LOG_DEBUG))
		return 0;

	time(&now);
	localtime_r(&now, &tm);

	fwts_log_html(log, "<TR>\n");

	if ((field & LOG_FIELD_MASK) != LOG_HEADING) {
		fwts_log_html(log, "  <TD>%2.2d/%2.2d/%-2.2d</TD>\n",
			tm.tm_mday, tm.tm_mon + 1, (tm.tm_year+1900) % 100);
		fwts_log_html(log, "  <TD>%2.2d:%2.2d:%2.2d</TD>\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	if (field & LOG_VERBATUM) {
		code_start = "<PRE class=style_code>";
		code_end   = "</PRE>";
	} else {
		code_start = "";
		code_end   = "";
	}

	switch (field & LOG_FIELD_MASK) {
	case LOG_ERROR:
		fwts_log_html(log, "  <TD class=style_error>Error</TD><TD COLSPAN=3>");
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "</TD>\n");
		break;
	case LOG_WARNING:
		fwts_log_html(log, "  <TD class=style_error>Warning</TD><TD COLSPAN=3 class=style_advice_info>%s", code_start);
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "%s</TD>\n", code_end);
		break;
	case LOG_HEADING:
		fwts_log_html(log, "<TD COLSPAN=4 class=style_heading>%s", code_start);
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "%s</TD>\n", code_end);
		break;
	case LOG_INFO:
		fwts_log_html(log, "  <TD></TD><TD COLSPAN=3 class=style_infos>%s", code_start);
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "%s</TD>\n", code_end);
		break;
	case LOG_PASSED:
		fwts_log_html(log, "<TD class=style_passed>PASSED</TD><TD>");
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "</TD>\n");
		break;
	case LOG_FAILED:
		switch (level) {
		case LOG_LEVEL_CRITICAL:
			style = " class=style_critical";
			break;
		case LOG_LEVEL_HIGH:
			style = " class=style_high";
			break;
		case LOG_LEVEL_MEDIUM:
			style = " class=style_medium";
			break;
		case LOG_LEVEL_LOW:
			style = " class=style_low";
			break;
		case LOG_LEVEL_INFO:
		case LOG_LEVEL_NONE:
		default:
			style = "";
			break;
		}
		str = fwts_log_level_to_str(level);

		fwts_log_html(log, "  <TD%s>%s [%s]</TD>\n", style, *status ? status : "", str);

		fwts_log_html(log, "  <TD>");
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "</TD>\n");
		break;

	case LOG_SKIPPED:
		fwts_log_html(log, "<TD class=style_skipped>Skipped</TD><TD>%s", code_start);
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "%s</TD>\n", code_end);
		break;

	case LOG_SUMMARY:
		fwts_log_html(log, "  <TD></TD><TD COLSPAN=3 class=style_summary>%s", code_start);
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "%s</TD>\n", code_end);
		break;

	case LOG_ADVICE:
		fwts_log_html(log, "  <TD class=style_advice>Advice</TD><TD COLSPAN=3 class=style_advice_info>%s", code_start);
		vfprintf(log->fp, fmt, args);
		fprintf(log->fp, "%s</TD>\n", code_end);
		break;

	default:
		break;
	}

	fwts_log_html(log, "</TR>\n");
	fflush(log->fp);

	log->line_number++;

	return 0;
}

/*
 *  fwts_log_underline_html()
 *	write an underline across log, using character ch as the underline
 */
static void fwts_log_underline_html(fwts_log *log, const int ch)
{
	/* No-op for html */
}

/*
 *  fwts_log_newline()
 *	write newline to log
 */
static void fwts_log_newline_html(fwts_log *log)
{
	/* No-op for html */
}

static void fwts_log_section_begin_html(fwts_log *log, const char *name)
{
	html_stack[html_stack_index].name = name;

	if (!strcmp(name, "summary")) {
		fwts_log_html(log, "<TR><TD class=style_heading COLSPAN=4>Summary</TD></TR>\n");
	} else if (!strcmp(name, "heading")) {
		fwts_log_html(log, "<TR><TD class=style_heading COLSPAN=4>Firmware Test Suite</TD></TR>\n");
	} else if (!strcmp(name, "subtest_info")) {
		fwts_log_html(log, "<TR><TD class=style_subtest COLSPAN=4></TD></TR>\n");
	} else if (!strcmp(name, "failure")) {
		fwts_log_html(log, "<TR><TD class=style_heading COLSPAN=4></TD></TR>\n");
	}

	fflush(log->fp);

	if (html_stack_index < MAX_HTML_STACK)
		html_stack_index++;
	else  {
		fprintf(stderr, "html log stack overflow pushing section %s.\n", name);
		exit(EXIT_FAILURE);
	}
}

static void fwts_log_section_end_html(fwts_log *log)
{
	if (html_stack_index > 0) {
		html_stack_index--;
		fflush(log->fp);
	} else {
		fprintf(stderr, "html log stack underflow.\n");
		exit(EXIT_FAILURE);
	}

}

static void fwts_log_open_html(fwts_log *log)
{
	fwts_log_html(log, "<HTML>\n");
	fwts_log_html(log, "<HEAD>\n");
	fwts_log_html(log, "  <TITLE>fwts log</TITLE>\n");
	fwts_log_html(log, "</HEAD>\n");
	fwts_log_html(log, "<BODY>\n");
	fwts_log_html(log, "<STYLE>\n");
	fwts_log_html(log,
		".style_critical { background-color: red; font-weight: bold; "
		"text-align: center; vertical-align: center  }\n"
		".style_high { background-color: orange; font-weight: bold; "
		"text-align: center; vertical-align: center  }\n"
		".style_medium { background-color: yellow; font-weight: bold; "
		"text-align: center; vertical-align: center  }\n"
		".style_low { background-color: #9acd32; font-weight: bold; "
		"text-align: center; vertical-align: center  }\n"
		".style_passed { background-color: green; font-weight: bold; "
		"text-align: center; vertical-align: center  }\n"
		".style_advice { text-align: center; vertical-align: center; "
		"font-weight: bold }\n"
		".style_advice_info { font-style: italic; font-weight: bold }"
		".style_skipped { background-color: wheat; text-align: center; "
		"vertical-align: center }\n"
		".style_heading { background-color: wheat; font-weight: bold; "
		"text-align: center }\n"
		".style_summary { font-weight: bold }\n"
		".style_error { background-color: orange; font-weight: bold; "
		"text-align: center; vertical-align: center }\n"
		".style_subtest { background-color: lightgray; }\n"
		".style_info { }\n"
		".style_code { font-family: \"courier\",\"mono\"; font-size:0.75em; overflow:auto; "
		"width:90%; line-height:0.82em; font-stretch:extra-condensed; word-wrap:normal }\n");
	fwts_log_html(log, "</STYLE>\n");
	fflush(log->fp);

	fwts_log_html(log, "<TABLE WIDTH=1024>\n");
	fwts_log_html(log, "</TR>\n");

	fwts_log_section_begin_html(log, "fwts");
}

static void fwts_log_close_html(fwts_log *log)
{
	fwts_log_section_end_html(log);

	fwts_log_html(log, "</TABLE>\n");
	fwts_log_html(log, "</BODY>\n");
	fwts_log_html(log, "</HTML>\n");
	fflush(log->fp);
}

fwts_log_ops fwts_log_html_ops = {
	.vprintf = 	 fwts_log_vprintf_html,
	.underline =	 fwts_log_underline_html,
	.newline =	 fwts_log_newline_html,
	.section_begin = fwts_log_section_begin_html,
	.section_end   = fwts_log_section_end_html,
	.open          = fwts_log_open_html,
	.close	       = fwts_log_close_html
};
