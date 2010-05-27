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

void s3_headline(log *results)
{
	log_info(results, "S3 suspend resume test");
}

static char *klog;

int s3_init(log *results, framework *fw)
{
	int ret;

	if (klog_clear()) {
		log_error(results, "cannot clear kernel log");
		return 1;
	}

	ret = wakealarm_test_firing(results, fw, 1);
	if (ret != 0) {
		log_error(results, "cannot automatically wake machine up - aborting S3 test");
		framework_failed(fw, "check if wakealarm works reliably for S3 tests");
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
	char *test = "S3 suspend/resume test";
	int warnings = 0;
	int errors = 0;
	char *output;
	int status;

	log_info(results, test);

	if (klog_clear()) {
		log_error(results, "cannot clear kernel log");
		framework_failed(fw, test);
		return 1;
	}

	wakealarm_trigger(results, fw, 30);

	/* Do s3 here */
	status = pipe_exec("pm-suspend", &output);
	if (output)
		free(output);

	log_info(results, "pm-suspend returned %d", status);
	
	if ((klog = klog_read()) == NULL) {
		log_error(results, "cannot read kernel log");
		framework_failed(fw, test);
	}

	if (klog_pm_check(results, klog, &warnings, &errors)) {
		log_error(results, "error parsing kernel log");
	}

	if (klog_firmware_check(results, klog, &warnings, &errors)) {
		log_error(results, "error parsing kernel log");
	}

	/* Add in error check for pm-suspend status */
	if ((status > 0) && (status < 128)) {
		errors++;
		log_error(results, "pm-action failed before trying to put the system\n"
				   "in the requested power saving state");
	} else if (status == 128) {
		errors++;
		log_error(results, "pm-action tried to put the machine in the requested\n"
       				   "power state but failed");
	} else if (status > 128) {
		errors++;
		log_error(results, "pm-action encountered an error and also failed to\n"
				   "enter the requested power saving state");
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
