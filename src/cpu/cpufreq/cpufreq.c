/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2015 Canonical
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <sched.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <inttypes.h>

#define FWTS_CPU_PATH	"/sys/devices/system/cpu"

#define MAX_FREQS	256

typedef struct {
	uint64_t	Hz;
	uint64_t	speed;
} fwts_cpu_freq;

struct cpu {
	int		idx;
	char		sysfs_path[PATH_MAX];
	bool		online;

	int		n_freqs;
	fwts_cpu_freq	freqs[MAX_FREQS];

	/* saved state */
	char		*orig_governor;
	uint64_t	orig_frequency;
};

static struct cpu *cpus;
static int num_cpus;
static int number_of_speeds = -1;
static int total_tests = 1;
static int performed_tests = 0;
static bool no_cpufreq = false;
static uint64_t top_speed = 0;

#define GET_PERFORMANCE_MAX (0)
#define GET_PERFORMANCE_MIN (1)
#define GET_PERFORMANCE_AVG (2)


#define MAX_ABSOLUTE_ERROR	20.0		/* In Hz */
#define MAX_RELATIVE_ERROR	0.0025		/* as fraction */

static inline void cpu_mkpath(
	char *const path,
	const int len,
	const struct cpu *cpu,
	const char *const name)
{
	snprintf(path, len, "%s/%s/cpufreq/%s", FWTS_CPU_PATH,
			cpu->sysfs_path, name);
}

static void cpu_set_governor(fwts_framework *fw, struct cpu *cpu,
		const char *governor)
{
	char path[PATH_MAX];
	int rc;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_governor");
	rc = fwts_set(governor, path);
	if (rc != FWTS_OK && !no_cpufreq) {
		fwts_warning(fw, "Cannot set CPU governor to %s.", governor);
		no_cpufreq = true;
	}
}

static void cpu_set_frequency(fwts_framework *fw, struct cpu *cpu,
		uint64_t freq_hz)
{
	char path[PATH_MAX];
	char buffer[64];
	int rc;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_setspeed");
	snprintf(buffer, sizeof(buffer), "%" PRIu64 , freq_hz);
	rc = fwts_set(buffer, path);
	if (rc != FWTS_OK)
		fwts_warning(fw, "Cannot set CPU frequency to %s.", buffer);
}

static void cpu_set_lowest_frequency(fwts_framework *fw, struct cpu *cpu)
{
	cpu_set_frequency(fw, cpu, cpu->freqs[0].Hz);
}

static void cpu_set_highest_frequency(fwts_framework *fw, struct cpu *cpu)
{
	cpu_set_frequency(fw, cpu, cpu->freqs[cpu->n_freqs-1].Hz);
}


#ifdef FWTS_ARCH_INTEL
static int get_performance_repeat(
	fwts_framework *fw,
	struct cpu *cpu,
	const int count,
	const int type,
	uint64_t *retval)
{
	int i;

	uint64_t max = 0;
	uint64_t min = ~0;
	uint64_t real_count = 0;
	uint64_t cumulative = 0;

	for (i = 0; i < count; i++) {
		uint64_t temp;

		if (fwts_cpu_performance(fw, cpu->idx, &temp) != FWTS_OK)
			return FWTS_ERROR;

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
		*retval = max;
		break;
	case GET_PERFORMANCE_MIN:
		*retval = min;
		break;
	case GET_PERFORMANCE_AVG:
		if (real_count)
			*retval = cumulative / real_count;
		else
			*retval = 0;
		break;
	default:
		*retval = 0;
		break;
	}
	return FWTS_OK;
}

/*
 *  hz_almost_equal()
 *	used to compare CPU _PSS levels, are they almost
 *	equal?  E.g. within MAX_ABSOLUTE_ERROR Hz difference
 *	between each other, or a relative difference of
 *	MAX_RELATIVE_ERROR.  If they are, then they are deemed
 *	almost equal.
 */
static int hz_almost_equal(const uint64_t a, const uint64_t b)
{
	double da = (double)a, db = (double)b;
	double relative_error, abs_diff = fabs(da - db);

	if (a == b)
		return true;
	if (abs_diff < MAX_ABSOLUTE_ERROR)
		return true;
	if (db > da)
		relative_error = abs_diff / db;
	else
		relative_error = abs_diff / da;

	return relative_error <= MAX_RELATIVE_ERROR;
}

#endif

static char *hz_to_human(const uint64_t hz)
{
	static char buffer[32];

	if (hz > 1000000) {
		snprintf(buffer, sizeof(buffer), "%6.3f GHz",
			(double)hz / 1000000.0);
		return buffer;
	} else if (hz > 1000) {
		snprintf(buffer, sizeof(buffer), "%6.3f MHz",
			(double)hz / 1000.0);
		return buffer;
	} else {
		snprintf(buffer, sizeof(buffer), "%" PRIu64 " Hz", hz);
		return buffer;
	}
}

static uint64_t get_claimed_hz(struct cpu *cpu)
{
	char path[PATH_MAX];
	char *buffer;
	uint64_t value = 0;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_max_freq");

	if ((buffer = fwts_get(path)) != NULL) {
		value = strtoul(buffer, NULL, 10);
		free(buffer);
	}
	return value;
}

static uint64_t get_bios_limit(struct cpu *cpu)
{
	char path[PATH_MAX];
	char *buffer;
	uint64_t value = 0;

	cpu_mkpath(path, sizeof(path), cpu, "bios_limit");

	if ((buffer = fwts_get(path)) != NULL) {
		value = strtoul(buffer, NULL, 10);
		free(buffer);
	}
	return value;
}

static void do_cpu(fwts_framework *fw, struct cpu *cpu)
{
	int i;
	static int warned = 0;
	bool warned_PSS = false;
	uint64_t cpu_top_perf = 0;
	int claimed_hz_too_low = 0;
	int bios_limit_too_low = 0;
	const uint64_t claimed_hz = get_claimed_hz(cpu);
	const uint64_t bios_limit = get_bios_limit(cpu);

	cpu_set_governor(fw, cpu, "userspace");

	if (cpu->n_freqs == 0) {
		if (!no_cpufreq) {
			char path[PATH_MAX];
			char *driver;

			no_cpufreq = true;
			fwts_warning(fw, "CPU frequency scaling not supported.");

			cpu_mkpath(path, sizeof(path), cpu, "scaling_driver");
			driver = fwts_get(path);
			if (driver) {
				fwts_advice(fw, "Scaling driver '%s' is enabled and this "
					"does not seem to allow CPU frequency scaling.", driver);
				free(driver);
			}
		}
		return;
	}
	if (total_tests == 1)
		total_tests = ((2 + cpu->n_freqs) * num_cpus) + 2;

	for (i = 0; i < cpu->n_freqs; i++) {
		cpu_set_frequency(fw, cpu, cpu->freqs[i].Hz);

		if ((claimed_hz != 0) && (claimed_hz < cpu->freqs[i].Hz))
			claimed_hz_too_low++;
		if ((bios_limit != 0) && (bios_limit < cpu->freqs[i].Hz))
			bios_limit_too_low++;

		if (fwts_cpu_performance(fw, cpu->idx, &cpu->freqs[i].speed)
				!= FWTS_OK) {
			fwts_log_error(fw, "Failed to get CPU performance for "
				"CPU frequency %" PRId64 " Hz.",
				cpu->freqs[i].Hz);
			cpu->freqs[i].speed = 0;
		}
		if (cpu->freqs[i].speed > cpu_top_perf)
			cpu_top_perf = cpu->freqs[i].speed;

		performed_tests++;
		fwts_progress(fw, 100 * performed_tests/total_tests);
	}

	if (cpu_top_perf > top_speed)
		top_speed = cpu_top_perf;

	if (claimed_hz_too_low) {
		char path[PATH_MAX];

		cpu_mkpath(path, sizeof(path), cpu, "scaling_max_freq");
		fwts_warning(fw,
			"There were %d CPU frequencies larger than the _PSS "
			"maximum CPU frequency of %s for CPU %d. Has %s "
			"been set too low?",
			claimed_hz_too_low, hz_to_human(claimed_hz),
			cpu->idx, path);
	}

	if (bios_limit_too_low) {
		char path[PATH_MAX];

		cpu_mkpath(path, sizeof(path), cpu, "bios_limit");
		fwts_warning(fw,
			"The CPU frequency BIOS limit %s for CPU %d was set to %s "
			"which is lower than some of the ACPI scaling frequencies.",
			path, cpu->idx, hz_to_human(bios_limit));
	}

	if (claimed_hz_too_low || bios_limit_too_low)
		fwts_log_nl(fw);

	fwts_log_info(fw, "CPU %d: %i CPU frequency steps supported.",
			cpu->idx, cpu->n_freqs);
	fwts_log_info_verbatum(fw, " Frequency | Relative Speed | Bogo loops");
	fwts_log_info_verbatum(fw, "-----------+----------------+-----------");
	for (i = 0; i < cpu->n_freqs; i++) {
		char *turbo = "";
#ifdef FWTS_ARCH_INTEL
		if ((i == 0) && (cpu->n_freqs > 1) &&
		    (hz_almost_equal(cpu->freqs[i].Hz, cpu->freqs[i + 1].Hz)))
			turbo = " (Turbo Boost)";
#endif
		fwts_log_info_verbatum(fw, "%10s |     %5.1f %%    | %9" PRIu64
				"%s",
			hz_to_human(cpu->freqs[i].Hz),
			100.0 * cpu->freqs[i].speed / cpu_top_perf,
			cpu->freqs[i].speed, turbo);
	}

	fwts_log_nl(fw);

	if (cpu->n_freqs < 2)
		return;

	/* now check for 1) increasing HZ and 2) increasing speed */
	for (i = 0; i < cpu->n_freqs - 1; i++) {
		if (cpu->freqs[i].Hz == cpu->freqs[i+1].Hz && !warned++)
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"CPUFreqDupFreq",
				"Duplicate frequency reported.");
		if (cpu->freqs[i].speed > cpu->freqs[i+1].speed)
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"CPUFreqSlowerOnCPU",
				"Supposedly higher frequency %s is slower (%" PRIu64
				" bogo loops) than frequency %s (%" PRIu64
				" bogo loops) on CPU %i.",
				hz_to_human(cpu->freqs[i+1].Hz),
				cpu->freqs[i+1].speed,
				hz_to_human(cpu->freqs[i].Hz),
				cpu->freqs[i].speed,
				cpu->idx);

		if ((cpu->freqs[i].Hz > claimed_hz) && !warned_PSS) {
			warned_PSS = true;
			fwts_warning(fw, "Frequency %" PRIu64
				" not achievable; _PSS limit of %" PRIu64
					" in effect?",
				cpu->freqs[i].Hz, claimed_hz);
		}
	}
}


#ifdef FWTS_ARCH_INTEL
/*
 * 4) Is BIOS wrongly doing Sw_All P-state coordination across cpus
 * - Change frequency on all CPU to the lowest value
 * - Change frequency on one particular CPU to the highest
 * - If BIOS is doing Sw_All, the last high freq request will not work
 */
static void do_sw_all_test(fwts_framework *fw)
{
	uint64_t highperf, lowperf;
	int i;

	/* All CPUs at the lowest frequency */
	for (i = 0; i < num_cpus; i++)
		cpu_set_lowest_frequency(fw, &cpus[i]);

	if (get_performance_repeat(fw, &cpus[0], 5, GET_PERFORMANCE_MIN, &lowperf) != FWTS_OK) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "CPUFreqSW_ALLGetPerf",
			"Failed to get CPU performance.");
		return;
	}
	lowperf = (lowperf * 100) / top_speed;

	cpu_set_highest_frequency(fw, &cpus[0]);
	if (get_performance_repeat(fw, &cpus[0], 5, GET_PERFORMANCE_MAX, &highperf) != FWTS_OK) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "CPUFreqSW_ALLGetPerf",
			"Failed to get CPU performance.");
		return;
	}
	highperf = (highperf * 100) / top_speed;

	if (lowperf >= highperf)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"CPUFreqSW_ALL",
			"Firmware not implementing hardware "
			"coordination cleanly. Firmware using SW_ALL "
			"instead?");
}


/*
 * 5) Is BIOS wrongly doing Sw_Any P-state coordination across cpus
 * - Change frequency on all CPU to the lowest value
 * - Change frequency on one particular CPU to the highest
 * - Change frequency on all CPU to the lowest value
 * - If BIOS is doing Sw_Any, the high freq request will not work
 */
static void do_sw_any_test(fwts_framework *fw)
{
	uint64_t highperf, lowperf;
	int i, rc;

	for (i = 0; i < num_cpus; i++)
		cpu_set_lowest_frequency(fw, &cpus[i]);

	/* All CPUs at the lowest frequency */
	rc = get_performance_repeat(fw, &cpus[0], 5,
			GET_PERFORMANCE_MIN, &lowperf);

	if (rc != FWTS_OK) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "CPUFreqSW_ANYGetPerf",
			"Failed to get CPU performance.");
		return;
	}

	lowperf = (100 * lowperf) / top_speed;

	cpu_set_highest_frequency(fw, &cpus[0]);

	for (i = 0; i < num_cpus; i++)
		cpu_set_lowest_frequency(fw, &cpus[i]);

	rc = get_performance_repeat(fw, &cpus[0], 5, GET_PERFORMANCE_MAX,
				&highperf);
	if (rc != FWTS_OK) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "CPUFreqSW_ANYGetPerf",
			"Failed to get CPU performance.");
		return;
	}
	highperf = (100 * highperf) / top_speed;

	if (lowperf >= highperf)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"CPUFreqSW_ANY",
			"Firmware not implementing hardware "
			"coordination cleanly. Firmware using SW_ANY "
			"instead?.");
}

static void check_sw_any(fwts_framework *fw)
{
	uint64_t low_perf, high_perf, newhigh_perf;
	static int once = 0;
	int i, j;

	/* Single processor machine, no point in checking anything */
	if (num_cpus < 2)
		return;

	/* First set all processors to their lowest speed */
	for (i = 0; i < num_cpus; i++)
		cpu_set_lowest_frequency(fw, &cpus[i]);

	/* assume that all processors have the same low performance */
	if (fwts_cpu_performance(fw, cpus[0].idx, &low_perf) != FWTS_OK) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"CPUFreqCPsSetToSW_ANYGetPerf",
			"Cannot get CPU performance.");
		return;
	}

	for (i = 0; i <= num_cpus; i++) {
		struct cpu *cpu = &cpus[i];

		cpu_set_highest_frequency(fw, cpu);
		if (!cpu->online)
			continue;

		if (fwts_cpu_performance(fw, cpu->idx, &high_perf) != FWTS_OK) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"CPUFreqCPsSetToSW_ANYGetPerf",
				"Cannot get CPU performance.");
			return;
		}
		performed_tests++;
		fwts_progress(fw, 100 * performed_tests/total_tests);
		/*
		 * now set all the others to low again; sw_any will cause
		 * the core in question to now also get the low speed, while
		 * hardware max will keep the performance
		 */
		for (j = 0; j < num_cpus; j++)
			if (i != j)
				cpu_set_lowest_frequency(fw, &cpus[j]);
		if (fwts_cpu_performance(fw, cpu->idx, &newhigh_perf)
				!= FWTS_OK) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"CPUFreqCPsSetToSW_ANYGetPerf",
				"Cannot get CPU performance.");
			return;
		}
		if ((high_perf > newhigh_perf) &&
		    (high_perf - newhigh_perf > (high_perf - low_perf)/4) &&
		    (once == 0) && (high_perf - low_perf > 20)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"CPUFreqCPUsSetToSW_ANY",
				"Processors are set to SW_ANY.");
			once++;
			cpu_set_lowest_frequency(fw, cpu);
		}
		performed_tests++;
		fwts_progress(fw, 100 * performed_tests/total_tests);
	}
	if (!once)
		fwts_passed(fw, "P-state coordination under hardware control.");
}
#endif

static int cpufreq_test1(fwts_framework *fw)
{
	int i;

#ifdef FWTS_ARCH_INTEL
	fwts_log_info(fw,
		"For each processor in the system, this test steps through the "
		"various frequency states (P-states) that the BIOS advertises "
		"for the processor. For each processor/frequency combination, "
		"a quick performance value is measured. The test then validates that:");
	fwts_log_info_verbatum(fw, "  1. Each processor has the same number of frequency states.");
	fwts_log_info_verbatum(fw, "  2. Higher advertised frequencies have a higher performance.");
	fwts_log_info_verbatum(fw, "  3. No duplicate frequency values are reported by the BIOS.");
	fwts_log_info_verbatum(fw, "  4. BIOS doing Sw_All P-state coordination across cores.");
	fwts_log_info_verbatum(fw, "  5. BIOS doing Sw_Any P-state coordination across cores.");
#else
	fwts_log_info(fw,
		"For each processor in the system, this test steps through the "
		"various frequency states that the CPU supports. "
		"For each processor/frequency combination, "
		"a quick performance value is measured. The test then validates that:");
	fwts_log_info_verbatum(fw, "  1. Each processor has the same number of frequency states.");
	fwts_log_info_verbatum(fw, "  2. Higher advertised frequencies have a higher performance.");
	fwts_log_info_verbatum(fw, "  3. No duplicate frequency values exist.");
#endif
	fwts_log_nl(fw);

	for (i = 0; i < num_cpus; i++)
		cpu_set_lowest_frequency(fw, &cpus[i]);

	/* then do the benchmark */
	for (i = 0; i < num_cpus; i++) {
		do_cpu(fw, &cpus[i]);
		cpu_set_lowest_frequency(fw, &cpus[i]);
		if (no_cpufreq)
			break;
	}

	/* set everything back to the highest speed again */
	for (i = 0; i < num_cpus; i++)
		cpu_set_highest_frequency(fw, &cpus[i]);


#ifdef FWTS_ARCH_INTEL
	if (!no_cpufreq)
		check_sw_any(fw);

	/*
	 * Check for more than one CPU and more than one frequency and
	 * then do the benchmark set 2
	 */
	if (num_cpus > 1 && number_of_speeds > 1) {
		do_sw_all_test(fw);
		do_sw_any_test(fw);
	} else if (number_of_speeds > 1) {
		performed_tests += 4;
		fwts_progress(fw, 100 * performed_tests/total_tests);
	}
#endif
	fwts_progress(fw, 100);

	return FWTS_OK;
}

static int cpufreq_compare_freqs(fwts_framework *fw, struct cpu *c1,
		struct cpu *c2)
{
	int i;

	if (c1->n_freqs != c2->n_freqs) {
		fwts_log_warning(fw, "cpu %d has %d freqs, cpu %d has %d freqs",
				c1->idx, c1->n_freqs,
				c2->idx, c2->n_freqs);
		return FWTS_ERROR;
	}

	for (i = 0; i < c1->n_freqs; i++) {
		if (c1->freqs[i].Hz != c2->freqs[i].Hz) {
			fwts_log_warning(fw, "freq entry %d: "
						"cpu %d is %" PRId64 ", "
						"cpu %d is %" PRId64, i,
					c1->idx, c1->freqs[i].Hz,
					c2->idx, c2->freqs[i].Hz);
			return FWTS_ERROR;
		}
	}

	return FWTS_OK;
}

static int cpufreq_test_consistency(fwts_framework *fw)
{
	struct cpu *cpu, *cpu0;
	bool consistent = true;
	int i;

	if (num_cpus < 2) {
		fwts_skipped(fw, "Test skipped, only one processor present");
		return FWTS_SKIP;
	}

	cpu0 = &cpus[0];

	for (i = 1; i < num_cpus; i++) {
		cpu = &cpus[i];
		if (cpufreq_compare_freqs(fw, cpu0, cpu) != FWTS_OK) {
			consistent = false;
			fwts_log_error(fw,
				"cpu %d has an inconsistent frequency table",
				cpu->idx);
		}
	}

	if (!consistent)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "CPUFreqInconsistent",
				"inconsistencies found in CPU "
				"frequency tables");
	else
		fwts_passed(fw, "CPU frequency tables are consistent");

	return FWTS_OK;
}

static int cpufreq_test_duplicates(fwts_framework *fw)
{
	struct cpu *cpu0 = &cpus[0];
	bool dup = false;
	uint64_t freq;
	int i;

	/* the frequency list is sorted, so we can do this in one pass */
	for (i = 0; i < cpu0->n_freqs - 1; i++) {

		freq = cpu0->freqs[i].Hz;

		if (cpu0->freqs[i+1].Hz != freq)
			continue;

		dup = true;
		fwts_log_error(fw, "duplicate cpu frequency %" PRIx64,
				freq);

		/* don't report further duplicates for this entry */
		for (i++; i < cpu0->n_freqs - 1; i++)
			if (cpu0->freqs[i].Hz != freq)
				break;
	}

	if (dup) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "CPUFreqDuplicate",
				"duplicates found in CPU frequency table");
	} else {
		fwts_passed(fw, "No duplicates in CPU frequency table");
	}

	return FWTS_OK;
}

static int cpu_freq_compare(const void *v1, const void *v2)
{
	const fwts_cpu_freq *f1 = v1;
	const fwts_cpu_freq *f2 = v2;
	return f1->Hz - f2->Hz;
}

static int parse_cpu_info(struct cpu *cpu, struct dirent *dir)
{
	char *end, path[PATH_MAX+1], *str, *tmp, *tok;
	int i;

	strcpy(cpu->sysfs_path, dir->d_name);
	cpu->idx = strtoul(cpu->sysfs_path + strlen("cpu"), &end, 10);
	cpu->online = true;

	cpu_mkpath(path, sizeof(path), cpu, "scaling_governor");
	cpu->orig_governor = fwts_get(path);

	if (cpu->orig_governor && !strcmp(cpu->orig_governor, "userspace")) {
		cpu_mkpath(path, sizeof(path), cpu, "scaling_setspeed");
		tmp = fwts_get(path);
		cpu->orig_frequency = strtoull(tmp, NULL, 10);
		free(tmp);
	}

	/* parse available frequencies */
	cpu_mkpath(path, sizeof(path), cpu, "scaling_available_frequencies");
	str = fwts_get(path);

	for (tmp = str, i = 0; ; tmp = NULL) {
		tok = strtok(tmp, " ");
		if (!tok)
			break;
		if (!isdigit(tok[0]))
			continue;
		cpu->freqs[i++].Hz = strtoull(tok, NULL, 10);
	}

	free(str);

	cpu->n_freqs = i;
	qsort(cpu->freqs, cpu->n_freqs, sizeof(cpu->freqs[0]),
			cpu_freq_compare);

	return FWTS_OK;
}

static int is_cpu_dir(const struct dirent *dir)
{
	return strncmp(dir->d_name, "cpu", 3) == 0 &&
		isdigit(dir->d_name[3]);
}

static int cpufreq_init(fwts_framework *fw __attribute__((unused)))
{
	struct dirent **dirs;
	int i;

	num_cpus = scandir(FWTS_CPU_PATH, &dirs, is_cpu_dir, versionsort);
	cpus = calloc(num_cpus, sizeof(*cpus));

	for (i = 0; i < num_cpus; i++)
		parse_cpu_info(&cpus[i], dirs[i]);

	return FWTS_OK;
}

static int cpufreq_deinit(fwts_framework *fw)
{
	int i;

	for (i = 0; i < num_cpus; i++) {
		struct cpu *cpu = &cpus[i];

		if (cpu->orig_governor) {
			cpu_set_governor(fw, cpu, cpu->orig_governor);
			free(cpu->orig_governor);
		}

		if (cpu->orig_frequency)
			cpu_set_frequency(fw, cpu, cpu->orig_frequency);
	}
	free(cpus);

	return FWTS_OK;
}

static fwts_framework_minor_test cpufreq_tests[] = {
	{ cpufreq_test_consistency,	"CPU frequency table consistency" },
	{ cpufreq_test_duplicates,	"CPU frequency table duplicates" },
#ifdef FWTS_ARCH_INTEL
	{ cpufreq_test1, "CPU P-State tests." },
#else
	{ cpufreq_test1, "CPU Frequency tests." },
#endif
	{ NULL, NULL }
};

static fwts_framework_ops cpufreq_ops = {
	.init        = cpufreq_init,
	.deinit      = cpufreq_deinit,
	.description = "CPU frequency scaling tests.",
	.minor_tests = cpufreq_tests
};

FWTS_REGISTER("cpufreq", &cpufreq_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV)
