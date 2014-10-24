/*
 * Copyright (C) 2014 Canonical
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
#include "authvardefs.h"

static int fd;

#define TEST_GUID {0x7f5c5d52, 0x2f14, 0x4f12, {0x96, 0x7c, 0xdb, 0x60, 0xdb, 0x05, 0xa0, 0xfd} }

#define getvar_buf_size 100

static EFI_GUID gtestguid = TEST_GUID;

static const uint32_t attributes =
	FWTS_UEFI_VAR_NON_VOLATILE |
	FWTS_UEFI_VAR_BOOTSERVICE_ACCESS |
	FWTS_UEFI_VAR_RUNTIME_ACCESS |
	FWTS_UEFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

static uint16_t variablenametest[] = {'A', 'u', 't', 'h', 'V', 'a', 'r', 'T', 'e', 's', 't', '\0'};

static long setvar(
	EFI_GUID *guid,
	uint32_t attributes,
	uint64_t datasize,
	void *data,
	uint64_t *status)
{
	long ioret;
	struct efi_setvariable setvariable;

	setvariable.VariableName = variablenametest;
	setvariable.VendorGuid = guid;
	setvariable.Attributes = attributes;
	setvariable.DataSize = datasize;
	setvariable.Data = data;
	setvariable.status = status;
	ioret = ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	return ioret;
}

static long getvar(
	EFI_GUID *guid,
	uint32_t *attributestest,
	uint64_t *getdatasize,
	void *data,
	uint64_t *status)
{
	long ioret;
	struct efi_getvariable getvariable;

	getvariable.VariableName = variablenametest;
	getvariable.VendorGuid = guid;
	getvariable.Attributes = attributestest;
	getvariable.DataSize = getdatasize;
	getvariable.Data = data;
	getvariable.status = status;
	ioret = ioctl(fd, EFI_RUNTIME_GET_VARIABLE, &getvariable);

	return ioret;
}

static void uefirtvariable_env_cleanup(void)
{
	uint64_t status;
	uint8_t data[getvar_buf_size];
	uint64_t getdatasize = sizeof(data);
	uint32_t attributestest;

	long ioret = getvar(&gtestguid, &attributestest, &getdatasize, data, &status);
	if (ioret != -1 && status == EFI_SUCCESS)
		setvar(&gtestguid, attributes, sizeof(AuthVarDel), AuthVarDel, &status);
	return;
}

static int uefirtauthvar_init(fwts_framework *fw)
{
	if (fwts_firmware_detect() != FWTS_FIRMWARE_UEFI) {
		fwts_log_info(fw, "Cannot detect any UEFI firmware. Aborted.");
		return FWTS_ABORTED;
	}

	if (fwts_lib_efi_runtime_load_module(fw) != FWTS_OK) {
		fwts_log_info(fw, "Cannot load efi_runtime module. Aborted.");
		return FWTS_ABORTED;
	}

	fd = open("/dev/efi_runtime", O_WRONLY | O_RDWR);
	if (fd == -1) {
		fwts_log_info(fw, "Cannot open efi_runtime driver. Aborted.");
		return FWTS_ABORTED;
	}

	uefirtvariable_env_cleanup();

	return FWTS_OK;
}

static int uefirtauthvar_deinit(fwts_framework *fw)
{
	close(fd);
	fwts_lib_efi_runtime_unload_module(fw);

	return FWTS_OK;
}

static int check_fw_support(fwts_framework *fw, uint64_t status)
{
	if ((status == EFI_INVALID_PARAMETER) &&
		((attributes & FWTS_UEFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) ||
		(attributes & FWTS_UEFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) ||
		(attributes & FWTS_UEFI_VARIABLE_APPEND_WRITE))) {
		fwts_uefi_print_status_info(fw, status);
		fwts_skipped(fw,
			"EFI_INVALID_PARAMETER shall be returned, "
			"when firmware doesn't support these operations "
			"with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, "
			"EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS "
			"or EFI_VARIABLE_APPEND_WRITE attributes is set.");
		return FWTS_SKIP;
	}
	if (status == EFI_OUT_OF_RESOURCES) {
		fwts_uefi_print_status_info(fw, status);
		fwts_skipped(fw,
			"Run out of resources for SetVariable "
			"UEFI runtime interface: cannot test.");
		fwts_advice(fw,
			"Firmware may reclaim some resources "
			"after rebooting. Reboot and test "
			"again may be helpful to continue "
			"the test.");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 * Set the created authenticated variable, AuthVarCreate,
 * and checking the data size and data.
 * expect EFI_SUCCESS returned.
 */
static int uefirtauthvar_test1(fwts_framework *fw)
{
	long ioret;

	uint8_t data[getvar_buf_size];
	uint64_t getdatasize = sizeof(data);
	uint64_t status;
	uint32_t attributestest;
	size_t i;
	int supcheck;

	ioret = setvar(&gtestguid, attributes, sizeof(AuthVarCreate), AuthVarCreate, &status);

	if (ioret == -1) {
		supcheck = check_fw_support(fw, status);
		if (supcheck != FWTS_OK)
			return supcheck;

		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFICreateAuthVar",
			"Failed to create authenticated variable with UEFI "
			"runtime service.");

		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	ioret = getvar(&gtestguid, &attributestest, &getdatasize, data, &status);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFICreateAuthVar",
			"Failed to get authenticated variable with UEFI "
			"runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}
	if (getdatasize != sizeof(AuthVarCreateData)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFICreateAuthVar",
			"Get authenticated variable data size is not the "
			"same as it set.");
	}
	for (i = 0; i < sizeof(AuthVarCreateData); i++) {
		if (data[i] != AuthVarCreateData[i]) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFICreateAuthVar",
			"Get authenticated variable data are not the "
			"same as it set.");
			return FWTS_ERROR;
		}
	}

	fwts_passed(fw, "Create authenticated variable test passed.");

	return FWTS_OK;
}

static fwts_framework_minor_test uefirtauthvar_tests[] = {
	{ uefirtauthvar_test1, "Create authenticated variable test." },
	{ NULL, NULL }
};

static fwts_framework_ops uefirtauthvar_ops = {
	.description     = "Authenticated variable tests.",
	.init            = uefirtauthvar_init,
	.deinit          = uefirtauthvar_deinit,
	.minor_tests     = uefirtauthvar_tests,
};

FWTS_REGISTER("uefirtauthvar", &uefirtauthvar_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_TEST_UEFI | FWTS_FLAG_UNSAFE | FWTS_FLAG_ROOT_PRIV)
