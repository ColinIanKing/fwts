/*
 * Copyright (C) 2010-2024 Canonical
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
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#include "brightness-helper.h"

static bool skip_tests;

static int auto_brightness_test1(fwts_framework *fw)
{
	struct dirent *entry;
	int actual_brightness;
	int max_brightness;
	DIR *brightness_dir = brightness_get_dir();

	skip_tests = true;

	rewinddir(brightness_dir);
	do {
		entry = readdir(brightness_dir);
		if (entry == NULL || entry->d_name[0] == '.')
			continue;

		if (brightness_get_setting(entry->d_name, "actual_brightness", &actual_brightness) != FWTS_OK) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessNotFound",
				"Actual brightness could not be accessed for %s.",
				entry->d_name);
			continue;
		}

		if (brightness_get_setting(entry->d_name, "max_brightness", &max_brightness) != FWTS_OK) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessNotExist",
				"Maximum brightness could not be accessed for %s.",
				entry->d_name);
			continue;
		}
		skip_tests = false;

		if (max_brightness <= 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessMaxTest1",
				"Maximum brightness for %s is %d and should be > 0.",
				entry->d_name, max_brightness);
			continue;
		}
		fwts_passed(fw, "Maximum brightness for %s is %d which is sane.", entry->d_name, max_brightness);

		if ((actual_brightness >= 0) &&
		    (actual_brightness <= max_brightness))
			fwts_passed(fw, "Actual brightness for %s is %d which is in range 0..%d.",
				entry->d_name, actual_brightness, max_brightness);
		else
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessOutofRange",
				"Actual brightness for %s not in range 0..%d.",
				entry->d_name, max_brightness);
	} while (entry);

	return FWTS_OK;
}

static int auto_brightness_test2(fwts_framework *fw)
{
	struct dirent *entry;
	int max_brightness;
	int actual_brightness;
	int saved_brightness;
	DIR *brightness_dir = brightness_get_dir();

	rewinddir(brightness_dir);
	do {
		int i;
		int failures = 0;
		bool *brightness_fail;

		entry = readdir(brightness_dir);
		if (entry == NULL || entry->d_name[0] == '.')
			continue;

		if (brightness_get_setting(entry->d_name, "max_brightness", &max_brightness) != FWTS_OK) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessNotExist",
				"Maximum brightness could not be accessed for %s.",
				entry->d_name);
			continue;
		}

		if (max_brightness <= 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessMaxTest2",
				"Maximum brightness for %s is %d and should be > 0.",
				entry->d_name, max_brightness);
			continue;
		}

		if (brightness_get_setting(entry->d_name, "actual_brightness", &saved_brightness) != FWTS_OK) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"BrightnessNotExist",
				"Maximum brightness could not be accessed for %s.",
				entry->d_name);
			continue;
		}

		brightness_fail = calloc(sizeof(bool), max_brightness + 1);
		if (brightness_fail == NULL) {
			fwts_log_error(fw, "Cannot allocate brightness table.");
			continue;
		}

		for (i = 0; i <= max_brightness; i++) {
			brightness_set_setting(entry->d_name, "brightness", i);
			if (brightness_get_setting(entry->d_name, "actual_brightness", &actual_brightness) != FWTS_OK) {
				fwts_log_info(fw, "Cannot get brightness setting %d for backlight %s.", i, entry->d_name);
				failures++;
				brightness_fail[i] = true;
				continue;
			}
			if (actual_brightness != i) {
				failures++;
				brightness_fail[i] = true;
			} else {
				brightness_fail[i] = false;
			}
		}

		if (failures) {
			char *msg = NULL;
			char buf[40];

			/* Find the ranges of the failed levels */
			for (i = 0; i <= max_brightness; i++) {
				int end = i;

				if (brightness_fail[i]) {
					int j;

					/* Scan until we don't find a failure */
					for (j = i; j <= max_brightness && brightness_fail[j]; j++)
						end = j;

					if (i == end) {
						/* Just one failure */
						snprintf(buf, sizeof(buf), " %d", i);
					} else {
						/* A contiguous range of failures */
						snprintf(buf, sizeof(buf), " %d-%d", i, end);
						i = end;
					}
					msg = fwts_realloc_strcat(msg, buf);
				}
			}
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"BrightnessMismatch",
				"%d brightness levels did not match the brightness level "
				"just set for backlight %s.",
				failures, entry->d_name);
			fwts_log_info(fw, "The failed brightness levels were:%s.", msg);
			free(msg);
		} else
			fwts_passed(fw, "Actual brightness matches the brightness level for "
				"all %d levels for backlight %s.", max_brightness, entry->d_name);

		free(brightness_fail);

		/* Restore original setting */
		brightness_set_setting(entry->d_name, "brightness", saved_brightness);
	} while (entry);

	return FWTS_OK;
}

static fwts_framework_minor_test auto_brightness_tests[] = {
	{ auto_brightness_test1, "Test for maximum and actual brightness." },
	{ auto_brightness_test2, "Change actual brightness." },
	{ NULL, NULL }
};

static fwts_framework_ops auto_brightness_ops = {
	.description = "Automated LCD brightness test.",
	.init        = brightness_init,
	.deinit      = brightness_deinit,
	.minor_tests = auto_brightness_tests
};

FWTS_REGISTER("autobrightness", &auto_brightness_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH)

#endif
