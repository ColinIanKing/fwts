/*
 * Copyright (C) 2016 IBM Corporation
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

#include <stdio.h>

#include "fwts.h"

int fwts_devicetree_read(fwts_framework *fwts)
{
	char *command, *data = NULL;
	int fd, rc, status;
	ssize_t len;
	pid_t pid;

	if (!fwts_firmware_has_features(FWTS_FW_FEATURE_DEVICETREE))
		return FWTS_OK;

	rc = asprintf(&command, "dtc -I fs -O dtb %s", DT_FS_PATH);
	if (rc < 0)
		return FWTS_ERROR;

	rc = fwts_pipe_open_ro(command, &pid, &fd);
	if (rc < 0) {
		free(command);
		return FWTS_ERROR;
	}
	free(command);

	rc = fwts_pipe_read(fd, &data, &len);
	if (rc) {
		fwts_pipe_close(fd, pid);
		return FWTS_ERROR;
	}

	status = fwts_pipe_close(fd, pid);

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || len == 0) {
		fprintf(stderr, "Cannot read devicetree data: dtc failed\n");
		free(data);
		return FWTS_ERROR;
	}

	fwts->fdt = data;

	return FWTS_OK;
}

