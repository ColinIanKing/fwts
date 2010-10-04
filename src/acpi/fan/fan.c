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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

static void do_fan(fwts_framework *fw, char *dir, char *name)
{
	FILE *file;
	char path[PATH_MAX];
	char buffer[4096];
	char *state = NULL;

	if (!dir)
		return;

	snprintf(path, sizeof(path), "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL) {
		fwts_failed(fw, "Fan present but is undersupported - no state present.");
		return;
	}

	if (fgets(buffer, sizeof(buffer)-1, file) == NULL) {
		fwts_failed(fw, "Fan present but is undersupported - no state present.");
		fclose(file);
		return;
	}
	fclose(file);

	if ((state = strstr(buffer, "status:")) == NULL) {
		fwts_failed(fw, "Fan present but is undersupported - no state present.");
		return;
	}

	for (state += 8; (*state == ' ') && (*state != '\n'); state++)
		;

	fwts_chop_newline(state);

	fwts_passed(fw, "Fan %s status is '%s'", name, state);
}

static char *fan_headline(void)
{
	return "Simple Fan Tests.";
}

static int fan_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int fandir = 0;

	fwts_log_info(fw, 
		"Test how many fans there are in the system. "
		"Check for the current status of the fan(s).");

	if (!(dir = opendir("/proc/acpi/fan/"))) {
		fwts_log_info(fw, "No fan information present: cannot test.");
		return FWTS_SKIP;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name)>2) {
			char batpath[2048];

			snprintf(batpath, sizeof(batpath), "/proc/acpi/fan/%s", entry->d_name);
			do_fan(fw, batpath, entry->d_name);
			fandir++;
		}
	} while (entry);

	closedir(dir);

	if (fandir == 0) {
		fwts_log_info(fw, "No fan information present: cannot test.");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static fwts_framework_minor_test fan_tests[] = {
	{ fan_test1, "Check fan status." },
	{ NULL, NULL} 
};

static fwts_framework_ops fan_ops = {
	.headline    = fan_headline,
	.minor_tests = fan_tests
};

FWTS_REGISTER(fan, &fan_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
