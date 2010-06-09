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
	return "Re-assemble DSDT and find syntax errors and warnings";
}

static fwts_list* error_output;

static int syntaxcheck_init(fwts_framework *fw)
{
	struct stat buffer;

	if (fwts_check_root_euid(fw))
		return 1;

        if (stat(fw->iasl ? fw->iasl : IASL, &buffer)) {
                fwts_log_error(fw, "Make sure iasl is installed");
                return 1;
        }

	error_output = fwts_iasl_reassemble(fw, "DSDT", 0);
	if (error_output == NULL) {
		fwts_log_error(fw, "cannot re-assasemble with iasl");
		return 1;
	}

	return 0;
}

static int syntaxcheck_deinit(fwts_framework *fw)
{
	if (error_output)
		fwts_text_list_free(error_output);

	return 0;
}

static int syntaxcheck_test1(fwts_framework *fw)
{	
	char *test = "DSDT re-assembly, syntax check";
	int warnings = 0;
	int errors = 0;
	fwts_list_element *item;

	if (error_output == NULL)
		return 1;

	fwts_log_info(fw, test);

	for (item = error_output->head; item != NULL; item = item->next) {
		int num;
		char ch;
		char *line = fwts_text_list_text(item);

		if ((sscanf(line, "%*s %d%c", &num, &ch) == 2) && ch == ':') {
			if (item->next != NULL) {
				char *nextline = fwts_text_list_text(item->next);
				if (!strncmp(nextline, "Error", 5)) {
					fwts_log_info(fw, "%s", line);
					fwts_log_info(fw, "%s", nextline);
					errors++;
				}
				if (!strncmp(nextline, "Warning", 7)) {
					fwts_log_info(fw, "%s", line);
					fwts_log_info(fw, "%s", nextline);
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
	if (warnings + errors > 0) {
		fwts_log_info(fw, "Found %d errors, %d warnings in DSDT", errors, warnings);
		fwts_framework_failed(fw, test);
	}
	else
		fwts_framework_passed(fw, test);

	return 0;
}

static fwts_framework_tests syntaxcheck_tests[] = {
	syntaxcheck_test1,
	NULL
};

static fwts_framework_ops syntaxcheck_ops = {
	syntaxcheck_headline,
	syntaxcheck_init,	
	syntaxcheck_deinit,
	syntaxcheck_tests
};

FRAMEWORK(syntaxcheck, &syntaxcheck_ops, TEST_ANYTIME);
