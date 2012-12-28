/*
 * Copyright (C) 2010-2013 Canonical
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

typedef struct {
	uint16_t	error;
	char 		*advice;
} syntaxcheck_adviceinfo;

static syntaxcheck_adviceinfo *adviceinfo;

static int syntaxcheck_load_advice(fwts_framework *fw);
static void syntaxcheck_free_advice(void);

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
	FWTS_UNUSED(fw);

	syntaxcheck_free_advice();

	return FWTS_OK;
}

static const char *syntaxcheck_error_level(uint32_t error_code)
{
	/* iasl encodes error_level as follows: */
	uint16_t error_level  = (error_code / 1000) - 1;

	static char *error_levels[] = {
		"warning level 0",
		"warning level 1",
		"warning level 2",
		"error",
		"unknown",
	};

	if (error_level > 3)
		error_level = 4;

	return error_levels[error_level];
}

/*
 *  syntaxcheck_dump_code()
 *	output a block of source around where the error is reported
 */
static void syntaxcheck_dump_code(fwts_framework *fw,
	int error_code,
	int carat_offset,
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
			fwts_log_info_verbatum(fw, "%5.5d| %s\n", i,
				fwts_text_list_text(item));
			if (i == error_line) {
				fwts_log_info_verbatum(fw, "     | %*.*s", carat_offset, carat_offset, "^");
				fwts_log_info_verbatum(fw, "     | %s %d: %s\n",
					syntaxcheck_error_level(error_code), error_code, error_message);
			}
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

	syntaxcheck_objs = json_object_from_file(json_data_path);
	if (FWTS_JSON_ERROR(syntaxcheck_objs)) {
		fwts_log_error(fw, "Cannot load objects table from %s.", json_data_path);
		return FWTS_ERROR;
	}

        syntaxcheck_table = json_object_object_get(syntaxcheck_objs, "erroradvice");
        if (FWTS_JSON_ERROR(syntaxcheck_table)) {
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

		obj = json_object_array_get_idx(syntaxcheck_table, i);
		if (FWTS_JSON_ERROR(obj)) {
			fwts_log_error(fw, "Cannot fetch %d item from syntaxcheck table.", i);
			free(adviceinfo);
			adviceinfo = NULL;
			break;
		}
		str = json_object_get_string(json_object_object_get(obj, "advice"));
		if (FWTS_JSON_ERROR(str)) {
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
	json_object_put(syntaxcheck_objs);

	return ret;
}

/*
 *  syntaxcheck_free_advice()
 * 	free up the adviceinfo data
 */
static void syntaxcheck_free_advice(void)
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
	uint16_t error_number = (error_code % 1000);

	if (adviceinfo == NULL)
		return;	/* No advice! */

	for (i=0; adviceinfo[i].advice != NULL; i++) {
		if (adviceinfo[i].error == error_number) {
			fwts_advice(fw, "(for %s #%d): %s",
				syntaxcheck_error_level(error_code),
				error_code, adviceinfo[i].advice);
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

	if (iasl_errors) {
		/* Scan error text from assembly */
		fwts_list_foreach(item, iasl_errors) {
			int num;
			char ch;
			char *line = fwts_text_list_text(item);

			if ((sscanf(line, "%*s %d%c", &num, &ch) == 2) && ch == ':') {
				if (item->next != NULL) {
					char *error_text = fwts_text_list_text(item->next);
					int iasl_error = (strstr(error_text, "Error") != NULL);
					int iasl_warning = (strstr(error_text, "Warning") != NULL);
					int error_code;

					sscanf(error_text, "%*s %d", &error_code);

					/* Valid error or warning, go and report */
					if (iasl_error || iasl_warning) {
						char label[64];
						char *colon = strstr(line, ":");
						char *carat = strstr(error_text, "^");
						char *ptr;
						int colon_offset = (colon == NULL) ? 0 : colon + 1 - line;
						int carat_offset = (carat == NULL) ? 0 : carat - error_text;

						/* trim */
						fwts_chop_newline(error_text);

						/* Strip out the ^ from the error message */
						for (ptr = error_text; *ptr; ptr++)
							if (*ptr == '^')
								*ptr = ' ';

						/* Look for error message after: "Error:  4042 - " prefix */
						ptr = strstr(error_text, "-");
						if (ptr)
							ptr += 2;
						else
							ptr = error_text;	/* Urgh, none found, default */

						/* Skip over leading white space */
						while (*ptr == ' ')
							ptr++;

						snprintf(label, sizeof(label), "AMLAssemblerError%d", error_code);
						fwts_failed(fw, LOG_LEVEL_HIGH, label, "Assembler error in line %d", num);
						syntaxcheck_dump_code(fw, error_code,
							carat_offset - colon_offset, ptr,
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
	}
	fwts_text_list_free(iasl_disassembly);
	fwts_text_list_free(iasl_errors);

	if (errors > 0) {
		fwts_log_info(fw, "Table %s (%d) reassembly: Found %d errors, %d warnings.", tablename, which, errors, warnings);
	} else if (warnings > 0) {
		fwts_log_info(fw, "Table %s (%d) reassembly: Found 0 errors, %d warnings.", tablename, which, warnings);
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
	.description = "Re-assemble DSDT and find syntax errors and warnings.",
	.init        = syntaxcheck_init,
	.deinit      = syntaxcheck_deinit,
	.minor_tests = syntaxcheck_tests
};

FWTS_REGISTER(syntaxcheck, &syntaxcheck_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH);

#endif
