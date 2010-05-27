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

char *wkalarm = "/sys/class/rtc/rtc0/wakealarm";

int wakealarm_get_irq_state(void)
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

int wakealarm_trigger(log *results, framework *fw, int seconds)
{
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "+%d", seconds);

	if (set("0", wkalarm)) {
		log_error(results, "Cannot write '0' to %s", wkalarm);
		return -1;
	}
	if (set(buffer, wkalarm)) {
		log_error(results, "Cannot write '%s' to %s", wkalarm);
		return -1;
	}
	if (!wakealarm_get_irq_state()) {
		log_error(results, "Wakealarm %s did not get set", wkalarm);
		return -1;
	}
	return 0;
}

int wakealarm_test_firing(log *results, framework *fw, int seconds)
{
	int ret;

	if ((ret = wakealarm_trigger(results, fw, seconds)) != 0)
		return ret;

	sleep(seconds+1);
	if (wakealarm_get_irq_state()) {
		log_error(results, "Wakealarm %s did not fire", wkalarm);
		return 1;
	}
	return 0;
}
