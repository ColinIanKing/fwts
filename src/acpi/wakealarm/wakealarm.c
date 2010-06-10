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
#include <string.h>

#include "fwts.h"

static char *wkalarm = WAKEALARM;

static int wakealarm_init(fwts_framework *fw)
{
	return fwts_check_root_euid(fw);
}

static char *wakealarm_headline(void)
{
	return "Test ACPI Wakealarm";
}

static int wakealarm_test1(fwts_framework *fw)
{
	struct stat buf;
	char *test = "Check existance of " WAKEALARM;

	fwts_log_info(fw, test);

	if (stat(wkalarm, &buf) == 0)
		fwts_passed(fw, test);
	else
		fwts_failed(fw, test);

	return 0;
}

static int wakealarm_test2(fwts_framework *fw)
{
	char *test = "Trigger RTC wakealarm";

	fwts_log_info(fw, test);
	
	fwts_log_info(fw, "Trigger wakealarm for 1 seconds in the future");
	if (fwts_wakealarm_trigger(fw, 1)) {
		fwts_failed(fw, test);
		return 0;
	}

	fwts_passed(fw, test);

	return 0;
}

static int wakealarm_test3(fwts_framework *fw)
{
	char *test = "Check if wakealarm is fired";
	int ret;

	fwts_log_info(fw, test);

	fwts_log_info(fw, "Trigger wakealarm for 2 seconds in the future");
	ret = fwts_wakealarm_test_firing(fw, 2);
	if (ret < 0) {
		fwts_failed(fw, test);
		return 1;	/* Really went wrong */
	}
	if (ret == 0)
		fwts_passed(fw, test);
	else
		fwts_failed(fw, test);
		
	return 0;
}

static int wakealarm_test4(fwts_framework *fw)
{
	int i;
	char *test = "Multiple wakealarm firing tests";

	fwts_log_info(fw, test);
	int failed = 0;

	for (i=1; i<5; i++) {
		fwts_log_info(fw, "Trigger wakealarm for %d seconds in the future", i);
		int ret = fwts_wakealarm_test_firing(fw, i);
		if (ret < 0) {
			fwts_failed(fw, test);
			return 1;	/* Really went wrong */
		}
		if (ret != 0) {
			fwts_failed(fw, test);	
			failed++;
		}
	}
	if (failed == 0)
		fwts_passed(fw, test);

	return 0;
}

static fwts_framework_tests wakealarm_tests[] = {
	wakealarm_test1,
	wakealarm_test2,
	wakealarm_test3,	
	wakealarm_test4,
	NULL
};

static fwts_framework_ops wakealarm_ops = {
	wakealarm_headline,
	wakealarm_init,
	NULL,
	wakealarm_tests
};

FRAMEWORK(wakealarm, &wakealarm_ops, TEST_ANYTIME);
