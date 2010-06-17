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
#include <string.h>

#include "fwts.h"

typedef struct {
	char *test;
	char *text;
	int  log_line;
} fwts_summary_item;

enum {
	SUMMARY_CRITICAL = 0,
	SUMMARY_HIGH,
	SUMMARY_MEDIUM,
	SUMMARY_LOW,
	SUMMARY_UNKNOWN,

	SUMMARY_MAX = SUMMARY_UNKNOWN+1
};

char *summary_names[] = {
	"Critical",
	"High",
	"Medium",
	"Low",
	"Other"
};

fwts_list *fwts_summaries[SUMMARY_MAX];

void fwts_summary_deinit(void);

int fwts_summary_init(void)
{
	int i;

	for (i=0;i<SUMMARY_MAX;i++)
		if ((fwts_summaries[i] = fwts_list_init()) == NULL) {
			fwts_summary_deinit();
			return FWTS_ERROR;
		}

	return FWTS_OK;
}

void fwts_summary_deinit(void)
{
	int i;

	for (i=0;i<SUMMARY_MAX;i++)
		if (fwts_summaries[i])
			fwts_list_free(fwts_summaries[i], free);
}

int fwts_summary_add(char *test, fwts_log_level level, char *text)
{
	fwts_summary_item *item;

	if ((item = malloc(sizeof(fwts_summary_item))) == NULL)
		return FWTS_ERROR;

	if ((item->test = strdup(test)) == NULL) {
		free(item);
		return FWTS_ERROR;
	}
	if ((item->text = strdup(text)) == NULL) {
		free(item->test);	
		free(item);	
		return FWTS_ERROR;
	}

	item->log_line = fwts_log_line_number();

	switch (level) {
	case LOG_LEVEL_CRITICAL:
		fwts_list_append(fwts_summaries[SUMMARY_CRITICAL], item);
		break;
	case LOG_LEVEL_HIGH:
		fwts_list_append(fwts_summaries[SUMMARY_HIGH], item);
		break;
	case LOG_LEVEL_MEDIUM:
		fwts_list_append(fwts_summaries[SUMMARY_MEDIUM], item);
		break;
	case LOG_LEVEL_LOW:
		fwts_list_append(fwts_summaries[SUMMARY_LOW], item);
		break;
	default:
		fwts_list_append(fwts_summaries[SUMMARY_UNKNOWN], item);
		break;
	}
	return FWTS_OK;
}

int fwts_summary_report(fwts_framework *fw)
{
	int i;

	fwts_log_summary(fw, "Test Failure Summary");
	fwts_log_summary(fw, "====================");
	fwts_log_summary(fw, "");

	for (i=0;i<SUMMARY_MAX;i++) {
		if (fwts_summaries[i]->len) {
			fwts_list_element *item;
			fwts_log_summary(fw, "%s failures: %d", summary_names[i], fwts_summaries[i]->len);

			for (item = fwts_summaries[i]->head; item != NULL; item = item->next) {
				fwts_summary_item *summary_item = (fwts_summary_item*)item->data;
				fwts_log_summary_verbatum(fw, " %s test at log line %d:", 
					summary_item->test, 
					summary_item->log_line);
				fwts_log_summary_verbatum(fw, "  \"%s\"",
					summary_item->text);
			}
		}
		else
			fwts_log_summary(fw, "%s failures: NONE", summary_names[i]);

		fwts_log_summary(fw, "");
	}

	return FWTS_OK;
}
