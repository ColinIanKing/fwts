/*
 * Copyright (C) 2017-2023 Canonical
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

#if defined(FWTS_HAS_ACPI)

#include "fwts_acpi_object_eval.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#define FWTS_ACPI_DEVICE_HID "PNP0C0C"
#define FWTS_ACPI_DEVICE "power button"

static ACPI_HANDLE device;

static ACPI_STATUS get_device_handle(ACPI_HANDLE handle, uint32_t level,
					  void *context, void **ret_val)
{
	FWTS_UNUSED(level);
	FWTS_UNUSED(context);
	FWTS_UNUSED(ret_val);

	device = handle;
	return AE_CTRL_TERMINATE;
}

static int power_button_init(fwts_framework *fw)
{
	ACPI_STATUS status;

	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	status = AcpiGetDevices(FWTS_ACPI_DEVICE_HID, get_device_handle, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		fwts_log_error(fw, "Cannot find the ACPI device");
		return FWTS_ERROR;
	}

	if (!device) {
		fwts_log_error(fw, "ACPI %s device does not exist, skipping test", FWTS_ACPI_DEVICE);
		fwts_acpica_deinit();
		return FWTS_SKIP;
	} else {
		ACPI_BUFFER buffer;
		char full_name[128];

		buffer.Length = sizeof(full_name);
		buffer.Pointer = full_name;

		status = AcpiGetName(device, ACPI_FULL_PATHNAME, &buffer);
		if (ACPI_SUCCESS(status)) {
			fwts_log_info_verbatim(fw, "ACPI %s device: %s", FWTS_ACPI_DEVICE, full_name);
			fwts_log_nl(fw);
		}
	}

	return FWTS_OK;
}

static int method_test_HID(fwts_framework *fw)
{
	return fwts_method_test_HID(fw, &device);
}

/* Evaluate Device Identification Objects - all are optional */
static int method_test_ADR(fwts_framework *fw)
{
	return fwts_method_test_ADR(fw, &device);
}

static int method_test_CID(fwts_framework *fw)
{
	return fwts_method_test_CID(fw, &device);
}

static int method_test_CLS(fwts_framework *fw)
{
	return fwts_method_test_CLS(fw, &device);
}

static int method_test_DDN(fwts_framework *fw)
{
	return fwts_method_test_DDN(fw, &device);
}

static int method_test_HRV(fwts_framework *fw)
{
	return fwts_method_test_HRV(fw, &device);
}

static int method_test_MLS(fwts_framework *fw)
{
	return fwts_method_test_MLS(fw, &device);
}

static int method_test_PLD(fwts_framework *fw)
{
	return fwts_method_test_PLD(fw, &device);
}

static int method_test_SUB(fwts_framework *fw)
{
	return fwts_method_test_SUB(fw, &device);
}

static int method_test_SUN(fwts_framework *fw)
{
	return fwts_method_test_SUN(fw, &device);
}

static int method_test_STR(fwts_framework *fw)
{
	return fwts_method_test_STR(fw, &device);
}

static int method_test_UID(fwts_framework *fw)
{
	return fwts_method_test_UID(fw, &device);
}

static int facp_test_pwrbutton(fwts_framework *fw)
{
	fwts_acpi_table_fadt *fadt;
	fwts_acpi_table_info *table;

	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK)
		return FWTS_SKIP;

	if (table == NULL)
		return FWTS_SKIP;

	fadt = (fwts_acpi_table_fadt *)table->data;

	/* check PWR_BUTTON as in Table 4-13 Power Button Support */
	if (fadt->flags & 0x10)
		fwts_passed(fw, "Fixed hardware power button does not exist.");
	else
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PWRBFixedHardwareError",
			"PWR_Button field in FACP should not be zero "
			"with ACPI PNP0C0C device.");

	return FWTS_OK;
}

static fwts_framework_minor_test power_button_tests[] = {
	/* Device Specific Objects */
	{ method_test_HID, "Test _HID (Hardware ID)." },
	/* Device Identification Objects - all are optional */
	{ method_test_ADR, "Test _ADR (Return Unique ID for Device)." },
	{ method_test_CID, "Test _CID (Compatible ID)." },
	{ method_test_CLS, "Test _CLS (Class Code)." },
	{ method_test_DDN, "Test _DDN (DOS Device Name)." },
	{ method_test_HRV, "Test _HRV (Hardware Revision Number)." },
	{ method_test_MLS, "Test _MLS (Multiple Language String)." },
	{ method_test_PLD, "Test _PLD (Physical Device Location)." },
	{ method_test_SUB, "Test _SUB (Subsystem ID)." },
	{ method_test_SUN, "Test _SUN (Slot User Number)." },
	{ method_test_STR, "Test _STR (String)." },
	{ method_test_UID, "Test _UID (Unique ID)." },
	/* Check against fixed hardware in FACP */
	{ facp_test_pwrbutton, "Test FACP PWR_BUTTON." },
	{ NULL, NULL }
};

static int power_button_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);
	fwts_acpica_deinit();

	return FWTS_OK;
}

static fwts_framework_ops power_button_ops = {
	.description = "Power button device test",
	.init        = power_button_init,
	.deinit      = power_button_deinit,
	.minor_tests = power_button_tests
};

FWTS_REGISTER("acpi_pwrb", &power_button_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
