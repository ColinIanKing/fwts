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

#include "text_list.h"
#include "pipeio.h"
#include "fileio.h"

int pipe_open(const char *command, pid_t *childpid)
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
		//close(STDERR_FILENO);
		if (pipefds[0] != STDOUT_FILENO) {
			dup2(pipefds[1], STDOUT_FILENO);
			close(pipefds[1]);
		}
		close(pipefds[0]);
		fprintf(stderr,"CHILD EXEC'ing %s\n", command);
		execl(_PATH_BSHELL, "sh", "-c", command, NULL);
		fprintf(stderr,"CHILD EXEC FAILED!!\n");
		_exit(127);
	default:
		/* Parent */
		close(pipefds[1]);
		*childpid = pid;
	
		return pipefds[0];
	}
}

char *pipe_read(int fd, int *length)
{
	char *ptr = NULL;
	char buffer[8192];	
	int n;
	int size = 0;
	*length = 0;

	ptr = calloc(1, 1);	/* Empty string! */

	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
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
	*length = size;
	return ptr;
}

int pipe_close(int fd, pid_t pid)
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

int pipe_exec(const char *command, text_list **list)
{
	pid_t 	pid;
	int	fd;
	int 	len;
	char 	*text;

	if ((fd = pipe_open(command, &pid)) < 0) 
		return -1;

	text = pipe_read(fd, &len);
	*list = text_list_from_text(text);
	free(text);

	return pipe_close(fd, pid);
}
