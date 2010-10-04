/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 *
 * This file was originally part of the Linux-ready Firmware Developer Kit
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
#define _GNU_SOURCE

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <stdint.h>
#include <sched.h>
#include <time.h>
#include <math.h>

#define FWTS_CPU_PATH	"/sys/devices/system/cpu"

typedef struct {
	unsigned long	Hz;
	unsigned long	speed;
} fwts_cpu_freq;

static int nrspeeds = -1;
static int totaltests = 1;
static int performedtests = 0;
static int no_cpufreq = 0;
static unsigned long topspeed=1;

#define GET_PERFORMANCE_MAX (0)
#define GET_PERFORMANCE_MIN (1)
#define GET_PERFORMANCE_AVG (2)

static void cpu_mkpath(char *path, const int len, const int cpu, const char *name)
{
	snprintf(path, len, "%s/cpu%i/cpufreq/%s", FWTS_CPU_PATH, cpu, name);
}

static int count_ints(char *text)
{
	char *str = text;
	int count = 0;

	while (str && strlen(str) > 0) {
		char *str2 = strchr(str,' ');
		if (!str2)
			break;
		str = str2 + 1;
		count++;
	}

	return count;
}

static void set_governor(fwts_framework *fw, const int cpu)
{
	char path[PATH_MAX];

	cpu_mkpath(path, sizeof(path), cpu, "scaling_governor");

	if (fwts_set("userspace", path) != FWTS_OK) {
		if (!no_cpufreq) {
			fwts_log_warning(fw,
				"Frequency scaling not supported.");
			no_cpufreq = 1;
		}
	}
}

static int cpu_exists(int cpu)
{
	char path[PATH_MAX];

	cpu_mkpath(path, sizeof(path), cpu, "scaling_governor");
	return !access(path, R_OK);
}

static void  set_HZ(fwts_framework *fw, const int cpu, const unsigned long Hz)
{
	cpu_set_t mask, oldset;
	char path[PATH_MAX];
	char buffer[64];

	/* First, go to the right cpu */

	sched_getaffinity(0, sizeof(oldset), &oldset);

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
	
	set_governor(fw, cpu);	

	/* then set the speed */
	cpu_mkpath(path, sizeof(path), cpu, "scaling_setspeed");
	snprintf(buffer, sizeof(buffer), "%lu", Hz);
	fwts_set(buffer, path);

	sched_setaffinity(0, sizeof(oldset), &oldset);

}

static unsigned long  get_performance(const int cpu)
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
	while (current == time(NULL))
		sched_yield();
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

	return loopcount;
}


static unsigned long  get_performance_repeat(fwts_framework *fw, int cpu, unsigned long Hz, int count, int type)
{
	int i;

	unsigned long max = 0, min = ULONG_MAX, real_count = 0;
	unsigned long long cumulative = 0;
	unsigned long retval;

	set_HZ(fw, cpu, Hz);
	for (i = 0; i < count; i++) {
		unsigned long temp;
		temp = get_performance(cpu);
		if (temp) {
			if (temp < min)
				min = temp;

			if (temp > max)
				max = temp;

			cumulative += temp;
			real_count++;
		}
	}

	switch (type) {
	    case GET_PERFORMANCE_MAX:
		retval = max;
		break;
	    case GET_PERFORMANCE_MIN:
		retval = min;
		break;
	    case GET_PERFORMANCE_AVG:
		retval = cumulative/real_count;
		break;
	    default:
		retval = 0;
		break;
	}
	return retval;
}

static char *HzToHuman(unsigned long hz)
{	
	static char buffer[1024];
	memset(buffer, 0, 1024);
	unsigned long long Hz;

	Hz = hz;

	/* default: just put the Number in */
	snprintf(buffer, sizeof(buffer), "%9lli", Hz);

	if (Hz>1000)
		snprintf(buffer, sizeof(buffer), "%6lli Mhz", (Hz+500)/1000);

	if (Hz>1500000)
		snprintf(buffer, sizeof(buffer), "%6.2f Ghz", (Hz+50000.0)/1000000);


	return buffer;
}



static unsigned long get_claimed_hz(const int cpu)
{
	char path[PATH_MAX];
	char *buffer;
	unsigned long value = 0;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_max_freq");
	
	if ((buffer = fwts_get(path)) != NULL) {
		value = strtoul(buffer, NULL, 10);
		free(buffer);
	}
	return value;
}

static void do_cpu(fwts_framework *fw, int cpu)
{
	char path[PATH_MAX];
	char line[4096];
	fwts_cpu_freq freqs[32];
	FILE *file;
	char *c, *c2;
	int i, delta; 
	int speedcount;
	static int warned=0;
	int warned_PSS = 0;

	memset(freqs, 0, sizeof(freqs));
	memset(line, 0, 4096);

	set_governor(fw, cpu);

	cpu_mkpath(path, sizeof(path), cpu, "scaling_available_frequencies");
	if ((file = fopen(path, "r")) == NULL) {
		if (!no_cpufreq) {
			fwts_log_warning(fw, "Frequency scaling not supported.");
			no_cpufreq = 1;
		}
		return;
	}
	if (fgets(line, 4095, file)==NULL)
		return;
	fclose(file);

	if (totaltests==1) 
		totaltests = (2+count_ints(line)) * sysconf(_SC_NPROCESSORS_CONF) + 2;
	
	c = line;
	i = 0;
	while (c && strlen(c)>1) {
		c2 = strchr(c, ' ');
		if (c2) {
			*c2 = 0;
			c2++;
		} else
			c2 = NULL;

		freqs[i].Hz = strtoull(c, NULL, 10);
		set_HZ(fw, cpu, freqs[i].Hz);
		freqs[i].speed = get_performance(cpu);
		if (freqs[i].speed > topspeed)
			topspeed = freqs[i].speed;

		performedtests++;
		fwts_progress(fw, (75 * performedtests)/totaltests);

		i++;	
		c = c2;
	}
	speedcount = i;

	fwts_log_info_verbatum(fw, " Frequency | Speed \n-----------+---------\n");
	for (i=0; i < speedcount; i++) 
		fwts_log_info_verbatum(fw, "%9s | %5.1f %%\n", HzToHuman(freqs[i].Hz), 100.0*freqs[i].speed/topspeed);
	
	if (nrspeeds == -1)  {
		fwts_log_info(fw, "%i CPU frequency steps supported", speedcount);
		nrspeeds = speedcount;
	}

	if (nrspeeds != speedcount)
		fwts_failed_medium(fw, "Not all processors support the same number of P states.");

	if (speedcount<2)
		return;

	/* Now.. sort the frequencies */

	delta=1;
	while (delta) {
		fwts_cpu_freq tmp;
		delta = 0;
		for (i=0; i<speedcount-1; i++) {
			if (freqs[i].Hz > freqs[i+1].Hz) {
				tmp = freqs[i];
				freqs[i] = freqs[i+1];
				freqs[i+1] = tmp;
				delta = 1;
			}
		}
	}

	/* now check for 1) increasing HZ and 2) increasing speed */
	for (i=0; i<speedcount-1; i++) { 
		if (freqs[i].Hz == freqs[i+1].Hz && !warned++)
			fwts_failed_medium(fw, "Duplicate frequency reported.");
		if (freqs[i].speed > freqs[i+1].speed)
			fwts_failed_medium(fw, "Supposedly higher frequency is slower on CPU %i!", cpu);
		if (freqs[i].Hz > get_claimed_hz(cpu) && !warned_PSS) {
			warned_PSS = 1;
			fwts_warning(fw, "Frequency %lu not achievable; _PSS limit of %lu in effect?", freqs[i].Hz, get_claimed_hz(cpu));
		}
	}
}


static void lowest_speed(fwts_framework *fw, const int cpu)
{
	char path[PATH_MAX];
	char *line;
	unsigned long Hz;
	char *c, *c2;
	int i; 
	unsigned long lowspeed=0;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_available_frequencies");
	if ((line = fwts_get(path)) == NULL)
		return;

	c = line;
	i = 0;
	while (c && strlen(c)>1) {
		c2 = strchr(c, ' ');
		if (c2) {
			*c2 = 0;
			c2++;
		} else 
			c2 = NULL;

		Hz = strtoull(c, NULL, 10);
		if (Hz < lowspeed || lowspeed==0)
			lowspeed = Hz;
		c = c2;
	}
	free(line);

	set_HZ(fw, cpu, lowspeed);
}

static void highest_speed(fwts_framework *fw, const int cpu)
{
	char path[PATH_MAX];
	char *line;
	uint64_t Hz;
	char *c, *c2;
	int i; 
	unsigned long highspeed=0;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_available_frequencies");
	if ((line = fwts_get(path)) == NULL)
		return;

	c = line;
	i = 0;
	while (c && strlen(c)>1) {
		c2 = strchr(c, ' ');
		if (c2) {
			*c2=0;
			c2++;
		} else
			c2 = NULL;

		Hz = strtoull(c, NULL, 10);
		if (Hz > highspeed || highspeed==0)
			highspeed = Hz;
		c = c2;
	}
	free(line);

	set_HZ(fw, cpu, highspeed);
}


/*
  4) Is BIOS wrongly doing Sw_All P-state coordination across cpus
  - Change frequency on all CPU to the lowest value
  - Change frequency on one particular CPU to the highest
  - If BIOS is doing Sw_All, the last high freq request will not work
 */
static void do_sw_all_test(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	unsigned long highperf, lowperf;
	int first_cpu_index = -1;
	int cpu;

	if ((dir = opendir(FWTS_CPU_PATH)) == NULL) {
		fwts_log_error(fw, "FATAL: cpufreq: sysfs not mounted.");
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3, NULL, 10);
			if (first_cpu_index == -1)
				first_cpu_index = cpu;

			lowest_speed(fw, cpu);
		}
	}
	closedir(dir);

	/* All CPUs at the lowest frequency */
	lowperf = 100 * get_performance_repeat(fw, first_cpu_index,
	                                       0,
	                                       5,
	                                       GET_PERFORMANCE_MIN) / topspeed;
	highest_speed(fw, first_cpu_index);
	highperf = 100 * get_performance_repeat(fw, first_cpu_index,
	                                        0,
	                                        5,
	                                        GET_PERFORMANCE_MAX) / topspeed;

	if (lowperf >= highperf)
		fwts_failed(fw, 
			"Firmware not implementing hardware "
			"coordination cleanly. Firmware using SW_ALL "
			"instead?");
}


/*
  5) Is BIOS wrongly doing Sw_Any P-state coordination across cpus
  - Change frequency on all CPU to the lowest value
  - Change frequency on one particular CPU to the highest
  - Change frequency on all CPU to the lowest value
  - If BIOS is doing Sw_Any, the high freq request will not work
 */
static void do_sw_any_test(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	unsigned long highperf, lowperf;
	int first_cpu_index = -1;
	int cpu;

	if ((dir = opendir(FWTS_CPU_PATH)) == NULL) {
		fwts_log_error(fw, "FATAL: cpufreq: sysfs not mounted.");
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3, NULL, 10);
			if (first_cpu_index == -1)
				first_cpu_index = cpu;

			lowest_speed(fw, cpu);
		}
	}
	rewinddir(dir);

	/* All CPUs at the lowest frequency */
	lowperf = 100 * get_performance_repeat(fw, first_cpu_index,
	                                       0,
	                                       5,
	                                       GET_PERFORMANCE_MIN) / topspeed;

	highest_speed(fw, first_cpu_index);

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3, NULL, 10);
			if (cpu == first_cpu_index)
				continue;
			lowest_speed(fw, cpu);
		}
	}
	closedir(dir);

	highperf = 100 * get_performance_repeat(fw, first_cpu_index,
	                                        0,
	                                        5,
	                                        GET_PERFORMANCE_MAX) / topspeed;

	if (lowperf >= highperf)
		fwts_failed(fw, 
			"Firmware not implementing hardware "
			"coordination cleanly. Firmware using SW_ANY "
			"instead?.");
}


static void check_sw_any(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	uint64_t low_perf, high_perf, newhigh_perf;
	static int once = 0;
	int max_cpu = 0, i,j;
	int cpu;

	/* First set all processors to their lowest speed */
	if ((dir = opendir(FWTS_CPU_PATH)) == NULL) {
		fwts_log_error(fw, "FATAL: cpufreq: sysfs not mounted.");
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3,NULL,10);
			lowest_speed(fw, cpu);
			if (cpu > max_cpu) 
				max_cpu = cpu;
		}
	}
	closedir(dir);
	
	if (max_cpu == 0)
		return; /* Single processor machine, no point in checking anything */
		
	/* assume that all processors have the same low performance */
	low_perf = get_performance(max_cpu);
	for (i=0; i<= max_cpu; i++) {
		highest_speed(fw, i);
		if (!cpu_exists(i))
			continue;

		high_perf = get_performance(i);
		performedtests++;
		fwts_progress(fw, (75 * performedtests)/totaltests);
		/* now set all the others to low again; sw_any will cause the core in question
		   to now also get the low speed, while hardware max will keep the performance
		 */
		for (j=0; j <= max_cpu; j++)
			if (i!=j)
				lowest_speed(fw, j);
		newhigh_perf = get_performance(i);
		if (high_perf - newhigh_perf > (high_perf - low_perf)/4 && once==0 && (high_perf - low_perf > 20)) {
			fwts_failed(fw, "Processors are set to SW_ANY.");
			once++;
			lowest_speed(fw, i);
		}
		performedtests++;
		fwts_progress(fw, (75 * performedtests)/totaltests);
	}
	if (!once)
		fwts_passed(fw, "P-state coordination done by Hardware.");
}

static char *cpufreq_headline(void)
{
	/* Return the name of the test scenario */
	return "CPU frequency scaling tests (takes ~1-2 mins).";
}

static int cpufreq_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *entry;
	int cpu;

	/* Do your test */
	fwts_log_info(fw, 
		"For each processor in the system, this test steps through the "
		"various frequency states (P-states) that the BIOS advertises "
		"for the processor. For each processor/frequency combination, "
		"a quick performance value is measured. The test then validates that:");
	fwts_log_info_verbatum(fw,
		"  1) Each processor has the same number of frequency states\n"
		"  2) Higher advertised frequencies have a higher performance\n"
		"  3) No duplicate frequency values are reported by the BIOS\n"
		"  4) Is BIOS wrongly doing Sw_All P-state coordination across cores\n"
		"  5) Is BIOS wrongly doing Sw_Any P-state coordination across cores\n");
	 

	/* First set all processors to their lowest speed */
	if ((dir = opendir(FWTS_CPU_PATH)) == NULL) {
		fwts_log_error(fw, "FATAL: cpufreq: sysfs not mounted\n");
		return FWTS_ERROR;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3,NULL,10);
			lowest_speed(fw, cpu);
		}
	} 
	rewinddir(dir);

	/* then do the benchmark */

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3,NULL,10);
			do_cpu(fw, cpu);
			lowest_speed(fw, cpu);
			if (no_cpufreq)
				break;
		}
	} 
	rewinddir(dir);

	/* set everything back to the highest speed again */

	while ((entry = readdir(dir)) != NULL) {
		if (entry && strlen(entry->d_name)>3) {
			cpu = strtoul(entry->d_name+3,NULL,10);
			highest_speed(fw, cpu);
		}
	}
	closedir(dir);

	if (!no_cpufreq)
		check_sw_any(fw);

	/*
	 * Check for more than one CPU and more than one frequency and
	 * then do the benchmark set 2 
	 */
	if (sysconf(_SC_NPROCESSORS_CONF) > 1 && nrspeeds > 1) {
		do_sw_all_test(fw);
		performedtests++;
		do_sw_any_test(fw);
		performedtests++;
	} else if (nrspeeds > 1) {
		performedtests += 2;
	}

	fwts_progress(fw, 100);

	return FWTS_OK;
}

static fwts_framework_minor_test cpufreq_tests[] = {
	{ cpufreq_test1, "CPU P-State Checks." },
	{ NULL, NULL }
};

static fwts_framework_ops cpufreq_ops = {
	.headline    = cpufreq_headline,
	.minor_tests = cpufreq_tests
};

FWTS_REGISTER(cpufreq, &cpufreq_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
