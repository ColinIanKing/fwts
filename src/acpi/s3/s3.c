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

#include "framework.h"
#include "pipeio.h"

void s3_headline(log *results)
{
	log_info(results, "S3 suspend resume test");
}

static char *klog;

int s3_init(log *results, framework *fw)
{
	if (klog_clear()) {
		log_error(results, "cannot clear kernel log");
		return 1;
	}
	return 0;
}

int s3_deinit(log *results, framework *fw)
{
	free(klog);

	return 0;
}

int s3_test1(log *results, framework *fw)
{	
	char *test = "kernel log error check";
	int warnings = 0;
	int errors = 0;
	int fd;

	log_info(results, test);

	if ((klog = klog_read()) == NULL) {
		log_error(results, "cannot read kernel log");
		return 1;
	}

	/* Do s3 here */
	/*
	fd = piperead("pm-suspend");
	pipeclose(fd);
	*/

	if (klog_check(results, klog, &warnings, &errors)) {
		log_error(results, "error parsing kernel log");
		return 1;
	}
	if (warnings + errors > 0) {
		log_info(results, "Found %d errors, %d warnings in kernel log", errors, warnings);
		framework_failed(fw, test);
	}
	else
		framework_passed(fw, test);

	return 0;
}

framework_tests s3_tests[] = {
	s3_test1,
	NULL
};

framework_ops s3_ops = {
	s3_headline,
	s3_init,	
	s3_deinit,
	s3_tests
};

FRAMEWORK(s3, &s3_ops);
