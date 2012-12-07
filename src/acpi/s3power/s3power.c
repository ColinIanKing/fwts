/*
 * Copyright (C) 2011-2012 Canonical
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
#include <getopt.h>

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

#define PM_SUSPEND "pm-suspend"

static int 	s3power_sleep_delay = 600;	/* time between start of suspend and wakeup */
static uint32_t battery_capacity_mAh;
static uint32_t battery_capacity_mWh;

static int s3power_adapter_offline(fwts_framework *fw, bool *offline)
{
	int matching = 0;
	int not_matching = 0;

	if (fwts_ac_adapter_get_state(FWTS_AC_ADAPTER_ONLINE, &matching, &not_matching) != FWTS_OK) {
		fwts_log_error(fw, "Cannot detect if running on AC or battery.");
		return FWTS_ERROR;
	}

	/* Any online, then we're not totally offlined */

	*offline = matching > 0 ? false : true;
	
	return FWTS_OK;
}

static int s3power_wait_for_adapter_offline(fwts_framework *fw, bool *offline)
{
	int fd;
	size_t len;
	char *buffer;
	int i;

	if (s3power_adapter_offline(fw, offline) == FWTS_ERROR)
		return FWTS_ERROR;
	if (*offline)
		return FWTS_OK;	/* Already offline, so no need to wait */

	if ((fd = fwts_acpi_event_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}

	fwts_printf(fw, "==== Please unplug the laptop power. ====\n");

	for (i=0; (i<=20) && !*offline ;i++) {
		if ((buffer = fwts_acpi_event_read(fd, &len, 1)) != NULL) {
			char *str;
			if ((str = strstr(buffer, "ac_adapter")) != NULL)
				s3power_adapter_offline(fw, offline);
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	fwts_acpi_event_close(fd);

	return FWTS_OK;
}

static int s3power_get_design_capacity(fwts_framework *fw,
	uint32_t *capacity_mAh, uint32_t *capacity_mWh)
{
	return fwts_battery_get_capacity(fw, FWTS_BATTERY_DESIGN_CAPACITY, FWTS_BATTERY_ALL, capacity_mAh, capacity_mWh);
}

static int s3power_get_remaining_capacity(fwts_framework *fw,
	uint32_t *capacity_mAh, uint32_t *capacity_mWh)
{
	return fwts_battery_get_capacity(fw, FWTS_BATTERY_REMAINING_CAPACITY, FWTS_BATTERY_ALL, capacity_mAh, capacity_mWh);
}

static int s3power_init(fwts_framework *fw)
{
	if (fwts_wakealarm_test_firing(fw, 1) != FWTS_OK) {
		fwts_log_error(fw, "Cannot automatically wake machine up - aborting S3power test.");
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "BadWakeAlarmS3Power",
			"Wakealarm does not work reliably for s3power test.");
		return FWTS_ABORTED;
	}

	if (s3power_get_design_capacity(fw, &battery_capacity_mAh, &battery_capacity_mWh) != FWTS_OK) {
		fwts_log_error(fw, "Cannot determine battery capacity, aborting test.");
		return FWTS_ABORTED;
	}

	return FWTS_OK;
}

static void s3power_difference(fwts_framework *fw,
	uint32_t before, uint32_t after,
	uint32_t battery_capacity, char *units)
{
	int32_t diff = before - after;
	float hourly_loss;

	if (before != 0) {
		fwts_log_info(fw, "Change in capacity: %" PRId32 " %s\n", diff, units);
		hourly_loss = ((float)diff * 3600.0) / (float)s3power_sleep_delay;
		if (diff < 0) {
			fwts_log_error(fw, "Negative loss of power, are you sure the machine was not charging?");
		}
		fwts_log_info(fw, "Loss of %7.4f %s per hour.", hourly_loss, units);
		if ((diff > 0) && (battery_capacity > 0)) {
			float duration = (float)battery_capacity / hourly_loss;
			fwts_log_info(fw, "The %" PRIu32 " %s battery will provide %5.2f hours of suspend time.",
				battery_capacity, units, duration);

			if (duration < 24.0) {
				fwts_failed(fw, LOG_LEVEL_CRITICAL,
					"ShortSuspendLife24hrs",
					"Machine cannot remain suspended for 1 day.");
			} else if (duration < 36.0) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"ShortSuspendLife36hrs",
					"Machine cannot remain suspended for 1.5 days.");
			} else if (duration < 48.0) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"ShortSuspendLife48hrs",
					"Machine cannot remain suspended for 2 days.");
			} else if (duration < 60.0) {
				fwts_failed(fw, LOG_LEVEL_LOW,
					"ShortSuspendLife60hrs",
					"Machine cannot remain suspended for 2.5 days.");
			} else {
				fwts_passed(fw, "Machine can remain suspended for %5.2f hours.", duration);
			}

			fwts_log_info(fw, "Note: Accuracy of results are increased with longer sleep delay durations.");
		}
	}
}

static int s3power_test(fwts_framework *fw)
{
	int status;
	int duration;

	time_t t_start;
	time_t t_end;
	bool offline;

	uint32_t capacity_before_mAh;
	uint32_t capacity_after_mAh;
	uint32_t capacity_before_mWh;
	uint32_t capacity_after_mWh;

	fwts_list *output;

	if (s3power_wait_for_adapter_offline(fw, &offline) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot check if machine is running on battery, aborting test.");
		return FWTS_ABORTED;
	}
	if (!offline) {
		fwts_log_error(fw, "Machine needs to be running on battery to run test, aborting test.");
		return FWTS_ABORTED;
	}

	s3power_get_remaining_capacity(fw, &capacity_before_mAh, &capacity_before_mWh);

	fwts_wakealarm_trigger(fw, s3power_sleep_delay);

	/* Do S3 here */
	fwts_progress_message(fw, 100, "(Suspending)");
	time(&t_start);
	(void)fwts_pipe_exec(PM_SUSPEND, &output, &status);
	time(&t_end);
	fwts_progress_message(fw, 100, "(Resumed)");
	fwts_text_list_free(output);

	s3power_get_remaining_capacity(fw, &capacity_after_mAh, &capacity_after_mWh);

	s3power_difference(fw, capacity_before_mAh, capacity_after_mAh, battery_capacity_mAh, "mAh");
	s3power_difference(fw, capacity_before_mWh, capacity_after_mWh, battery_capacity_mWh, "mWh");

	duration = (int)(t_end - t_start);
	fwts_log_info(fw, "pm-suspend returned %d after %d seconds.", status, duration);

	if (duration < s3power_sleep_delay) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "ShortSuspend",
			"Unexpected: S3 slept for %d seconds, less than the expected %d seconds.",
			duration, s3power_sleep_delay);
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}
	if (duration > (s3power_sleep_delay*2))
		fwts_failed(fw, LOG_LEVEL_HIGH, "LongSuspend",
			"Unexpected: S3 much longer than expected (%d seconds).", duration);

	/* Add in error check for pm-suspend status */
	if ((status > 0) && (status < 128)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "PMActionFailedPreS3",
			"pm-action failed before trying to put the system "
			"in the requested power saving state.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	} else if (status == 128) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "PMActionPowerStateS3",
			"pm-action tried to put the machine in the requested "
			"power state but failed.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	} else if (status > 128) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "PMActionFailedS3",
			"pm-action encountered an error and also failed to "
			"enter the requested power saving state.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	return FWTS_OK;
}

static int s3power_options_check(fwts_framework *fw)
{
	FWTS_UNUSED(fw);

	if ((s3power_sleep_delay < 600) || (s3power_sleep_delay > 24*60*60)) {
		fprintf(stderr, "--s3power-sleep-delay is %d, it cannot be less than 600 seconds or more than 24 hours!\n", s3power_sleep_delay);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static int s3power_options_handler(fwts_framework *fw, int argc, char * const argv[], int option_char, int long_index)
{
	FWTS_UNUSED(fw);
	FWTS_UNUSED(argc);
	FWTS_UNUSED(argv);

        switch (option_char) {
        case 0:
                switch (long_index) {
		case 0:
			s3power_sleep_delay = atoi(optarg);
			break;
		}
	}
	return FWTS_OK;
}

static fwts_option s3power_options[] = {
	{ "s3power-sleep-delay","", 1, "Sleep N seconds between start of suspend and wakeup, e.g. --s3power-sleep-delay=60" },
	{ NULL, NULL, 0, NULL }
};

static fwts_framework_minor_test s3power_tests[] = {
	{ s3power_test, "S3 power loss during suspend test." },
	{ NULL, NULL }
};

static fwts_framework_ops s3power_ops = {
	.description = "S3 power loss during suspend test (takes minimum of 10 minutes to run).",
	.init        = s3power_init,
	.minor_tests = s3power_tests,
	.options     = s3power_options,
	.options_handler = s3power_options_handler,
	.options_check = s3power_options_check,
};

FWTS_REGISTER(s3power, &s3power_ops, FWTS_TEST_LATE, FWTS_FLAG_POWER_STATES | FWTS_FLAG_ROOT_PRIV);

#endif
