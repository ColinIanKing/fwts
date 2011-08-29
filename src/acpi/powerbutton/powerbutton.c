/*
 * Copyright (C) 2010-2011 Canonical
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

static DIR *power_buttondir;

#define POWER_BUTTON_PATH	"/proc/acpi/button/power"

static int power_button_init(fwts_framework *fw)
{
	if (!(power_buttondir = opendir(POWER_BUTTON_PATH))) {
		fwts_failed(fw, LOG_LEVEL_LOW, "NoButtonPath",
			"No %s directory available: cannot test.",
			POWER_BUTTON_PATH);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int power_button_deinit(fwts_framework *fw)
{
	if (power_buttondir)
		closedir(power_buttondir);

	return FWTS_OK;
}

static void power_button_check_field(fwts_framework *fw,
	char *field, char *contents, int *matching, int *not_matching)
{
	struct dirent *entry;

	rewinddir(power_buttondir);
	do {
		entry = readdir(power_buttondir);
		if (entry && strlen(entry->d_name)>2) {
			char path[PATH_MAX];
			char *data;

			snprintf(path, sizeof(path), POWER_BUTTON_PATH "/%s/%s",
				entry->d_name, field);
			if ((data = fwts_get(path)) != NULL) {
				if (strstr(data, contents)) {
					(*matching)++;
					fwts_log_info(fw, "Found power button %s.", entry->d_name);
				}
				else
					(*not_matching)++;
			}
			free(data);
		}
	} while (entry);
}

static int power_button_test1(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	power_button_check_field(fw, "info", "Power Button", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed(fw, LOG_LEVEL_LOW, "NoPowerButtonField",
			"Failed to detect a Power Button in power button info field.");
	else
		fwts_passed(fw, "Detected a Power Button in a power button info field.");

	return FWTS_OK;
}

static int power_button_test2(fwts_framework *fw)
{
	int fd;
	size_t len;
	char *buffer;
	int matching;
	int i;

	fwts_printf(fw, "==== Please press the laptop power button. ====\n");

	if ((fd = fwts_acpi_event_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}

	matching = 0;

	for (i=0;i<=20;i++) {
		if ((buffer = fwts_acpi_event_read(fd, &len, 1)) != NULL) {
			if (strstr(buffer, "button/power")) {
				matching++;
				break;
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	if (matching == 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "NoPowerButtonEvents",
			"Did not detect any ACPI power buttons events while waiting for power button to be pressed.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_EVENT);
	} else  {
		char button[4096];
		memset(button, 0, sizeof(button));
		sscanf(buffer, "%*s %s", button);

		fwts_passed(fw, "Detected %s power button event.", button);
	}

	fwts_acpi_event_close(fd);

	return FWTS_OK;
}

static fwts_framework_minor_test power_button_tests[] = {
	{ power_button_test1, "Test power button(s) report they are really Power Buttons." },
	{ power_button_test2, "Test press of power button and ACPI event." },
	{ NULL, NULL }
};

static fwts_framework_ops power_button_ops = {
	.description = "Interactive power_button button test.",
	.init        = power_button_init,
	.deinit      = power_button_deinit,
	.minor_tests = power_button_tests
};

FWTS_REGISTER(power_button, &power_button_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);

#endif
