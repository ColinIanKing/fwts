/*
 * Copyright (C) 2010-2013 Canonical
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

static int wakealarm_test1(fwts_framework *fw)
{
	if (fwts_wakealarm_exits(fw) == FWTS_OK)
		fwts_passed(fw, "RTC with a RTC alarm ioctl() interface found.");
	else {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "NoWakeAlarmTest1",
			"Could not find an RTC with an alarm ioctl() interface.");
#ifdef FWTS_ARCH_INTEL
		/* For x86 devices, this is considered a failure */
		fwts_advice(fw,
			"x86 devices generally should have an RTC wake alarm that "
			"is normally controlled by the RTC alarm ioctl() interface. This interface "
			"does not exist, so the wake alarm tests will be aborted.");
#else
		fwts_advice(fw,
			"non-x86 devices sometimes do not have an RTC wake alarm that "
			"is normally controlled by the RTC alarm ioctl() interface. This "
			"interface does not exist, so the wake alarm tests will be aborted.");
#endif
		return FWTS_ABORTED;
	}

	return FWTS_OK;
}

static int wakealarm_test2(fwts_framework *fw)
{
	fwts_log_info(fw, "Trigger wakealarm for 1 seconds in the future.");
	if (fwts_wakealarm_trigger(fw, 1)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "WakeAlarmNotTriggeredTest2",
			"RTC wakealarm did not trigger.");
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
		return FWTS_OK;
	}

	(void)fwts_wakealarm_cancel(fw);

	fwts_passed(fw, "RTC wakealarm was triggered successfully.");

	return FWTS_OK;
}

static int wakealarm_test3(fwts_framework *fw)
{
	int ret;

	ret = fwts_wakealarm_test_firing(fw, 2);
	if (ret < 0) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "WakeAlarmNotTriggeredTest3",
			"Failed to trigger and fire wakealarm.");
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
		return FWTS_ERROR;	/* Really went wrong */
	}
	if (ret == 0)
		fwts_passed(fw, "RTC wakealarm triggered and fired successfully.");
	else {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "WakeAlarmNotFiredTest3",
			"RTC wakealarm was triggered but did not fire.");
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
	}

	return FWTS_OK;
}

static int wakealarm_test4(fwts_framework *fw)
{
	int i;
	int failed = 0;

	for (i = 1; i < 5; i++) {
		fwts_log_info(fw, "Trigger wakealarm for %d seconds in the future.", i);
		int ret = fwts_wakealarm_test_firing(fw, i);
		if (ret < 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"WakeAlarmNotTriggeredTest4",
				"Failed to trigger and fire wakealarm.");
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			return FWTS_ERROR;	/* Really went wrong */
		}
		if (ret != 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"WakeAlarmNotFiredTest4",
				"RTC wakealarm was triggered but did not fire.");
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			failed++;
		}
		fwts_progress(fw, 25 * i);
	}
	if (failed == 0)
		fwts_passed(fw, "RTC wakealarm triggered and fired successfully.");

	return FWTS_OK;
}

static fwts_framework_minor_test wakealarm_tests[] = {
	{ wakealarm_test1, "Check existence of RTC with alarm interface." },
	{ wakealarm_test2, "Trigger wakealarm for 1 seconds in the future." },
	{ wakealarm_test3, "Check if wakealarm is fired." },
	{ wakealarm_test4, "Multiple wakealarm firing tests." },
	{ NULL, NULL }
};

static fwts_framework_ops wakealarm_ops = {
	.description = "Test ACPI Wakealarm.",
	.minor_tests = wakealarm_tests
};

FWTS_REGISTER("wakealarm", &wakealarm_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);
