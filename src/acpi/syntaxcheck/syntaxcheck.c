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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

static char *syntaxcheck_headline(void)
{
	return "Re-assemble DSDT and find syntax errors and warnings.";
}

static fwts_list* error_output;

static int syntaxcheck_init(fwts_framework *fw)
{
	if (fwts_check_executable(fw, fw->iasl, "iasl"))
		return FWTS_ERROR;

	return FWTS_OK;
}

static int syntaxcheck_table(fwts_framework *fw, char *tablename, int which)
{
	fwts_list_link *item;
	int errors = 0;
	int warnings = 0;
	fwts_acpi_table_info *table;

	if (fwts_acpi_find_table(fw, tablename, which, &table) != FWTS_OK) {
		fwts_aborted(fw, "Cannot load ACPI table %s.", tablename);
		return FWTS_ERROR;
	}

	if (table == NULL) 
		return FWTS_NO_TABLE;		/* Table does not exist */

	if (fwts_iasl_reassemble(fw, table->data, table->length, &error_output) != FWTS_OK) {
		fwts_aborted(fw, "Cannot re-assasemble with iasl.");
		return FWTS_ERROR;
	}

	fwts_list_foreach(item, error_output) {
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
		fwts_failed_high(fw, "Table %s (%d) reassembly: Found %d errors, %d warnings.", table, which, errors, warnings);
	} else if (warnings > 0) {
		fwts_failed_low(fw, "Table %s (%d) reassembly: Found 0 errors, %d warnings.", table, which, warnings);
	} else 
		fwts_passed(fw, "%s (%d) reassembly, Found 0 errors, 0 warnings.", table, which);

	return FWTS_OK;
}

static int syntaxcheck_DSDT(fwts_framework *fw)
{
	return syntaxcheck_table(fw, "DSDT", 0);
}

static int syntaxcheck_SSDT(fwts_framework *fw)
{
	int i;

	for (i=0; i < 100; i++) {
		int ret = syntaxcheck_table(fw, "SSDT", i);
		if (ret == FWTS_NO_TABLE)
			return FWTS_OK;	/* Hit the last table */
		if (ret != FWTS_OK)
			return FWTS_ERROR;
	}

	return FWTS_OK;
}

static fwts_framework_minor_test syntaxcheck_tests[] = {
	{ syntaxcheck_DSDT, "Disassemble and reassemble DSDT" },
	{ syntaxcheck_SSDT, "Disassemble and reassemble SSDT" },
	{ NULL, NULL }
};

static fwts_framework_ops syntaxcheck_ops = {
	.headline    = syntaxcheck_headline,
	.init        = syntaxcheck_init,	
	.minor_tests = syntaxcheck_tests
};

FWTS_REGISTER(syntaxcheck, &syntaxcheck_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
