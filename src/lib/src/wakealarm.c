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

static char *fwts_wkalarm = "/sys/class/rtc/rtc0/wakealarm";

int fwts_wakealarm_get_irq_state(void)
{
	FILE *fp;
	char field[32];
	char value[32];

	if ((fp = fopen("/proc/driver/rtc", "r")) == NULL)
		return FWTS_ERROR;

	while (fscanf(fp, "%s : %s\n", field, value) != EOF) {	
		if (!strcmp(field, "alarm_IRQ")) {
			fclose(fp);
			return strcmp(value, "no");
		}
	}
	fclose(fp);
	
	return FWTS_ERROR;
}

int fwts_wakealarm_trigger(fwts_framework *fw, const int seconds)
{
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "+%d", seconds);

	if (fwts_set("0", fwts_wkalarm)) {
		fwts_log_error(fw, "Cannot write '0' to %s", fwts_wkalarm);
		return FWTS_ERROR;
	}
	if (fwts_set(buffer, fwts_wkalarm)) {
		fwts_log_error(fw, "Cannot write '%s' to %s", fwts_wkalarm);
		return FWTS_ERROR;
	}
	if (!fwts_wakealarm_get_irq_state()) {
		fwts_log_error(fw, "Wakealarm %s did not get set", fwts_wkalarm);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

int fwts_wakealarm_test_firing(fwts_framework *fw, const int seconds)
{
	int ret;

	if ((ret = fwts_wakealarm_trigger(fw, seconds)) != 0)
		return ret;

	sleep(seconds+1);
	if (fwts_wakealarm_get_irq_state()) {
		fwts_log_error(fw, "Wakealarm %s did not fire", fwts_wkalarm);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}
