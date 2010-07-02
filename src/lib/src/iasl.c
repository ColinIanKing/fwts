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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

#include "fwts.h"

fwts_list *fwts_iasl_disassemble(fwts_framework *fw, char *table, int which)
{
	char tmpbuf[PATH_MAX+128];
	char tmpname[PATH_MAX];
	char cwd[PATH_MAX];
	int len;
	int fd;
	uint8 *data;
	pid_t pid;
	fwts_list *output;

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fwts_log_error(fw, "Cannot get current working directory");
		return NULL;
	}

	if (chdir("/tmp") < 0) {
		fwts_log_error(fw, "Cannot change directory to /tmp");
		return NULL;
	}

	if (fwts_check_executable(fw, fw->iasl, "iasl"))
                return NULL;

	if (fwts_check_executable(fw, fw->acpidump, "acpidump"))
                return NULL;

	if ((data = fwts_acpi_table_load(fw, table, which, &len)) == NULL)
		return NULL;

	snprintf(tmpname, sizeof(tmpname), "tmp_iasl_%d_%s", getpid(), table);
	if ((fd = open(tmpname, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file %s", tmpname);
		free(data);
		return NULL;
	}
	if (write(fd, data, len) != len) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		free(data);
		close(fd);
		return NULL;
	}	
	free(data);
	close(fd);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s -d %s", fw->iasl, tmpname);
	fd = fwts_pipe_open(tmpbuf, &pid);
	if (fd < 0) {
		fwts_log_warning(fw, "exec of %s -d %s (disassemble) failed\n", fw->iasl, tmpname);
	}
	fwts_pipe_close(fd, pid);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s.dsl", tmpname);
	output = fwts_file_open_and_read(tmpbuf);

	snprintf(tmpbuf,sizeof(tmpbuf),"%s.dsl", tmpname);
	unlink(tmpname);
	unlink(tmpbuf);

	if (chdir(cwd) < 0) {
		fwts_log_error(fw, "Cannot change directory to %s", cwd);
	}

	return output;
}

fwts_list* fwts_iasl_reassemble(fwts_framework *fw, uint8 *data, int len)
{
	char tmpbuf[PATH_MAX+128];
	char tmpname[PATH_MAX];
	char cwd[PATH_MAX];
	fwts_list *output;
	int ret;
	int fd;

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fwts_log_error(fw, "Cannot get current working directory");
		return NULL;
	}

	if (chdir("/tmp") < 0) {
		fwts_log_error(fw, "Cannot change directory to /tmp");
		return NULL;
	}

	snprintf(tmpname, sizeof(tmpname), "tmp_iasl_%d", getpid());
	if ((fd = open(tmpname, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file");
		return NULL;
	}
	if (write(fd, data, len) != len) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		close(fd);
		return NULL;
	}	
	close(fd);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s -d %s", fw->iasl, tmpname);
	ret = fwts_pipe_exec(tmpbuf, &output);
	if (ret)
		fwts_log_warning(fw, "exec of %s -d %s (disassemble) returned %d\n", fw->iasl, tmpname, ret);
	if (output)
		fwts_text_list_free(output);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s %s.dsl", fw->iasl, tmpname);
	ret = fwts_pipe_exec(tmpbuf, &output);
	if (ret)
		fwts_log_error(fw, "exec of %s (assemble) returned %d\n", fw->iasl, ret);

	snprintf(tmpbuf,sizeof(tmpbuf),"%s.dsl", tmpname);
	unlink(tmpname);
	unlink(tmpbuf);

	if (chdir(cwd) < 0) {
		fwts_log_error(fw, "Cannot change directory to %s", cwd);
	}

	return output;
}
