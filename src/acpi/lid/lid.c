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
#include <limits.h>
#include <dirent.h>

#include "fwts.h"

DIR *liddir;

#define LID_PATH	"/proc/acpi/button/lid"

static int lid_init(fwts_framework *fw)
{
	if (!(liddir = opendir(LID_PATH))) {
		fwts_failed_low(fw, "No %s directory available: cannot test.", LID_PATH);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int lid_deinit(fwts_framework *fw)
{
	if (liddir)
		closedir(liddir);

	return FWTS_OK;
}

static char *lid_headline(void)
{
	return "Interactive lid button test.";
}

void lid_check_field(char *field, char *contents, int *matching, int *not_matching)
{
	struct dirent *entry;

	rewinddir(liddir);
	do {
		entry = readdir(liddir);
		if (entry && strlen(entry->d_name)>2) {
			char path[PATH_MAX];
			char *data;

			snprintf(path, sizeof(path), LID_PATH "/%s/%s", entry->d_name, field);
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

static int lid_test1(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	fwts_log_info(fw, "Test LID buttons are reporting they are really Lid Switches.");

	lid_check_field("info", "Lid Switch", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed_low(fw, "Failed to detect a Lid Switch in LID info field.");
	else
		fwts_passed(fw, "Detected a Lid Switch in LID info field.");

	return FWTS_OK;
}

static int lid_test2(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	fwts_log_info(fw, "Test LID buttons report open correcly.");

	fwts_printf(fw, "==== Make sure laptop lid is open. ====\n");
	fwts_press_enter(fw);

	lid_check_field("state", "open", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0))
		fwts_failed_high(fw, "Detected a closed LID state.");
	else
		fwts_passed(fw, "Detected open LID state.");

	return FWTS_OK;
}

static int lid_test3(fwts_framework *fw)
{
	int fd;
	int len;
	char *buffer;
	int matching;
	int not_matching;
	int events;

	fwts_log_info(fw, "Test LID buttons report close correcly.");

	fwts_printf(fw, "==== Please close laptop lid and then re-open. ====\n");

	if ((fd = acpi_even_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}

	events = 0;
	matching = 0;
	not_matching = 0;

	while ((buffer = acpi_event_read(fd, &len, 20)) != NULL) {
		if (strstr(buffer, "button/lid")) {
			events++;
			lid_check_field("state", "closed", &matching, &not_matching);
			break;
		}
		free(buffer);
	}
	if (events == 0)
		fwts_failed_high(fw, "Did not detect any ACPI LID events while waiting for LID close.");
	else 
		if ((matching == 0) || (not_matching > 0))
			fwts_failed_high(fw, "Could not detect lid closed state.");
		else
			fwts_passed(fw, "Detected lid closed state.");

	events = 0;
	matching = 0;
	not_matching = 0;

	while ((buffer = acpi_event_read(fd, &len, 10)) != NULL) {
		events++;
		if (strstr(buffer, "button/lid")) {
			events++;
			lid_check_field("state", "open", &matching, &not_matching);
			break;
		}
		free(buffer);
	}
	if (events == 0)
		fwts_failed_high(fw, "Did not detect any ACPI LID events while waiting for LID open.");
	else
		if ((matching == 0) || (not_matching > 0))
			fwts_failed_high(fw, "Could not detect lid re-opening state.");
		else
			fwts_passed(fw, "Detected lid re-opening state.");

	acpi_even_close(fd);

	return FWTS_OK;
}

static fwts_framework_tests lid_tests[] = {
	lid_test1,
	lid_test2,
	lid_test3,
	NULL
};

static fwts_framework_ops lid_ops = {
	lid_headline,
	lid_init,
	lid_deinit,
	lid_tests
};

FWTS_REGISTER(lid, &lid_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);
