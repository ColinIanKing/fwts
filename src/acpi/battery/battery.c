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

#include "framework.h"
#include "stringextras.h"

static unsigned long get_full(char *dir)
{
	FILE *file;
	char path[PATH_MAX];
	unsigned long value = 0;

	if (!dir)
		return 0;
	
	sprintf(path, "%s/state", dir);
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

static void check_charging(framework *fw, char *dir, char *uri, char *name)
{
	int i;
	/* when we get here we KNOW the state is "charging" */
	unsigned long initial_value, new_value;

	if (!name)
		return;

	initial_value = get_full(dir);
	for (i=1; i<30; i++) {
		new_value = get_full(dir);
		if (new_value>initial_value) {
			framework_passed(fw, "Battery %s charge is incrementing as expected", name);
			return;
		}		
		sleep(1);
	}
	framework_failed(fw, "Battery %s claims it's charging but no charge is added", name);
}

static void check_discharging(framework *fw, char *dir, char *uri, char *name)
{
	int i;
	/* when we get here we KNOW the state is "discharging" */
	unsigned long initial_value, new_value;

	if (!name)
		return;

	initial_value = get_full(dir);
	for (i=1; i<30; i++) {
		new_value = get_full(dir);
		if (new_value<initial_value) {
			framework_passed(fw, "Battery %s charge is decrementing as expected", name);
			return;
		}		
		sleep(1);
	}
	framework_failed(fw, "Battery %s claims it's discharging but no charge is used", name);
}


static void do_battery(log *results, framework *fw, char *dir, char *name)
{
	FILE *file;
	char path[PATH_MAX];
	char uri[1024];
	int present=1;
	char *state = NULL;
	char *model = NULL;

	if (!dir)
		return;

	sprintf(path, "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL) {
		framework_failed(fw, "Battery present but undersupported - no state present");
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

	sprintf(path, "%s/info", dir);
	file = fopen(path, "r");
	if (file == NULL) {
		log_warning(results, "Battery present but undersupported - no info present");
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
		log_warning(results, "Battery present but name or state unsupported");
		return;
	}

	chop_newline(model);
	chop_newline(state);

	log_info(results, "Battery %s is model %s and is currently %s", name, model, state);

	if (strstr(state,"discharging"))
		check_discharging(fw, dir, uri, name);
	else if (strstr(state,"charging"))
		check_charging(fw, dir, uri, name);

	free(state);
	free(model);
	
}

static char *battery_headline(void)
{
	return "Battery tests";
}

static int battery_test1(log *results, framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int battdir = 0;

	log_info(results, 
	   "This test reports which (if any) batteries there are in the system.\n"
	   "In addition, for charging or discharging batteries, the test validates\n"
	   "that the reported 'current capacity' properly increments/decrements\n"
	   "in line with the charge/discharge state.\n\n"
	   "This test also stresses the entire battery state reporting codepath\n"
	   "in the ACPI BIOS, and any warnings given by the ACPI interpreter\n"
	   "will be reported.");

	if (!(dir = opendir("/proc/acpi/battery/"))) {
		log_info(results, "No battery information present: cannot test");
		return 0;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name)>2) {
			char batpath[2048];

			sprintf(batpath, "/proc/acpi/battery/%s", entry->d_name);
			do_battery(results, fw, batpath, entry->d_name);
			battdir++;
		}
	} while (entry);

	if (battdir == 0)
		log_info(results, "No battery information present: cannot test");

	return 0;
}

static framework_tests battery_tests[] = {
	battery_test1,
	NULL
};

static framework_ops battery_ops = {
	battery_headline,
	NULL,
	NULL,
	battery_tests
};

FRAMEWORK(battery, &battery_ops, TEST_ANYTIME);
