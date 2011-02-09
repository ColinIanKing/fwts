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

#define PM_HIBERNATE	"pm-hibernate"

#define FWTS_TRACING_BUFFER_SIZE	"/sys/kernel/debug/tracing/buffer_size_kb"

static int  s4_multiple = 1;		/* number of S4 multiple tests to run */
static int  s4_min_delay = 2;		/* min time between resume and next suspend */
static int  s4_max_delay = 4;		/* max time between resume and next suspend */
static float s4_delay_delta = 0.5;	/* amount to add to delay between each S4 tests */
static int  s4_sleep_delay = 90;	/* delay between initiating S4 and wakeup */
static bool s4_device_check = false;	/* check for device config changes */
static char *s4_quirks = NULL;		/* Quirks to be passed to pm-hibernate */
static int  s4_device_check_delay = 15;	/* Time to sleep after waking up and then running device check */

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

static void s4_check_log(fwts_framework *fw, fwts_list *klog, int *errors, int *oopses)
{
	int error;
	int oops;

	/* Check for kernel errors reported in the log */
	if (fwts_klog_pm_check(fw, NULL, klog, &error))
		fwts_log_error(fw, "Error parsing kernel log.");
	*errors += error;

	if (fwts_klog_firmware_check(fw, NULL, klog, &error))
		fwts_log_error(fw, "Error parsing kernel log.");
	*errors += error;

	if (fwts_klog_common_check(fw, NULL, klog, &error))
		fwts_log_error(fw, "Error parsing kernel log.");
	*errors += error;

	if (fwts_oops_check(fw, klog, &oops))
		fwts_log_error(fw, "Error parsing kernel log.");
	*oopses += oops;
}

static int s4_hibernate(fwts_framework *fw, char *test, int *errors, int *oopses, int *failed_alloc_image)
{	
	fwts_list *output;
	fwts_list *klog;
	fwts_hwinfo hwinfo1, hwinfo2;
	int status;
	int differences;
	char *command;
	char *quirks;

	fwts_log_info(fw, "%s", test);

	if (fwts_klog_clear()) {
		fwts_log_error(fw, "Cannot clear kernel log.");
		fwts_failed(fw, "%s", test);
		return FWTS_ERROR;
	}

	if (s4_device_check)
		fwts_hwinfo_get(fw, &hwinfo1);

	/* Format up pm-hibernate command with optional quirking arguments */
	if ((command = fwts_realloc_strcat(NULL, PM_HIBERNATE)) == NULL)
		return FWTS_OUT_OF_MEMORY;

	if (s4_quirks) {
		if ((command = fwts_realloc_strcat(command, " ")) == NULL)
			return FWTS_OUT_OF_MEMORY;
		if ((quirks = fwts_args_comma_list(s4_quirks)) == NULL) {
			free(command);
			return FWTS_OUT_OF_MEMORY;
		}
		if ((command = fwts_realloc_strcat(command, quirks)) == NULL) {
			free(quirks);
			return FWTS_OUT_OF_MEMORY;
		}
	}

	fwts_wakealarm_trigger(fw, s4_sleep_delay);

	/* Do s4 here */
	status = fwts_pipe_exec(command, &output);
	fwts_text_list_free(output);
	free(command);

	if (s4_device_check) {
		sleep(s4_device_check_delay);
		fwts_hwinfo_get(fw, &hwinfo2);
		fwts_hwinfo_compare(fw, &hwinfo1, &hwinfo2, &differences);
		fwts_hwinfo_free(&hwinfo1);
		fwts_hwinfo_free(&hwinfo2);

		if (differences > 0) {
			fwts_failed_high(fw, "Found %d differences in device configuation during S4 cycle.", differences);
			(*errors)++;
		}
	}

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Cannot read kernel log.");
		fwts_failed(fw, "%s", test);
		return FWTS_ERROR;
	}
	
	s4_check_log(fw, klog, errors, oopses);

	/* Add in error check for pm-hibernate status */
	if ((status > 0) && (status < 128)) {
		fwts_failed_medium(fw, "pm-action failed before trying to put the system "
				   "in the requested power saving state.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	} else if (status == 128) {
		fwts_failed_medium(fw, "pm-action tried to put the machine in the requested "
       				   "power state but failed.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	} else if (status > 128) {
		fwts_failed_medium(fw, "pm-action encountered an error and also failed to "
				   "enter the requested power saving state.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	}

	if (fwts_klog_regex_find(fw, klog, "Freezing user space processes.*done") < 1) {
		fwts_failed_high(fw, "Failed to freeze user space processes.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	}

	if (fwts_klog_regex_find(fw, klog, "Freezing remaining freezable tasks.*done") < 1) {
		fwts_failed_high(fw, "Failed to freeze remaining non-user space processes.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	}

	if ((fwts_klog_regex_find(fw, klog, "PM: freeze of devices complete") < 1) &&
	    (fwts_klog_regex_find(fw, klog, "PM: late freeze of devices complete") < 1)) {
		fwts_failed_high(fw, "Failed to freeze devices.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	}

	if (fwts_klog_regex_find(fw, klog, "PM: Allocated.*kbytes") < 1) {
		fwts_failed_high(fw, "Failed to allocate memory for hibernate image.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		*failed_alloc_image = 1;
		(*errors)++;
	}

	if (fwts_klog_regex_find(fw, klog, "PM: Image restored successfully") < 1) {
		fwts_failed_high(fw, "Failed to restore hibernate image.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	}
	
	if (fwts_klog_regex_find(fw, klog, "Restarting tasks.*done") < 1) {
		fwts_failed_high(fw, "Failed to restart tasks.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
		(*errors)++;
	}

	fwts_klog_free(klog);

	return FWTS_OK;
}

static int s4_test_multiple(fwts_framework *fw)
{
	int i;
	int errors = 0;
	int oopses = 0;
	int awake_delay = s4_min_delay * 1000;
	int delta = (int)(s4_delay_delta * 1000.0);
	int tracing_buffer_size = -1;
	int ret = FWTS_OK;
	bool retried = false;
	char tmp[32];

        if (s4_multiple == 1)
                fwts_log_info(fw, "Defaulted to run 1 test, run --s4-multiple=N to run more S4 cycles\n");

	for (i=0; i<s4_multiple; i++) {
		struct timeval tv;
		int failed_alloc_image = 0;

		fwts_log_info(fw, "S4 cycle %d of %d\n",i+1,s4_multiple);
		fwts_progress(fw, ((i+1) * 100) / s4_multiple);


		if (s4_hibernate(fw, "S4 hibernate/resume test.", &errors, &oopses, &failed_alloc_image) != FWTS_OK) {
			fwts_log_error(fw, "Aborting S4 multiple tests.");
			return FWTS_ERROR;
		}

		/* Sometimes we just fail at the first S4 cycle, so
		   shrink tracing buffer size and retry */
		if (failed_alloc_image) {
			if (fwts_get_int(FWTS_TRACING_BUFFER_SIZE, &tracing_buffer_size) != FWTS_OK) {
				fwts_log_error(fw, "Could not get size from %s.", FWTS_TRACING_BUFFER_SIZE);
			} else {
				if ((!retried) && (tracing_buffer_size > 4096)) {
					retried = true;

					fwts_failed_medium(fw,
						"/sys/kernel/debug/tracing/buffer_size_kb is set to %d Kbytes which "
						"may cause hibernate to fail. Programs such as ureadahead may have "
						"set this enable fast boot and not freed up the tracing buffer.", tracing_buffer_size);
	
					fwts_log_info(fw, "Setting tracing buffer size to 1K for subsequent tests.");
	
					fwts_set("1", FWTS_TRACING_BUFFER_SIZE);
					failed_alloc_image = 0;
	
					if (s4_hibernate(fw, "S4 hibernate/resume re-test with 1K tracing buffer size.", &errors, &oopses, &failed_alloc_image) != FWTS_OK) {
						ret = FWTS_ABORTED;
						break;
					};
				
					if (failed_alloc_image) {
						ret = FWTS_ABORTED;
						break;
					}
				}
			}
		}

		tv.tv_sec  = awake_delay / 1000;
		tv.tv_usec = (awake_delay % 1000)*1000;

		select(0, NULL, NULL, NULL, &tv);

		awake_delay += delta;
		if (awake_delay > (s4_max_delay * 1000))
			awake_delay = s4_min_delay * 1000;
	}

	if (tracing_buffer_size > 0) {
		/* Restore tracking buffer size */
		snprintf(tmp, sizeof(tmp), "%d", tracing_buffer_size);
		fwts_set(tmp, FWTS_TRACING_BUFFER_SIZE);
	}

	/* Really passed or failed? */
	if (errors + oopses > 0) {
                fwts_log_info(fw, "Found %d errors and %d oopses doing %d hibernate/resume cycle(s).",
			errors, oopses, s4_multiple);
	} else
		fwts_passed(fw, "Found no errors doing %d hibernate/resume cycle(s).", s4_multiple);

	return ret;
}

static int s4_options_handler(fwts_framework *fw, int argc, char * const argv[], int option_char, int long_index)
{

        switch (option_char) {
        case 0:
                switch (long_index) {
		case 0:
			s4_multiple = atoi(optarg);	
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
		case 5:
			s4_device_check = true;
			break;
		case 6:
			s4_quirks = optarg;
			break;
		case 7:
			s4_device_check_delay = atoi(optarg);
			s4_device_check = true;
		}
	}

	if ((s4_multiple < 0) || (s4_multiple > 100000)) {
		fprintf(stderr, "--s4-multiple be 1..100000\n");
		return FWTS_ERROR;
	}
	if ((s4_sleep_delay < 10) || (s4_sleep_delay > 3600)) {
		fprintf(stderr, "--s4-sleep-delay cannot be less than 10 seconds or more than 1 hour!\n");
		return FWTS_ERROR;
	}
	if ((s4_min_delay < 0) || (s4_min_delay > 3600)) {
		fprintf(stderr, "--s4-min-delay cannot be less than zero or more than 1 hour!\n");
		return FWTS_ERROR;
	}
	if (s4_max_delay < s4_min_delay || s4_max_delay > 3600)  {
		fprintf(stderr, "--s4-max-delay cannot be less than --s4-min-delay or more than 1 hour!\n");
		return FWTS_ERROR;
	}
	if (s4_delay_delta <= 0.001) {
		fprintf(stderr, "--s4-delay-delta cannot be less than 0.001\n");
		return FWTS_ERROR;
	}
	if ((s4_device_check_delay < 1) || (s4_device_check_delay > 3600)) {
		fprintf(stderr, "--s4-device-check-delay cannot be less than 1 second or more than 1 hour!\n");
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
	{ "s4-device-check",	"", 0, "Check differences between device configurations over a S4 cycle. Note we add a default of 15 seconds to allow wifi to re-associate." },
	{ "s4-quirks",		"", 1, "Comma separated list of quirk arguments to pass to pm-hibernate." },
	{ "s4-device-check-delay", "", 1, "Sleep N seconds before we run a device check after waking up from suspend. Default is 15 seconds." },
        { NULL, NULL, 0, NULL }
};

static fwts_framework_minor_test s4_tests[] = {
	{ s4_test_multiple, "S4 hibernate/resume test." },
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
