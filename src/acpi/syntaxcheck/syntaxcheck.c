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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "fwts.h"

static char *syntaxcheck_headline(void)
{
	return "Re-assemble DSDT and find syntax errors and warnings.";
}

static fwts_list* error_output;

static int syntaxcheck_init(fwts_framework *fw)
{
	struct stat buffer;

	if (fwts_check_root_euid(fw))
		return 1;

        if (stat(fw->iasl ? fw->iasl : IASL, &buffer)) {
                fwts_log_error(fw, "Make sure iasl is installed.");
                return 1;
        }

	return 0;
}

static int syntaxcheck_deinit(fwts_framework *fw)
{
	return 0;
}

static int syntaxcheck_table(fwts_framework *fw, char *test, char *table, int which)
{
	fwts_list_element *item;
	int errors = 0;
	int warnings = 0;
	uint8 *tabledata;
	int size;

	tabledata = fwts_acpi_table_load(fw, table, which, &size);
	if (size == 0)
		return 2;		/* Table does not exist */

	if (tabledata == NULL) {
		fwts_log_error(fw, "Failed to load table for some reason!");
		return 1;
	}


	error_output = fwts_iasl_reassemble(fw, tabledata, size);
	free(tabledata);
	if (error_output == NULL) {
		fwts_log_error(fw, "Cannot re-assasemble with iasl.");
		return 1;
	}

	for (item = error_output->head; item != NULL; item = item->next) {
		int num;
		char ch;
		char *line = fwts_text_list_text(item);

		if ((sscanf(line, "%*s %d%c", &num, &ch) == 2) && ch == ':') {
			if (item->next != NULL) {
				char *nextline = fwts_text_list_text(item->next);
				if (!strncmp(nextline, "Error", 5)) {
					fwts_log_info_verbatum(fw, "%s", line);
					fwts_log_info_verbatum(fw, "%s", nextline);
					errors++;
				}
				if (!strncmp(nextline, "Warning", 7)) {
					fwts_log_info_verbatum(fw, "%s", line);
					fwts_log_info_verbatum(fw, "%s", nextline);
					warnings++;
				}
				item = item->next;
			}
			else {
				fwts_log_info(fw, "%s", line);
				fwts_log_error(fw, 
					"Could not find parser error message "
					"(this can happen if iasl segfaults!)");
			}
		}
	}
	fwts_text_list_free(error_output);

	if (errors > 0) {
		fwts_log_info(fw, "Found %d errors, %d warnings in %s.", errors, warnings, table);
		fwts_failed(fw, test);
	} else if (warnings > 0) {
		fwts_log_info(fw, "Found 0 errors, %d warnings in %s.", warnings, table);
		fwts_warning(fw, test);
	} else 
		fwts_passed(fw, test);

	return 0;
}

static int syntaxcheck_DSDT(fwts_framework *fw)
{
	return syntaxcheck_table(fw, "DSDT table reassembly (syntax check)", "DSDT", 0);
}

static int syntaxcheck_SSDT(fwts_framework *fw)
{
	int i;

	for (i=0; i < 100; i++) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "SSTD table #%d reassembly (syntax check).", i);
		int ret = syntaxcheck_table(fw, buffer, "SSDT", i);
		if (ret == 2)
			return 0;	/* Hit the last table */
		if (ret != 0)
			return 1;	/* Error! */
	}

	return 0;
}

static fwts_framework_tests syntaxcheck_tests[] = {
	syntaxcheck_DSDT,
	syntaxcheck_SSDT,
	NULL
};

static fwts_framework_ops syntaxcheck_ops = {
	syntaxcheck_headline,
	syntaxcheck_init,	
	syntaxcheck_deinit,
	syntaxcheck_tests
};

FRAMEWORK(syntaxcheck, &syntaxcheck_ops, TEST_ANYTIME);
