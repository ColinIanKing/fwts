/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2012 Canonical
 *
 * This file is was originally from the Linux-ready Firmware Developer Kit
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
#define _GNU_SOURCE	/* for sched_setaffinity */

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#define PROCESSOR_PATH	"/sys/devices/system/cpu"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>
#include <sched.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#define MIN_CSTATE	1
#define MAX_CSTATE	16

typedef struct {
	int counts[MAX_CSTATE];
	bool used[MAX_CSTATE];
	bool present[MAX_CSTATE];
} fwts_cstates;

static int statecount = -1;
static int firstcpu = -1;

static void keep_busy_for_one_second(int cpu)
{
	cpu_set_t mask, oldset;
	time_t current;
	unsigned long loopcount = 0;

	/* First, go to the right cpu */

	sched_getaffinity(0, sizeof(oldset), &oldset);

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
	current = time(NULL);

	do {
		double A, B;
		int i;
		A = 1.234567;
		B = 3.121213;
		for (i=0; i<100; i++) {
			A = A * B;
			B = A * A;
			A = A - B + sqrt(A);
			A = A * B;
			B = A * A;
			A = A - B + sqrt(A);
			A = A * B;
			B = A * A;
			A = A - B + sqrt(A);
			A = A * B;
			B = A * A;
			A = A - B + sqrt(A);
		}
		loopcount++;
	} while (current == time(NULL));

	sched_setaffinity(0, sizeof(oldset), &oldset);
}

static void get_cstates(char *path, fwts_cstates *state)
{
	struct dirent *entry;
	char filename[PATH_MAX];
	char *data;
	DIR *dir;
	int i;

	for (i=MIN_CSTATE; i<MAX_CSTATE; i++) {
		state->counts[i] = 0;
		state->present[i] = false;
		state->used[i] = false;
	}

	if ((dir = opendir(path)) == NULL)
		return;

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			int nr = 0;
			int count;
			int len;

			snprintf(filename, sizeof(filename), "%s/%s/name",
				path, entry->d_name);
			if ((data = fwts_get(filename)) == NULL)
				break;

			/*
			 * Names can be Cx\n, or ATM-Cx\n, or SNB-Cx\n,
			 * where x is the C state number
			 */
			len = strlen(data);
			if ((len > 2) && (data[len-3] == 'C'))
				nr = strtoull(data+len-2, NULL, 10);
			free(data);

			snprintf(filename, sizeof(filename), "%s/%s/usage",
				path, entry->d_name);
			if ((data = fwts_get(filename)) == NULL)
				break;
			count = strtoull(data, NULL, 10);
			free(data);

			if ((nr>=0) && (nr < MAX_CSTATE))
				state->counts[nr] = count;

			state->present[nr] = true;
		}
	}
	closedir(dir);
}

#define TOTAL_WAIT_TIME		20

static void do_cpu(fwts_framework *fw, int nth, int cpus, int cpu, char *path)
{
	fwts_cstates initial, current;
	int	count;
	char	buffer[128];
	char	tmp[8];
	bool	keepgoing = true;
	int	i;

	get_cstates(path, &initial);

	for (i=0; (i < TOTAL_WAIT_TIME) && keepgoing; i++) {
		int j;

		snprintf(buffer, sizeof(buffer),"(CPU %d of %d)", nth+1, cpus);
		fwts_progress_message(fw,
			100 * (i+ (TOTAL_WAIT_TIME*nth))/
				(cpus * TOTAL_WAIT_TIME), buffer);

		if ((i & 7) < 4)
			sleep(1);
		else
			keep_busy_for_one_second(cpu);

		get_cstates(path, &current);

		keepgoing = false;
		for (j=MIN_CSTATE; j<MAX_CSTATE;j++) {
			if (initial.counts[j] != current.counts[j]) {
				initial.counts[j] = current.counts[j];
				initial.used[j] = true;
			}
			if (initial.present[j] && !initial.used[j])
				keepgoing = true;
		}
	}

	*buffer = '\0';
	if (keepgoing) {
		/* Not a failure, but not a pass either! */
		for (i=MIN_CSTATE; i<MAX_CSTATE;i++)  {
			if (initial.present[i] && !initial.used[i]) {
				snprintf(tmp, sizeof(tmp), "C%i ", i);
				strcat(buffer, tmp);
			}
		}
		fwts_log_info(fw, "Processor %i has not reached %s during tests. "
				  "This is not a failure, however it is not a "
				  "complete and thorough test.", cpu, buffer);
	} else {
		for (i=MIN_CSTATE; i<MAX_CSTATE;i++)  {
			if (initial.present[i] && initial.used[i]) {
				snprintf(tmp, sizeof(tmp), "C%i ", i);
				strcat(buffer, tmp);
			}
		}
		fwts_passed(fw, "Processor %i has reached all C-states: %s",
			cpu, buffer);
	}

	count = 0;
	for (i=MIN_CSTATE; i<MAX_CSTATE; i++)
		if (initial.present[i])
			count++;

	if (statecount == -1)
		statecount = count;

	if (statecount != count)
		fwts_failed(fw, LOG_LEVEL_HIGH, "CPUNoCState",
			"Processor %i is expected to have %i C-states but has %i.",
			cpu, statecount, count);
	else
		if (firstcpu == -1)
			firstcpu = cpu;
		else
			fwts_passed(fw, "Processor %i has the same number of C-states as processor %d", cpu, firstcpu);
}

static int cstates_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int cpus;
	int i;

	fwts_log_info(fw,
		"This test checks if all processors have the same number of "
		"C-states, if the C-state counter works and if C-state "
		"transitions happen.");

	if ((dir = opendir(PROCESSOR_PATH)) == NULL) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "CPUNoSysMounted",
			"Cannot open %s: /sys not mounted?", PROCESSOR_PATH);
		return FWTS_ERROR;
	}

	/* How many CPUs are there? */
	for (cpus=0;(entry = readdir(dir)) != NULL;)
		if (entry &&
		    (strlen(entry->d_name)>3) &&
		    (strncmp(entry->d_name, "cpu", 3) == 0) &&
		    (isdigit(entry->d_name[3])))
			cpus++;

	rewinddir(dir);

	for (i=0;(entry = readdir(dir)) != NULL;) {
		if (entry &&
		    (strlen(entry->d_name)>3) &&
		    (strncmp(entry->d_name, "cpu", 3) == 0) &&
		    (isdigit(entry->d_name[3]))) {
			char cpupath[PATH_MAX];

			snprintf(cpupath, sizeof(cpupath), "%s/%s/cpuidle",
				PROCESSOR_PATH, entry->d_name);
			do_cpu(fw, i++, cpus, strtoul(entry->d_name+3, NULL, 10), cpupath);
		}
	}

	closedir(dir);

	return FWTS_OK;
}

static fwts_framework_minor_test cstates_tests[] = {
	{ cstates_test1, "Check all CPUs C-states." },
	{ NULL, NULL }
};

static fwts_framework_ops cstates_ops = {
	.description = "Check processor C state support.",
	.minor_tests = cstates_tests
};

FWTS_REGISTER(cstates, &cstates_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
