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

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int pipe_open(const char *command)
{
	int pipefds[2];
	pid_t pid;
	
	if (pipe(pipefds) < 0) 
		return -1;

	pid = fork();
	switch (pid) {
	case -1:
		/* Ooops */
		close(pipefds[0]);
		close(pipefds[1]);
		return -1;
	case 0:
		/* Child */
		close(STDERR_FILENO);
		if (pipefds[0] != STDOUT_FILENO) {
			dup2(pipefds[1], STDOUT_FILENO);
			close(pipefds[1]);
		}
		close(pipefds[0]);
		execl(_PATH_BSHELL, "sh", "-c", command, NULL);
		_exit(127);
	default:
		/* Parent */
		close(pipefds[1]);
	
		return pipefds[0];
	}
}

char *pipe_read(int fd)
{
	char *ptr = NULL;
	char buffer[32];	
	int n;
	int size = 0;

	while ((n = read(fd, buffer, sizeof(buffer))) != 0) {
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN)
				free(ptr);
				return NULL;
		}
		else {
			ptr = realloc(ptr, size + n + 1);
			memcpy(ptr + size, buffer, n);
			size += n;
			*(ptr+size) = 0;
		}
	}
	return ptr;
}

int pipe_close(int fd)
{
	int status;

	close(fd);
	wait(&status);
}
