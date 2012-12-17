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

#define TEST_GUID2 \
{ \
	0xBD6D18A3, 0xEE9E, 0x445E, {0xA8, 0x1B, 0xD3, \
						0xDD, 0xB7, 0x11, 0xD0, 0x6E} \
}

#define MAX_DATA_LENGTH		1024

static int fd;
EFI_GUID gtestguid1 = TEST_GUID1;
EFI_GUID gtestguid2 = TEST_GUID2;

uint32_t attributes = FWTS_UEFI_VAR_NON_VOLATILE | FWTS_UEFI_VAR_BOOTSERVICE_ACCESS | FWTS_UEFI_VAR_RUNTIME_ACCESS;
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

static int getvariable_test(fwts_framework *fw, uint64_t datasize, uint16_t *varname)
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
	data[dataindex] = '\0';

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
		fwts_uefi_print_status_info(fw, status);
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
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (*getvariable.status != EFI_SUCCESS) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariableStatus",
			"Failed to get variable, return status isn't EFI_SUCCESS.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	} else if (*getvariable.Attributes != attributes) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariableAttributes",
			"Failed to get variable with right attributes, "
			"attributes we got is %" PRIu32
			", but it should be %" PRIu32 ".",
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
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static bool compare_guid(EFI_GUID *guid1, EFI_GUID *guid2)
{
	bool ident = true;
	int i;

	if ((guid1->Data1 != guid2->Data1) || (guid1->Data2 != guid2->Data2) || (guid1->Data3 != guid2->Data3))
		ident = false;
	else {
		for (i = 0; i < 8; i++) {
			if (guid1->Data4[i] != guid2->Data4[i])
				ident = false;
		}
	}
	return ident;
}

static bool compare_name(uint16_t *name1, uint16_t *name2)
{
	bool ident = true;
	int i = 0;

	while (true) {
		if ((name1[i] != name2[i])) {
			ident = false;
			break;
		} else if (name1[i] == '\0')
			break;
		i++;
	}
	return ident;
}

static int getnextvariable_test(fwts_framework *fw)
{
	long ioret;
	uint64_t status;

	struct efi_setvariable setvariable;

	uint64_t dataindex, datasize = 10;
	uint8_t data[MAX_DATA_LENGTH];

	struct efi_getnextvariablename getnextvariablename;
	uint64_t variablenamesize = MAX_DATA_LENGTH;
	uint16_t variablename[MAX_DATA_LENGTH];
	EFI_GUID vendorguid;
	bool found_name = false, found_guid = false;

	for (dataindex = 0; dataindex < datasize; dataindex++)
		data[dataindex] = (uint8_t)dataindex;
	data[dataindex] = '\0';

	setvariable.VariableName = variablenametest;
	setvariable.VendorGuid = &gtestguid1;
	setvariable.Attributes = attributes;
	setvariable.DataSize = datasize;
	setvariable.Data = data;
	setvariable.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariable",
			"Failed to set variable with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	getnextvariablename.VariableNameSize = &variablenamesize;
	getnextvariablename.VariableName = variablename;
	getnextvariablename.VendorGuid = &vendorguid;
	getnextvariablename.status = &status;

	/* To start the search, need to pass a Null-terminated string in VariableName */
	variablename[0] = '\0';
	while (true) {
		variablenamesize = MAX_DATA_LENGTH;
		ioret = ioctl(fd, EFI_RUNTIME_GET_NEXTVARIABLENAME, &getnextvariablename);

		if (ioret == -1) {

			/* no next variable was found*/
			if (*getnextvariablename.status == EFI_NOT_FOUND)
				break;

			fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetNextVariableName",
				"Failed to get next variable name with UEFI runtime service.");
			fwts_uefi_print_status_info(fw, status);
			return FWTS_ERROR;
		}
		if (compare_name(getnextvariablename.VariableName, variablenametest))
			found_name = true;
		if (compare_guid(getnextvariablename.VendorGuid, &gtestguid1))
			found_guid = true;
		if (found_name && found_guid)
			break;
	};

	if (!found_name) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetNextVariableNameName",
			"Failed to get next variable name with right name.");
		return FWTS_ERROR;
	}
	if (!found_guid) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetNextVariableNameGuid",
			"Failed to get next variable name correct guid.");
		return FWTS_ERROR;
	}

	/* delete the variable */
	setvariable.DataSize = 0;

	ioret = ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariable",
			"Failed to set variable with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int setvariable_insertvariable(fwts_framework *fw, uint32_t attributes, uint64_t datasize,
					uint16_t *varname, EFI_GUID *gtestguid, uint8_t datadiff)
{
	long ioret;
	struct efi_setvariable setvariable;

	uint64_t status;
	uint64_t dataindex;

	uint8_t data[datasize+1];

	for (dataindex = 0; dataindex < datasize; dataindex++)
		data[dataindex] = (uint8_t)dataindex + datadiff;
	data[dataindex] = '\0';

	setvariable.VariableName = varname;
	setvariable.VendorGuid = gtestguid;
	setvariable.Attributes = attributes;
	setvariable.DataSize = datasize;
	setvariable.Data = data;
	setvariable.status = &status;
	ioret = ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariable",
			"Failed to set variable with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static int setvariable_checkvariable(fwts_framework *fw, uint64_t datasize,
					uint16_t *varname, EFI_GUID *gtestguid, uint8_t datadiff)
{
	long ioret;
	struct efi_getvariable getvariable;

	uint64_t status;
	uint8_t testdata[datasize+1];
	uint64_t dataindex;
	uint64_t getdatasize;
	uint32_t attributestest;

	getvariable.VariableName = varname;
	getvariable.VendorGuid = gtestguid;
	getvariable.Attributes = &attributestest;
	getvariable.DataSize = &getdatasize;
	getvariable.Data = testdata;
	getvariable.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_VARIABLE, &getvariable);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetVariable",
			"Failed to get variable with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (*getvariable.Attributes != attributes) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariableAttributes",
			"Failed to set variable with right attributes, "
			"attributes we got is %" PRIu32
			", but it should both be %" PRIu32 ".",
			attributestest, attributes);
		return FWTS_ERROR;
	} else if (*getvariable.DataSize != datasize) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariableDataSize",
			"Failed to set variable with correct datasize.");
		return FWTS_ERROR;
	} else {
		for (dataindex = 0; dataindex < datasize; dataindex++) {
			if (testdata[dataindex] != ((uint8_t)dataindex + datadiff)) {
				fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariableData",
					"Failed to set variable with correct data.");
				return FWTS_ERROR;
			}
		}
	}
	return FWTS_OK;
}

static int setvariable_checkvariable_notfound(fwts_framework *fw, uint16_t *varname, EFI_GUID *gtestguid)
{
	long ioret;
	struct efi_getvariable getvariable;

	uint64_t status;
	uint8_t testdata[MAX_DATA_LENGTH];
	uint64_t getdatasize;
	uint32_t attributestest;

	getvariable.VariableName = varname;
	getvariable.VendorGuid = gtestguid;
	getvariable.Attributes = &attributestest;
	getvariable.DataSize = &getdatasize;
	getvariable.Data = testdata;
	getvariable.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_VARIABLE, &getvariable);

	/* expect the uefi runtime interface return EFI_NOT_FOUND */
	if (ioret == -1) {
		if (*getvariable.status == EFI_NOT_FOUND)
			return FWTS_OK;
	}
	fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetVariable",
		"Failed to set variable with UEFI runtime service., "
		"expect the status return EFI_NOT_FOUND.");
	fwts_uefi_print_status_info(fw, status);
	return FWTS_ERROR;
}

static int setvariable_invalidattr(fwts_framework *fw, uint32_t attributes, uint64_t datasize,
					uint16_t *varname, EFI_GUID *gtestguid, uint8_t datadiff)
{
	struct efi_setvariable setvariable;
	uint64_t status;
	uint64_t dataindex;
	uint8_t data[datasize+1];

	for (dataindex = 0; dataindex < datasize; dataindex++)
		data[dataindex] = (uint8_t)dataindex + datadiff;
	data[dataindex] = '\0';

	setvariable.VariableName = varname;
	setvariable.VendorGuid = gtestguid;
	setvariable.Attributes = attributes;
	setvariable.DataSize = datasize;
	setvariable.Data = data;
	setvariable.status = &status;

	ioctl(fd, EFI_RUNTIME_SET_VARIABLE, &setvariable);

	if (status == EFI_SUCCESS) {
		fwts_warning(fw, "After ExitBootServices() is performed, the attributes %" PRIu32 ", "
			"for SetVariable shouldn't be set successfully.", attributes);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static int setvariable_test1(fwts_framework *fw, uint64_t datasize1,
							uint64_t datasize2, uint16_t *varname)
{
	uint8_t datadiff_g2 = 2, datadiff_g1 = 0;

	if (setvariable_insertvariable(fw, attributes, datasize2, varname,
					&gtestguid2, datadiff_g2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, datasize1, varname,
					&gtestguid1, datadiff_g1) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize2, varname,
					&gtestguid2, datadiff_g2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize1, varname,
					&gtestguid1, datadiff_g1) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, varname,
					&gtestguid2, datadiff_g2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, varname,
					&gtestguid1, datadiff_g1) == FWTS_ERROR)
		return FWTS_ERROR;

	return FWTS_OK;
}

static int setvariable_test2(fwts_framework *fw, uint16_t *varname)
{
	uint64_t datasize = 10;
	uint8_t datadiff1 = 0, datadiff2 = 2, datadiff3 = 4;

	if (setvariable_insertvariable(fw, attributes, datasize, varname,
					&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;

	/* insert the same data */
	if (setvariable_insertvariable(fw, attributes, datasize, varname,
					&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize, varname,
					&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, varname,
					&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;


	/* insert different data */
	datasize = 20;

	if (setvariable_insertvariable(fw, attributes, datasize, varname,
					&gtestguid1, datadiff2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize, varname,
					&gtestguid1, datadiff2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, varname,
					&gtestguid1, datadiff2) == FWTS_ERROR)
		return FWTS_ERROR;

	datasize = 5;

	if (setvariable_insertvariable(fw, attributes, datasize, varname,
					&gtestguid1, datadiff3) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize, varname,
					&gtestguid1, datadiff3) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, varname,
					&gtestguid1, datadiff3) == FWTS_ERROR)
		return FWTS_ERROR;

	return FWTS_OK;
}

static int setvariable_test3(fwts_framework *fw)
{
	uint64_t datasize = 10;
	uint8_t datadiff1 = 0, datadiff2 = 1, datadiff3 = 2;
	uint16_t variablenametest2[] = {'T', 'e', 's', 't', 'v', 'a', 'r', ' ', '\0'};
	uint16_t variablenametest3[] = {'T', 'e', 's', 't', 'v', 'a', '\0'};

	if (setvariable_insertvariable(fw, attributes, datasize, variablenametest2,
						&gtestguid1, datadiff2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, datasize, variablenametest3,
						&gtestguid1, datadiff3) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, datasize, variablenametest,
						&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize, variablenametest2,
						&gtestguid1, datadiff2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize, variablenametest3,
						&gtestguid1, datadiff3) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable(fw, datasize, variablenametest,
						&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, variablenametest2,
						&gtestguid1, datadiff2) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, variablenametest3,
						&gtestguid1, datadiff3) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, variablenametest,
						&gtestguid1, datadiff1) == FWTS_ERROR)
		return FWTS_ERROR;

	return FWTS_OK;
}

static int setvariable_test4(fwts_framework *fw)
{
	uint64_t datasize = 10;
	uint8_t datadiff = 0;

	if (setvariable_insertvariable(fw, attributes, datasize, variablenametest,
						&gtestguid1, datadiff) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, attributes, 0, variablenametest,
						&gtestguid1, datadiff) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable_notfound(fw, variablenametest, &gtestguid1) == FWTS_ERROR)
		return FWTS_ERROR;

	return FWTS_OK;
}

static int setvariable_test5(fwts_framework *fw)
{
	uint64_t datasize = 10;
	uint8_t datadiff = 0;

	if (setvariable_insertvariable(fw, attributes, datasize, variablenametest,
							&gtestguid1, datadiff) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_insertvariable(fw, 0, datasize, variablenametest,
							&gtestguid1, datadiff) == FWTS_ERROR)
		return FWTS_ERROR;

	if (setvariable_checkvariable_notfound(fw, variablenametest, &gtestguid1) == FWTS_ERROR)
		return FWTS_ERROR;

	return FWTS_OK;
}

static int setvariable_test6(fwts_framework *fw)
{
	uint64_t datasize = 10;
	uint8_t datadiff = 0;
	uint32_t attributesarray[] = {  FWTS_UEFI_VAR_BOOTSERVICE_ACCESS,
					FWTS_UEFI_VAR_NON_VOLATILE | FWTS_UEFI_VAR_BOOTSERVICE_ACCESS,
					FWTS_UEFI_VAR_BOOTSERVICE_ACCESS | FWTS_UEFI_VAR_RUNTIME_ACCESS };
	uint64_t index;

	for (index = 0; index < (sizeof(attributesarray)/(sizeof attributesarray[0])); index++) {
		setvariable_invalidattr(fw, attributesarray[index], datasize, variablenametest, &gtestguid1, datadiff);

		if (setvariable_checkvariable_notfound(fw, variablenametest, &gtestguid1) == FWTS_ERROR) {
			fwts_log_info(fw, "Get the variable which is set by SetVariable with invalid attribute %"
				PRIu32 " after ExitBootServices() is performed, "
				"test failed.", attributesarray[index]);
			setvariable_insertvariable(fw, 0, datasize, variablenametest, &gtestguid1, datadiff);
			return FWTS_ERROR;
		}
	}

	return FWTS_OK;
}

static int uefirtvariable_test1(fwts_framework *fw)
{
	uint64_t datasize = 10;

	if (getvariable_test(fw, datasize, variablenametest) == FWTS_ERROR)
		return FWTS_ERROR;

	fwts_passed(fw, "UEFI runtime service GetVariable interface test passed.");

	return FWTS_OK;
}

static int uefirtvariable_test2(fwts_framework *fw)
{
	if (getnextvariable_test(fw) == FWTS_ERROR)
		return FWTS_ERROR;

	fwts_passed(fw, "UEFI runtime service GetNextVariableName interface test passed.");

	return FWTS_OK;
}

static int uefirtvariable_test3(fwts_framework *fw)
{
	uint64_t datasize1 = 10, datasize2 = 20;

	fwts_log_info(fw, "Testing SetVariable on two different GUIDs and the same variable name.");
	if (setvariable_test1(fw, datasize1, datasize2, variablenametest) == FWTS_ERROR)
		return FWTS_ERROR;
	fwts_passed(fw, "SetVariable on two different GUIDs and the same variable name passed.");

	fwts_log_info(fw, "Testing SetVariable on the same and different variable data.");
	if (setvariable_test2(fw, variablenametest) == FWTS_ERROR)
		return FWTS_ERROR;
	fwts_passed(fw, "SetVariable on the same and different variable data passed.");

	fwts_log_info(fw, "Testing SetVariable on similar variable name.");
	if (setvariable_test3(fw) == FWTS_ERROR)
		return FWTS_ERROR;
	fwts_passed(fw, "SetVariable on similar variable name passed.");

	fwts_log_info(fw, "Testing SetVariable on DataSize is 0.");
	if (setvariable_test4(fw) == FWTS_ERROR)
		return FWTS_ERROR;
	fwts_passed(fw, "SetVariable on DataSize is 0 passed.");

	fwts_log_info(fw, "Testing SetVariable on Attributes is 0.");
	if (setvariable_test5(fw) == FWTS_ERROR)
		return FWTS_ERROR;
	fwts_passed(fw, "SetVariable on Attributes is 0 passed.");

	fwts_log_info(fw, "Testing SetVariable on Invalid Attributes.");
	if (setvariable_test6(fw) == FWTS_ERROR)
		return FWTS_ERROR;
	fwts_passed(fw, "SetVariable on Invalid Attributes passed.");

	return FWTS_OK;
}

static fwts_framework_minor_test uefirtvariable_tests[] = {
	{ uefirtvariable_test1, "Test UEFI RT service get variable interface." },
	{ uefirtvariable_test2, "Test UEFI RT service get next variable name interface." },
	{ uefirtvariable_test3, "Test UEFI RT service set variable interface." },
	{ NULL, NULL }
};

static fwts_framework_ops uefirtvariable_ops = {
	.description = "UEFI Runtime service variable interface tests.",
	.init        = uefirtvariable_init,
	.deinit      = uefirtvariable_deinit,
	.minor_tests = uefirtvariable_tests
};

FWTS_REGISTER(uefirtvariable, &uefirtvariable_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UNSAFE | FWTS_FLAG_ROOT_PRIV);
