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

static DIR *ac_power_dir;

#define SYS_INTERFACE 		(0x0)
#define PROC_INTERFACE		(0x1)

#define AC_POWER_ANY		(0x0)
#define AC_POWER_ONLINE		(0x1)
#define AC_POWER_OFFLINE	(0x2)

#define SYS_CLASS_POWER_SUPPLY	"/sys/class/power_supply"
#define PROC_ACPI_AC_ADAPTER	"/proc/acpi/ac_adapter"

typedef struct {
	char *path;	/* Path name of interface */
	char *state;	/* Name of online/offline status */
	char *offline;	/* Contents of state when offline */
	char *online;	/* Contents of state when online */
	char *type;	/* /sys/class type to indicate Mains power, NULL if not used */
} ac_interface_info;

static ac_interface_info ac_interfaces[] = {
	{
		SYS_CLASS_POWER_SUPPLY,
		"online",
		"0",
		"1",
		"Mains"
	},
	{
		PROC_ACPI_AC_ADAPTER,
		"state",
		"off-line",
		"on-line",
		NULL
	}
};

static ac_interface_info *ac_interface;

static int ac_adapter_init(fwts_framework *fw)
{
	/* Try to user newer /sys interface first */
	if ((ac_power_dir = opendir(SYS_CLASS_POWER_SUPPLY))) {
		ac_interface = &ac_interfaces[SYS_INTERFACE];
		return FWTS_OK;
	/* then try older /proc interface  */
	} else if ((ac_power_dir = opendir(PROC_ACPI_AC_ADAPTER))) {
		ac_interface = &ac_interfaces[PROC_INTERFACE];
		return FWTS_OK;
	} else {
		ac_interface = NULL;
		fwts_failed(fw, LOG_LEVEL_LOW, "NoACAdapterEntry",
			"No %s or %s directory available: cannot test.",
			SYS_CLASS_POWER_SUPPLY, PROC_ACPI_AC_ADAPTER);
		return FWTS_ERROR;
	}
}

static int ac_adapter_deinit(fwts_framework *fw)
{
	if (ac_power_dir)
		closedir(ac_power_dir);

	return FWTS_OK;
}

static void ac_adapter_get_state(int state, int *matching, int *not_matching)
{
	struct dirent *entry;

	rewinddir(ac_power_dir);
	do {
		entry = readdir(ac_power_dir);
		if (entry && strlen(entry->d_name) > 2) {
			char path[PATH_MAX];
			char *data;

			/* Check that type field matches the expected type */
			if (ac_interface->type != NULL) {
				snprintf(path, sizeof(path), "%s/%s/type", ac_interface->path, entry->d_name);
				if ((data = fwts_get(path)) != NULL) {
					bool mismatch = (strstr(ac_interface->type, data) != NULL);
					free(data);
					if (mismatch)
						continue;	/* type don't match, skip this entry */
				} else
					continue;		/* can't check type, skip this entry */
			}

			snprintf(path, sizeof(path), "%s/%s/%s", ac_interface->path, entry->d_name, ac_interface->state);
			if ((data = fwts_get(path)) != NULL) {
				char *state_text = "";

				switch (state) {
				case AC_POWER_ANY:
					(*matching)++;
					continue;
				case AC_POWER_ONLINE:
					state_text = ac_interface->online;
					break;
				case AC_POWER_OFFLINE:
					state_text = ac_interface->offline;
					break;
				}
				if (strstr(data, state_text) != NULL)
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

	ac_adapter_get_state(AC_POWER_ANY, &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed(fw, LOG_LEVEL_LOW, "NoACAdapterState",
			"Failed to detect any state in the ac_adapter state info.");
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

	ac_adapter_get_state(AC_POWER_ONLINE, &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed(fw, LOG_LEVEL_HIGH, "NoACAdapterOnLine",
			"Failed to detect an ac_adapter on-line state.");
	else
		fwts_passed(fw, "Detected ac_adapter on-line state.");

	return FWTS_OK;
}

static int ac_adapter_test3(fwts_framework *fw)
{
	int fd;
	size_t len;
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
				ac_adapter_get_state(AC_POWER_OFFLINE, &matching, &not_matching);
				break;
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	if (events == 0)
		fwts_failed(fw, LOG_LEVEL_HIGH, "NoACAdapterEventsOff",
			"Did not detect any ACPI ac-adapter events while waiting for power to be disconnected.");
	else
		if ((matching == 0) || (not_matching > 0))
			fwts_failed(fw, LOG_LEVEL_HIGH, "NoACAdapterOffline",
				"Could not detect ac_adapter off-line state.");
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
				ac_adapter_get_state(AC_POWER_ONLINE, &matching, &not_matching);
				break;
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	if (events == 0)
		fwts_failed(fw, LOG_LEVEL_HIGH, "NoACAdapterEventsOn",
			"Did not detect any ACPI ac-adapter events while waiting for power to be re-connected.");
	else
		if ((matching == 0) || (not_matching > 0))
			fwts_failed(fw, LOG_LEVEL_HIGH, "NoACAdapterOnline",
				"Could not detect ac_adapter on-line state.");
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
	.description = "Interactive ac_adapter power test.",
	.init        = ac_adapter_init,
	.deinit      = ac_adapter_deinit,
	.minor_tests = ac_adapter_tests
};

FWTS_REGISTER(ac_adapter, &ac_adapter_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);

#endif
