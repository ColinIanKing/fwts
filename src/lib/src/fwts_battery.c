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

static int fwts_battery_get_capacity_sys_fs(fwts_framework *fw,
	DIR 	*dir,
	int	 type,
	int	 index,
	uint32_t *capacity_mAh,	/* charge */
	uint32_t *capacity_mWh, /* energy */
	int 	*count)
{
	struct dirent *entry;
	char *field_mAh;
	char *field_mWh;
	size_t field_mAh_len;
	size_t field_mWh_len;
	int  i = 0;

	switch (type) {
	case FWTS_BATTERY_DESIGN_CAPACITY:
		field_mAh = "POWER_SUPPLY_CHARGE_FULL_DESIGN=";
		field_mWh = "POWER_SUPPLY_ENERGY_FULL_DESIGN=";
		break;
	case FWTS_BATTERY_REMAINING_CAPACITY:
		field_mAh = "POWER_SUPPLY_CHARGE_NOW=";
		field_mWh = "POWER_SUPPLY_ENERGY_NOW=";
		break;
	default:
		return FWTS_ERROR;
	}

	field_mAh_len = strlen(field_mAh);
	field_mWh_len = strlen(field_mWh);

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			char path[PATH_MAX];
			char *data;
			int  val;
			FILE *fp;
			bool match;

			/* Check that type field matches the expected type */
			snprintf(path, sizeof(path), "%s/%s/type", FWTS_SYS_CLASS_POWER_SUPPLY, entry->d_name);
			if ((data = fwts_get(path)) != NULL) {
				bool mismatch = (strstr(data, "Battery") == NULL);
				free(data);
				if (mismatch)
					continue;	/* type don't match, skip this entry */
			} else
				continue;		/* can't check type, skip this entry */

			match = ((index == FWTS_BATTERY_ALL) || (index == i));
			i++;
			if (!match)
				continue;
	
			snprintf(path, sizeof(path), "%s/%s/uevent", FWTS_SYS_CLASS_POWER_SUPPLY, entry->d_name);
			if ((fp = fopen(path, "r")) == NULL) {
				fwts_log_info(fw, "Battery %s present but undersupported - no state present.", entry->d_name);
			} else {
				char buffer[4096];
				while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
					if (strstr(buffer, field_mAh) &&
					    strlen(buffer) > field_mAh_len) {
						sscanf(buffer+field_mAh_len, "%d", &val);
						*capacity_mAh += (val / 1000);
						(*count)++;
					}
					if (strstr(buffer, field_mWh) &&
					    strlen(buffer) > field_mWh_len) {
						sscanf(buffer+field_mWh_len, "%d", &val);
						*capacity_mWh += (val / 1000);
						(*count)++;
					}
				}
				fclose(fp);
			}
		}
	} while (entry);

	return FWTS_OK;
}

static int fwts_battery_get_capacity_proc_fs(fwts_framework *fw,
	DIR 	*dir,
	int	 type,
	int	 index,
	uint32_t *capacity_mAh,
	uint32_t *capacity_mWh,
	int 	*count)
{
	struct dirent *entry;
	char *file;
	char *field;
	int  i = 0;

	switch (type) {
	case FWTS_BATTERY_DESIGN_CAPACITY:
		file = "info";
		field = "design capacity";
		break;
	case FWTS_BATTERY_REMAINING_CAPACITY:
		file = "state";
		field = "remaining capacity";
		break;
	default:
		return FWTS_ERROR;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			char path[PATH_MAX];
			char units[64];
			int  val;
			FILE *fp;
			bool match = ((index == FWTS_BATTERY_ALL) || (index == i));

			i++;
			if (!match)
				continue;

			snprintf(path, sizeof(path), "%s/%s/%s", FWTS_PROC_ACPI_BATTERY, entry->d_name, file);
			if ((fp = fopen(path, "r")) == NULL) {
				fwts_log_info(fw, "Battery %s present but undersupported - no state present.", entry->d_name);
			} else {
				char buffer[4096];
				while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
					if (strstr(buffer, field) &&
					    strlen(buffer) > 25) {
						sscanf(buffer+25, "%d %s", &val, units);
						if (strncmp(units, "mAh",3) == 0) {
							*capacity_mAh += val;
							(*count)++;
						}
						if (strncmp(units, "mWh",3) == 0) {
							*capacity_mWh += val;
							(*count)++;
						}
						break;
					}
				}
				fclose(fp);
			}
		}
	} while (entry);

	return FWTS_OK;
}

static int fwts_battery_get_count_sys_fs(fwts_framework *fw, DIR *dir, int *count)
{
	struct dirent *entry;
	char path[PATH_MAX];
	char *data;

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			/* Check that type field matches the expected type */
			snprintf(path, sizeof(path), "%s/%s/type", FWTS_SYS_CLASS_POWER_SUPPLY, entry->d_name);
			if ((data = fwts_get(path)) != NULL) {
				if (strstr(data, "Battery") != NULL)
					(*count)++;
				free(data);
			}
		}
	} while (entry);
	return FWTS_OK;
}

static int fwts_battery_get_count_proc_fs(fwts_framework *fw, DIR *dir, int *count)
{
	struct dirent *entry;
	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2)
			(*count)++;
	} while (entry);
	return FWTS_OK;
}

static int fwts_battery_get_name_sys_fs(fwts_framework *fw, DIR *dir, int index, char *name)
{
	struct dirent *entry;
	char path[PATH_MAX];
	char *data;
	int i = 0;

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			bool match;
			/* Check that type field matches the expected type */
			snprintf(path, sizeof(path), "%s/%s/type", FWTS_SYS_CLASS_POWER_SUPPLY, entry->d_name);
			if ((data = fwts_get(path)) != NULL) {
				bool mismatch = (strstr(data, "Battery") == NULL);
				free(data);
				if (mismatch)
					continue;	/* type don't match, skip this entry */
			} else
				continue;		/* can't check type, skip this entry */

			match = ((index == FWTS_BATTERY_ALL) || (index == i));
			i++;
			if (!match)
				continue;
				
			strcpy(name, entry->d_name);
			return FWTS_OK;
		}
	} while (entry);

	return FWTS_ERROR;
}

static int fwts_battery_get_name_proc_fs(fwts_framework *fw, DIR *dir, int index, char *name)
{
	struct dirent *entry;
	int i = 0;

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			bool match = ((index == FWTS_BATTERY_ALL) || (index == i));
			i++;
			if (!match)
				continue;

			strcpy(name, entry->d_name);
			return FWTS_OK;
		}
	} while (entry);

	return FWTS_ERROR;
}

static int fwts_battery_get_cycle_count_sys_fs(fwts_framework *fw, DIR *dir, int index, int *cycle_count)
{
	struct dirent *entry;
	char *field_cycle_count;
	size_t field_cycle_count_len;
	int  i = 0;

	*cycle_count = 0;
	field_cycle_count = "POWER_SUPPLY_CYCLE_COUNT=";
	field_cycle_count_len = strlen(field_cycle_count);

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			char path[PATH_MAX];
			char *data;
			int  val;
			FILE *fp;
			bool match;

			/* Check that type field matches the expected type */
			snprintf(path, sizeof(path), "%s/%s/type", FWTS_SYS_CLASS_POWER_SUPPLY, entry->d_name);
			if ((data = fwts_get(path)) != NULL) {
				bool mismatch = (strstr(data, "Battery") == NULL);
				free(data);
				if (mismatch)
					continue;	/* type don't match, skip this entry */
			} else
				continue;		/* can't check type, skip this entry */
			match = ((index == FWTS_BATTERY_ALL) || (index == i));
			i++;
			if (!match)
				continue;

			snprintf(path, sizeof(path), "%s/%s/uevent", FWTS_SYS_CLASS_POWER_SUPPLY, entry->d_name);
			if ((fp = fopen(path, "r")) == NULL) {
				fwts_log_info(fw, "Battery %s present but undersupported - no state present.", entry->d_name);
			} else {
				char buffer[4096];
				while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
					if (strstr(buffer, field_cycle_count) &&
					    strlen(buffer) > field_cycle_count_len) {
						sscanf(buffer+field_cycle_count_len, "%d", &val);
						*cycle_count = val;
					}
				}
				fclose(fp);
			}
		}
	} while (entry);

	return FWTS_OK;
}

static int fwts_battery_get_cycle_count_proc_fs(fwts_framework *fw, DIR *dir, int index, int *cycle_count)
{
	struct dirent *entry;
	char *file;
	char *field;
	int  i = 0;

	*cycle_count = 0;
	file = "info";
	field = "cycle count";

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name) > 2) {
			char path[PATH_MAX];
			int  val;
			FILE *fp;
			bool match = ((index == FWTS_BATTERY_ALL) || (index == i));

			i++;
			if (!match)
				continue;

			snprintf(path, sizeof(path), "%s/%s/%s", FWTS_PROC_ACPI_BATTERY, entry->d_name, file);
			if ((fp = fopen(path, "r")) == NULL) {
				fwts_log_info(fw, "Battery %s present but undersupported - no state present.", entry->d_name);
			} else {
				char buffer[4096];
				while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
					if (strstr(buffer, field) &&
					    strlen(buffer) > 25) {
						sscanf(buffer+25, "%d", &val);
						*cycle_count = val;
						break;
					}
				}
				fclose(fp);
			}
		}
	} while (entry);
	return FWTS_OK;
}

int fwts_battery_get_cycle_count(fwts_framework *fw, int index, int *cycle_count)
{
	int ret;
	DIR *dir;

	if ((dir = opendir(FWTS_SYS_CLASS_POWER_SUPPLY)) != NULL) {
		ret = fwts_battery_get_cycle_count_sys_fs(fw, dir, index, cycle_count);
		closedir(dir);
	} else if ((dir = opendir(FWTS_PROC_ACPI_BATTERY)) != NULL) {
		ret = fwts_battery_get_cycle_count_proc_fs(fw, dir, index, cycle_count);
		closedir(dir);
	} else {
		return FWTS_ERROR;
	}

	return ret;
}

int fwts_battery_get_name(fwts_framework *fw, int index, char *name)
{
	int ret;
	DIR *dir;

	if ((dir = opendir(FWTS_SYS_CLASS_POWER_SUPPLY)) != NULL) {
		ret = fwts_battery_get_name_sys_fs(fw, dir, index, name);
		closedir(dir);
	} else if ((dir = opendir(FWTS_PROC_ACPI_BATTERY)) != NULL) {
		ret = fwts_battery_get_name_proc_fs(fw, dir, index, name);
		closedir(dir);
	} else {
		return FWTS_ERROR;
	}
	return ret;
}

int fwts_battery_get_count(fwts_framework *fw, int *count)
{
	*count = 0;
	int ret;
	DIR *dir;

	if ((dir = opendir(FWTS_SYS_CLASS_POWER_SUPPLY)) != NULL) {
		ret = fwts_battery_get_count_sys_fs(fw, dir, count);
		closedir(dir);
	} else if ((dir = opendir(FWTS_PROC_ACPI_BATTERY)) != NULL) {
		ret = fwts_battery_get_count_proc_fs(fw, dir, count);
		closedir(dir);
	} else {
		return FWTS_ERROR;
	}
	return ret;
}

int fwts_battery_get_capacity(fwts_framework *fw,
	int type,
	int index,
	uint32_t *capacity_mAh,
	uint32_t *capacity_mWh)
{
	int ret;
	DIR *dir;
	int n = 0;

	*capacity_mAh = 0;
	*capacity_mWh = 0;

	if ((dir = opendir(FWTS_SYS_CLASS_POWER_SUPPLY)) != NULL) {
		ret = fwts_battery_get_capacity_sys_fs(fw, dir, type, index, capacity_mAh, capacity_mWh, &n);
		closedir(dir);
	} else if ((dir = opendir(FWTS_PROC_ACPI_BATTERY)) != NULL) {
		ret = fwts_battery_get_capacity_proc_fs(fw, dir, type, index, capacity_mAh, capacity_mWh, &n);
		closedir(dir);
	} else {
		fwts_log_info(fw, "No battery information present: cannot test.");
		return FWTS_ERROR;
	}

	if ((ret == FWTS_ERROR) || (n == 0)) {
		fwts_log_info(fw, "No valid battery information present: cannot test.");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}
