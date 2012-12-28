/*
 * Copyright (C) 2010-2013 Canonical
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

#include "fwts.h"

/*
 *  fwts_pipe_open()
 *	execl a command, return pid in *childpid and
 *	return fd. fd < 0 indicates error.
 */
int fwts_pipe_open(const char *command, pid_t *childpid)
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
		if (freopen("/dev/null", "w", stderr) == NULL) {
			fprintf(stderr, "Cannot redirect stderr\n");
		}
		if (pipefds[0] != STDOUT_FILENO) {
			dup2(pipefds[1], STDOUT_FILENO);
			close(pipefds[1]);
		}
		close(pipefds[0]);
		execl(_PATH_BSHELL, "sh", "-c", command, NULL);
		_exit(FWTS_EXEC_ERROR);
	default:
		/* Parent */
		close(pipefds[1]);
		*childpid = pid;

		return pipefds[0];
	}
}

/*
 *  fwts_pipe_read()
 *	read output from fwts_pipe_open(), *length is
 *	set to the number of chars read and we return
 *	a buffer of read data.
 */
char *fwts_pipe_read(const int fd, ssize_t *length)
{
	char *ptr = NULL;
	char buffer[8192];
	ssize_t n;
	ssize_t size = 0;
	*length = 0;

	ptr = NULL;

	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				free(ptr);
				return NULL;
			}
		}
		else {
			ptr = realloc(ptr, size + n + 1);
			if (ptr == NULL)
				return NULL;
			memcpy(ptr + size, buffer, n);
			size += n;
			*(ptr+size) = 0;
		}
	}
	*length = size;
	return ptr;
}

/*
 *  fwts_pipe_close()
 *	close fd, wait for child of given pid to exit
 */
int fwts_pipe_close(const int fd, const pid_t pid)
{
	int status;

	close(fd);

	for (;;) {
		if (waitpid(pid, &status, WUNTRACED | WCONTINUED) == -1)
			return EXIT_FAILURE;
		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		if (WIFSIGNALED(status))
			return WTERMSIG(status);
	}
}

/*
 *  fwts_pipe_exec()
 *	execute a command, return a list containing lines
 *	of the stdout output from the command.
 *	Return FWTS_OK if the exec worked, FWTS_EXEC_ERROR if
 *	it failed.  status contains the child exit status.
 */
int fwts_pipe_exec(const char *command, fwts_list **list, int *status)
{
	pid_t 	pid;
	int	fd;
	ssize_t	len;
	char 	*text;

	if ((fd = fwts_pipe_open(command, &pid)) < 0)
		return FWTS_ERROR;

	text = fwts_pipe_read(fd, &len);
	*list = fwts_list_from_text(text);
	free(text);

	*status = fwts_pipe_close(fd, pid);
	if (*status) {
		fwts_list_free(*list, free);
		*list = NULL;
		return FWTS_EXEC_ERROR;
	}
	return FWTS_OK;
}
