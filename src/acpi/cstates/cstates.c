/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2011 Canonical
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

int statecount = -1;
int firstcpu = -1;

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

			snprintf(filename, sizeof(filename), "%s/%s/name", path, entry->d_name);
			if ((data = fwts_get(filename)) == NULL)
				break;

			/* Names can be Cn\n, or ATM-Cn\n, or SNB-Cn\n, where n is the C state number */
			len = strlen(data);
			if ((len > 2) && (data[len-3] == 'C'))
				nr = strtoull(data+len-2, NULL, 10);
			free(data);

			snprintf(filename, sizeof(filename), "%s/%s/usage", path, entry->d_name);
			if ((data = fwts_get(filename)) == NULL)
				break;
			count = strtoull(data, NULL, 10);
			free(data);
			
			if (nr>=0 && nr <MAX_CSTATE)
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
	int  count;
	char buffer[4096];
	char tmp[64];
	int  keepgoing = 1;
	int  first = 1;
	int  i;

	memset(&initial, 0, sizeof(fwts_cstates));
	get_cstates(path, &initial);


	for (i=0; (i < TOTAL_WAIT_TIME) && keepgoing; i++) {
		int j;

		fwts_progress(fw, 100 * (i+ (TOTAL_WAIT_TIME*nth))/(cpus * TOTAL_WAIT_TIME));

		if ((i & 7) < 4)
			sleep(1);
		else
			keep_busy_for_one_second(cpu);

		get_cstates(path, &current);
		for (j=MIN_CSTATE; j<MAX_CSTATE;j++) {
			if (initial.counts[j] != current.counts[j]) {
				initial.counts[j] = current.counts[j];
				initial.used[j] = true;
			}
		}

		keepgoing = 0;
		for (j=MIN_CSTATE; j<MAX_CSTATE; j++)
			if (initial.present[j] && !initial.used[j])
				keepgoing = 1;
	}

	if (keepgoing) {
		/* Not a failure, but not a pass either! */
		snprintf(buffer, sizeof(buffer), "Processor %i has not reached ", cpu);
		for (i=MIN_CSTATE; i<MAX_CSTATE;i++)  {
			if (initial.present[i] && !initial.used[i]) {
				snprintf(tmp, sizeof(tmp), "C%i ", i);
				strcat(buffer, tmp);
			}
		}
		strcat(buffer, "during tests. This is not a failure, but also it is not a complete and thorough test.");
		fwts_log_info(fw, "%s", buffer);
	} else {
		snprintf(buffer, sizeof(buffer), "Processor %i has reached all C-states", cpu);
		for (i=MIN_CSTATE; i<MAX_CSTATE;i++)  {
			if (initial.present[i] && initial.used[i]) {
				snprintf(tmp, sizeof(tmp), "C%i ", i);
				if (first)
					strcat(buffer, ": ");
				first = 0;
				strcat(buffer, tmp);
			}
		}
		fwts_passed(fw, "%s", buffer);
	}

	count = 0;
	for (i=1; i<MAX_CSTATE; i++)
		if (initial.present[i])
			count++;

	if (statecount == -1)
		statecount = count;
	
	if (statecount != count)
		fwts_failed_high(fw, "Processor %i is expected to have %i C-states but has %i.", cpu, statecount, count);
	else
		if (firstcpu == -1)
			firstcpu = cpu;
		else
			fwts_passed(fw, "Processor %i has the same number of C-states as processor %d", cpu, firstcpu);
}

static char *cstates_headline(void)
{
	return "Check processor C state support.";
}

static int cstates_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int cpus = 0;
	int i;

	fwts_log_info(fw, "This test checks if all processors have the same number of C-states, "
			  "if the C-state counter works and if C-state transitions happen.");

	if ((dir = opendir(PROCESSOR_PATH)) == NULL) {
		fwts_failed_high(fw, "Cannot open %s: /sys not mounted?", PROCESSOR_PATH);
		return FWTS_ERROR;
	}

	while ((entry = readdir(dir)) != NULL)
		if (entry && strlen(entry->d_name)>3)
			cpus++;

	rewinddir(dir);

	for (i=0;(entry = readdir(dir)) != NULL;) {
		if (entry &&
		    (strlen(entry->d_name)>3) && 
		    (strncmp(entry->d_name, "cpu", 3) == 0) &&
		    (isdigit(entry->d_name[3]))) {
			char cpupath[PATH_MAX];
			int cpu;
			snprintf(cpupath, sizeof(cpupath), "%s/%s/cpuidle", PROCESSOR_PATH, entry->d_name);
			cpu = strtoul(entry->d_name+3,NULL,10);
			do_cpu(fw, i, cpus, cpu, cpupath);
			i++;
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
	.headline    = cstates_headline,
	.minor_tests = cstates_tests
};

FWTS_REGISTER(cstates, &cstates_ops, FWTS_TEST_ANYTIME,  FWTS_BATCH_EXPERIMENTAL);

#endif
