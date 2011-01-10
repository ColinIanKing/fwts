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

#define PROCESSOR_PATH	"/proc/acpi/processor"

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

#define MIN_CSTATE	1
#define MAX_CSTATE	16

typedef struct {
	int counts[MAX_CSTATE];
	int used[MAX_CSTATE];
	int warned[MAX_CSTATE];
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

static void get_cstates(char *dir, fwts_cstates *state)
{
	char path[PATH_MAX];
	char line[4096];
	FILE *file;
	int i;
	memset(state, 0, sizeof(fwts_cstates));

	for (i=0; i<MAX_CSTATE; i++)
		state->used[i] = -1;

	snprintf(path, sizeof(path), "%s/power", dir);
	if ((file = fopen(path, "r")) == NULL)
		return;

	while (fgets(line, 4095, file) != NULL) {
		char *str;
		int nr=0, count=0, active=0;

		str = strchr(line, 'C');
		if (!str)
			continue;
		nr = strtoull(str+1, NULL, 10);
		str = strstr(line, "usage[");
		if (!str)
			continue;
		count = strtoull(str+6, NULL, 10);
		str = strchr(line, '*');
		if (str)
			active=1;
		if (nr>=0 && nr <MAX_CSTATE) {
			state->counts[nr] = count;
			state->used[nr] = active;
		}
	}

	fclose(file);
}

#define TOTAL_WAIT_TIME		20

static void do_cpu(fwts_framework *fw, int nth, int cpus, int cpu, char *dir)
{
	fwts_cstates initial, current;
	int  count;
	char buffer[4096];
	char tmp[64];
	int  keepgoing = 1;
	int  first = 1;
	int  i;
	int  warned = 0;

	memset(&initial, 0, sizeof(fwts_cstates));
	get_cstates(dir, &initial);

	for (i=0; (i < TOTAL_WAIT_TIME) && keepgoing; i++) {
		int j;

		fwts_progress(fw, 100 * (i+ (TOTAL_WAIT_TIME*nth))/(cpus * TOTAL_WAIT_TIME));

		if (i < 4)
			sleep(1);
		else
			keep_busy_for_one_second(cpu);

		get_cstates(dir, &current);
		for (j=MIN_CSTATE; j<MAX_CSTATE;j++) {
			if (current.used[j]>0){
				if (initial.counts[j] == current.counts[j]) {
					initial.warned[j]++;
					warned++;
				}
				initial.counts[j] = current.counts[j];
				initial.used[j]++;
			}
		}

		keepgoing = 0;
		for (j=MIN_CSTATE; j<MAX_CSTATE; j++)
			if (initial.used[j]==0)
				keepgoing = 1;
	}

	if (warned) {
		snprintf(buffer, sizeof(buffer),
			"Processor %i doesn't increment C-state count in all C-states, failed in ", cpu);
		for (i=MIN_CSTATE; i<MAX_CSTATE; i++) {
			if ((initial.used[i]>0) && (initial.warned[i] != 0))  {
				snprintf(tmp, sizeof(tmp), "C%i ",i);
				strcat(buffer, tmp);
			}
		}
		fwts_failed_medium(fw, "%s", buffer);
	}
#if 0
	else {
		snprintf(buffer, sizeof(buffer), "Processor %i incremented C-states ", cpu);
		for (i=0; i<MAX_CSTATE; i++) {
printf("%d %d %d\n",i, initial.used[i],initial.warned[i]);
			if ((initial.used[i]>0) && (initial.warned[i] == 0))  {
				snprintf(tmp, sizeof(tmp), "C%i ",i);
				strcat(buffer, tmp);
			}
		}
		fwts_passed(fw, "%s", buffer);
	}
#endif

	if (keepgoing) {
		/* Not a failure, but not a pass either! */
		snprintf(buffer, sizeof(buffer), "Processor %i has not reached ", cpu);
		for (i=MIN_CSTATE; i<MAX_CSTATE;i++)  {
			snprintf(tmp, sizeof(tmp), "C%i ", i);
			if (initial.used[i] == 0)
				strcat(buffer, tmp);
		}
		strcat(buffer, "during tests. This is not a failure, but also it is not a complete and thorough test.");
		fwts_log_info(fw, "%s", buffer);
	} else {
		snprintf(buffer, sizeof(buffer), "Processor %i has reached all C-states", cpu);
		for (i=MIN_CSTATE; i<MAX_CSTATE;i++)  {
			snprintf(tmp, sizeof(tmp), "C%i ", i);
			if (initial.used[i]==0) {
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
		if (initial.used[i] >= 0)
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
		fwts_failed_high(fw, "Cannot open %s: /proc not mounted?", PROCESSOR_PATH);
		return FWTS_ERROR;
	}

	while ((entry = readdir(dir)) != NULL)
		if (entry && strlen(entry->d_name)>3)
			cpus++;

	rewinddir(dir);

	for (i=0;(entry = readdir(dir)) != NULL;) {
		if (entry && strlen(entry->d_name)>3) {
			char cpupath[PATH_MAX];
			int cpu;
			snprintf(cpupath, sizeof(cpupath), "%s/%s", PROCESSOR_PATH, entry->d_name);
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
