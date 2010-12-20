/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 *
 * This file is derived from part of the Linux-ready Firmware Developer Kit
 *
 * This was originally from microcode.c from the
 * Linux Firmware Test Kit.
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

/*
 * This test checks if the microcode in the processor has
 * recent enough microcode loaded.
 */
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

typedef struct {
	char cpu[16];
	int version;
} fwts_microcode_info;

static void gather_info(fwts_framework *fw, fwts_list *cpus)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	int warned = 0;

	if ((dir = opendir("/sys/devices/system/cpu")) == NULL)
		return;

	while ((entry = readdir(dir)) != NULL) {
	        if (entry &&
		    (strlen(entry->d_name)>3) &&
		    (strncmp(entry->d_name,"cpu",3) == 0) &&
		    (isdigit(entry->d_name[3]))) {
			char *data;
	        	fwts_microcode_info *cpu;

	        	snprintf(path, sizeof(path), "/sys/devices/system/cpu/%s/microcode/version", entry->d_name);
			if ((data = fwts_get(path)) == NULL) {
				if (!warned) {
					fwts_failed_low(fw,
						"The kernel does not export microcode version. "
						"This test needs a 2.6.19-rc1 kernel or later to function");
					warned++;
				}
			} else {
				if ((cpu = (fwts_microcode_info *)calloc(1, sizeof(fwts_microcode_info))) == NULL) {
					fwts_log_error(fw, "Cannot allocate memory.");
					break;
				
				}
				strncpy(cpu->cpu, entry->d_name, 16);
				cpu->version = strtoul(data, NULL, 16);	
				free(data);
	
				fwts_list_append(cpus, cpu);
			}
		}
	}
	closedir(dir);
}

static void check_info(fwts_framework *fw, fwts_list *cpus)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	fwts_list_link *item;
	int failed = 0;
	int passed = 0;
	char *data;
	int version;

	if ((dir = opendir("/sys/devices/system/cpu")) == NULL)
		return;

	while ((entry = readdir(dir)) != NULL) {
	        if (entry &&
		    (strlen(entry->d_name)>3) &&
		    (strncmp(entry->d_name,"cpu",3) == 0) &&
		    (isdigit(entry->d_name[3]))) {

	        	snprintf(path, sizeof(path), "/sys/devices/system/cpu/%s/microcode/version", entry->d_name);
			if ((data = fwts_get(path)) != NULL) {
				version = strtoul(data, NULL, 16);	
				free(data);
				fwts_list_foreach(item, cpus) {
					fwts_microcode_info *cpu = fwts_list_data(fwts_microcode_info*, item);
					if (strcmp(entry->d_name, cpu->cpu) == 0) {
						if (version == cpu->version)
							passed++;
						else {
							failed++;
							fwts_failed_low(fw,
								"Cpu %s has outdated microcode (version %x while version %x is available)",
								cpu->cpu, cpu->version, version);
						}
					}
				}
			}
		}
	}
	if (!failed)
		fwts_passed(fw, "%d CPU(s) have the latest microcode loaded.", passed);

	closedir(dir);
}

static char *microcode_headline(void)
{
	return "Check if system is using latest microcode.";
}

static int microcode_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw) != FWTS_OK)
		return FWTS_ERROR;

	if (access(FWTS_MICROCODE_DEVICE, W_OK) != 0) {
		fwts_log_error(fw, "Cannot get write access to %s.", FWTS_MICROCODE_DEVICE);
		return FWTS_ERROR;
	}

	if (access(FWTS_MICROCODE_FILE, R_OK) != 0) {
		fwts_log_error(fw, "Cannot read microcode file %s.", FWTS_MICROCODE_FILE);
		return FWTS_ERROR;
	}

	if (fwts_check_executable(fw, "/sbin/modprobe", "modprobe") != FWTS_OK)
		return FWTS_ERROR;

	return FWTS_OK;
}

static int microcode_test1(fwts_framework *fw)
{
	fwts_list *cpus;
	pid_t pid;
	int fd;

	fwts_log_info(fw,
		"This test verifies if the firmware has put a recent version "
		"of the microcode into the processor at boot time. Recent "
		"microcode is important to have all the required "
		"features and errata updates for the processor.");

	if ((cpus = fwts_list_init()) == NULL) {
		fwts_log_error(fw, "Cannot allocate memory.");
		return FWTS_ERROR;
	}

	if ((fd = fwts_pipe_open("/sbin/modprobe microcode", &pid)) < 0) {
		fwts_log_error(fw, "Cannot modprobe microcode module,");
		return FWTS_ERROR;
	}
	fwts_pipe_close(fd, pid);
	
	gather_info(fw, cpus);

	/* now run the microcode update */

	if (fwts_update_microcode(fw, FWTS_MICROCODE_DEVICE, FWTS_MICROCODE_FILE)  != FWTS_OK) {
		fwts_log_error(fw, "Failed to upload latest microcode.");
	} else {
		/* and check for lacking updates */
		check_info(fw, cpus);

		fwts_list_free(cpus, free);
	}

	return FWTS_OK;
}

static fwts_framework_minor_test microcode_tests[] = {
	{ microcode_test1, "Check for most recent microcode being loaded." },
	{ NULL, NULL }
};

static fwts_framework_ops microcode_ops = {
	.headline    = microcode_headline,
	.init        = microcode_init,
	.minor_tests = microcode_tests
};

FWTS_REGISTER(microcode, &microcode_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
