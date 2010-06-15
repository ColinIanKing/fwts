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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static int fwts_cpu_num;
static pid_t *fwts_cpu_pids;

int fwts_cpu_enumerate(void)
{
	DIR *dir;
	int cpus = 0;
	struct dirent *directory;

	if ((dir = opendir("/proc/acpi/processor")) == NULL)
		return -1;

	while ((directory = readdir(dir)) != NULL)
		if (strncmp(directory->d_name, "CPU", 3) == 0)
			cpus++;

	closedir(dir);
	
	return cpus;
}

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

static void fwts_cpu_consume_sighandler(int dummy)
{
	exit(0);
}

static void fwts_cpu_sigint_handler(int dummy)
{
	fwts_cpu_consume_kill();
	exit(0);
}

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

static void fwts_cpu_consume_cleanup(void)
{
	fwts_cpu_consume_kill();
	free(fwts_cpu_pids);	
}

int fwts_cpu_consume(const int seconds)
{
	int i;

	if ((fwts_cpu_num = fwts_cpu_enumerate()) < 0) 
		return 1;

	if ((fwts_cpu_pids = (pid_t*)calloc(fwts_cpu_num, sizeof(pid_t))) == NULL)
		return 1;

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
			fwts_cpu_consume_cleanup();
			return 1;
		default:
			fwts_cpu_pids[i] = pid;
			break;
		}
	}
	sleep(seconds);

	fwts_cpu_consume_cleanup();

	return 0;
}
