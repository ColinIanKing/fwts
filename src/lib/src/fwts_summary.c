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
#include <string.h>

#include "fwts.h"

typedef struct {
	char *test;		/* test that found the error */
	char *text;		/* text of failure message */
	fwts_list log_lines;	/* line where failure was reported */
} fwts_summary_item;

enum {
	SUMMARY_CRITICAL = 0,
	SUMMARY_HIGH,
	SUMMARY_MEDIUM,
	SUMMARY_LOW,
	SUMMARY_UNKNOWN,

	SUMMARY_MAX = SUMMARY_UNKNOWN+1
};

static const char *summary_names[] = {
	"Critical",
	"High",
	"Medium",
	"Low",
	"Other"
};

/* list of summary items per error level */
static fwts_list *fwts_summaries[SUMMARY_MAX];

void fwts_summary_deinit(void);

/*
 *  fwts_summary_init()
 *	initialise
 */
int fwts_summary_init(void)
{
	int i;

	/* initialise list of summary items for all error levels */
	for (i=0;i<SUMMARY_MAX;i++)
		if ((fwts_summaries[i] = fwts_list_new()) == NULL) {
			fwts_summary_deinit();
			return FWTS_ERROR;
		}

	return FWTS_OK;
}

/*
 *  fwts_summary_item_free()
 *	free a summary item
 */
static void fwts_summary_item_free(void *data)
{
	fwts_summary_item *item = (fwts_summary_item *)data;

	fwts_list_free_items(&(item->log_lines), free);
	free(item->test);
	free(item->text);
	free(item);
}

/*
 *  fwts_summary_init()
 *	free up summary lists
 */
void fwts_summary_deinit(void)
{
	int i;

	for (i=0;i<SUMMARY_MAX;i++)
		if (fwts_summaries[i])
			fwts_list_free(fwts_summaries[i], fwts_summary_item_free);
}

static int fwts_summary_level_to_index(fwts_log_level level)
{
	switch (level) {
	case LOG_LEVEL_CRITICAL:
		return SUMMARY_CRITICAL;
	case LOG_LEVEL_HIGH:
		return SUMMARY_HIGH;
	case LOG_LEVEL_MEDIUM:
		return SUMMARY_MEDIUM;
	case LOG_LEVEL_LOW:
		return SUMMARY_LOW;
	default:
		return SUMMARY_UNKNOWN;
	}
}

/*
 *  fwts_summary_add()
 *	add an error summary for a test with error message text at given
 *	error level to the list of summaries.
 */
int fwts_summary_add(fwts_framework *fw, const char *test, fwts_log_level level, char *text)
{
	fwts_list_link	*item;
	fwts_summary_item *summary_item = NULL;
	bool summary_item_found = false;
	int index = fwts_summary_level_to_index(level);
	int *line_num;

	if ((line_num = calloc(1, sizeof(int))) == NULL)
		return FWTS_ERROR;

	/* Does the text already exist? - search for it */
	fwts_list_foreach(item, fwts_summaries[index]) {
		summary_item = fwts_list_data(fwts_summary_item *,item);
		if (strcmp(text, summary_item->text) == 0) {
			summary_item_found = true;
			break;
		}
	}

	/* Not found, create a new one */
	if (!summary_item_found) {
		if ((summary_item = calloc(1, sizeof(fwts_summary_item))) == NULL) {
			free(line_num);
			return FWTS_ERROR;
		}
		if ((summary_item->test = strdup(test)) == NULL) {
			free(line_num);
			free(summary_item);
			return FWTS_ERROR;
		}
		if ((summary_item->text = strdup(text)) == NULL) {
			free(line_num);
			free(summary_item->test);	
			free(summary_item);	
			return FWTS_ERROR;
		}
		fwts_list_init(&(summary_item->log_lines));
		fwts_chop_newline(summary_item->text);
	}

	/* Now append a new line number to list of line numbers */

	*line_num = fwts_log_line_number(fw->results);
	fwts_list_append(&summary_item->log_lines, line_num);

	/* And append new item if not done so already */
	if (!summary_item_found)
		fwts_list_append(fwts_summaries[index], summary_item);

	return FWTS_OK;
}

static void fwts_summary_format_field(char *buffer, int buflen, uint32_t value)
{
	if (value)
		snprintf(buffer, buflen, "%5u", value);
	else
		*buffer = '\0';
}

static char *fwts_summary_lines(fwts_list *list)
{
	fwts_list_link *item;
	char *text = NULL;
	char tmp[16];
	int i = 0;

	fwts_list_foreach(item, list) {
		int *num = fwts_list_data(int *, item);
		snprintf(tmp, sizeof(tmp), "%s%d", 
			text == NULL ? "" : ", ", *num);
		text = fwts_realloc_strcat(text, tmp);
		if (i++ > 20) {
			text = fwts_realloc_strcat(text, "...");
			break;
		}
	}
	return text;
}

/*
 *  fwts_summary_report()
 *  	report test failure summary, sorted by error levels
 */
int fwts_summary_report(fwts_framework *fw, fwts_list *test_list)
{
	int i;
	fwts_list      *sorted;
	fwts_list_link *item;

	fwts_log_summary(fw, "Test Failure Summary");
	fwts_log_underline(fw->results, '=');
	fwts_log_nl(fw);

	for (i=0;i<SUMMARY_MAX;i++) {
		fwts_log_section_begin(fw->results, "failure");

		if (fwts_summaries[i]->len) {
			fwts_list_link *item;
			fwts_log_summary(fw, "%s failures: %d", summary_names[i], fwts_summaries[i]->len);

			fwts_log_section_begin(fw->results, "failures");
			fwts_list_foreach(item, fwts_summaries[i]) {
				fwts_summary_item *summary_item = fwts_list_data(fwts_summary_item *,item);
				char *lines = fwts_summary_lines(&summary_item->log_lines);
				fwts_log_summary_verbatum(fw, " %s test, at %d log line%s: %s: %s",
					summary_item->test,
					fwts_list_len(&summary_item->log_lines),
					fwts_list_len(&summary_item->log_lines) > 1 ? "s" : "",
					lines,
					summary_item->text);
				free(lines);
			}
			fwts_log_section_end(fw->results);
		}
		else
			fwts_log_summary(fw, "%s failures: NONE", summary_names[i]);

		fwts_log_section_end(fw->results);
		fwts_log_nl(fw);
	}

	if (fw->log_type == LOG_TYPE_PLAINTEXT && fw->total_run > 0) {		
		sorted = fwts_list_new();
		fwts_list_foreach(item, test_list)
			fwts_list_add_ordered(sorted, fwts_list_data(fwts_framework_test *,item), fwts_framework_compare_test_name);

		fwts_log_summary_verbatum(fw, "Test           |Pass |Fail |Abort|Warn |Skip |Info |");
		fwts_log_summary_verbatum(fw, "---------------+-----+-----+-----+-----+-----+-----+");
		fwts_list_foreach(item, sorted) {
			fwts_framework_test *test = fwts_list_data(fwts_framework_test*,item);
			if (test->was_run) {
				char passed[6];
				char failed[6];
				char aborted[6];
				char warning[6];
				char skipped[6];
				char infoonly[6];

				fwts_summary_format_field(passed, sizeof(passed), test->results.passed);
				fwts_summary_format_field(failed, sizeof(failed), test->results.failed);
				fwts_summary_format_field(aborted, sizeof(aborted), test->results.aborted);
				fwts_summary_format_field(warning, sizeof(warning), test->results.warning);
				fwts_summary_format_field(skipped, sizeof(skipped), test->results.skipped);
				fwts_summary_format_field(infoonly, sizeof(infoonly), test->results.infoonly);

				fwts_log_summary_verbatum(fw, "%-15.15s|%5.5s|%5.5s|%5.5s|%5.5s|%5.5s|%5.5s|",
					test->name, passed, failed, aborted, warning, skipped, infoonly);
			}
		}
		fwts_log_summary_verbatum(fw, "---------------+-----+-----+-----+-----+-----+-----+");
		fwts_log_summary_verbatum(fw, "Total:         |%5d|%5d|%5d|%5d|%5d|%5d|",
			fw->total.passed, fw->total.failed, fw->total.aborted,
			fw->total.warning, fw->total.skipped, fw->total.infoonly);
		fwts_log_summary_verbatum(fw, "---------------+-----+-----+-----+-----+-----+-----+");
		fwts_list_free(sorted, NULL);
	}
	return FWTS_OK;
}
