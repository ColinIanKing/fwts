/*
 * Copyright (C) 2010 Canonical
 *
 * This was originally from the thermal_trip.sh script form the  
 * Linux Firmware Test Kit.
 * thermal_trip.sh was Copyright (C) 2006 SUSE Linux Products GmbH 
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

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define FWTS_THERM_ZONE_PATH		"/proc/acpi/thermal_zone"
#define FWTS_PROC_PROCESSOR_PATH	"/proc/acpi/processor"
#define FWTS_SYS_CPU_PATH		"/sys/devices/system/cpu"


static int thermal_trip_init(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int zones = 0;

	if (access(FWTS_THERM_ZONE_PATH, R_OK) != 0) {
		fwts_log_error(fw, "Cannot access %s, test aborted.", FWTS_THERM_ZONE_PATH);
		return FWTS_ERROR;
	}

	if (access(FWTS_PROC_PROCESSOR_PATH, R_OK) != 0) {
		fwts_log_error(fw, "Cannot access %s, test aborted.", FWTS_PROC_PROCESSOR_PATH);
		return FWTS_ERROR;
	}

	if ((dir = opendir(FWTS_THERM_ZONE_PATH)) == NULL) {
		fwts_log_error(fw, "Cannot open directory %s.", FWTS_THERM_ZONE_PATH);
		return FWTS_ERROR;
	}

        while ((entry = readdir(dir)) != NULL) {
                if (entry && strlen(entry->d_name)>3)
			zones++;
        }
        closedir(dir);

	if (zones == 0) {
		fwts_log_warning(fw, "No thermal zones found on this machine.");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static char *thermal_trip_headline(void)
{
	/* Return the name of the test scenario */
	return "Test ACPI passive thermal trip points.";
}

static int thermal_trip_get_temp(char *field, fwts_list *data)
{
	int len = strlen(field);
	fwts_list_link *item;

	if (data == NULL)
		return 0;

	fwts_list_foreach(item, data) {
		char *text = fwts_text_list_text(item);
		if (strncmp(text, field, len) == 0)
			return strtoul(text + len, NULL, 10);
	}
	return 0;
}


static int thermal_trip_get_throttling(fwts_framework *fw, const int warn)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	int value = 0;
	fwts_list *data;

	if ((dir = opendir(FWTS_PROC_PROCESSOR_PATH)) == NULL) {
		fwts_log_error(fw, "Cannot open directory %s.", FWTS_THERM_ZONE_PATH);
		return FWTS_ERROR;
	}
        while ((entry = readdir(dir)) != NULL) {
                if (entry && strlen(entry->d_name)>3) {
			snprintf(path, sizeof(path), "%s/%s/throttling", FWTS_PROC_PROCESSOR_PATH, entry->d_name);
			if ((data = fwts_file_open_and_read(path)) != NULL) {
				fwts_list_link *item;
				fwts_list_foreach(item, data) {
					char *text = fwts_text_list_text(item);
					if (strstr(text, "<not supported>")) {
						if (warn)
							fwts_log_warning(fw, "%s does not support throttling.", entry->d_name);
						break;
					}
					if ((text = strstr(text, "*T")) != NULL) {
						value = strtoul(text + 5, NULL, 10);
						break;
					}
				}
				fwts_list_free(data, free);
			}
		}	
	}

	closedir(dir);

	return value;

}

int thermal_trip_get_cpu_freq(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	int value = 0;
	char *data;

	if ((dir = opendir(FWTS_SYS_CPU_PATH)) == NULL) {
		fwts_log_error(fw, "Cannot open directory %s.", FWTS_THERM_ZONE_PATH);
		return FWTS_ERROR;
	}
        while ((entry = readdir(dir)) != NULL) {
                if (entry && strlen(entry->d_name)>3) {
			if (strncmp(entry->d_name, "cpu", 3) == 0) {
				snprintf(path, sizeof(path), "%s/%s/cpufreq/scaling_cur_freq", FWTS_SYS_CPU_PATH, entry->d_name);
				if ((data = fwts_get(path)) != NULL) {
					value = strtoul(data, NULL, 10);
					free(data);
					break;
				}
			}
		}	
	}

	closedir(dir);

	return value;
}

int thermal_trip_get_polling(fwts_framework *fw, char *zone)
{
	char path[PATH_MAX];
	char *data;
	int value = 0;

	snprintf(path, sizeof(path), "%s/%s/polling_frequency", FWTS_THERM_ZONE_PATH, zone);
	if ((data = fwts_get(path)) != NULL) {
		if (strstr(data, "<polling disabled>") == NULL)
			value = strtoul(data, NULL, 10);
		free(data);
	}

	return value;
}

static void thermal_trip_progress(fwts_framework *fw, int secs, int *sofar, int total)
{
	int i;

	for (i=0;i<secs;i++) {
		sleep(1);
		(*sofar)++;
		fwts_progress(fw, *sofar * 100 / total);
	}
}

static int thermal_trip_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];

	int cur_passive;
	int cur_critical;
	int cur_hot;
	int cur_active;
	int cur_throttling;
	int cur_cpu_freq;
	int cur_polling;
	int new_throttling;
	int new_cpu_freq;
	int sofar = 0;
	
	if ((dir = opendir(FWTS_THERM_ZONE_PATH)) == NULL) {
		fwts_log_error(fw, "Cannot open directory %s.", FWTS_THERM_ZONE_PATH);
		return FWTS_ERROR;
	}

        while ((entry = readdir(dir)) != NULL) {
                if (entry && strlen(entry->d_name)>3) {
			fwts_list *data;

			snprintf(path, sizeof(path), "%s/%s/trip_points", FWTS_THERM_ZONE_PATH, entry->d_name);
			if ((data = fwts_file_open_and_read(path)) == NULL) {
				fwts_log_warning(fw, "Zone %s does not support passive trip points.", entry->d_name);
			} else {
				char tmp[64];
				fwts_progress(fw, 0);

				cur_passive  = thermal_trip_get_temp("passive:", data);
				cur_critical = thermal_trip_get_temp("critical (S5):", data);
				cur_hot      = thermal_trip_get_temp("hot:", data);
				cur_active   = thermal_trip_get_temp("active:", data);
				fwts_list_free(data, free);

				cur_throttling = thermal_trip_get_throttling(fw, 1);

				fwts_cpu_consume_start();
				thermal_trip_progress(fw, 10, &sofar, 25);

				cur_cpu_freq = thermal_trip_get_cpu_freq(fw);
				cur_polling  = thermal_trip_get_polling(fw, entry->d_name);

				snprintf(path, sizeof(path), "%s/%s/polling_frequency", FWTS_THERM_ZONE_PATH, entry->d_name);
				snprintf(tmp, sizeof(tmp), "%d", cur_polling);
				fwts_set("1", path);
				fwts_set("2", path);
				fwts_set(tmp, path);
				
				snprintf(path, sizeof(path), "%s/%s/trip_point", FWTS_THERM_ZONE_PATH, entry->d_name);
				snprintf(tmp, sizeof(tmp), "%d:%d:20:%d",
					cur_critical, cur_hot, cur_active);
				fwts_set(tmp, path);
			
				snprintf(tmp, sizeof(tmp), "%d", cur_polling);
				fwts_set(tmp, path);

				thermal_trip_progress(fw, 10, &sofar, 25);

				new_throttling = thermal_trip_get_throttling(fw, 0);
				new_cpu_freq = thermal_trip_get_cpu_freq(fw);

				fwts_cpu_consume_complete();

				if ((new_throttling > cur_throttling) || (new_cpu_freq < cur_cpu_freq)) {		
					fwts_passed(fw, "Zone %s supports passive trip point.", entry->d_name);
					if (new_cpu_freq != 0) {
						fwts_log_info(fw, "Throttled from T%d to T%d, CPU scaled from %d to %d kHz.", cur_throttling, new_throttling, cur_cpu_freq, new_cpu_freq);	
					} else {
						fwts_log_info(fw, "Throttled from T%d to T%d",cur_throttling, new_throttling);
					}
				} else {
					fwts_failed(fw, "Changing passive trip point seems uneffective for Zone %s.", entry->d_name);
				}

				/* Restore */
				snprintf(path, sizeof(path), "%s/%s/polling_frequency", FWTS_THERM_ZONE_PATH, entry->d_name);
				snprintf(tmp, sizeof(tmp), "%d", cur_polling);
				fwts_set("1", path);
				fwts_set("2", path);
				fwts_set(tmp, path);

				snprintf(path, sizeof(path), "%s/%s/trip_point", FWTS_THERM_ZONE_PATH, entry->d_name);
				snprintf(tmp, sizeof(tmp), "%d:%d:%d:%d",
					cur_critical, cur_hot, cur_passive, cur_active);
				fwts_set(tmp, path);
				thermal_trip_progress(fw, 5, &sofar, 25);
			}
		}
        }
	closedir(dir);

	return FWTS_OK;
}

static fwts_framework_minor_test thermal_trip_tests[] = {
	{ thermal_trip_test1, "Test ACPI passive thermal trip points." },
	{ NULL, NULL }
};

static fwts_framework_ops thermal_trip_ops = {
	.headline    = thermal_trip_headline,
	.init        = thermal_trip_init,
	.minor_tests = thermal_trip_tests
};

FWTS_REGISTER(thermal_trip, &thermal_trip_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
