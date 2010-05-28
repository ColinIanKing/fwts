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

#include "framework.h"
#include "wakealarm.h"

static char *wkalarm = WAKEALARM;

int wakealarm_init(log *results, framework *fw)
{
	return check_root_euid(results);
}

int wakealarm_deinit(log *results, framework *fw)
{
	return 0;
}

void wakealarm_headline(log *results)
{
	log_info(results, "Test ACPI Wakealarm");
}

int wakealarm_test1(log *results, framework *fw)
{
	struct stat buf;
	char *test = "Check existance of " WAKEALARM;

	log_info(results, test);

	if (stat(wkalarm, &buf) == 0)
		framework_passed(fw, test);
	else
		framework_failed(fw, test);

	return 0;
}

int wakealarm_test2(log *results, framework *fw)
{
	char *test = "Trigger RTC wakealarm";

	log_info(results, test);
	
	log_info(results, "Trigger wakealarm for 1 seconds in the future");
	if (wakealarm_trigger(results, fw, 1)) {
		framework_failed(fw, test);
		return 0;
	}

	framework_passed(fw, test);

	return 0;
}

int wakealarm_get_irq_state(void);
int wakealarm_test_firing(log *results, framework *fw, int sleep);


int wakealarm_test3(log *results, framework *fw)
{
	char *test = "Check if wakealarm is fired";
	int ret;

	log_info(results, test);

	log_info(results, "Trigger wakealarm for 2 seconds in the future");
	ret = wakealarm_test_firing(results, fw, 2);
	if (ret < 0) {
		framework_failed(fw, test);
		return 1;	/* Really went wrong */
	}
	if (ret == 0)
		framework_passed(fw, test);
	else
		framework_failed(fw, test);
		
	return 0;
}

int wakealarm_test4(log *results, framework *fw)
{
	int i;
	char *test = "Multiple wakealarm firing tests";

	log_info(results, test);

	for (i=1; i<5; i++) {
		log_info(results, "Trigger wakealarm for %d seconds in the future", i);
		int ret = wakealarm_test_firing(results, fw, i);
		if (ret < 0) {
			framework_failed(fw, test);
			return 1;	/* Really went wrong */
		}
		if (ret != 0)
			framework_failed(fw, test);
	}
	framework_passed(fw, test);

	return 0;
}

framework_tests wakealarm_tests[] = {
	wakealarm_test1,
	wakealarm_test2,
	wakealarm_test3,	
	wakealarm_test4,
	NULL
};

framework_ops wakealarm_ops = {
	wakealarm_headline,
	wakealarm_init,
	wakealarm_deinit,
	wakealarm_tests
};

FRAMEWORK(wakealarm, &wakealarm_ops, TEST_ANYTIME);
