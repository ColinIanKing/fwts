/*
 * Copyright (C) 2017-2018 Canonical
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

#define FWTS_ACPI_WPC_HID "ACPI0014"

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

static int acpi_wpc_init(fwts_framework *fw)
{
	ACPI_STATUS status;

	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	status = AcpiGetDevices(FWTS_ACPI_WPC_HID, get_device_handle, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		fwts_log_error(fw, "Cannot find the ACPI device");
		return FWTS_ERROR;
	}

	if (!device) {
		fwts_log_error(fw, "ACPI wireless power calibration device does not exist, skipping test");
		fwts_acpica_deinit();
		return FWTS_SKIP;
	} else {
		ACPI_BUFFER buffer;
		char full_name[128];

		buffer.Length = sizeof(full_name);
		buffer.Pointer = full_name;

		status = AcpiGetName(device, ACPI_FULL_PATHNAME, &buffer);
		if (ACPI_SUCCESS(status)) {
			fwts_log_info_verbatim(fw, "ACPI Wireless Power Calibration Device: %s", full_name);
			fwts_log_nl(fw);
		}
	}

	return FWTS_OK;
}

static void method_test_WPC_return(
 fwts_framework *fw,
 char *name,
 ACPI_BUFFER *buf,
 ACPI_OBJECT *obj,
 void *private)
{
 FWTS_UNUSED(private);

 if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) != FWTS_OK)
	 return;

 if (obj->Integer.Value <= 0x02 || obj->Integer.Value == 0xff)
	 fwts_method_passed_sane(fw, name, "integer");
 else
	 fwts_failed(fw, LOG_LEVEL_CRITICAL,
		 "Method_WPCInvalidInteger",
		 "%s returned an invalid integer 0x%8.8" PRIx64,
		 name, (uint64_t)obj->Integer.Value);
}

static int method_test_WPC(fwts_framework *fw)
{
 return fwts_evaluate_method(fw, METHOD_MANDATORY, &device,
	 "_WPC", NULL, 0, method_test_WPC_return, "_WPC");
}

static int method_test_WPP(fwts_framework *fw)
{
 return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
	 "_WPP", NULL, 0, fwts_method_test_integer_return, NULL);
}

static fwts_framework_minor_test acpi_wpc_tests[] = {
	{ method_test_WPC, "Test _WPC (Wireless Power Calibration)." },
	{ method_test_WPP, "Test _WPP (Wireless Power Polling)." },
	{ NULL, NULL }
};

static int acpi_wpc_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);
	fwts_acpica_deinit();

	return FWTS_OK;
}

static fwts_framework_ops acpi_wpc_ops = {
	.description = "Wireless power calibration device test",
	.init        = acpi_wpc_init,
	.deinit      = acpi_wpc_deinit,
	.minor_tests = acpi_wpc_tests
};

FWTS_REGISTER("acpi_wpc", &acpi_wpc_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
