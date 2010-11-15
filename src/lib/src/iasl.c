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

int fwts_iasl_disassemble(fwts_framework *fw, const char *tablename, const int which, fwts_list **iasl_output)
{
	char tmpbuf[PATH_MAX+128];
	char tmpname[PATH_MAX];
	char cwd[PATH_MAX];
	int fd;
	pid_t pid;
	fwts_list *output;
	fwts_acpi_table_info *table;

	if (iasl_output == NULL)
		return FWTS_ERROR;

	*iasl_output = NULL;

	if (fwts_acpi_find_table(fw, tablename, which, &table) != FWTS_OK)
		return FWTS_ERROR;

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fwts_log_error(fw, "Cannot get current working directory");
		return FWTS_ERROR;
	}

	if (chdir("/tmp") < 0) {
		fwts_log_error(fw, "Cannot change directory to /tmp");
		return FWTS_ERROR;
	}

	if (fwts_check_executable(fw, fw->iasl, "iasl"))
                return FWTS_ERROR;
		
	if (table == NULL)
		return FWTS_ERROR;

	snprintf(tmpname, sizeof(tmpname), "tmp_iasl_%d_%s", getpid(), tablename);
	if ((fd = open(tmpname, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file %s", tmpname);
		return FWTS_ERROR;
	}
	if (write(fd, table->data, table->length) != table->length) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		close(fd);
		return FWTS_ERROR;
	}	
	close(fd);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s -d %s", fw->iasl, tmpname);
	if ((fd = fwts_pipe_open(tmpbuf, &pid)) < 0) 
		fwts_log_warning(fw, "exec of %s -d %s (disassemble) failed\n", fw->iasl, tmpname);

	fwts_pipe_close(fd, pid);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s.dsl", tmpname);
	output = fwts_file_open_and_read(tmpbuf);

	snprintf(tmpbuf,sizeof(tmpbuf),"%s.dsl", tmpname);
	unlink(tmpname);
	unlink(tmpbuf);

	if (chdir(cwd) < 0) {
		fwts_log_error(fw, "Cannot change directory to %s", cwd);
		fwts_list_free(output, free);
		return FWTS_ERROR;
	}

	*iasl_output = output;

	return FWTS_OK;
}

int fwts_iasl_reassemble(fwts_framework *fw, const uint8_t *data, const int len, fwts_list **iasl_output)
{
	char tmpbuf[PATH_MAX+128];
	char tmpname[PATH_MAX];
	char cwd[PATH_MAX];
	fwts_list *output;
	int ret;
	int fd;

	if (iasl_output == NULL)
		return FWTS_ERROR;

	*iasl_output = NULL;

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fwts_log_error(fw, "Cannot get current working directory");
		return FWTS_ERROR;
	}

	if (chdir("/tmp") < 0) {
		fwts_log_error(fw, "Cannot change directory to /tmp");
		return FWTS_ERROR;
	}

	snprintf(tmpname, sizeof(tmpname), "tmp_iasl_%d", getpid());
	if ((fd = open(tmpname, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) < 0) {
		fwts_log_error(fw, "Cannot create temporary file");
		return FWTS_ERROR;
	}
	if (write(fd, data, len) != len) {
		fwts_log_error(fw, "Cannot write all data to temporary file");
		close(fd);
		return FWTS_ERROR;
	}	
	close(fd);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s -d %s", fw->iasl, tmpname);
	if ((ret = fwts_pipe_exec(tmpbuf, &output)))
		fwts_log_warning(fw, "exec of %s -d %s (disassemble) returned %d\n", fw->iasl, tmpname, ret);
	if (output)
		fwts_text_list_free(output);

	snprintf(tmpbuf, sizeof(tmpbuf), "%s %s.dsl", fw->iasl, tmpname);
	if ((ret = fwts_pipe_exec(tmpbuf, &output)))
		fwts_log_error(fw, "exec of %s (assemble) returned %d\n", fw->iasl, ret);

	snprintf(tmpbuf,sizeof(tmpbuf),"%s.dsl", tmpname);
	unlink(tmpname);
	unlink(tmpbuf);

	if (chdir(cwd) < 0) {
		fwts_log_error(fw, "Cannot change directory to %s", cwd);
		fwts_list_free(output, free);
		return FWTS_ERROR;
	}
	
	*iasl_output = output;

	return FWTS_OK;
}
