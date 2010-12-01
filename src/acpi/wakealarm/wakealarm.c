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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

static char *wkalarm = WAKEALARM;

static int wakealarm_init(fwts_framework *fw)
{
	return fwts_check_root_euid(fw);
}

static char *wakealarm_headline(void)
{
	return "Test ACPI Wakealarm.";
}

static int wakealarm_test1(fwts_framework *fw)
{
	struct stat buf;

	if (stat(wkalarm, &buf) == 0)
		fwts_passed(fw, WAKEALARM " found.");
	else
		fwts_failed(fw, "Could not find " WAKEALARM ".");

	return FWTS_OK;
}

static int wakealarm_test2(fwts_framework *fw)
{
	fwts_log_info(fw, "Trigger wakealarm for 1 seconds in the future.");
	if (fwts_wakealarm_trigger(fw, 1)) {
		fwts_failed(fw, "RTC wakealarm did not trigger.");
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
		return FWTS_OK;
	}

	fwts_passed(fw, "RTC wakealarm was triggered successfully.");

	return FWTS_OK;
}

static int wakealarm_test3(fwts_framework *fw)
{
	int ret;

	ret = fwts_wakealarm_test_firing(fw, 2);
	if (ret < 0) {
		fwts_failed(fw, "Failed to trigger and fire wakealarm.");
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
		return FWTS_ERROR;	/* Really went wrong */
	}
	if (ret == 0)
		fwts_passed(fw, "RTC wakealarm triggered and fired successfully.");
	else {
		fwts_failed(fw, "RTC wakealarm was triggered but did not fire.");
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
	}
		
	return FWTS_OK;
}

static int wakealarm_test4(fwts_framework *fw)
{
	int i;
	int failed = 0;

	for (i=1; i<5; i++) {
		fwts_log_info(fw, "Trigger wakealarm for %d seconds in the future.", i);
		int ret = fwts_wakealarm_test_firing(fw, i);
		if (ret < 0) {
			fwts_failed(fw, "Failed to trigger and fire wakealarm.");
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			return FWTS_ERROR;	/* Really went wrong */
		}
		if (ret != 0) {
			fwts_failed(fw, "RTC wakealarm was triggered but did not fire.");
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
	{ wakealarm_test1, "Check existance of " WAKEALARM "." },
	{ wakealarm_test2, "Trigger wakealarm for 1 seconds in the future." },
	{ wakealarm_test3, "Check if wakealarm is fired." },
	{ wakealarm_test4, "Multiple wakealarm firing tests." },
	{ NULL, NULL }
};

static fwts_framework_ops wakealarm_ops = {
	wakealarm_headline,
	wakealarm_init,
	NULL,
	wakealarm_tests
};

FWTS_REGISTER(wakealarm, &wakealarm_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
