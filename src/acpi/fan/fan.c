/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 *
 * This file was originally part of the Linux-ready Firmware Developer Kit
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
#include <limits.h>
#include <dirent.h>

#include "fwts.h"

static void do_fan(fwts_framework *fw, char *dir, char *name)
{
	FILE *file;
	char path[PATH_MAX];
	char buffer[4096];
	char *state = NULL;

	if (!dir)
		return;

	sprintf(path, "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL) {
		fwts_failed(fw, "Fan present but is undersupported - no state present");
		return;
	}

	if (fgets(buffer, 4095, file) == NULL) {
		fwts_failed(fw, "Fan present but is undersupported - no state present");
		fclose(file);
		return;
	}
	fclose(file);

	if ((state = strstr(buffer, "status:")) == NULL) {
		fwts_failed(fw, "Fan present but is undersupported - no state present");
		return;
	}

	for (state += 8; (*state == ' ') && (*state != '\n'); state++)
		;

	fwts_chop_newline(state);

	fwts_passed(fw, "Fan %s status is '%s'", name, state);
}

static char *fan_headline(void)
{
	return "Simple Fan Tests";
}

static int fan_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int fandir = 0;

	fwts_log_info(fw, 
		"Test how many fans there are in the system.\n"
		"Check for the current status of the fan(s).");

	if (!(dir = opendir("/proc/acpi/fan/"))) {
		fwts_log_info(fw, "No fan information present: cannot test");
		return 0;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name)>2) {
			char batpath[2048];

			sprintf(batpath, "/proc/acpi/fan/%s", entry->d_name);
			do_fan(fw, batpath, entry->d_name);
			fandir++;
		}
	} while (entry);

	if (fandir == 0)
		fwts_log_info(fw, "No fan information present: cannot test");

	return 0;
}

static fwts_framework_tests fan_tests[] = {
	fan_test1,
	NULL
};

static fwts_framework_ops fan_ops = {
	fan_headline,
	NULL,
	NULL,
	fan_tests
};

FRAMEWORK(fan, &fan_ops, TEST_ANYTIME);
