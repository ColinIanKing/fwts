/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2011 Canonical
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

static void battery_discharge(fwts_framework *fw, const int secs)
{
	int i;
	fwts_cpu_consume_start();

	for (i=0;i<secs;i++) {
		fwts_printf(fw, "Waiting %2.2d/%d\r", secs-i, secs);
		sleep(1);
	}

	fwts_cpu_consume_complete();
}

static unsigned long get_full(char *dir)
{
	FILE *file;
	char path[PATH_MAX];
	unsigned long value = 0;
	char buffer[4096];

	if (!dir)
		return 0;
	
	snprintf(path, sizeof(path), "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL)
		return 0;
	
	while (fgets(buffer, 4095, file) != NULL) {
		if (strstr(buffer,"remaining capacity:") && strlen(buffer)>25) {
			value = strtoull(buffer+25, NULL, 10);	
			break;
		}
	}
	fclose(file);
	return value;
}

static int wait_for_acpi_event(fwts_framework *fw, char *name)
{
	int gpe_count = 0;
	int fd;
	int events = 0;
	int matching = 0;
	int len;
	char *buffer;
	int i;

	fwts_gpe *gpes_start;
	fwts_gpe *gpes_end;

	if ((gpe_count = fwts_gpe_read(&gpes_start)) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot read GPEs.");
		return FWTS_ERROR;
	}

	if ((fd = acpi_event_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}

	for (i=0;i<=20;i++) {
		if ((buffer = acpi_event_read(fd, &len, 1)) != NULL) {
			char *str;
			if ((str = strstr(buffer, "battery")) != NULL) {
				events++;
				if (strstr(str, name) != NULL) {
					matching++;
					free(buffer);
					break;
				}
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	acpi_event_close(fd);

	if ((gpe_count = fwts_gpe_read(&gpes_end)) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot read GPEs.");
	}

	fwts_gpe_test(fw, gpes_start, gpes_end, gpe_count);
	fwts_gpe_free(gpes_start, gpe_count);
	fwts_gpe_free(gpes_end, gpe_count);

	if (events == 0)
		fwts_failed_high(fw, "Did not detect any ACPI battery events.");
	else
		fwts_passed(fw, "Detected ACPI battery events.");
		if (matching == 0)
			fwts_failed_high(fw, "Could not detect ACPI events for battery %s.", name);
		else
			fwts_passed(fw, "Detected ACPI event for battery %s.", name);

	return FWTS_OK;
}
static void check_charging(fwts_framework *fw, char *dir, char *name)
{
	int i;
	/* when we get here we KNOW the state is "charging" */
	unsigned long initial_value, new_value;

	if (!name)
		return;

	fwts_printf(fw, "==== Waiting to see if battery charges ====\n");

	initial_value = get_full(dir);
	for (i=0; i<=120; i++) {
		new_value = get_full(dir);
		if (new_value>initial_value) {
			fwts_passed(fw, "Battery %s charge is incrementing as expected.", name);
			return;
		}		
		fwts_printf(fw, "Waiting %3.3d/120\r", 120-i);
		sleep(1);
	}
	fwts_failed(fw, "Battery %s claims it's charging but no charge is added", name);
}

static void check_discharging(fwts_framework *fw, char *dir, char *name)
{
	int i;
	/* when we get here we KNOW the state is "discharging" */
	unsigned long initial_value, new_value;

	if (!name)
		return;

	fwts_printf(fw, "==== Waiting to see if battery discharges ====\n");
	fwts_cpu_consume_start();

	initial_value = get_full(dir);
	for (i=0; i<=120; i++) {
		new_value = get_full(dir);
		if (new_value<initial_value) {
			fwts_passed(fw, "Battery %s charge is decrementing as expected.", name);
			fwts_cpu_consume_complete();
			return;
		}		
		fwts_printf(fw, "Waiting %3.3d/120\r", 120-i);
		sleep(1);
	}
	fwts_cpu_consume_complete();
	fwts_failed(fw, "Battery %s claims it is discharging but no charge is used.", name);
}


static void do_battery_test(fwts_framework *fw, char *dir, char *name)
{
	FILE *file;
	char path[PATH_MAX];
	char model[1024];
	char state[1024];
	char buffer[4096];

	if (!dir)
		return;

	*model = '\0';
	*state = '\0';

	snprintf(path, sizeof(path), "%s/state", dir);
	if ((file = fopen(path, "r")) == NULL) {
		fwts_failed(fw, "Battery present but undersupported - no state present.");
		return;
	}

	while (fgets(buffer, 4095, file) != NULL) {
		if (strstr(buffer,"charging state:") && strlen(buffer)>25) {
			strncpy(state, buffer+25, sizeof(state));
			break;
		}
	}
	fclose(file);

	snprintf(path, sizeof(path), "%s/info", dir);
	file = fopen(path, "r");
	if (file == NULL) {
		fwts_log_warning(fw, "Battery present but undersupported - no info present.");
		return;
	}
	while (fgets(buffer, 4095, file) != NULL) {
		if (strstr(buffer,"model number:") && strlen(buffer)>25) {
			strncpy(model, buffer+25, sizeof(model));
			break;
		}
	}
	fclose(file);

	if ((*state == '\0') || (*model == '\0')) {
		fwts_log_warning(fw, "Battery present but name or state unsupported.");
		return;
	}

	fwts_chop_newline(model);
	fwts_chop_newline(state);

	fwts_log_info(fw, "Battery %s model %s is currently %s.", name, model, state);

	fwts_printf(fw, "==== Please PLUG IN the AC power of the machine ====\n");
	fwts_press_enter(fw);

	fwts_printf(fw, "==== Please now UNPLUG the AC power of the machine ====\n");
	wait_for_acpi_event(fw, name);
	check_discharging(fw, dir, name);
	fwts_printf(fw, "==== Please wait 30 seconds while battery is discharged a little ====\n");
	battery_discharge(fw, 30);
	fwts_printf(fw, "==== Please now PLUG IN the AC power of the machine ====\n");
	wait_for_acpi_event(fw, name);
	check_charging(fw, dir, name);
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
	   "This test also stresses the battery state reporting codepath "
	   "in the ACPI BIOS, and any warnings given by the ACPI interpreter "
	   "will be reported.");

	if (!(dir = opendir("/proc/acpi/battery/"))) {
		fwts_log_info(fw, "No battery information present: cannot test.");
		return FWTS_OK;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name)>2) {
			char batpath[2048];

			snprintf(batpath, sizeof(batpath), "/proc/acpi/battery/%s", entry->d_name);
			do_battery_test(fw, batpath, entry->d_name);
			battdir++;
		}
	} while (entry);

	closedir(dir);

	if (battdir == 0)
		fwts_log_info(fw, "No battery information present: cannot test.");

	return FWTS_OK;
}

static fwts_framework_minor_test battery_tests[] = {
	{ battery_test1, "Check batteries." },
	{ NULL, NULL }
};

static fwts_framework_ops battery_ops = {
	.headline    = battery_headline,
	.minor_tests = battery_tests
};

FWTS_REGISTER(battery, &battery_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);

#endif
