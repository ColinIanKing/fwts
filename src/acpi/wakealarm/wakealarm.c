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

static char *wkalarm = "/sys/class/rtc/rtc0/wakealarm";

int get_alarm_irq_state(void)
{
	FILE *fp;
	char field[32];
	char value[32];

	if ((fp = fopen("/proc/driver/rtc", "r")) == NULL) {
		return -1;
	}
	while (fscanf(fp, "%s : %s\n", field, value) != EOF) {	
		if (!strcmp(field, "alarm_IRQ")) {
			fclose(fp);
			return strcmp(value, "no");
		}
	}
	fclose(fp);
	
	return -1;
}

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
	char *test = "Check existance of /sys/class/rtc/rtc0/wakealarm";

	log_info(results, test);

	if (stat(wkalarm, &buf) == 0)
		framework_passed(fw, test);
	else
		framework_failed(fw, test);

	return 0;
}

int wakealarm_test2(log *results, framework *fw)
{
	char *test = "Trigger rtc wakealarm";
	char *wkalarm = "/sys/class/rtc/rtc0/wakealarm";

	log_info(results, test);
	
	if (set("0", wkalarm)) {
		log_error(results, "Cannot write to %s", wkalarm);
		framework_failed(fw, test);
		return 0;
	}
	if (set("+2", wkalarm)) {
		log_error(results, "Cannot write to %s", wkalarm);
		framework_failed(fw, test);
		return 0;
	}
	if (!get_alarm_irq_state()) {
		log_error(results, "Wakealarm %s did not get set", wkalarm);
		framework_failed(fw, test);
		return 0;
	}

	framework_passed(fw, test);

	return 0;
}

int wakealarm_test3(log *results, framework *fw)
{
	char *test = "Check if wakealarm is fired";

	log_info(results, test);

	if (set("0", wkalarm)) {
		log_error(results, "Cannot write to %s", wkalarm);
		framework_failed(fw, test);
		return 0;
	}
	if (set("+2", wkalarm)) {
		log_error(results, "Cannot write to %s", wkalarm);
		framework_failed(fw, test);
		return 0;
	}
	if (!get_alarm_irq_state()) {
		log_error(results, "Wakealarm %s did not get set", wkalarm);
		framework_failed(fw, test);
		return 0;
	}
	log_info(results, "Wait 3 seconds for alarm to fire");
	sleep(3);
	if (get_alarm_irq_state()) {
		log_error(results, "Wakealarm %s did not fire", wkalarm);
		framework_failed(fw, test);
		return 0;
	}

	framework_passed(fw, test);

	return 0;
}

int wakealarm_test4(log *results, framework *fw)
{
	int i;
	char *test = "Multiple wakealarm firing tests";

	log_info(results, test);

	for (i=1; i<30; i+= 10) {
		char seconds[16];

		if (set("0", wkalarm)) {
			log_error(results, "Cannot write to %s", wkalarm);
			framework_failed(fw, test);
			return 0;
		}

		snprintf(seconds, sizeof(seconds), "+%d",i);
		if (set(seconds, wkalarm)) {
			log_error(results, "Cannot write to %s", wkalarm);
			framework_failed(fw, test);
			return 0;
		}
		if (!get_alarm_irq_state()) {
			log_error(results, "Wakealarm %s did not get set", wkalarm);
			framework_failed(fw, test);
			return 0;
		}
		log_info(results, "Wait %d seconds for %d second alarm to fire", i + 1, i);
		sleep(i + 1);
		if (get_alarm_irq_state()) {
			log_error(results, "Wakealarm %s did not fire", wkalarm);
			framework_failed(fw, test);
			return 0;
		}
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

FRAMEWORK(wakealarm, "wakealarm.log", &wakealarm_ops, &private);
