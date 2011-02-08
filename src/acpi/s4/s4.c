/*
 * Copyright (C) 2010-2011 Canonical
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

#define FWTS_TRACING_BUFFER_SIZE	"/sys/kernel/debug/tracing/buffer_size_kb"

static int  s4_multiple;	/* number of S4 multiple tests to run */
static int  s4_min_delay;	/* min time between resume and next suspend */
static int  s4_max_delay;	/* max time between resume and next suspend */
static float s4_delay_delta;	/* amount to add to delay between each S4 tests */
static int  s4_sleep_delay;	/* delay between initiating S4 and wakeup */

static char *s4_headline(void)
{
	return "S4 hibernate/resume test.";
}

static int s4_init(fwts_framework *fw)
{
	fwts_list* swap_devs;

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "Cannot clear kernel log.");
		return FWTS_ERROR;
	}

	swap_devs = fwts_file_open_and_read("/proc/swaps");
	if (fwts_text_list_strstr(swap_devs, "/dev/") == NULL) {
		fwts_list_free(swap_devs, free);
		fwts_failed(fw, "Cannot run hibernate test - machine appears to have NO swap.");
		return FWTS_ERROR;
	}
	fwts_list_free(swap_devs, free);

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
	int oopses = 0;
	fwts_list *output;
	int status;
	fwts_list *klog;

	fwts_log_info(fw, "%s", test);

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "Cannot clear kernel log.");
		fwts_failed(fw, "%s", test);
		return FWTS_ERROR;
	}

	fwts_wakealarm_trigger(fw, s4_sleep_delay);

	/* Do s4 here */
	status = fwts_pipe_exec("pm-hibernate", &output);
	if (output)
		fwts_text_list_free(output);

	fwts_log_info(fw, "pm-hibernate returned %d.", status);
	
	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Cannot read kernel log.");
		fwts_failed(fw, "%s", test);
		return FWTS_ERROR;
	}

	if (fwts_klog_pm_check(fw, NULL, klog, &errors))
		fwts_log_error(fw, "Error parsing kernel log.");

	if (fwts_klog_firmware_check(fw, NULL, klog, &errors))
		fwts_log_error(fw, "Error parsing kernel log.");

	if (fwts_klog_common_check(fw, NULL, klog, &errors))
		fwts_log_error(fw, "Error parsing kernel log.");

	if (fwts_oops_check(fw, klog, &oopses))
		fwts_log_error(fw, "Error parsing kernel log.");

	if (errors + oopses > 0)
		fwts_log_info(fw, "Found %d errors and %d oopses in kernel log.", errors, oopses);
	else
		fwts_passed(fw, "%s", test);

	/* Add in error check for pm-hibernate status */
	if ((status > 0) && (status < 128)) {
		fwts_failed_medium(fw, "pm-action failed before trying to put the system "
				   "in the requested power saving state.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	} else if (status == 128) {
		fwts_failed_medium(fw, "pm-action tried to put the machine in the requested "
       				   "power state but failed.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	} else if (status > 128) {
		fwts_failed_medium(fw, "pm-action encountered an error and also failed to "
				   "enter the requested power saving state.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	if (fwts_klog_regex_find(fw, klog, "Freezing user space processes.*done") > 0)
		fwts_passed(fw, "User space processes successfully frozen.");
	else {
		fwts_failed_high(fw, "Failed to freeze user space processes.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	if (fwts_klog_regex_find(fw, klog, "Freezing remaining freezable tasks.*done") > 0)
		fwts_passed(fw, "Remaining non-user spaces tasks successfully frozen.");
	else {
		fwts_failed_high(fw, "Failed to freeze remaining non-user space processes.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	if ((fwts_klog_regex_find(fw, klog, "PM: freeze of devices complete") > 0) &&
	    (fwts_klog_regex_find(fw, klog, "PM: late freeze of devices complete") > 0))
		fwts_passed(fw, "Successfully frozen devices.");
	else {
		fwts_failed_high(fw, "Failed to freeze devices.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	if (fwts_klog_regex_find(fw, klog, "PM: Allocated.*kbytes") > 0)
		fwts_passed(fw, "Allocated memory for hibernate image.");
	else {
		fwts_failed_high(fw, "Failed to allocate memory for hibernate image.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		*failed_alloc_image = 1;
	}

	if (fwts_klog_regex_find(fw, klog, "PM: Image restored successfully") > 0)
		fwts_passed(fw, "Hibernate image restored successfully.");
	else {
		fwts_failed_high(fw, "Failed to restore hibernate image.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}
	
	if (fwts_klog_regex_find(fw, klog, "Restarting tasks.*done") > 0)
		fwts_passed(fw, "Tasks restarted successfully.");
	else {
		fwts_failed_high(fw, "Failed to restart tasks.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	fwts_klog_free(klog);

	return FWTS_OK;
}


static int s4_test1(fwts_framework *fw)
{
	int failed_alloc_image = 0;

	if (s4_hibernate(fw, "S4 hibernate/resume test.", &failed_alloc_image) != FWTS_OK)
		return FWTS_ERROR;

	if (failed_alloc_image) {
		int size;
		if (fwts_get_int(FWTS_TRACING_BUFFER_SIZE, &size) != FWTS_OK) {
			fwts_log_error(fw, "Could not get size from %s.", FWTS_TRACING_BUFFER_SIZE);
		} else {
			if (size > 4096) {
				char tmp[32];
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

				/* Still failed to allocate S4 hibernate image - abort all further testing! */
				if (failed_alloc_image)
					return FWTS_ABORTED;

				if (ret != FWTS_OK)
					return FWTS_ERROR;
			}
		}
	}

	/* We put a small delay in between test1 and test2 */
	sleep(2);
	return FWTS_OK;
}

static int s4_test2(fwts_framework *fw)
{
	int i;
	int awake_delay = s4_min_delay * 1000;
	int delta = (int)(s4_delay_delta * 1000.0);

        if (s4_multiple == 0) {
                s4_multiple = 2;
                fwts_log_info(fw, "Defaulted to run 2 multiple tests, run --s4-multiple=N to run more S4 cycles\n");
        }

	for (i=0; i<s4_multiple; i++) {
		struct timeval tv;
		int failed_alloc_image = 0;

		fwts_log_info(fw, "S4 cycle %d of %d\n",i+1,s4_multiple);
		fwts_progress(fw, ((i+1) * 100) / s4_multiple);

		if (s4_hibernate(fw, "Multiple S4 hibernate/resume test.", &failed_alloc_image) != FWTS_OK) {
			fwts_log_error(fw, "Aborting S4 multiple tests.");
			return FWTS_ERROR;
		}

		tv.tv_sec  = awake_delay / 1000;
		tv.tv_usec = (awake_delay % 1000)*1000;

		select(0, NULL, NULL, NULL, &tv);

		awake_delay += delta;
		if (awake_delay > (s4_max_delay * 1000))
			awake_delay = s4_min_delay * 1000;
	}
	return FWTS_OK;
}

static int s4_options_handler(fwts_framework *fw, int argc, char * const argv[], int option_char, int long_index)
{
	s4_multiple = 0;
	s4_min_delay = 2;
        s4_max_delay = 4;
        s4_delay_delta = 0.5;
	s4_sleep_delay = 90;

        switch (option_char) {
        case 0:
                switch (long_index) {
		case 0:
			s4_multiple = atoi(optarg);	
			printf("HERE! multiple = %d\n", s4_multiple);
			break;
		case 1:
			s4_min_delay = atoi(optarg);
			break;
		case 2:
			s4_max_delay = atoi(optarg);
			break;
		case 3:
			s4_delay_delta = atof(optarg);
			break;
		case 4:
			s4_sleep_delay = atoi(optarg);
			break;
		}
	}

	if ((s4_sleep_delay < 10) || (s4_sleep_delay > 3600)) {
		fprintf(stderr, "--s4-sleep_delay cannot be less than 10 or more than 1 hour!\n");
		return FWTS_ERROR;
	}
	if ((s4_min_delay < 0) || (s4_min_delay > 3600)) {
		fprintf(stderr, "--s3-min-delay cannot be less than zero or more than 1 hour!\n");
		return FWTS_ERROR;
	}
	if (s4_max_delay < s4_min_delay || s4_max_delay > 3600)  {
		fprintf(stderr, "--s3-max-delay cannot be less than --s3-min-delay or more than 1 hour!\n");
		return FWTS_ERROR;
	}
	if (s4_delay_delta <= 0.001) {
		fprintf(stderr, "--s3-delay-delta cannot be less than 0.001\n");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static fwts_option s4_options[] = {
	{ "s4-multiple",        "", 1, "Time to be added to delay between S4 iterations." },
	{ "s4-min-delay",       "", 1, "Minimum time between S4 iterations." },
	{ "s4-max-delay",       "", 1, "Maximum time between S4 iterations." },
	{ "s4-delay-delta",     "", 1, "Run S4 tests N times." },
	{ "s4-sleep-delay",	"", 1, "Sleep N seconds between start of hibernate and wakeup." },
        { NULL, NULL, 0, NULL }
};

static fwts_framework_minor_test s4_tests[] = {
	{ s4_test1, "S4 hibernate/resume test (single run)." },
	{ s4_test2, "S4 hibernate/resume test (multiple runs)." },
	{ NULL, NULL }
};

static fwts_framework_ops s4_ops = {
	.headline    = s4_headline,
	.init        = s4_init,
	.minor_tests = s4_tests,
	.options     = s4_options,
	.options_handler = s4_options_handler
};

FWTS_REGISTER(s4, &s4_ops, FWTS_TEST_LAST, FWTS_POWER_STATES);

#endif
