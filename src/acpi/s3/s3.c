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

void s3_headline(log *results)
{
	log_info(results, "S3 suspend resume test");
}

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
	return 0;
}

int s3_do_test(log *results, framework *fw, char *message, int *warnings, int *errors, int delay, int *duration)
{
	char *output;
	int status;
	time_t t_start;
	time_t t_end;
	char *klog;

	log_info(results, message);

	if (klog_clear()) {
		log_error(results, "cannot clear kernel log");
		framework_failed(fw, message);
		return 1;
	}

	wakealarm_trigger(results, fw, delay);

	time(&t_start);

	/* Do S3 here */
	status = pipe_exec("pm-suspend", &output);

	time(&t_end);
	if (output)
		free(output);

	*duration = (int)(t_end - t_start);

	log_info(results, "pm-suspend returned %d after %d seconds", status, *duration);

	if ((t_end - t_start) < delay) {
		log_error(results, "Unexpected: S3 slept for less than %d seconds", delay);
		framework_failed(fw, message);
	}
	if ((t_end - t_start) > delay*3) {
		log_error(results, "Unexpected: S3 much longer than expected (%d seconds)", *duration);
		framework_failed(fw, message);
	}
	
	if ((klog = klog_read()) == NULL) {
		log_error(results, "cannot read kernel log");
	}

	if (klog_pm_check(results, klog, warnings, errors)) {
		log_error(results, "error parsing kernel log");
	}

	if (klog_firmware_check(results, klog, warnings, errors)) {
		log_error(results, "error parsing kernel log");
	}
	free(klog);

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

	return 0;
}

int s3_test_single(log *results, framework *fw)
{	
	char *test = "S3 suspend/resume test (single run)";
	int warnings = 0;
	int errors = 0;
	int ret;
	int duration;

	ret = s3_do_test(results, fw, test, &warnings, &errors, 30, &duration);

	if (warnings + errors > 0) {
		log_info(results, "Found %d errors, %d warnings in kernel log", errors, warnings);
		framework_failed(fw, test);
	}
	else
		framework_passed(fw, test);

	return ret;
}

int s3_test_multiple(log *results, framework *fw)
{	
	char *test = "S3 suspend/resume test (multiple runs)";
	int warnings = 0;
	int errors = 0;
	int ret = 0;
	int i;
	int delay = 30;
	int duration;

	if (fw->s3_multiple == 0) {
		fw->s3_multiple = 2;
		log_info(results, "Defaulted to run 2 multiple tests, run --s3-multiple=N to run more S3 cycles\n");
		return 0;
	}

	for (i=0; i<fw->s3_multiple; i++) {
		int timetaken;

		log_info(results, "S3 cycle %d of %d\n",i+1,fw->s3_multiple);
		if ((ret = s3_do_test(results, fw, test, &warnings, &errors, delay, &duration)) != 0)
			break;

		timetaken = duration - delay;
		delay = timetaken + 10;		/* Shorten test time, plus some slack */
	}

	if (warnings + errors > 0) {
		log_info(results, "Found %d errors, %d warnings in kernel log", errors, warnings);
		framework_failed(fw, test);
	}
	else
		framework_passed(fw, test);

	return ret;
}

framework_tests s3_tests[] = {
/*
	s3_test_single,
	s3_test_multiple,
*/
	NULL
};

framework_ops s3_ops = {
	s3_headline,
	s3_init,	
	s3_deinit,
	s3_tests
};

FRAMEWORK(s3, &s3_ops, TEST_LATE);
