/*
 * Copyright (C) 2012-2014 Canonical
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fwts_pipeio.h"

static bool module_already_loaded = false;

static int check_module_loaded(void)
{
	FILE *fp;

	module_already_loaded = false;
	if ((fp = fopen("/proc/modules", "r")) != NULL) {
		char buffer[1024];

		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
			if (strstr(buffer, "efi_runtime") != NULL) {
				module_already_loaded = true;
				break;
			}
		}
		fclose(fp);
		return FWTS_OK;
	}
	return FWTS_ERROR;
}

int fwts_lib_efi_runtime_load_module(fwts_framework *fw)
{
	struct stat statbuf;

	if (check_module_loaded() != FWTS_OK) {
		fwts_log_error(fw, "Could not open /proc/modules for checking module loaded.");
		return FWTS_ERROR;
	}

	if (!module_already_loaded) {
		int status;

		if (fwts_exec("modprobe efi_runtime", &status) != FWTS_OK) {
			fwts_log_error(fw, "Load efi_runtime module error.");
			return FWTS_ERROR;
		} else {
			(void)check_module_loaded();
			if (!module_already_loaded) {
				fwts_log_error(fw, "Could not load efi_runtime module.");
				return FWTS_ERROR;
			}
		}
	}

	if (stat("/dev/efi_runtime", &statbuf)) {
		fwts_log_error(fw, "Loaded efi_runtime module but /dev/efi_runtime is not present.");
		return FWTS_ERROR;
	}

	if (!S_ISCHR(statbuf.st_mode)) {
		fwts_log_error(fw, "Loaded efi_runtime module but /dev/efi_runtime is not a char device.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}


int fwts_lib_efi_runtime_unload_module(fwts_framework *fw)
{
	if (check_module_loaded() != FWTS_OK) {
		fwts_log_error(fw, "Could not open /proc/modules for checking module loaded.");
		return FWTS_ERROR;
	}
	if (module_already_loaded) {
		int status;

		if (fwts_exec("modprobe -r efi_runtime", &status) != FWTS_OK) {
			fwts_log_error(fw, "Unload efi_runtime module error.");
			return FWTS_ERROR;
		} else {
			(void)check_module_loaded();
			if (module_already_loaded) {
				fwts_log_error(fw, "Could not unload efi_runtime module.");
				return FWTS_ERROR;
			}
		}
	}

	return FWTS_OK;
}
