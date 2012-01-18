/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2012 Canonical
 *
 * This file was originally part of the Linux-ready Firmware Developer Kit
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

static fwts_list *klog;

static int dmesg_common_init(fwts_framework *fw)
{
	if (fw->klog)
		klog = fwts_file_open_and_read(fw->klog);
	else
		klog = fwts_klog_read();

	if (klog == NULL) {
		fwts_log_error(fw, "cannot read kernel log");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static int dmesg_common_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return FWTS_OK;
}

static int dmesg_common_test1(fwts_framework *fw)
{
	int errors = 0;

	if (fwts_klog_common_check(fw, NULL, klog, &errors)) {
		fwts_log_error(fw, "failed to scan kernel log");
		return FWTS_ERROR;
	}

	if (errors > 0)
		/* Already logged these errors if found */
		fwts_log_info(fw, "Found %d errors in kernel log", errors);
	else
		fwts_passed(fw, "No common error messages found in kernel message log.");

	return FWTS_OK;
}

static fwts_framework_minor_test dmesg_common_tests[] = {
	{ dmesg_common_test1, "Check common errors in kernel log." },
	{ NULL, NULL },
};

static fwts_framework_ops dmesg_common_ops = {
	.description = "General dmesg common errors check.",
	.init        = dmesg_common_init,
	.deinit      = dmesg_common_deinit,
	.minor_tests = dmesg_common_tests
};

FWTS_REGISTER(dmesg_common, &dmesg_common_ops, FWTS_TEST_EARLY, FWTS_BATCH);

#endif
