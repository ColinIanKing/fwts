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
#include <time.h>

#include "framework.h"
#include "pipeio.h"
#include "wakealarm.h"
#include "klog.h"
#include "text_list.h"

static char *s3_headline(void)
{
	return "S3 suspend resume test";
}

static int s3_init(fwts_log *results, fwts_framework *fw)
{
	int ret;

	if (fwts_klog_clear()) {
		fwts_log_error(results, "cannot clear kernel log");
		return 1;
	}

	ret = fwts_wakealarm_test_firing(results, fw, 1);
	if (ret != 0) {
		fwts_log_error(results, "cannot automatically wake machine up - aborting S3 test");
		fwts_framework_failed(fw, "check if wakealarm works reliably for S3 tests");
		return 1;
	}

	return 0;
}

static int s3_deinit(fwts_log *results, fwts_framework *fw)
{
	return 0;
}

static void s3_do_suspend_resume(fwts_log *results, fwts_framework *fw, int *warnings, int *errors, int delay, int *duration)
{
	fwts_text_list *output;
	int status;
	time_t t_start;
	time_t t_end;

	fwts_klog_clear();

	fwts_wakealarm_trigger(results, fw, delay);

	time(&t_start);

	/* Do S3 here */
	status = fwts_pipe_exec("pm-suspend", &output);

	time(&t_end);
	if (output)
		fwts_text_list_free(output);

	*duration = (int)(t_end - t_start);

	fwts_log_info(results, "pm-suspend returned %d after %d seconds", status, *duration);

	if ((t_end - t_start) < delay) {
		errors++;
		fwts_log_error(results, "Unexpected: S3 slept for less than %d seconds", delay);
	}
	if ((t_end - t_start) > delay*3) {
		errors++;
		fwts_log_error(results, "Unexpected: S3 much longer than expected (%d seconds)", *duration);
	}

	/* Add in error check for pm-suspend status */
	if ((status > 0) && (status < 128)) {
		errors++;
		fwts_log_error(results, "pm-action failed before trying to put the system\n"
				   "in the requested power saving state");
	} else if (status == 128) {
		errors++;
		fwts_log_error(results, "pm-action tried to put the machine in the requested\n"
       				   "power state but failed");
	} else if (status > 128) {
		errors++;
		fwts_log_error(results, "pm-action encountered an error and also failed to\n"
				   "enter the requested power saving state");
	}
}

static int s3_test_single(fwts_log *results, fwts_framework *fw)
{	
	char *test = "S3 suspend/resume test (single run)";
	int warnings = 0;
	int errors = 0;
	int duration;

	fwts_log_info(results, test);

	s3_do_suspend_resume(results, fw, &warnings, &errors, 30, &duration);
	if (warnings + errors > 0) {
		fwts_log_info(results, "Found %d errors doing suspend/resume", errors, warnings);
		fwts_framework_failed(fw, test);
	}
	else
		fwts_framework_passed(fw, test);

	return 0;
}

static int s3_check_log(fwts_log *results, fwts_framework *fw)
{
	char *test = "S3 suspend/resume check kernel log";
	fwts_text_list *klog;
	int warnings = 0;
	int errors = 0;

	fwts_log_info(results, test);

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(results, "cannot read kernel log");
		fwts_framework_failed(fw, test);
		return 1;
	}

	if (fwts_klog_pm_check(results, klog, &warnings, &errors))
		fwts_log_error(results, "error parsing kernel log");

	if (fwts_klog_firmware_check(results, klog, &warnings, &errors))
		fwts_log_error(results, "error parsing kernel log");

	fwts_klog_free(klog);

	if (warnings + errors > 0) {
		fwts_log_info(results, "Found %d errors, %d warnings in kernel log", errors, warnings);
		fwts_framework_failed(fw, test);
	}
	else
		fwts_framework_passed(fw, test);

	return 0;
}

static int s3_test_multiple(fwts_log *results, fwts_framework *fw)
{	
	char *test = "S3 suspend/resume test (multiple runs)";
	int warnings = 0;
	int errors = 0;
	int delay = 30;
	int duration = 0;
	int i;

	if (fw->s3_multiple == 0) {
		fw->s3_multiple = 2;
		fwts_log_info(results, "Defaulted to run 2 multiple tests, run --s3-multiple=N to run more S3 cycles\n");
		return 0;
	}

	for (i=0; i<fw->s3_multiple; i++) {
		int timetaken;

		fwts_log_info(results, "S3 cycle %d of %d\n",i+1,fw->s3_multiple);
		s3_do_suspend_resume(results, fw, &warnings, &errors, delay, &duration);

		timetaken = duration - delay;
		delay = timetaken + 10;		/* Shorten test time, plus some slack */
	}

	if (warnings + errors > 0) {
		fwts_log_info(results, "Found %d errors doing suspend/resume", errors, warnings);
		fwts_framework_failed(fw, test);
	}
	else
		fwts_framework_passed(fw, test);

	return 0;
}

static fwts_framework_tests s3_tests[] = {
	s3_test_single,
	s3_check_log,
	s3_test_multiple,
	s3_check_log,
	NULL
};

static fwts_framework_ops s3_ops = {
	s3_headline,
	s3_init,	
	s3_deinit,
	s3_tests
};

FRAMEWORK(s3, &s3_ops, TEST_LATE);
