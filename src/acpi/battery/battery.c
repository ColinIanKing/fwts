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

static unsigned long get_full(char *dir)
{
	FILE *file;
	char path[PATH_MAX];
	unsigned long value = 0;

	if (!dir)
		return 0;
	
	snprintf(path, sizeof(path), "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL)
		return 0;
	
	while (!feof(file)) {
		char buffer[4096];
		memset(buffer, 0, 4096);

		if (fgets(buffer, 4095, file) == NULL)
			break;
		if (strstr(buffer,"remaining capacity:") && strlen(buffer)>25) 
			value = strtoull(buffer+25, NULL, 10);
		
	}
	fclose(file);
	return value;
}

static void check_charging(fwts_framework *fw, char *dir, char *uri, char *name)
{
	int i;
	/* when we get here we KNOW the state is "charging" */
	unsigned long initial_value, new_value;

	if (!name)
		return;

	initial_value = get_full(dir);
	for (i=1; i<120; i++) {
		new_value = get_full(dir);
		if (new_value>initial_value) {
			fwts_passed(fw, "Battery %s charge is incrementing as expected.", name);
			return;
		}		
		sleep(1);
	}
	fwts_failed(fw, "Battery %s claims it's charging but no charge is added", name);
}

static void check_discharging(fwts_framework *fw, char *dir, char *uri, char *name)
{
	int i;
	/* when we get here we KNOW the state is "discharging" */
	unsigned long initial_value, new_value;

	if (!name)
		return;

	initial_value = get_full(dir);
	for (i=1; i<24; i++) {
		new_value = get_full(dir);
		if (new_value<initial_value) {
			fwts_passed(fw, "Battery %s charge is decrementing as expected.", name);
			return;
		}		
		fwts_cpu_consume(5);
	}
	fwts_failed(fw, "Battery %s claims it is discharging but no charge is used.", name);
}


static void do_battery(fwts_framework *fw, char *dir, char *name)
{
	FILE *file;
	char path[PATH_MAX];
	char uri[1024];
	int present=1;
	char *state = NULL;
	char *model = NULL;

	if (!dir)
		return;

	snprintf(path, sizeof(path), "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL) {
		fwts_failed(fw, "Battery present but undersupported - no state present.");
		return;
	}

	while (!feof(file)) {
		char buffer[4096];
		memset(buffer, 0, 4096);

		if (fgets(buffer, 4095, file) == NULL)
			break;
		if (strcmp(buffer,"present:                 yes") == 0)
			present = 1;
		if (strstr(buffer,"charging state:") && strlen(buffer)>25) 
			state=strdup(buffer+25);
	}
	fclose(file);

	snprintf(path, sizeof(path), "%s/info", dir);
	file = fopen(path, "r");
	if (file == NULL) {
		fwts_log_warning(fw, "Battery present but undersupported - no info present.");
		return;
	}
	while (!feof(file)) {
		char buffer[4096];
		memset(buffer, 0, 4096);

		if (fgets(buffer, 4095, file) == NULL)
			break;
		if (strcmp(buffer,"present:                 yes") == 0)
			present = 1;
		if (strstr(buffer,"model number:") && strlen(buffer)>25) 
			model=strdup(buffer+25);
	}
	fclose(file);

	if ((state == NULL) || (model == NULL)) {
		fwts_log_warning(fw, "Battery present but name or state unsupported.");
		return;
	}

	fwts_chop_newline(model);
	fwts_chop_newline(state);

	fwts_log_info(fw, "Battery %s is model %s and is currently %s.", name, model, state);

	if (strstr(state,"discharging"))
		check_discharging(fw, dir, uri, name);
	else if (strstr(state,"charging"))
		check_charging(fw, dir, uri, name);

	free(state);
	free(model);
	
}

static char *battery_headline(void)
{
	return "Battery Tests.";
}

static int battery_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int battdir = 0;

	fwts_log_info(fw, 
	   "This test reports which (if any) batteries there are in the system. "
	   "In addition, for charging or discharging batteries, the test validates "
	   "that the reported 'current capacity' properly increments/decrements "
	   "in line with the charge/discharge state. "
	   "This test also stresses the entire battery state reporting codepath "
	   "in the ACPI BIOS, and any warnings given by the ACPI interpreter "
	   "will be reported.");

	if (!(dir = opendir("/proc/acpi/battery/"))) {
		fwts_log_info(fw, "No battery information present: cannot test.");
		return 0;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name)>2) {
			char batpath[2048];

			snprintf(batpath, sizeof(batpath), "/proc/acpi/battery/%s", entry->d_name);
			do_battery(fw, batpath, entry->d_name);
			battdir++;
		}
	} while (entry);

	if (battdir == 0)
		fwts_log_info(fw, "No battery information present: cannot test.");

	return 0;
}

static fwts_framework_tests battery_tests[] = {
	battery_test1,
	NULL
};

static fwts_framework_ops battery_ops = {
	battery_headline,
	NULL,
	NULL,
	battery_tests
};

FRAMEWORK(battery, &battery_ops, TEST_ANYTIME);
