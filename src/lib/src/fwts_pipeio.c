/*
 * Copyright (C) 2010-2016 Canonical
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
#include <fcntl.h>
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
 *  fwts_pipe_open_rw()
 *	execl a command, return pid in *childpid and a pipe connected
 *	to stdout in *in_fd, and stdout in *out_fd. Return value < 0
 *	indicates error.
 */
int fwts_pipe_open_rw(const char *command, pid_t *childpid,
		int *in_fd, int *out_fd)
{
	int out_pipefds[2];
	int in_pipefds[2];
	pid_t pid;
	FILE *fp;

	if (in_fd != NULL) {
		if (pipe(in_pipefds) < 0)
			return -1;
	} else {
		in_pipefds[0] = open("/dev/null", O_RDONLY);
		in_pipefds[1] = -1;
	}

	if (out_fd != NULL) {
		if (pipe(out_pipefds) < 0)
			goto err_close_in;
	} else {
		out_pipefds[0] = -1;
		out_pipefds[1] = open("/dev/null", O_WRONLY);
	}

	pid = fork();
	switch (pid) {
	case -1:
		/* Ooops */
		goto err_close_out;
	case 0:
		/* Child */
		if ((fp = freopen("/dev/null", "w", stderr)) == NULL) {
			fprintf(stderr, "Cannot redirect stderr\n");
		}
		if (out_pipefds[0] != STDOUT_FILENO) {
			dup2(out_pipefds[1], STDOUT_FILENO);
			close(out_pipefds[1]);
		}
		if (in_pipefds[1] != STDIN_FILENO) {
			dup2(in_pipefds[0], STDIN_FILENO);
			close(in_pipefds[0]);
		}

		close(out_pipefds[0]);
		close(in_pipefds[1]);
		execl(_PATH_BSHELL, "sh", "-c", command, NULL);
		if (fp)
			fclose(fp);
		_exit(FWTS_EXEC_ERROR);
	default:
		/* Parent */
		close(out_pipefds[1]);
		close(in_pipefds[0]);

		*childpid = pid;
		if (out_fd)
			*out_fd = out_pipefds[0];
		if (in_fd)
			*in_fd = in_pipefds[1];

		return 0;
	}

err_close_in:
	close(in_pipefds[0]);
	close(in_pipefds[1]);
err_close_out:
	close(out_pipefds[0]);
	close(out_pipefds[1]);
	return -1;
}

/*
 *  fwts_pipe_open_ro()
 *	execl a command, return pid in *childpid and a pipe connected
 *	to stdout in *fd. Return value < 0 indicates error.
 */
int fwts_pipe_open_ro(const char *command, pid_t *childpid, int *fd)
{
	return fwts_pipe_open_rw(command, childpid, NULL, fd);
}

/*
 *  fwts_pipe_read()
 *	read output from fwts_pipe_open_ro(), *out_buf is populated with
 *	returned data (allocated, must be free()-ed after use), and length in
 *	*out_len.
 *	Returns non-zero on failure.
 */
int fwts_pipe_read(const int fd, char **out_buf, ssize_t *out_len)
{
	char *ptr = NULL;
	char buffer[8192];
	ssize_t size = 0;
	*out_len = 0;

	ptr = NULL;

	for (;;) {
		ssize_t n = read(fd, buffer, sizeof(buffer));
		char *tmp;

		if (n == 0)
			break;
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				free(ptr);
				return -1;
			}
			continue;
		}

		if ((tmp = realloc(ptr, size + n + 1)) == NULL) {
			free(ptr);
			return -1;
		}
		ptr = tmp;
		memcpy(ptr + size, buffer, n);
		size += n;
		*(ptr+size) = 0;
	}
	*out_len = size;
	*out_buf = ptr;
	return 0;
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
	int	rc, fd;
	ssize_t	len;
	char 	*text;

	if (fwts_pipe_open_ro(command, &pid, &fd) < 0)
		return FWTS_ERROR;

	rc = fwts_pipe_read(fd, &text, &len);
	if (!rc && len > 0) {
		*list = fwts_list_from_text(text);
		free(text);
	} else {
		list = NULL;
	}

	*status = fwts_pipe_close(fd, pid);
	if (rc || *status) {
		if (*list)
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

	if (fwts_pipe_open_ro(command, &pid, &fd) < 0)
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

