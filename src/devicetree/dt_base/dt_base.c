/*
 * Copyright (C) 2014-2016 Jeremy Kerr <jk@ozlabs.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>

#include <libfdt.h>

#include "fwts.h"

static int dt_base_check_present(fwts_framework *fw)
{
	if (fw->fdt == NULL) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "DeviceTreeBaseAbsent",
				"No device tree could be loaded");
		return FWTS_ERROR;
	}

	fwts_passed(fw, "Device tree present");
	return FWTS_OK;
}

static int dt_base_check_valid(fwts_framework *fw)
{
	int rc;

	if (fw->fdt == NULL)
		return FWTS_ABORTED;

	rc = fdt_check_header(fw->fdt);
	if (rc != 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "DeviceTreeBaseInvalid",
				"Device tree data is invalid");
		return FWTS_ERROR;
	}

	fwts_passed(fw, "Device tree data valid");
	return FWTS_OK;
}

static int dt_base_check_warnings(fwts_framework *fw)
{
	int rc, status, in_fd, out_fd;
	ssize_t in_len, out_len;
	const char *command;
	char *output;
	pid_t pid;

	if (!fw->fdt)
		return FWTS_ABORTED;

	command = "dtc -I dtb -O dtb -o /dev/null 2>&1";
	rc = fwts_pipe_open_rw(command, &pid, &in_fd, &out_fd);
	if (rc)
		return FWTS_ABORTED;

	in_len = fdt_totalsize(fw->fdt);

	rc = fwts_pipe_readwrite(in_fd, fw->fdt, in_len,
			out_fd, &output, &out_len);
	if (rc) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "DeviceTreeBaseDTCPipe",
				"failed to pipe data to dtc");
		fwts_pipe_close2(in_fd, out_fd, pid);
		return FWTS_ERROR;
	}

	status = fwts_pipe_close2(in_fd, out_fd, pid);

	if (status) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "DeviceTreeBaseDTCFailed",
				"dtc reports fatal device tree errors:\n%s\n",
				output);
		return FWTS_ERROR;
	}

	if (out_len > 0) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "DeviceTreeBaseDTCWarnings",
				"dtc reports warnings from device tree:\n%s\n",
				output);
		return FWTS_ERROR;
	}

	fwts_passed(fw, "No warnings from dtc");
	return FWTS_OK;
}

static fwts_framework_minor_test dt_base_tests[] = {
	{ dt_base_check_present,  "Check device tree presence" },
	{ dt_base_check_valid,    "Check device tree baseline validity" },
	{ dt_base_check_warnings, "Check device tree warnings" },
	{ NULL, NULL },
};

static fwts_framework_ops dt_base_ops = {
	.description	= "Base device tree validity check",
	.minor_tests	= dt_base_tests,
};

FWTS_REGISTER_FEATURES("dt_base", &dt_base_ops, FWTS_TEST_ANYTIME,
		FWTS_FLAG_BATCH, FWTS_FW_FEATURE_DEVICETREE);
