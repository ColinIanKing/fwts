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

#include <json/json.h>

#define JSON_ERROR      ((json_object*)-1)

typedef struct {
	uint16_t	error;
	char 		*advice;
} syntaxcheck_adviceinfo;

static syntaxcheck_adviceinfo *adviceinfo;

static int syntaxcheck_load_advice(fwts_framework *fw);
static void syntaxcheck_free_advice(fwts_framework *fw);

/*
 *  syntaxcheck advice data file, data stored in json format
 */
#define SYNTAXCHECK_JSON_FILE		"syntaxcheck.json"

#define ASL_WARNING             0
#define ASL_WARNING2            1
#define ASL_WARNING3            2
#define ASL_ERROR               3

static int syntaxcheck_init(fwts_framework *fw)
{

	(void)syntaxcheck_load_advice(fw);
	
	return FWTS_OK;
}

static int syntaxcheck_deinit(fwts_framework *fw)
{
	syntaxcheck_free_advice(fw);

	return FWTS_OK;
}

static char *syntaxcheck_headline(void)
{
	return "Re-assemble DSDT and find syntax errors and warnings.";
}

/*
 *  syntaxcheck_dump_code()
 *	output a block of source around where the error is reported
 */
static void syntaxcheck_dump_code(fwts_framework *fw,
	int error_code,
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
}

/*
 *  syntaxcheck_load_advice()
 *	load error, advice string tuple from json formatted data file
 *	this populates adviceinfo - note that this is allocated to be
 * 	one item bigger than the number of tuples read, the last entry
 *	is null and indicates the end.
 */
static int syntaxcheck_load_advice(fwts_framework *fw)
{
	int ret = FWTS_ERROR;
	int n;
	int i;
	json_object *syntaxcheck_table;
	json_object *syntaxcheck_objs;
	char json_data_path[PATH_MAX];

	snprintf(json_data_path, sizeof(json_data_path), "%s/%s", fw->json_data_path, SYNTAXCHECK_JSON_FILE);

	if ((syntaxcheck_objs = json_object_from_file(json_data_path)) == JSON_ERROR) {
		fwts_log_error(fw, "Cannot load objects table from %s.", json_data_path);
		return FWTS_ERROR;
	}

        if ((syntaxcheck_table = json_object_object_get(syntaxcheck_objs, "erroradvice")) == JSON_ERROR) {
                fwts_log_error(fw, "Cannot fetch syntaxcheck table from %s.", json_data_path);
                goto fail_put;
        }

	n = json_object_array_length(syntaxcheck_table);

	/* Last entry is null to indicate end, so alloc n+1 items */
	if ((adviceinfo = calloc(n+1, sizeof(syntaxcheck_adviceinfo))) == NULL) {
		fwts_log_error(fw, "Cannot syntaxcheck advice table.");
		goto fail_put;
	}

	/* Now fetch json objects and compile regex */
	for (i=0; i<n; i++) {
		const char *str;
		json_object *obj;

		if ((obj = json_object_array_get_idx(syntaxcheck_table, i)) == JSON_ERROR) {
			fwts_log_error(fw, "Cannot fetch %d item from syntaxcheck table.", i);
			free(adviceinfo);
			adviceinfo = NULL;
			break;
		}
		if ((str = json_object_get_string(json_object_object_get(obj, "advice"))) == NULL) {
                	fwts_log_error(fw, "Cannot fetch advice from item %d.", i);
			free(adviceinfo);
			adviceinfo = NULL;
			break;
		}

		adviceinfo[i].error = json_object_get_int(json_object_object_get(obj, "error"));
		adviceinfo[i].advice = strdup(str);
	}

	ret = FWTS_OK;

fail_put:
	json_object_put(syntaxcheck_table);
	json_object_put(syntaxcheck_objs);
	
	return ret;
}

/*
 *  syntaxcheck_free_advice()
 * 	free up the adviceinfo data
 */
static void syntaxcheck_free_advice(fwts_framework *fw)
{
	if (adviceinfo) {
		int i;
		for (i=0; adviceinfo[i].advice != NULL; i++)
			free(adviceinfo[i].advice);

		free(adviceinfo);
	}
}

/*
 *  syntaxcheck_give_advice()
 *	look up advice on a given error number and give advice if any exists
 */
static void syntaxcheck_give_advice(fwts_framework *fw, uint32_t error_code)
{
	int i;
	/* iasl encodes error_codes as follows: */
	uint16_t error_level  = (error_code / 1000) - 1;
	uint16_t error_number = (error_code % 1000);

	static char *error_levels[] = {
		"warning level 0",
		"warning level 1",
		"warning level 2",
		"error",
		"unknown",
	};

	if (adviceinfo == NULL)
		return;	/* No advice! */

	if (error_level > 3)
		error_level = 4;

	for (i=0; adviceinfo[i].advice != NULL; i++) {
		if (adviceinfo[i].error == error_number) {
			fwts_advice(fw, "(for %s #%d): %s",
				error_levels[error_level], error_code, adviceinfo[i].advice);
			break;
		}
	}
}

/*
 *  syntaxcheck_table()
 *	disassemble and reassemble a table, check for errors. which indicates the Nth
 *	table, for example, SSDT may have tables 1..N
 */
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

	/* Scan error text from assembly */
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
				
				/* Valid error or warning, go and report */
				if (iasl_error || iasl_warning) {
					char *colon = strstr(line, ":");
					int offset;
					offset = (colon == NULL) ? 20 : colon - line + 2;
					
					syntaxcheck_dump_code(fw, error_code, nextline + offset, 
								iasl_disassembly, num, 8);
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
	.init        = syntaxcheck_init,
	.deinit      = syntaxcheck_deinit,
	.headline    = syntaxcheck_headline,
	.minor_tests = syntaxcheck_tests
};

FWTS_REGISTER(syntaxcheck, &syntaxcheck_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
