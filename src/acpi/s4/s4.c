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

#define FWTS_TRACING_BUFFER_SIZE	"/sys/kernel/debug/tracing/buffer_size_kb"

static char *s4_headline(void)
{
	return "S4 hibernate/resume test.";
}

static int s4_init(fwts_framework *fw)
{
	if (fw->flags & FWTS_FRAMEWORK_FLAGS_NO_S4) {
		fwts_log_info(fw, "Skipping S4 tests.");
		return FWTS_SKIP;
	}

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "Cannot clear kernel log.");
		return FWTS_ERROR;
	}

	if (fwts_wakealarm_test_firing(fw, 1)) {
		fwts_log_error(fw, "Cannot automatically wake machine up - aborting S4 test.");
		fwts_failed(fw, "Check if wakealarm works reliably for S4 tests.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int s4_hibernate(fwts_framework *fw, char *test, int *failed_alloc_image)
{	
	int errors = 0;
	fwts_list *output;
	int status;
	fwts_list *klog;

	fwts_log_info(fw, test);

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "Cannot clear kernel log.");
		fwts_failed(fw, test);
		return FWTS_ERROR;
	}

	fwts_wakealarm_trigger(fw, 90);

	/* Do s4 here */
	status = fwts_pipe_exec("pm-hibernate", &output);
	if (output)
		fwts_text_list_free(output);

	fwts_log_info(fw, "pm-hibernate returned %d.", status);
	
	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Cannot read kernel log.");
		fwts_failed(fw, test);
		return FWTS_ERROR;
	}

	if (fwts_klog_pm_check(fw, NULL, klog, &errors))
		fwts_log_error(fw, "Error parsing kernel log.");

	if (fwts_klog_firmware_check(fw, NULL, klog, &errors))
		fwts_log_error(fw, "Error parsing kernel log.");

	if (fwts_klog_common_check(fw, NULL, klog, &errors))
		fwts_log_error(fw, "Error parsing kernel log.");


	if (errors > 0) 
		fwts_log_info(fw, "Found %d errors in kernel log.", errors);
	else
		fwts_passed(fw, test);

	/* Add in error check for pm-hibernate status */
	if ((status > 0) && (status < 128)) {
		fwts_failed_medium(fw, "pm-action failed before trying to put the system "
				   "in the requested power saving state.");
	} else if (status == 128) {
		fwts_failed_medium(fw, "pm-action tried to put the machine in the requested "
       				   "power state but failed.");
	} else if (status > 128) {
		fwts_failed_medium(fw, "pm-action encountered an error and also failed to "
				   "enter the requested power saving state.");
	}

	if (fwts_klog_regex_find(fw, klog, "Freezing user space processes.*done") > 0)
		fwts_passed(fw, "User space processes successfully frozen.");
	else		
		fwts_failed_high(fw, "Failed to freeze user space processes.");

	if (fwts_klog_regex_find(fw, klog, "Freezing remaining freezable tasks.*done") > 0)
		fwts_passed(fw, "Remaining non-user spaces tasks successfully frozen.");
	else		
		fwts_failed_high(fw, "Failed to freeze remaining non-user space processes.");

	if ((fwts_klog_regex_find(fw, klog, "PM: freeze of devices complete") > 0) &&
	    (fwts_klog_regex_find(fw, klog, "PM: late freeze of devices complete") > 0))
		fwts_passed(fw, "Successfully frozen devices.");
	else		
		fwts_failed_high(fw, "Failed to freeze devices.");

	if (fwts_klog_regex_find(fw, klog, "PM: Allocated.*kbytes") > 0)
		fwts_passed(fw, "Allocated memory for hibernate image.");
	else {
		fwts_failed_high(fw, "Failed to allocate memory for hibernate image.");
		*failed_alloc_image = 1;
	}

	if (fwts_klog_regex_find(fw, klog, "PM: Image restored successfully") > 0)
		fwts_passed(fw, "Hibernate image restored successfully.");
	else		
		fwts_failed_high(fw, "Failed to restore hibernate image.");
	
	if (fwts_klog_regex_find(fw, klog, "Restarting tasks.*done") > 0)
		fwts_passed(fw, "Tasks restarted successfully.");
	else		
		fwts_failed_high(fw, "Failed to restart tasks.");

	fwts_klog_free(klog);

	return FWTS_OK;
}


static int s4_test1(fwts_framework *fw)
{
	int failed_alloc_image = 0;

	if (s4_hibernate(fw, "S4 hibernate/resume test.", &failed_alloc_image) != FWTS_OK)
		return FWTS_ERROR;

	if (failed_alloc_image) {
		char tmp[32];
		int size;
		if (fwts_get_int(FWTS_TRACING_BUFFER_SIZE, &size) != FWTS_OK) {
			fwts_log_error(fw, "Could not get size from %s.", FWTS_TRACING_BUFFER_SIZE);
		} else {
			if (size > 4096) {
				int ret;
				fwts_failed_medium(fw, 
					"/sys/kernel/debug/tracing/buffer_size_kb is set to %d Kbytes which "
					"may cause hibernate to fail. Programs such as ureadahead may have "
					"set this enable fast boot and not freed up the tracing buffer.", size);
	
				fwts_log_info(fw, "Setting tracing buffer size to 1K for this test.");
	
				fwts_set("1", FWTS_TRACING_BUFFER_SIZE);
				failed_alloc_image = 0;
	
				ret = s4_hibernate(fw, "S4 hibernate/resume re-test with 1K tracing buffer size.", &failed_alloc_image);
			
				/* Restore tracking buffer size */
				snprintf(tmp, sizeof(tmp), "%d", size);
				fwts_set(tmp, FWTS_TRACING_BUFFER_SIZE);

				if (ret != FWTS_OK)
					return FWTS_ERROR;
			}
		}
	}
	return FWTS_OK;
}

static int s4_test2(fwts_framework *fw)
{
	int i;

        if (fw->s4_multiple == 0) {
                fw->s4_multiple = 2;
                fwts_log_info(fw, "Defaulted to run 2 multiple tests, run --s4-multiple=N to run more S4 cycles\n");
                return FWTS_OK;
        }

	for (i=0; i<fw->s4_multiple; i++) {
		int failed_alloc_image = 0;
		fwts_log_info(fw, "S4 cycle %d of %d\n",i+1,fw->s4_multiple);
		fwts_progress(fw, ((i+1) * 100) / fw->s4_multiple);

		if (s4_hibernate(fw, "Multiple S4 hibernate/resume test.", &failed_alloc_image) != FWTS_OK) {
			fwts_log_error(fw, "Aborting S4 multiple tests.");
			return FWTS_ERROR;
		}
	}
	return FWTS_OK;
}

static fwts_framework_tests s4_tests[] = {
	s4_test1,
	s4_test2,
	NULL
};

static fwts_framework_ops s4_ops = {
	s4_headline,
	s4_init,	
	NULL,
	s4_tests
};

FWTS_REGISTER(s4, &s4_ops, FWTS_TEST_LAST, FWTS_POWER_STATES);
