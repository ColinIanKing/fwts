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
#include <stddef.h>
#include <inttypes.h>

#include "fwts.h"
#include "fwts_uefi.h"

typedef void (*securebootcert_func)(fwts_framework *fw, fwts_uefi_var *var, char *varname);

typedef struct {
	char *description;		/* UEFI var */
	securebootcert_func	func;	/* Function to dump this variable */
} securebootcert_info;

typedef struct {
	uint32_t	Data1;
	uint16_t	Data2;
	uint16_t	Data3;
	uint8_t		Data4[8];
} __attribute__ ((packed)) EFI_GUID;

#define VAR_SECUREBOOT_FOUND	1
#define VAR_SETUPMODE_FOUND	2
#define VAR_DB_FOUND		4
#define VAR_KEK_FOUND		8

#define EFI_GLOBAL_VARIABLE \
{ \
	0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA, 0x0D, 0x00, \
						0xE0, 0x98, 0x03, 0x2B, 0x8C} \
}

static uint8_t var_found;

static bool compare_guid(EFI_GUID *guid1, uint8_t *guid2)
{
	bool ident = true;
	int i;
	uint32_t data1 = guid2[0] + (guid2[1] << 8) + (guid2[2] << 16) + (guid2[3] << 24);
	uint16_t data2 = guid2[4] + (guid2[5] << 8);
	uint16_t data3 = guid2[6] + (guid2[7] << 8);

	if ((guid1->Data1 != data1) || (guid1->Data2 != data2) || (guid1->Data3 != data3))
		ident = false;
	else {
		for (i = 0; i < 8; i++) {
			if (guid1->Data4[i] != guid2[i+8])
				ident = false;
		}
	}
	return ident;
}

static void securebootcert_secure_boot(fwts_framework *fw, fwts_uefi_var *var, char *varname)
{
	bool ident = false;
	EFI_GUID global_var_guid = EFI_GLOBAL_VARIABLE;

	if (strcmp(varname, "SecureBoot"))
		return;

	var_found |= VAR_SECUREBOOT_FOUND;
	ident = compare_guid(&global_var_guid, var->guid);

	if (!ident) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableGUIDInvalid",
			"The secure boot variable %s GUID invalid.", varname);
		return;
	}
	if (var->datalen != 1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableSizeInvalid",
			"The secure boot variable %s size invalid.", varname);
		return;
	} else {
		char *mode;
		uint8_t value = (uint8_t)var->data[0];

		switch (value) {
		case 0:
			mode = " (Secure Boot Mode Off)";
			break;
		case 1:
			mode = " (Secure Boot Mode On)";
			break;
		default:
			fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableDataInvalid",
				"The secure boot variable data invalid.");
			return;
		}
		fwts_log_info_verbatum(fw, "  Value: 0x%2.2x%s.", value, mode);
		fwts_passed(fw, "Secure boot relative variable %s check passed.", varname);
	}
}

static void securebootcert_setup_mode(fwts_framework *fw, fwts_uefi_var *var, char *varname)
{

	bool ident = false;
	EFI_GUID global_var_guid = EFI_GLOBAL_VARIABLE;

	if (strcmp(varname, "SetupMode"))
		return;

	var_found |= VAR_SETUPMODE_FOUND;
	ident = compare_guid(&global_var_guid, var->guid);

	if (!ident) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableGUIDInvalid",
			"The secure boot variable %s GUID invalid.", varname);
		return;
	}
	if (var->datalen != 1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableSizeInvalid",
			"The secure boot variable %s size invalid.", varname);
	} else {
		char *mode;
		uint8_t value = (uint8_t)var->data[0];

		switch (value) {
		case 0:
			mode = " (User Mode)";
			break;
		case 1:
			mode = " (Setup Mode)";
			break;
		default:
			fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableDataInvalid",
				"The secure boot variable %s data invalid.", varname);
			return;
		}
		fwts_log_info_verbatum(fw, "  Value: 0x%2.2" PRIx8 "%s.", value, mode);
		fwts_passed(fw, "Secure boot relative variable %s check passed.", varname);
	}
}

static securebootcert_info securebootcert_info_table[] = {
	{ "SecureBoot",		securebootcert_secure_boot },
	{ "SetupMode",		securebootcert_setup_mode },
	{ NULL, NULL }
};

static char *securebootcert_attribute(uint32_t attr)
{
	static char str[100];

	*str = 0;

	if (attr & FWTS_UEFI_VAR_NON_VOLATILE)
		strcat(str, "NonVolatile");

	if (attr & FWTS_UEFI_VAR_BOOTSERVICE_ACCESS) {
		if (*str)
			strcat(str, ",");
		strcat(str, "BootServ");
	}

	if (attr & FWTS_UEFI_VAR_RUNTIME_ACCESS) {
		if (*str)
			strcat(str, ",");
		strcat(str, "RunTime");
	}

	return str;
}

static void securebootcert_var(fwts_framework *fw, fwts_uefi_var *var)
{
	char varname[512];
	char guid_str[37];
	securebootcert_info *info;

	fwts_uefi_get_varname(varname, sizeof(varname), var);

	for (info = securebootcert_info_table; info->description != NULL; info++) {
		if (strcmp(varname, info->description) == 0) {
			fwts_log_info_verbatum(fw, "The %s variable check.", varname);
			fwts_guid_buf_to_str(var->guid, guid_str, sizeof(guid_str));
			fwts_log_info_verbatum(fw, "  GUID: %s", guid_str);
			fwts_log_info_verbatum(fw, "  Attr: 0x%x (%s).", var->attributes,
							securebootcert_attribute(var->attributes));
			info->func(fw, var, varname);
			fwts_log_nl(fw);
			return;
		}
	}
}

static int securebootcert_init(fwts_framework *fw)
{
	if (fwts_firmware_detect() != FWTS_FIRMWARE_UEFI) {
		fwts_log_info(fw, "Cannot detect any UEFI firmware. Aborted.");
		return FWTS_ABORTED;
	}

	return FWTS_OK;
}

static int securebootcert_test1(fwts_framework *fw)
{
	fwts_list name_list;

	if (fwts_uefi_get_variable_names(&name_list) == FWTS_ERROR) {
		fwts_log_info(fw, "Cannot find any UEFI variables.");
	} else {
		fwts_list_link *item;

		fwts_list_foreach(item, &name_list) {
			fwts_uefi_var var;
			char *name = fwts_list_data(char *, item);

			if (fwts_uefi_get_variable(name, &var) == FWTS_OK) {
				securebootcert_var(fw, &var);
				fwts_uefi_free_variable(&var);
			}
		}
	}

	/* check all the secure boot variables be found */
	if (!(var_found & VAR_SECUREBOOT_FOUND))
		fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableNotFound",
			"The secure boot variable SecureBoot not found.");
	if (!(var_found & VAR_SETUPMODE_FOUND))
		fwts_failed(fw, LOG_LEVEL_HIGH, "SecureBootCertVariableNotFound",
			"The secure boot variable SetupMode not found.");

	fwts_uefi_free_variable_names(&name_list);

	return FWTS_OK;
}

static fwts_framework_minor_test securebootcert_tests[] = {
	{ securebootcert_test1, "Ubuntu UEFI secure boot test." },
	{ NULL, NULL }
};

static fwts_framework_ops securebootcert_ops = {
	.description = "Ubuntu UEFI secure boot test.",
	.init        = securebootcert_init,
	.minor_tests = securebootcert_tests
};

FWTS_REGISTER("securebootcert", &securebootcert_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);
