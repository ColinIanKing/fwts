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

void klog_headline(log *results)
{
	log_info(results, "Scan kernel log for errors and warnings");
}

static char *klog;

int klog_init(log *results, framework *fw)
{
	if ((klog = read_klog()) == NULL) {
		log_error(results, "cannot read kernel log");
		return 1;
	}
	return 0;
}

int klog_deinit(log *results, framework *fw)
{
	free(klog);
}

int klog_test1(log *results, framework *fw)
{	
	char *test = "kernel log error check";
	int warnings = 0;
	int errors = 0;

	log_info(results, test);

	if (check_klog(results, klog, &warnings, &errors)) {
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

framework_tests klog_tests[] = {
	klog_test1,
	NULL
};

framework_ops klog_ops = {
	klog_headline,
	klog_init,	
	klog_deinit,
	klog_tests
};

FRAMEWORK(klog, &klog_ops);
