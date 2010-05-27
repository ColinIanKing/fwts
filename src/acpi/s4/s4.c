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

void s4_headline(log *results)
{
	log_info(results, "S4 hibernate/resume test");
}

int s4_init(log *results, framework *fw)
{
	int ret;

	if (klog_clear()) {
		log_error(results, "cannot clear kernel log");
		return 1;
	}

	ret = wakealarm_test_firing(results, fw, 1);
	if (ret != 0) {
		log_error(results, "cannot automatically wake machine up - aborting S4 test");
		framework_failed(fw, "check if wakealarm works reliably for S4 tests");
		return 1;
	}

	return 0;
}

int s4_deinit(log *results, framework *fw)
{
	return 0;
}

int s4_test1(log *results, framework *fw)
{	
	char *test = "S4 hibernate/resume test";
	int warnings = 0;
	int errors = 0;
	char *output;
	int status;
	char *klog;

	log_info(results, test);

	if (klog_clear()) {
		log_error(results, "cannot clear kernel log");
		framework_failed(fw, test);
		return 1;
	}

	wakealarm_trigger(results, fw, 90);

	/* Do s4 here */
	status = pipe_exec("pm-hibernate", &output);
	if (output)
		free(output);

	log_info(results, "pm-hibernate returned %d", status);
	
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

	/* Add in error check for pm-hibernate status */
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
	free(klog);

	if (warnings + errors > 0) {
		log_info(results, "Found %d errors, %d warnings in kernel log", errors, warnings);
		framework_failed(fw, test);
	}
	else
		framework_passed(fw, test);

	return 0;
}

framework_tests s4_tests[] = {
	s4_test1,
	NULL
};

framework_ops s4_ops = {
	s4_headline,
	s4_init,	
	s4_deinit,
	s4_tests
};

FRAMEWORK(s4, &s4_ops);
