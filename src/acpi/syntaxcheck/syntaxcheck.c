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

#include "framework.h"
#include "dsdt.h"
#include "iasl.h"

void syntaxcheck_headline(log *log)
{
	log_info(log, "Scan kernel log for errors and warnings");
}

static char* error_output;

char *get_next_output_line(void)
{
	char *ret;
	static char *current_ptr = NULL;

	if (!error_output)
		return NULL;	/* No text! */

	if (current_ptr == NULL)	/* Initial request */
		current_ptr = error_output;

	if (*current_ptr == '\0')
		return NULL;

	ret = current_ptr;
	while (*current_ptr && *current_ptr != '\n')
		current_ptr++;

	if (*current_ptr == '\n') {
		*current_ptr = '\0';
		current_ptr++;
	}
	
	return ret;
}

int syntaxcheck_init(log *log, framework *fw)
{
	char *tmp;
	char *tmpdsl;
	int  len;
	FILE *cmd;
        struct stat buffer;

	tmp = "tmp_dstd";
	len = strlen(tmp);
	tmpdsl = malloc(len+5);
	snprintf(tmpdsl, len+5, "%s.dsl", tmp);

	if (check_root_euid(log))
		return 1;

        if (stat(fw->iasl ? fw->iasl : IASL, &buffer)) {
                log_error(log, "Make sure iasl is installed");
                return 1;
        }
	
	if (dsdt_copy(log, tmp)) {
		log_error(log, "cannot copy DSDT to %s", tmp);
		return 1;
	}

	if (iasl_disassemble(log, fw, tmp)) {
		log_error(log, "cannot dissasemble DSDT with iasl", tmp);
		return 1;
	}

	unlink(tmp);
	
	if ((error_output = iasl_assemble(log, fw, tmpdsl)) == NULL) {
		log_error(log, "cannot re-assasemble with iasl", tmp);
		return 1;
	}

	unlink(tmpdsl);

	return 0;
}

int syntaxcheck_deinit(log *log, framework *fw)
{
	if (error_output)
		free(error_output);
}

int syntaxcheck_test1(log *log, framework *fw)
{	
	char *test = "DSDT re-assembly, syntax check";
	int warnings = 0;
	int errors = 0;
	char *line;

	log_info(log, test);
	
	while ((line = get_next_output_line()) != NULL) {
		int num;
		char ch;
		if ((sscanf(line, "%*s %d%c", &num, &ch) == 2) && ch == ':') {
			char *nextline = get_next_output_line();
			if (nextline) {
				if (!strncmp(nextline, "Error", 5)) {
					log_info(log, "%s", line);
					log_info(log, "%s", nextline);
					errors++;
				}
				if (!strncmp(nextline, "Warning", 7)) {
					log_info(log, "%s", line);
					log_info(log, "%s", nextline);
					warnings++;
				}
			}
			else {
				log_info(log, "%s", line);
				log_error(log, "Could not find parser error message\n");
			}
		}
	}
	if (warnings + errors > 0) {
		log_info(log, "Found %d errors, %d warnings in DSDT", errors, warnings);
		framework_failed(fw, test);
	}
	else
		framework_passed(fw, test);

	return 0;
}

framework_tests syntaxcheck_tests[] = {
	syntaxcheck_test1,
	NULL
};

framework_ops syntaxcheck_ops = {
	syntaxcheck_headline,
	syntaxcheck_init,	
	syntaxcheck_deinit,
	syntaxcheck_tests
};

FRAMEWORK(syntaxcheck, &syntaxcheck_ops);
