/*
 * Copyright (C) 2013 Canonical
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

#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "fwts.h"
#include "fwts_uefi.h"
#include "efi_runtime.h"
#include "fwts_efi_module.h"

static int fd;

static int uefirtmisc_init(fwts_framework *fw)
{
	if (fwts_firmware_detect() != FWTS_FIRMWARE_UEFI) {
		fwts_log_info(fw, "Cannot detect any UEFI firmware. Aborted.");
		return FWTS_ABORTED;
	}

	if (fwts_lib_efi_runtime_load_module(fw) != FWTS_OK) {
		fwts_log_info(fw, "Cannot load efi_runtime module. Aborted.");
		return FWTS_ABORTED;
	}

	fd = open("/dev/efi_runtime", O_RDONLY);
	if (fd == -1) {
		fwts_log_info(fw, "Cannot open efi_runtime driver. Aborted.");
		return FWTS_ABORTED;
	}

	return FWTS_OK;
}

static int uefirtmisc_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);

	close(fd);
	fwts_lib_efi_runtime_unload_module(fw);

	return FWTS_OK;
}

static int getnexthighmonotoniccount_test(fwts_framework *fw, uint32_t multitesttime)
{
	long ioret;
	uint64_t status;
	struct efi_getnexthighmonotoniccount getnexthighmonotoniccount;
	uint32_t highcount;
	uint32_t i;

	getnexthighmonotoniccount.HighCount = &highcount;
	getnexthighmonotoniccount.status = &status;

	for (i = 0; i < multitesttime; i++) {
		ioret = ioctl(fd, EFI_RUNTIME_GET_NEXTHIGHMONOTONICCOUNT, &getnexthighmonotoniccount);
		if (ioret == -1) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetNextHighMonotonicCount",
				"Failed to get high monotonic count with UEFI runtime service.");
			fwts_uefi_print_status_info(fw, status);
			return FWTS_ERROR;
		}
	}

	return FWTS_OK;
}

static int uefirtmisc_test1(fwts_framework *fw)
{
	int ret;
	uint32_t multitesttime = 1;

	fwts_log_info(fw, "Testing UEFI runtime service GetNextHighMonotonicCount interface.");
	ret = getnexthighmonotoniccount_test(fw, multitesttime);
	if (ret != FWTS_OK)
		return ret;

	fwts_passed(fw, "UEFI runtime service GetNextHighMonotonicCount interface test passed.");

	return FWTS_OK;
}

static fwts_framework_minor_test uefirtmisc_tests[] = {
	{ uefirtmisc_test1, "Test for UEFI miscellaneous runtime service interfaces." },
	{ NULL, NULL }
};

static fwts_framework_ops uefirtmisc_ops = {
	.description = "UEFI miscellaneous runtime service interface tests.",
	.init        = uefirtmisc_init,
	.deinit      = uefirtmisc_deinit,
	.minor_tests = uefirtmisc_tests
};

FWTS_REGISTER("uefirtmisc", &uefirtmisc_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UNSAFE | FWTS_FLAG_ROOT_PRIV);
