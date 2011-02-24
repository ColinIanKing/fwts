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

DIR *ac_adapterdir;

#define AC_ADAPTER_PATH	"/proc/acpi/ac_adapter"

static int ac_adapter_init(fwts_framework *fw)
{
	if (!(ac_adapterdir = opendir(AC_ADAPTER_PATH))) {
		fwts_failed_low(fw, "No %s directory available: cannot test.", AC_ADAPTER_PATH);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int ac_adapter_deinit(fwts_framework *fw)
{
	if (ac_adapterdir)
		closedir(ac_adapterdir);

	return FWTS_OK;
}

static char *ac_adapter_headline(void)
{
	return "Interactive ac_adapter power test.";
}

void ac_adapter_check_field(char *field, char *contents, int *matching, int *not_matching)
{
	struct dirent *entry;

	rewinddir(ac_adapterdir);
	do {
		entry = readdir(ac_adapterdir);
		if (entry && strlen(entry->d_name)>2) {
			char path[PATH_MAX];
			char *data;

			snprintf(path, sizeof(path), AC_ADAPTER_PATH "/%s/%s", entry->d_name, field);
			if ((data = fwts_get(path)) != NULL) {
				if (strstr(data, contents))
					(*matching)++;
				else
					(*not_matching)++;
			}
			free(data);
		}
	} while (entry);
}

static int ac_adapter_test1(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	ac_adapter_check_field("state", "state:", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed_low(fw, "Failed to detect any state in the ac_adapter state info.");
	else
		fwts_passed(fw, "Detected a state in the ac_adapter state info.");

	return FWTS_OK;
}

static int ac_adapter_test2(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	fwts_printf(fw, "==== Make sure laptop is connected to the mains power. ====\n");
	fwts_press_enter(fw);

	ac_adapter_check_field("state", "on-line", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed_high(fw, "Failed to detect an ac_adapter on-line state.");
	else
		fwts_passed(fw, "Detected ac_adapter on-line state.");

	return FWTS_OK;
}

static int ac_adapter_test3(fwts_framework *fw)
{
	int fd;
	int len;
	char *buffer;
	int matching;
	int not_matching;
	int events;
	int i;

	if ((fd = fwts_acpi_event_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}

	events = 0;
	matching = 0;
	not_matching = 0;

	fwts_printf(fw, "==== Please unplug the laptop power. ====\n");

	for (i=0;i<20;i++) {
		if ((buffer = fwts_acpi_event_read(fd, &len, 1)) != NULL) {
			if (strstr(buffer, "ac_adapter")) {
				events++;
				ac_adapter_check_field("state", "off-line", &matching, &not_matching);
				break;
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	if (events == 0)
		fwts_failed_high(fw, "Did not detect any ACPI ac-adapter events while waiting for power to be disconnected.");
	else
		if ((matching == 0) || (not_matching > 0))
			fwts_failed_high(fw, "Could not detect ac_adapter off-line state.");
		else
			fwts_passed(fw, "Detected ac_adapter off-line state.");

	events = 0;
	matching = 0;
	not_matching = 0;

	fwts_printf(fw, "==== Please re-connect the laptop power. ====\n");

	for (i=0;i<20;i++) {
		if ((buffer = fwts_acpi_event_read(fd, &len, 1)) != NULL) {
			events++;
			if (strstr(buffer, "ac_adapter")) {
				events++;
				ac_adapter_check_field("state", "on-line", &matching, &not_matching);
				break;
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	if (events == 0)
		fwts_failed_high(fw, "Did not detect any ACPI ac-adapter events while waiting for power to be re-connected.");
	else
		if ((matching == 0) || (not_matching > 0))
			fwts_failed_high(fw, "Could not detect ac_adapter on-line state.");
		else
			fwts_passed(fw, "Detected ac_adapter on-line state.");

	fwts_acpi_event_close(fd);

	return FWTS_OK;
}

static fwts_framework_minor_test ac_adapter_tests[] = {
	{ ac_adapter_test1, "Test ACPI ac_adapter state." },
	{ ac_adapter_test2, "Test ac_adapter initial on-line state." },
	{ ac_adapter_test3, "Test ac_adapter state changes." },
	{ NULL, NULL }
};

static fwts_framework_ops ac_adapter_ops = {
	.headline    = ac_adapter_headline,
	.init        = ac_adapter_init,
	.deinit      = ac_adapter_deinit,
	.minor_tests = ac_adapter_tests
};

FWTS_REGISTER(ac_adapter, &ac_adapter_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);

#endif
