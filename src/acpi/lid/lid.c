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

DIR *liddir;

#define LID_PATH	"/proc/acpi/button/lid"

static int lid_init(fwts_framework *fw)
{
	if (!(liddir = opendir(LID_PATH))) {
		fwts_failed(fw, LOG_LEVEL_LOW, "No %s directory available: cannot test.", LID_PATH);
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

void lid_check_field_poll(char *field, char *contents, int *matching, int *not_matching)
{
	int i;

	int tmp_matching = 0;
	int tmp_not_matching = 0;

	for (i=0;i<100;i++) {
		lid_check_field(field, contents, &tmp_matching, &tmp_not_matching);
		usleep(10);
	
		if (tmp_matching != 0)
			break;
	}
	*matching = tmp_matching;
	*not_matching = tmp_not_matching;
}

static int lid_test1(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	lid_check_field_poll("info", "Lid Switch", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0)) {
		fwts_failed(fw, LOG_LEVEL_LOW, "Failed to detect a Lid Switch in LID info field.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_LID);
	} else
		fwts_passed(fw, "Detected a Lid Switch in LID info field.");

	return FWTS_OK;
}

static int lid_test2(fwts_framework *fw)
{
	int matching = 0;
	int not_matching = 0;

	fwts_printf(fw, "==== Make sure laptop lid is open. ====\n");
	fwts_press_enter(fw);

	lid_check_field_poll("state", "open", &matching, &not_matching);

	if ((matching == 0) || (not_matching > 0)) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "Detected a closed LID state in %s.", LID_PATH);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_LID);
	} else
		fwts_passed(fw, "Detected open LID state in %s.", LID_PATH);

	return FWTS_OK;
}

static int lid_test_state(fwts_framework *fw, char *state)
{
	int gpe_count = 0;
	int fd;
	int matching = 0;
	int not_matching = 0;
	int events = 0;
	size_t len;
	char *buffer;
	int i;

	fwts_gpe *gpes_start;
	fwts_gpe *gpes_end;

	if ((gpe_count = fwts_gpe_read(&gpes_start)) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot read GPEs.");
		return FWTS_ERROR;
	}

	if ((fd = fwts_acpi_event_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}

	for (i=0;i<=20;i++) {
		if ((buffer = fwts_acpi_event_read(fd, &len, 1)) != NULL) {
			if (strstr(buffer, "button/lid")) {
				events++;
				lid_check_field_poll("state", state, &matching, &not_matching);
				break;
			}
			free(buffer);
		}
		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}
	fwts_acpi_event_close(fd);

	if ((gpe_count = fwts_gpe_read(&gpes_end)) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot read GPEs.");
	}

	fwts_gpe_test(fw, gpes_start, gpes_end, gpe_count);
	fwts_gpe_free(gpes_start, gpe_count);
	fwts_gpe_free(gpes_end, gpe_count);

	if (events == 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "Did not detect any ACPI LID events while waiting for to LID %s.", state);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_LID);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_EVENT);
	} else
		fwts_passed(fw, "Detected ACPI LID events while waiting for LID to %s.", state);
		if ((matching == 0) || (not_matching > 0)) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "Could not detect lid %s state in %s.", state, LID_PATH);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_LID);
		} else
			fwts_passed(fw, "Detected lid %s state.", state);

	return FWTS_OK;
}


static int lid_test3(fwts_framework *fw)
{
	int ret;

	fwts_printf(fw, "==== Please close laptop lid for 2 seconds and then re-open. ====\n");

	if ((ret = lid_test_state(fw, "close")) != FWTS_OK)
		return ret;
	if ((ret = lid_test_state(fw, "open")) != FWTS_OK)
		return ret;

	return FWTS_OK;
}

static int lid_test4(fwts_framework *fw)
{
	int ret;
	int i;
	fwts_log_info(fw, "Some machines may have EC or ACPI faults that cause detection of multiple open/close events to fail.");

	for (i=1;i<=3;i++) {
		fwts_printf(fw, "==== %d of %d: Please close laptop lid for 2 seconds and then re-open. ====\n", i,3);

		if ((ret = lid_test_state(fw, "close")) != FWTS_OK)
			return ret;
		if ((ret = lid_test_state(fw, "open")) != FWTS_OK)
			return ret;
	}

	return FWTS_OK;
}

static fwts_framework_minor_test lid_tests[] = {
	{ lid_test1, "Test LID buttons are reporting they are really Lid Switches." },
	{ lid_test2, "Test LID buttons report open correctly." },
	{ lid_test3, "Test LID buttons on a single open/close." },
	{ lid_test4, "Test LID buttons on multiple open/close events." },
	{ NULL, NULL }
};

static fwts_framework_ops lid_ops = {
	.description = "Interactive lid button test.",
	.init        = lid_init,
	.deinit      = lid_deinit,
	.minor_tests = lid_tests
};

FWTS_REGISTER(lid, &lid_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);

#endif
