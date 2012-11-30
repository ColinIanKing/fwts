/*
 * Copyright (C) 2012 Canonical
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

#define TEST_GUID1 \
{ \
	0xF6FAB04F, 0xACAF, 0x4af3, {0xB9, 0xFA, 0xDC, \
						0xF9, 0x7F, 0xB4, 0x42, 0x6F} \
}

#define EFI_SUCCESS		0

#define MAX_DATA_LENGTH		1024

static int fd;
EFI_GUID gtestguid1 = TEST_GUID1;
uint32_t attributesarray[] = { FWTS_UEFI_VAR_BOOTSERVICE_ACCESS,
			       FWTS_UEFI_VAR_NON_VOLATILE | FWTS_UEFI_VAR_BOOTSERVICE_ACCESS,
			       FWTS_UEFI_VAR_BOOTSERVICE_ACCESS | FWTS_UEFI_VAR_RUNTIME_ACCESS,
			       FWTS_UEFI_VAR_NON_VOLATILE | FWTS_UEFI_VAR_BOOTSERVICE_ACCESS | FWTS_UEFI_VAR_RUNTIME_ACCESS};

uint16_t variablenametest[] = {'T', 'e', 's', 't', 'v', 'a', 'r', '\0'};

static int uefirtvariable_init(fwts_framework *fw)
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

static int uefirtvariable_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);

	close(fd);
	fwts_lib_efi_runtime_unload_module(fw);

	return FWTS_OK;
}

static int getvariable_test(fwts_framework *fw, uint32_t attributes, uint64_t datasize, uint16_t *varname)
{
	long ioret;
	struct efi_getvariable getvariable;
	struct efi_setvariable setvariable;

	uint64_t status;
	uint8_t testdata[MAX_DATA_LENGTH];
	uint64_t dataindex;
	uint64_t getdatasize;
	uint32_t attributestest;

	uint8_t data[datasize+1];
	for (dataindex = 0; dataindex < datasize; dataindex++)
		data[dataindex] = (uint8_t)dataindex;
	data[dataindex] = '0';

	setvariable.VariableName = varname;
	setvariable.VendorGuid = &gtestguid1;
	setvariable.Attributes = attributes;
	setvariable.DataSize = datasize;
	setvariable.Data = data;
	setvariable.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariable",
			"Failed to set variable with UEFI runtime service.");

		return FWTS_ERROR;
	}

	getvariable.VariableName = varname;
	getvariable.VendorGuid = &gtestguid1;
	getvariable.Attributes = &attributestest;
	getvariable.DataSize = &getdatasize;
	getvariable.Data = testdata;
	getvariable.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_VARIABLE, &getvariable);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariable",
			"Failed to get variable with UEFI runtime service.");
		return FWTS_ERROR;
	}

	if (*getvariable.status != EFI_SUCCESS) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariableStatus",
			"Failed to get variable, return status isn't EFI_SUCCESS.");
		return FWTS_ERROR;
	} else if (*getvariable.Attributes != attributes) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariableAttributes",
			"Failed to get variable with right attributes, "
			"attributes we got is %" PRIu32
			", but it should be %" PRIu32".",
			*getvariable.Attributes, attributes);
	return FWTS_ERROR;
	} else if (*getvariable.DataSize != datasize) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariableDataSize",
			"Failed to get variable with correct datasize.");
		return FWTS_ERROR;
	} else {
		for (dataindex = 0; dataindex < datasize; dataindex++) {
			if (data[dataindex] != (uint8_t)dataindex) {
				fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariableData",
					"Failed to get variable with correct data.");
				return FWTS_ERROR;
			}
		}
	}

	/* delete the variable */
	setvariable.DataSize = 0;

	ioret = ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariable",
			"Failed to set variable with UEFI runtime service.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int uefirtvariable_test1(fwts_framework *fw)
{
	uint64_t index;
	uint64_t datasize = 10;

	for (index = 0; index < (sizeof(attributesarray)/(sizeof attributesarray[0])); index++) {
		if (getvariable_test(fw, attributesarray[index], datasize, variablenametest) == FWTS_ERROR)
			return FWTS_ERROR;
	}

	fwts_passed(fw, "UEFI runtime service GetVariable interface test passed.");

	return FWTS_OK;
}

static fwts_framework_minor_test uefirtvariable_tests[] = {
	{ uefirtvariable_test1, "Test UEFI RT service get variable interface." },
	{ NULL, NULL }
};

static fwts_framework_ops uefirtvariable_ops = {
	.description = "UEFI Runtime service variable interface tests.",
	.init        = uefirtvariable_init,
	.deinit      = uefirtvariable_deinit,
	.minor_tests = uefirtvariable_tests
};

FWTS_REGISTER(uefirtvariable, &uefirtvariable_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UNSAFE | FWTS_FLAG_ROOT_PRIV);
