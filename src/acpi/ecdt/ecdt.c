/*
 * Copyright (C) 2015 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
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
#include "fwts.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "fwts_acpi_object_eval.h"

static fwts_acpi_table_info *table;

static int ecdt_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "ECDT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI ECDT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  See ACPI 6.0, Section 5.2.15
 */
static int ecdt_test1(fwts_framework *fw)
{
	const fwts_acpi_table_ecdt *ecdt = (const fwts_acpi_table_ecdt *)table->data;
	bool passed = true, found_null = false;
	uint32_t min_length;
	int i;


	/* Must be I/O Address Space or a Memory Space */
	if ((ecdt->ec_control.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (ecdt->ec_control.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"ECDTECControlInvalidAddrSpaceID",
			"ECDT EC_CONTROL address space ID is 0x%2.2" PRIx8
			"which is not a System I/O Space ID or a System "
			"Memory Space ID",
			ecdt->ec_control.address_space_id);
		passed = false;
	}
	/* Must be correct Access Size */
	if (ecdt->ec_control.access_width > 4) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"ECDTECControlAccessSize",
			"ECDT EC_CONTROL Access Size is 0x%2.2" PRIx8
			"which is not valid",
			ecdt->ec_control.access_width);
		passed = false;
	}
	/* Must be I/O Address Space or a Memory Space */
	if ((ecdt->ec_data.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (ecdt->ec_data.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"ECDTECControlInvalidAddrSpaceID",
			"ECDT EC_DATA address space ID is 0x%2.2" PRIx8
			"which is not a System I/O Space ID or a System "
			"Memory Space ID",
			ecdt->ec_data.address_space_id);
		passed = false;
	}
	/* Must be correct Access Size */
	if (ecdt->ec_data.access_width > 4) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"ECDTECDataAccessSize",
			"ECDT EC_DATA Access Size is 0x%2.2" PRIx8
			"which is not valid",
			ecdt->ec_data.access_width);
		passed = false;
	}

	/* EC_ID must be at least 1 byte long for the null terminator */
	min_length = (void *)&ecdt->ec_id[0] - (void *)ecdt;

	if (ecdt->header.length < min_length + 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"ECDTBadLength",
			"ECDT is too short to have a valid EC_ID string that is "
			"at least one byte long, length is only %" PRIu32
			" bytes and expecting at least %" PRIu32 "bytes",
			ecdt->header.length,
			min_length);
		passed = false;
		goto done;
	}

	/* Now find if EC_ID has a terminating null char */
	for (i = 0; min_length < ecdt->header.length; i++, min_length++) {
		if (ecdt->ec_id[i] == '\0') {
			found_null = true;
			break;
		}
	}
	if (!found_null) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"ECDTBadECIDString",
			"ECDT EC_ID string is not null terminated");
		passed = false;
		goto done;
	}

	/* Now we have got some sane data, dump the ECDT */
        fwts_log_info_verbatum(fw, "ECDT Embedded Controller Boot Resources Table:");
        fwts_log_info_verbatum(fw, "  EC_CONTROL:");
        fwts_log_info_verbatum(fw, "    Address Space ID:       0x%2.2" PRIx8, ecdt->ec_control.address_space_id);
        fwts_log_info_verbatum(fw, "    Register Bit Width      0x%2.2" PRIx8, ecdt->ec_control.register_bit_width);
        fwts_log_info_verbatum(fw, "    Register Bit Offset     0x%2.2" PRIx8, ecdt->ec_control.register_bit_offset);
        fwts_log_info_verbatum(fw, "    Access Size             0x%2.2" PRIx8, ecdt->ec_control.access_width);
        fwts_log_info_verbatum(fw, "    Address                 0x%16.16" PRIx64, ecdt->ec_control.address);
        fwts_log_info_verbatum(fw, "  EC_DATA:");
        fwts_log_info_verbatum(fw, "    Address Space ID:       0x%2.2" PRIx8, ecdt->ec_data.address_space_id);
        fwts_log_info_verbatum(fw, "    Register Bit Width      0x%2.2" PRIx8, ecdt->ec_data.register_bit_width);
        fwts_log_info_verbatum(fw, "    Register Bit Offset     0x%2.2" PRIx8, ecdt->ec_data.register_bit_offset);
        fwts_log_info_verbatum(fw, "    Access Size             0x%2.2" PRIx8, ecdt->ec_data.access_width);
        fwts_log_info_verbatum(fw, "    Address                 0x%16.16" PRIx64, ecdt->ec_data.address);
        fwts_log_info_verbatum(fw, "  UID:                      0x%8.8" PRIx32, ecdt->uid);
        fwts_log_info_verbatum(fw, "  GPE_BIT:                  0x%2.2" PRIx8, ecdt->gpe_bit);
        fwts_log_info_verbatum(fw, "  EC_ID:                    '%s'", (char *)ecdt->ec_id);
        fwts_log_nl(fw);

        if (fwts_acpi_init(fw) != FWTS_OK) {
                fwts_log_error(fw, "Cannot initialise ACPI, skipping UID evaluation check");
		goto done;
	} else {
		/*
		 *  Now evaluate the EC_ID UID object and compare it to the
		 *  UID in the ECDT. They should be the same.
		 */
		ACPI_OBJECT_LIST arg_list;
		ACPI_BUFFER buf;
		ACPI_OBJECT *obj;
		int ret;
		size_t len = strlen((char *)ecdt->ec_id) + 6;
		char name[len];

		snprintf(name, len, "%s._UID", (char *)ecdt->ec_id);

		arg_list.Count   = 0;
		arg_list.Pointer = NULL;

		ret = fwts_acpi_object_evaluate(fw, name, &arg_list, &buf);
		if ((ACPI_FAILURE(ret) != AE_OK) || (buf.Pointer == NULL)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"ECDTEvalUidFailed",
				"Failed to evaluate ECDT UID %s, cannot check UID", name);
			free(buf.Pointer);
			goto deinit;
		}
		/*  Do we have a valid buffer to dump? */
		obj = buf.Pointer;
		if (obj->Type != ACPI_TYPE_INTEGER) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"ECDTEvalUidInvalidType",
				"Evaluating ECDT UID %s returned non-integer object type", name);
			free(buf.Pointer);
			goto deinit;
		}

		if (obj->Integer.Value != ecdt->uid) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"ECDTEvalUidInvalidValue",
				"Evaluating ECDT UID %s returned 0x%" PRIx32
				" which is different from the ECDT UID value 0x%" PRIx32,
				name, (uint32_t)obj->Integer.Value, ecdt->uid);
		} else {
			fwts_passed(fw, "Found and evaluated %s, returns expected value 0x%" PRIx32,
				name, (uint32_t)obj->Integer.Value);
		}
		free(buf.Pointer);
	}

deinit:
        (void)fwts_acpi_deinit(fw);
done:
	if (passed)
		fwts_passed(fw, "No issues found in ECDT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test ecdt_tests[] = {
	{ ecdt_test1, "ECDT Embedded Controller Boot Resources Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops ecdt_ops = {
	.description = "Embedded Controller Boot Resources Table test.",
	.init        = ecdt_init,
	.minor_tests = ecdt_tests
};

FWTS_REGISTER("ecdt", &ecdt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV | FWTS_FLAG_TEST_ACPI)
