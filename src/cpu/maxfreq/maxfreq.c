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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#define CPU_FREQ_PATH	"/sys/devices/system/cpu"
#define CPU_INFO_PATH	"/proc/cpuinfo"

static int maxfreq_init(fwts_framework *fw)
{
	return FWTS_OK;
}

static int maxfreq_deinit(fwts_framework *fw)
{
	return FWTS_OK;
}

static char *maxfreq_headline(void)
{
	/* Return the name of the test scenario */
	return "Check max CPU frequencies against max scaling frequency.";
}

typedef struct {
	int cpu;
	int speed;
} fwts_cpuinfo;

static int maxfreq_max(const char *str)
{
	int max = -1;
	int val;

	while (str && *str) {
		while ((*str != '\0') && isspace(*str))
			str++;

		if (!isdigit(*str))
			break;
		
		val = atoi(str);
		if (val > max)
			max = val;
		str = strstr(str, " ");
	}
	return max;
}

static int maxfreq_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int cpus;
	int cpufreqs_read = 0;
	int cpu;
	fwts_list_link *item;
	fwts_list *cpuinfo;
	int *cpufreq;
	int failed = 0;
	int advice = 0;

	fwts_log_info(fw, 
		"This test checks the maximum CPU frequency as detected by the kernel for "
		"each CPU against maxiumum frequency as specified by the BIOS frequency "
		"scaling settings.");

	cpuinfo = fwts_file_open_and_read(CPU_INFO_PATH);
	if (cpuinfo == NULL) {
		fwts_log_error(fw, "Cannot create cpu info list.");
		return FWTS_ERROR;
	}

	/* Pass 1, figure out number of CPUs */
	for (cpus=0, item = cpuinfo->head; item != NULL; item = item->next) {
		if (strstr(fwts_text_list_text(item), "model name"))
			cpus++;
	}

	if ((cpufreq = calloc(1, sizeof(int)*cpus)) == NULL) {
		fwts_log_error(fw, "Cannot create cpu frequency array.");
		return FWTS_ERROR;
	}

	/* Pass 2, get speed */
	for (cpu=0, item = cpuinfo->head; item != NULL; item = item->next) {
		char *str = fwts_text_list_text(item);
		if (strstr(str, "model name")) {
			if ((str = strstr(str, "@")) != NULL) {
				double freq = atof(str+1);
				if (strstr(str, "GHz"))
					freq *= 1000000.0;
				if (strstr(str, "MHz"))
					freq *= 1000.0;
				cpufreq[cpu] = (int)freq;
				cpufreqs_read++;
			}
			else
				cpufreq[cpu] = -1;

			cpu++;
		}
	}
	fwts_list_free(cpuinfo, free);

	if (cpufreqs_read == 0) {
		fwts_log_info(fw, "Cannot read CPU frequencies from %s, this generally happens on AMD CPUs, skipping test.", CPU_INFO_PATH);
		return FWTS_SKIP;
	}
		
	if (!(dir = opendir(CPU_FREQ_PATH))) {
		fwts_failed_low(fw, "No %s directory available: cannot test.", CPU_FREQ_PATH);
		return FWTS_ERROR;
	}

	do {
		entry = readdir(dir);
		if (entry && strlen(entry->d_name)>2) {
			char path[PATH_MAX];
			char *data;

			snprintf(path, sizeof(path), CPU_FREQ_PATH "/%s/cpufreq/scaling_available_frequencies", entry->d_name);
			if ((data = fwts_get(path)) != NULL) {
				int maxfreq = maxfreq_max(data);
				int cpunum = atoi(entry->d_name + 3);
				if (maxfreq == -1) {
					fwts_failed(fw, "Cannot read cpu frequency from %s for CPU %s\n", CPU_FREQ_PATH, entry->d_name);
					failed++;
				} else {
					if (cpufreq[cpunum] != maxfreq) {
						failed++;
						fwts_failed(fw, "Maximum scaling frequency %f GHz do not match expected frequency %f GHz\n", 
								(double)maxfreq/1000000.0, (double)cpufreq[cpunum]/1000000.0);	
						if (!advice)  {
							advice++;
							fwts_advice(fw, 
								"The maximum scaling frequency %f GHz for CPU %d configured by the BIOS in %s "
								"does not match the expected maximum CPU frequency %f GHz "
								"that the CPU can run at. This usually indicates a misconfiguration of "
								"the ACPI _PSS (Performance Supported States) object. This is described in "
								"section 8.4.4.2 of the APCI specification.",
								(double)maxfreq/1000000.0, cpunum, path, 
								(double)cpufreq[cpunum]/1000000.0);	
						}
						else 
							fwts_advice(fw, "See advice for previous CPU.");
					}
				}
			} 
			free(data);
		}
	} while (entry);

	if (!failed)
		fwts_passed(fw, "%d CPUs passed the maximum frequency check.", cpus);

	free(cpufreq);
	closedir(dir);

	return FWTS_OK;
}


/*
 *  Null terminated array of tests to run, in this
 *  scenario, we just have one test.
 */
static fwts_framework_tests maxfreq_tests[] = {
	maxfreq_test1,
	NULL
};

static fwts_framework_ops maxfreq_ops = {
	maxfreq_headline,
	maxfreq_init,	/* Can be NULL if not required */
	maxfreq_deinit,	/* Can be NULL if not required */
	maxfreq_tests
};

/*
 *   See fwts_framework.h for flags,
 */
FWTS_REGISTER(maxfreq, &maxfreq_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
