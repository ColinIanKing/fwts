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
#define _FILE_OFFSET_BITS 64

#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "fwts_types.h"
#include "fwts_cpu.h"
#include "fwts_pipeio.h"

static int fwts_cpu_num;
static pid_t *fwts_cpu_pids;

#define MSR_AMD64_OSVW_ID_LENGTH	0xc0010140
#define MSR_AMD64_OSVW_STATUS		0xc0010141

/*
 *  fwts_cpu_readmsr()
 *	Read a given msr on a specificied CPU
 */
int fwts_cpu_readmsr(const int cpu, const uint32_t reg, uint64_t *val)
{
	struct stat statbuf;
	char buffer[PATH_MAX];
	uint64_t value;
	int fd;
	int ret;

	value = 0;

	snprintf(buffer, sizeof(buffer), "/dev/cpu/%d/msr", cpu);

	if (stat(buffer, &statbuf)) {
		/* Hrm, msr not there, so force modprobe msr and see what happens */
		pid_t pid;
		if ((fd = fwts_pipe_open("modprobe msr", &pid)) < 0)
			return FWTS_ERROR;
		fwts_pipe_close(fd, pid);

		if (stat(buffer, &statbuf))
			return FWTS_ERROR; /* Really failed */
	}
	
	if ((fd = open(buffer, O_RDONLY)) < 0)
                return FWTS_ERROR;

	ret = pread(fd, &value, 8, reg);
	close(fd);

	*val = value;

	if (ret<0)
		return FWTS_ERROR;

	return FWTS_OK;
}

/*
 *  fwts_cpu_free_info()
 *	free CPU information
 */
void fwts_cpu_free_info(fwts_cpuinfo_x86 *cpu)
{
	if (cpu) {
		free(cpu->vendor_id);
		free(cpu->model_name);
		free(cpu->flags);
	}
	free(cpu);
}

/*
 *  fwts_cpu_get_info()
 *	get CPU information for specified CPU
 */
fwts_cpuinfo_x86 *fwts_cpu_get_info(const int which_cpu)
{
	FILE *fp;
	char buffer[1024];
	fwts_cpuinfo_x86 *cpu;
	int cpu_num = -1;

	if ((cpu = (fwts_cpuinfo_x86*)calloc(1, sizeof(fwts_cpuinfo_x86))) == NULL)
		return NULL;

	if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
		return NULL;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		char *ptr = strstr(buffer, ":");
		if (ptr)
			ptr += 2;
		else
			continue;

		buffer[strlen(buffer)-1] = '\0';

		if (!strncmp(buffer, "processor", 9)) {
			sscanf(ptr, "%d", &cpu_num);
			if (cpu_num > which_cpu)
				break;
			continue;
		} else {
			if (cpu_num != which_cpu)
				continue;
		}

		if (!strncmp(buffer, "vendor_id", 9)) {
			cpu->vendor_id = strdup(ptr);
			continue;
		}
		if (!strncmp(buffer, "cpu family",10)) {
			sscanf(ptr, "%d", &cpu->x86);
			continue;
		}
		if (!strncmp(buffer, "model name", 10)) {
			cpu->model_name = strdup(ptr);
			continue;
		}
		if (!strncmp(buffer, "model", 5)) {
			sscanf(ptr, "%d", &cpu->x86_model);
			continue;
		}
		if (!strncmp(buffer, "stepping", 8)) {
			sscanf(ptr, "%d", &cpu->stepping);
			continue;
		}
		if (!strncmp(buffer, "flags", 4)) {
			cpu->flags = strdup(ptr);
			continue;
		}
	}
	fclose(fp);

	return cpu;
}


/*
 *  fwts_cpu_has_c1e()
 *	check if CPU has C1E bit
 */
fwts_bool fwts_cpu_has_c1e(void)
{
	uint64_t val;

	fwts_cpuinfo_x86 *cpu;

	if ((cpu = fwts_cpu_get_info(0)) == NULL)
		return FWTS_BOOL_ERROR;
	
        if (strstr(cpu->vendor_id, "AuthenticAMD") == NULL) {
		fwts_cpu_free_info(cpu);
		return FWTS_FALSE;
	}

        /* Family 0x0f models < rev F do not have C1E */
        if (cpu->x86 == 0x0F && cpu->x86_model >= 0x40) {
		fwts_cpu_free_info(cpu);
                return FWTS_TRUE;
	}

        if (cpu->x86 == 0x10) {
                /*
                 * check OSVW bit for CPUs that are not affected
                 * by erratum #400
                 */
		if (strstr(cpu->flags, "osvw") != NULL) {
			fwts_cpu_readmsr(0, MSR_AMD64_OSVW_ID_LENGTH, &val);
                        if (val >= 2) {
                                fwts_cpu_readmsr(0, MSR_AMD64_OSVW_STATUS, &val);
                                if (!(val & 2)) {
					fwts_cpu_free_info(cpu);
					return FWTS_FALSE;
				}
                        }
                }
		fwts_cpu_free_info(cpu);
                return FWTS_TRUE;
        }
	fwts_cpu_free_info(cpu);
	return FWTS_FALSE;
}

/*
 *  fwts_cpu_enumerate()
 *	enumerate all CPUs
 */
int fwts_cpu_enumerate(void)
{
	int cpus = sysconf(_SC_NPROCESSORS_CONF);

	if (cpus < 0)
		return FWTS_ERROR;
	
	return cpus;
}

/*
 *  fwts_cpu_consume_kill()
 *	kill CPU consumer processes as created by fwts_cpu_consume_cycles()
 */
static void fwts_cpu_consume_kill(void)
{
	int i;
	siginfo_t info;

	for (i=0;i<fwts_cpu_num;i++) {
		if (fwts_cpu_pids[i] != 0) {
			kill(fwts_cpu_pids[i], SIGUSR1);
			waitid(P_PID, fwts_cpu_pids[i], &info, WEXITED);
		}
	}
}

/*
 *  fwts_cpu_consume_sighandler()
 *	CPU consumer processes signal handler
 */
static void fwts_cpu_consume_sighandler(int dummy)
{
	exit(0);
}

/*
 *  fwts_cpu_sigint_handler()
 *	kill all CPU consumer processes and die
 */
static void fwts_cpu_sigint_handler(int dummy)
{
	fwts_cpu_consume_kill();
	exit(0);
}

/*
 *  fwts_cpu_consume_cycles()
 *	eat up CPU cycles
 */
static void fwts_cpu_consume_cycles(void)
{
	signal(SIGUSR1, fwts_cpu_consume_sighandler);

	float dummy = 0.000001;
	unsigned long long i = 0;

	while (dummy > 0.0) {
		dummy += 0.0000037;
		i++;
	}
}


/*
 *  fwts_cpu_consume_complete()
 *	kill all CPU consumes, free up pid info
 */
void fwts_cpu_consume_complete(void)
{
	fwts_cpu_consume_kill();
	free(fwts_cpu_pids);	
}

/*
 *  fwts_cpu_consume_start()
 *	kick off per CPU tasks to eat up CPU
 */
int fwts_cpu_consume_start(void)
{
	int i;

	if ((fwts_cpu_num = fwts_cpu_enumerate()) < 0)
		return FWTS_ERROR;

	if ((fwts_cpu_pids = (pid_t*)calloc(fwts_cpu_num, sizeof(pid_t))) == NULL)
		return FWTS_ERROR;

	signal(SIGINT, fwts_cpu_sigint_handler);

	for (i=0;i<fwts_cpu_num;i++) {
		pid_t pid;

		pid = fork();
		switch (pid) {
		case 0: /* Child */
			fwts_cpu_consume_cycles();	
			break;
		case -1:
			/* Went wrong */
			fwts_cpu_consume_complete();
			return FWTS_ERROR;
		default:
			fwts_cpu_pids[i] = pid;
			break;
		}
	}
	return FWTS_OK;
}

/*
 *  fwts_cpu_consume()
 *	consume a specified amount of CPU time
 *	on all CPUs
 */
int fwts_cpu_consume(const int seconds)
{
	if (fwts_cpu_consume_start() != FWTS_OK)
		return FWTS_ERROR;

	sleep(seconds);

	fwts_cpu_consume_complete();

	return FWTS_OK;
}
