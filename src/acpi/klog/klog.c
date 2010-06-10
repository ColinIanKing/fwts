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

#include "fwts.h"

static char *klog_headline(void)
{
	return "Scan kernel log for errors and warnings";
}

static fwts_list *klog;

static int klog_init(fwts_framework *fw)
{
	if (fw->klog)
		klog = fwts_file_open_and_read(fw->klog);
	else
		klog = fwts_klog_read();
	
	if (klog == NULL) {
		fwts_log_error(fw, "cannot read kernel log");
		return 1;
	}
	return 0;
}

static int klog_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return 0;
}

static int klog_test1(fwts_framework *fw)
{	
	char *test = "kernel log error check";
	int warnings = 0;
	int errors = 0;

	fwts_log_info(fw, test);

	if (fwts_klog_firmware_check(fw, klog, &warnings, &errors)) {
		fwts_log_error(fw, "error parsing kernel log");
		return 1;
	}

	fwts_log_info(fw, "Found %d errors, %d warnings in kernel log", errors, warnings);
	if (warnings + errors > 0) {
		fwts_failed(fw, test);
	}
	else
		fwts_passed(fw, test);

	return 0;
}

static fwts_framework_tests klog_tests[] = {
	klog_test1,
	NULL
};

static fwts_framework_ops klog_ops = {
	klog_headline,
	klog_init,	
	klog_deinit,
	klog_tests
};

FRAMEWORK(klog, &klog_ops, TEST_ANYTIME);
