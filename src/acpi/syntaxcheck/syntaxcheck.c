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

#define ASL_WARNING             0
#define ASL_WARNING2            1
#define ASL_WARNING3            2
#define ASL_ERROR               3

static char *syntaxcheck_headline(void)
{
	return "Re-assemble DSDT and find syntax errors and warnings.";
}

static void syntaxcheck_dump_code(fwts_framework *fw,
	int  error_code,
	char *error_message,
	fwts_list* iasl_disassembly,
	int error_line,
	int howmany)
{
	int i = 0;
	fwts_list_link *item;

	fwts_log_info_verbatum(fw, "Line | AML source\n");
	fwts_log_underline(fw->results, '-');

	fwts_list_foreach(item, iasl_disassembly) {
		i++;
		if (i >= error_line + (howmany / 2))
			break;
		if (i > error_line - (howmany / 2)) {
			fwts_log_info_verbatum(fw, "%5.5d| %s\n", i, fwts_text_list_text(item));
			if (i == error_line)
				fwts_log_info_verbatum(fw, "     | %s (Error %d)\n", error_message, error_code);
		}
	}
	fwts_log_underline(fw->results, '=');
	fwts_log_nl(fw);
}

static void syntaxcheck_give_advice(fwts_framework *fw, int error_code)
{
	//int error_level  = (error_code / 1000) - 1;
	//int error_number = (error_code % 1000);

	/* TO BE DONE: Look up advice in a json formatted table, see aslmessages.h for error number --> error message text */
}

static int syntaxcheck_table(fwts_framework *fw, char *tablename, int which)
{
	fwts_list_link *item;
	int errors = 0;
	int warnings = 0;
	fwts_acpi_table_info *table;
	fwts_list* iasl_errors;
	fwts_list* iasl_disassembly;

	if (fwts_acpi_find_table(fw, tablename, which, &table) != FWTS_OK) {
		fwts_aborted(fw, "Cannot load ACPI table %s.", tablename);
		return FWTS_ERROR;
	}

	if (table == NULL)
		return FWTS_NO_TABLE;		/* Table does not exist */

	if (fwts_iasl_reassemble(fw, table->data, table->length,
				&iasl_disassembly, &iasl_errors) != FWTS_OK) {
		fwts_aborted(fw, "Cannot re-assasemble with iasl.");
		return FWTS_ERROR;
	}

	fwts_list_foreach(item, iasl_errors) {
		int num;
		char ch;
		char *line = fwts_text_list_text(item);

		if ((sscanf(line, "%*s %d%c", &num, &ch) == 2) && ch == ':') {
			if (item->next != NULL) {
				char *nextline = fwts_text_list_text(item->next);
				int iasl_error = (strstr(nextline, "Error") != NULL);
				int iasl_warning = (strstr(nextline, "Warning") != NULL);
				int error_code;

				sscanf(nextline, "%*s %d", &error_code);
				
				if (iasl_error || iasl_warning) {
					char *colon = strstr(line, ":");
					int offset;
					offset = (colon == NULL) ? 20 : colon - line + 2;
					
					syntaxcheck_dump_code(fw, error_code, nextline + offset, iasl_disassembly, num, 8);
					syntaxcheck_give_advice(fw, error_code);
				}

				errors += iasl_error;
				warnings += iasl_warning;
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
	fwts_text_list_free(iasl_disassembly);
	fwts_text_list_free(iasl_errors);

	if (errors > 0) {
		fwts_failed_high(fw, "Table %s (%d) reassembly: Found %d errors, %d warnings.", tablename, which, errors, warnings);
	} else if (warnings > 0) {
		fwts_failed_low(fw, "Table %s (%d) reassembly: Found 0 errors, %d warnings.", tablename, which, warnings);
	} else
		fwts_passed(fw, "%s (%d) reassembly, Found 0 errors, 0 warnings.", tablename, which);

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
	.minor_tests = syntaxcheck_tests
};

FWTS_REGISTER(syntaxcheck, &syntaxcheck_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
