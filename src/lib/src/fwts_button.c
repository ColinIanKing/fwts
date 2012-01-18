/*
 * Copyright (C) 2011-2012 Canonical
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#define FWTS_PROC_ACPI_BUTTON	"/proc/acpi/button"

static int fwts_button_match_state_proc(fwts_framework *fw,
	int button, int *matched, int *not_matched)
{
	DIR *dir;
	struct dirent *entry;
	char *acpi_button_lid   = FWTS_PROC_ACPI_BUTTON "/lid";
	char *acpi_button_power = FWTS_PROC_ACPI_BUTTON "/power";
	char *button_dir;
	char *field;
	char *match;

	switch (button) {
	case FWTS_BUTTON_LID_ANY:
		button_dir = acpi_button_lid;
		field  = "state";
		match  = "";
		break;
	case FWTS_BUTTON_LID_OPENED:
		button_dir = acpi_button_lid;
		field  = "state";
		match  = "open";
		break;
	case FWTS_BUTTON_LID_CLOSED:
		button_dir = acpi_button_lid;
		field  = "state";
		match  = "close";
		break;
	case FWTS_BUTTON_POWER_EXISTS:
		button_dir = acpi_button_power;
		field  = "info";
		match  = "Power Button";
		break;
	default:
		return FWTS_ERROR;
	}

	if ((dir = opendir(button_dir)) == NULL)
		return FWTS_ERROR;
	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			char path[PATH_MAX];
			char *data;

			snprintf(path, sizeof(path),  "%s/%s/%s", button_dir, entry->d_name, field);
			if ((data = fwts_get(path)) != NULL) {
				if (strstr(data, match))
					(*matched)++;
				else
					(*not_matched)++;
			}
		}
	} while (entry);

	closedir(dir);

	return FWTS_OK;
}

int fwts_button_match_state(fwts_framework *fw, int button, int *matched, int *not_matched)
{
	*matched = 0;
	*not_matched = 0;

	if (access(FWTS_PROC_ACPI_BUTTON, R_OK) == 0)
		return fwts_button_match_state_proc(fw, button, matched, not_matched);

	return FWTS_ERROR;
}
