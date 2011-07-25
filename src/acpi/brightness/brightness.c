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

static DIR *brightnessdir;

static int skip_tests = 0;

#define BRIGHTNESS_PATH	"/sys/devices/virtual/backlight"

static int brightness_init(fwts_framework *fw)
{
	if (!(brightnessdir = opendir(BRIGHTNESS_PATH))) {
		fwts_failed_low(fw, "No %s directory available: cannot test.", BRIGHTNESS_PATH);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int brightness_deinit(fwts_framework *fw)
{
	if (brightnessdir)
		closedir(brightnessdir);

	return FWTS_OK;
}

int get_setting(char *entry_name, char *setting, int *value)
{
	char path[PATH_MAX];
	FILE *fp;

	snprintf(path, sizeof(path), BRIGHTNESS_PATH "/%s/%s", entry_name, setting);
	if ((fp = fopen(path, "r")) == NULL)
		return -1;

	if (fscanf(fp, "%d", value) != 1) {
		fclose(fp);
		return -2;
	}

	fclose(fp);

	return FWTS_OK;
}

int set_setting(char *entry_name, char *setting, int value)
{
	char path[PATH_MAX];
	FILE *fp;

	snprintf(path, sizeof(path), BRIGHTNESS_PATH "/%s/%s", entry_name, setting);
	if ((fp = fopen(path, "w")) == NULL)
		return -1;

	if (fprintf(fp, "%d", value) < 1) {
		fclose(fp);
		return -2;
	}

	fclose(fp);

	return FWTS_OK;
}

static int brightness_test1(fwts_framework *fw)
{
	struct dirent *entry;
	int actual_brightness;
	int max_brightness;

	skip_tests = 1;

	rewinddir(brightnessdir);
	do {
		entry = readdir(brightnessdir);
		if (entry && strlen(entry->d_name)>2) {
			if (get_setting(entry->d_name, "max_brightness", &max_brightness) == FWTS_OK) {
				if (max_brightness <= 0)
					fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness for %s is %d and should be > 0.", entry->d_name, max_brightness);
				else {
					fwts_passed(fw, "Maximum brightness for %s is %d which is sane.", entry->d_name, max_brightness);
					skip_tests = 0;

					if (get_setting(entry->d_name, "actual_brightness", &actual_brightness) == FWTS_OK)
						if ((actual_brightness >=0) && (actual_brightness <= max_brightness)) {
							fwts_passed(fw, "Actual brightness for %s is %d which is in range 0..%d.", entry->d_name, actual_brightness, max_brightness);
						}
						else
							fwts_failed(fw, LOG_LEVEL_HIGH, "Actual brightness for %s not in range 0..%d.", entry->d_name, max_brightness);
					else
						fwts_failed(fw, LOG_LEVEL_HIGH, "Actual brightness could not be accessed for %s.", entry->d_name);
				}
			} else
				fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness could not be accessed for %s.", entry->d_name);
		}
	} while (entry);

	return FWTS_OK;
}


static int brightness_test2(fwts_framework *fw)
{
	struct dirent *entry;
	int max_brightness;
	int actual_brightness;
	int saved_brightness;

	if (skip_tests)	{
		fwts_skipped(fw, "Test skipped because previous test failed.");
		return FWTS_SKIP;
	}

	rewinddir(brightnessdir);
	do {
		entry = readdir(brightnessdir);
		if (entry && strlen(entry->d_name)>2) {
			if (get_setting(entry->d_name, "max_brightness", &max_brightness) == FWTS_OK) {
				if (max_brightness <= 0) {
					fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness for %s is %d and should be > 0.", entry->d_name, max_brightness);
					continue;
				}
				
				if (get_setting(entry->d_name, "actual_brightness", &saved_brightness) == FWTS_OK) {
					int i;
					int failed = 0;

					for (i=0;i<=max_brightness;i++) {
						set_setting(entry->d_name, "brightness", i);
						get_setting(entry->d_name, "actual_brightness", &actual_brightness);
						if (actual_brightness != i)  {
							fwts_log_info(fw, "Actual brightness %d does not match the brightnesss level %d just set for backlight %s.", actual_brightness, i, entry->d_name);
							failed++;
						}
					}
					if (failed)
						fwts_failed_medium(fw, "Actual brightness %d does not match the brightnesss level %d just set for backlight %s.", actual_brightness, i, entry->d_name);
					else
						fwts_passed(fw, "Actual brightness match the brightnesss level for backlight %s.", entry->d_name);
					/* Restore */
					set_setting(entry->d_name, "brightness", saved_brightness);
				}
			} else
				fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness could not be accessed for %s.", entry->d_name);
		}
	} while (entry);

	return FWTS_OK;
}

static int brightness_test3(fwts_framework *fw)
{
	struct dirent *entry;
	int max_brightness;
	int saved_brightness;

	if (skip_tests)	{
		fwts_skipped(fw, "Test skipped because previous test failed.");
		return FWTS_SKIP;
	}

	rewinddir(brightnessdir);
	do {
		entry = readdir(brightnessdir);
		if (entry && strlen(entry->d_name)>2) {
			if (get_setting(entry->d_name, "max_brightness", &max_brightness) == FWTS_OK) {
				if (max_brightness <= 0) {
					fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness for %s is %d and should be > 0.", entry->d_name, max_brightness);
					continue;
				}
				
				if (get_setting(entry->d_name, "actual_brightness", &saved_brightness) == FWTS_OK) {
					int ch;

					fwts_printf(fw, "==== Setting backlight to a low level ====\n");
					/* Setting it to zero on some machines turns off the backlight, so set to a low value instead */
					set_setting(entry->d_name, "brightness", 1);
					ch = fwts_get_reply(fw, "==== Is the Backlight now set to a dim level? [Y/N]: ", "ynYN");
					if (ch == 'y' || ch == 'Y')
						fwts_passed(fw, "Backlight %s set to dim level.", entry->d_name);
					else
						fwts_failed_medium(fw, "Backlight %s was NOT set to dim level.", entry->d_name);

					fwts_printf(fw, "==== Setting backlight to highest level ====\n");
					set_setting(entry->d_name, "brightness", max_brightness);
					ch = fwts_get_reply(fw, "==== Is the Backlight now set to a bright level? [Y/N]: ", "ynYN");
					if (ch == 'y' || ch == 'Y')
						fwts_passed(fw, "Backlight %s set to bright level.", entry->d_name);
					else
						fwts_failed_medium(fw, "Backlight %s was NOT set to bright level.", entry->d_name);

					/* Restore */
					set_setting(entry->d_name, "brightness", saved_brightness);
				}
			} else
				fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness could not be accessed for %s.", entry->d_name);
		}
	} while (entry);

	return FWTS_OK;
}

static int brightness_test4(fwts_framework *fw)
{
	struct dirent *entry;
	int max_brightness;
	int saved_brightness;

	if (skip_tests)	{
		fwts_skipped(fw, "Test skipped because previous test failed.");
		return FWTS_SKIP;
	}

	rewinddir(brightnessdir);
	do {
		entry = readdir(brightnessdir);
		if (entry && strlen(entry->d_name)>2) {
			if (get_setting(entry->d_name, "max_brightness", &max_brightness) == FWTS_OK) {
				int i;

				if (max_brightness <= 0) {
					fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness for %s is %d and should be > 0.", entry->d_name, max_brightness);
					continue;
				}

				fwts_printf(fw, "==== Backlight will now slowly transition from dim to bright ====\n");
				if (get_setting(entry->d_name, "actual_brightness", &saved_brightness) == FWTS_OK) {
					long delay = 5000000/max_brightness;
					int ch;
		
					if (delay > 1000000)
						delay = 1000000;

					for (i=0;i<=max_brightness;i++) {
						fwts_printf(fw, "Setting to brightness level %d\r", i);
						set_setting(entry->d_name, "brightness", i);
						usleep(delay);
					}

					ch = fwts_get_reply(fw, "==== Did the backlight go from dim to bright? [Y/N]: ", "ynYN");
					if (ch == 'y' || ch == 'Y')
						fwts_passed(fw, "Backlight %s was observed going from dim to bright.", entry->d_name);
					else
						fwts_failed_medium(fw, "Backlight %s was NOT observed going from dim to bright.", entry->d_name);

					/* Restore */
					set_setting(entry->d_name, "brightness", saved_brightness);
				}
			} else
				fwts_failed(fw, LOG_LEVEL_HIGH, "Maximum brightness could not be accessed for %s.", entry->d_name);
		}
	} while (entry);

	return FWTS_OK;
}

static int brightness_wait_event(fwts_framework *fw)
{
	int fd;
	int events = 0;
	char *buffer;
	size_t len;
	int i;

	if ((fd = fwts_acpi_event_open()) < 0) {
		fwts_log_error(fw, "Cannot connect to acpid.");
		return FWTS_ERROR;
	}
	
	for (i=0;i<=20;i++) {
		if ((buffer = fwts_acpi_event_read(fd, &len, 1)) != NULL)
			if (strstr(buffer, "video")) {
				free(buffer);
				events++;
				break;
			}

		fwts_printf(fw, "Waiting %2.2d/20\r", 20-i);
	}

	fwts_acpi_event_close(fd);

	return events;
}

static int brightness_test5(fwts_framework *fw)
{
	struct dirent *entry;
	int saved_brightness;

	rewinddir(brightnessdir);
	do {
		entry = readdir(brightnessdir);
		if (entry && strlen(entry->d_name)>2) {
			if (get_setting(entry->d_name, "actual_brightness", &saved_brightness) != FWTS_OK) {
				fwts_failed(fw, LOG_LEVEL_HIGH, "Actual brightness could not be accessed for %s.", entry->d_name);
			} else {
				int tmp;

				set_setting(entry->d_name, "brightness", 1);
				fwts_printf(fw, "==== Press the brightness UP hotkey for %s ====\n", entry->d_name);

				if (brightness_wait_event(fw) == 0)
					fwts_failed(fw, LOG_LEVEL_HIGH, "Did not detect ACPI hotkey event.");
				else {
					int new_brightness;
					if (get_setting(entry->d_name, "actual_brightness", &new_brightness) != FWTS_OK) {
						fwts_failed(fw, LOG_LEVEL_HIGH, "Actual brightness could not be accessed for %s.", entry->d_name);
					} else {
						if (new_brightness > 0)
							fwts_passed(fw, "Brightness increased on UP hotkey for %s.", entry->d_name);
						else
							fwts_failed(fw, LOG_LEVEL_HIGH, "Did not see brightness increased for %s.", entry->d_name);
					}
				}

				tmp = (saved_brightness > 0) ? saved_brightness : 1;
					
				set_setting(entry->d_name, "brightness", tmp);
				fwts_printf(fw, "==== Press the brightness DOWN hotkey for %s ====\n", entry->d_name);

				if (brightness_wait_event(fw) == 0)
					fwts_failed(fw, LOG_LEVEL_HIGH, "Did not detect ACPI hotkey event.");
				else {
					int new_brightness;
					if (get_setting(entry->d_name, "actual_brightness", &new_brightness) != FWTS_OK) {
						fwts_failed(fw, LOG_LEVEL_HIGH, "Actual brightness could not be accessed for %s.", entry->d_name);
					} else {
						if (new_brightness < tmp)
							fwts_passed(fw, "Brightness decreased on DOWN hotkey for %s.", entry->d_name);
						else
							fwts_failed(fw, LOG_LEVEL_HIGH, "Did not see brightness decrease for %s.", entry->d_name);
					}
				}
			}
		}
	} while (entry);

	return FWTS_OK;
}

static fwts_framework_minor_test brightness_tests[] = {
	{ brightness_test1, "Check for maximum and actual brightness." },
	{ brightness_test2, "Change actual brightness." },
	{ brightness_test3, "Observe all brightness changes." },
	{ brightness_test4, "Observe min, max brightness changes." },
	{ brightness_test5, "Check brightness hotkeys." },
	{ NULL, NULL }
};

static fwts_framework_ops brightness_ops = {
	.description = "Interactive LCD brightness test.",
	.init        = brightness_init,
	.deinit      = brightness_deinit,
	.minor_tests = brightness_tests
};

FWTS_REGISTER(brightness, &brightness_ops, FWTS_TEST_ANYTIME, FWTS_INTERACTIVE);

#endif
