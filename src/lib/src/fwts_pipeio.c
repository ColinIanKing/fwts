/*
 * Copyright (C) 2010-2014 Canonical
 *
 * The following functions are derivative work from systemd, and
 * are covered by Copyright 2010 Lennart Poettering:
 *     fwts_write_string_to_file(), fwts_write_string_file(),
 *     fwts_write_string_file(), fwts_read_file_first_line()
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
#include <limits.h>
#include <stdbool.h>

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
	ssize_t size = 0;
	*length = 0;

	ptr = NULL;

	for (;;) {
		ssize_t n = read(fd, buffer, sizeof(buffer));
		char *tmp;

		if (n == 0)
			break;
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				free(ptr);
				return NULL;
			}
			continue;
		}

		if ((tmp = realloc(ptr, size + n + 1)) == NULL) {
			free(ptr);
			return NULL;
		}
		ptr = tmp;
		memcpy(ptr + size, buffer, n);
		size += n;
		*(ptr+size) = 0;
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

/*
 *  fwts_exec()
 *	execute a command
 *	Return FWTS_OK if the exec worked, FWTS_EXEC_ERROR if
 *	it failed.  status contains the child exit status.
 */
int fwts_exec(const char *command, int *status)
{
	pid_t 	pid;
	int	fd;

	if ((fd = fwts_pipe_open(command, &pid)) < 0)
		return FWTS_ERROR;

	*status = fwts_pipe_close(fd, pid);
	if (*status)
		return FWTS_EXEC_ERROR;
	return FWTS_OK;
}

/*
 *  fwts_write_string_to_file()
 *	write a string to a file pointer
 *	Return FWTS_OK if writing worked, FWTS_ERROR if it failed.
 */
int fwts_write_string_to_file(
	const fwts_framework *fw,
	FILE *file,
	const char *str)
{
	errno = 0;
	fputs(str, file);
	if (!fwts_string_endswith(str, "\n"))
		fputc('\n', file);

	fflush(file);

	if (ferror(file)) {
		fwts_log_error(fw,
			"Failed to write string '%s' to file descriptor, error: %d (%s).",
			str,
			errno,
			strerror(errno));
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

/*
 *  fwts_write_string_file()
 *	write a string to a file
 *	Return FWTS_OK if writing worked, FWTS_ERROR if it failed.
 */
int fwts_write_string_file(
	const fwts_framework *fw,
	const char *file_name,
	const char *str)
{
	FILE *file = NULL;
	int ret;

	errno = 0;
	file = fopen(file_name, "we");
	if (!file) {
		fwts_log_error(fw,
			"Failed to write string '%s' to %s, error: %d (%s).",
			str,
			file_name,
			errno,
			strerror(errno));
		return FWTS_ERROR;
	}

	ret = fwts_write_string_to_file(fw, file, str);
	fclose(file);

	return ret;
}

/*
 *  fwts_read_file_first_line()
 *	read the first line of a file
 *	Return FWTS_OK if reading worked, FWTS_ERROR if it failed,
 *      or FWTS_OUT_OF_MEMORY if it went out of memory.
 */
int fwts_read_file_first_line(
	const fwts_framework *fw,
	const char *file_name,
	char **line)
{
	FILE *file = NULL;
	char buffer[LINE_MAX], *temp;
	errno = 0;

	file = fopen(file_name, "re");
	if (!file) {
		fwts_log_error(fw,
			"Failed to read first line from %s, error: %d (%s).",
			file_name,
			errno,
			strerror(errno));
		return FWTS_ERROR;
	}

	if (!fgets(buffer, sizeof(buffer), file)) {
		if (ferror(file)) {
			fclose(file);
			fwts_log_error(fw,
				"Failed to read first line from %s, error: %d (%s).",
				file_name,
				errno,
				strerror(errno));
			return FWTS_ERROR;
		}
		buffer[0] = 0;
	}

	temp = strdup(buffer);
	if (!temp) {
		fclose(file);
		fwts_log_error(fw,
			"Failed to read first line from %s: ran out of memory.",
			file_name);
		return FWTS_OUT_OF_MEMORY;
	}

	fwts_chop_newline(temp);
	*line = temp;
	fclose(file);

	return FWTS_OK;
}

/*
 *  fwts_file_first_line_contains_string()
 *	read the first line of a file
 *	Return true if the line contained the string, false if it didn't.
 */
bool fwts_file_first_line_contains_string(
	const fwts_framework *fw,
	const char *file_name,
	const char *str)
{
	char *contents = NULL;
	int ret;
	bool contains;

	ret = fwts_read_file_first_line(fw, file_name, &contents);

	if (ret != FWTS_OK) {
		fwts_log_error(fw, "Failed to get the contents of %s.", file_name);
		contains = false;
	} else
		contains = (strstr(contents, str) != NULL);

	free(contents);
	return contains;
}

