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
#include "wakealarm.h"
#include "klog.h"
#include "text_list.h"

static char *s4_headline(void)
{
	return "S4 hibernate/resume test";
}

static int s4_init(fwts_framework *fw)
{
	int ret;

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "cannot clear kernel log");
		return 1;
	}

	ret = fwts_wakealarm_test_firing(fw, 1);
	if (ret != 0) {
		fwts_log_error(fw, "cannot automatically wake machine up - aborting S4 test");
		fwts_framework_failed(fw, "check if wakealarm works reliably for S4 tests");
		return 1;
	}

	return 0;
}

static int s4_test1(fwts_framework *fw)
{	
	char *test = "S4 hibernate/resume test";
	int warnings = 0;
	int errors = 0;
	fwts_text_list *output;
	int status;
	fwts_text_list *klog;

	fwts_log_info(fw, test);

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "cannot clear kernel log");
		fwts_framework_failed(fw, test);
		return 1;
	}

	fwts_wakealarm_trigger(fw, 90);

	/* Do s4 here */
	status = fwts_pipe_exec("pm-hibernate", &output);
	if (output)
		fwts_text_list_free(output);

	fwts_log_info(fw, "pm-hibernate returned %d", status);
	
	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "cannot read kernel log");
		fwts_framework_failed(fw, test);
	}

	if (fwts_klog_pm_check(fw, klog, &warnings, &errors))
		fwts_log_error(fw, "error parsing kernel log");

	if (fwts_klog_firmware_check(fw, klog, &warnings, &errors))
		fwts_log_error(fw, "error parsing kernel log");

	/* Add in error check for pm-hibernate status */
	if ((status > 0) && (status < 128)) {
		errors++;
		fwts_log_error(fw, "pm-action failed before trying to put the system\n"
				   "in the requested power saving state");
	} else if (status == 128) {
		errors++;
		fwts_log_error(fw, "pm-action tried to put the machine in the requested\n"
       				   "power state but failed");
	} else if (status > 128) {
		errors++;
		fwts_log_error(fw, "pm-action encountered an error and also failed to\n"
				   "enter the requested power saving state");
	}
	fwts_klog_free(klog);

	if (warnings + errors > 0) {
		fwts_log_info(fw, "Found %d errors, %d warnings in kernel log", errors, warnings);
		fwts_framework_failed(fw, test);
	}
	else
		fwts_framework_passed(fw, test);

	return 0;
}

static fwts_framework_tests s4_tests[] = {
	s4_test1,
	NULL
};

static fwts_framework_ops s4_ops = {
	s4_headline,
	s4_init,	
	NULL,
	s4_tests
};

FRAMEWORK(s4, &s4_ops, TEST_LAST);
