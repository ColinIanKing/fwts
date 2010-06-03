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
#include "text_list.h"

static char *syntaxcheck_headline(void)
{
	return "Scan kernel log for errors and warnings";
}

static text_list* error_output;

static int syntaxcheck_init(log *log, framework *fw)
{
	char *dsdtfile;
	char *tmpdsl;
	char *ptr;
	int  len;
        struct stat buffer;

        if (stat(fw->iasl ? fw->iasl : IASL, &buffer)) {
                log_error(log, "Make sure iasl is installed");
                return 1;
        }

	if (fw->dsdt) {
		dsdtfile = fw->dsdt;
	}
	else {
		/* Suck data from DSDT from kernel */
		dsdtfile = "tmp_dstd";

		if (check_root_euid(log))
			return 1;
	
		if (dsdt_copy(log, dsdtfile)) {
			log_error(log, "cannot copy DSDT to %s", dsdtfile);
			return 1;
		}
	}
	ptr = rindex(dsdtfile, '.');
	if (ptr)
		len = ptr - dsdtfile;
	else
		len = strlen(dsdtfile);

	tmpdsl = malloc(len+5);
	strncpy(tmpdsl, dsdtfile, len);
	strcpy(tmpdsl + len, ".dsl");

	if (iasl_disassemble(log, fw, dsdtfile)) {
		log_error(log, "cannot dissasemble DSDT with iasl", dsdtfile);
		return 1;
	}

	if (!fw->dsdt)
		unlink(dsdtfile);
	
	/* Get errors from assembly, to be parsed later */
	if ((error_output = iasl_assemble(log, fw, tmpdsl)) == NULL) {
		log_error(log, "cannot re-assasemble with iasl", dsdtfile);
		return 1;
	}

	unlink(tmpdsl);

	return 0;
}

static int syntaxcheck_deinit(log *log, framework *fw)
{
	if (error_output)
		text_list_free(error_output);

	return 0;
}

static int syntaxcheck_test1(log *log, framework *fw)
{	
	char *test = "DSDT re-assembly, syntax check";
	int warnings = 0;
	int errors = 0;
	text_list_element *item;

	if (error_output == NULL)
		return 1;

	log_info(log, test);

	for (item = error_output->head; item != NULL; item = item->next) {
		int num;
		char ch;
		char *line = item->text;

		if ((sscanf(line, "%*s %d%c", &num, &ch) == 2) && ch == ':') {
			item = item->next;
			if (item != NULL) {
				char *nextline = item->text;
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

static framework_tests syntaxcheck_tests[] = {
	syntaxcheck_test1,
	NULL
};

static framework_ops syntaxcheck_ops = {
	syntaxcheck_headline,
	syntaxcheck_init,	
	syntaxcheck_deinit,
	syntaxcheck_tests
};

FRAMEWORK(syntaxcheck, &syntaxcheck_ops, TEST_ANYTIME);
