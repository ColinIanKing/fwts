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

#define FWTS_POWER_BUTTON_HID "PNP0C0C"

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

	status = AcpiGetDevices(FWTS_POWER_BUTTON_HID, get_device_handle, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		fwts_log_error(fw, "Cannot find the ACPI device");
		return FWTS_ERROR;
	}

	if (!device) {
		fwts_log_error(fw, "ACPI power button device does not exist, skipping test");
		fwts_acpica_deinit();
		return FWTS_SKIP;
	} else {
		ACPI_BUFFER buffer;
		char full_name[128];

		buffer.Length = sizeof(full_name);
		buffer.Pointer = full_name;

		status = AcpiGetName(device, ACPI_FULL_PATHNAME, &buffer);
		if (ACPI_SUCCESS(status)) {
			fwts_log_info_verbatim(fw, "ACPI Power Button Device: %s", full_name);
			fwts_log_nl(fw);
		}
	}

	return FWTS_OK;
}


static void method_test_HID_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	char tmp[8];

	FWTS_UNUSED(buf);
	FWTS_UNUSED(private);

	if (obj == NULL) {
		fwts_method_failed_null_object(fw, name, "a buffer or integer");
		return;
	}

	switch (obj->Type) {
	case ACPI_TYPE_STRING:
		if (obj->String.Pointer) {
			if (fwts_method_valid_HID_string(obj->String.Pointer))
				fwts_passed(fw,
					"%s returned a string '%s' "
					"as expected.",
					name, obj->String.Pointer);
			else
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"MethodHIDInvalidString",
					"%s returned a string '%s' "
					"but it was not a valid PNP ID or a "
					"valid ACPI ID.",
					name, obj->String.Pointer);
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_HIDNullString",
				"%s returned a NULL string.", name);
		}
		break;
	case ACPI_TYPE_INTEGER:
		if (fwts_method_valid_EISA_ID((uint32_t)obj->Integer.Value,
			tmp, sizeof(tmp)))
			fwts_passed(fw, "%s returned an integer "
				"0x%8.8" PRIx64 " (EISA ID %s).",
				name, (uint64_t)obj->Integer.Value, tmp);
		else
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"MethodHIDInvalidInteger",
				"%s returned a integer 0x%8.8" PRIx64 " "
				"(EISA ID %s) but this is not a valid "
				"EISA ID encoded PNP ID.",
				name, (uint64_t)obj->Integer.Value, tmp);
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_HIDBadReturnType",
			"%s did not return a string or an integer.", name);
		break;
	}
}

static int method_test_HID(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_MANDATORY, &device,
		"_HID", NULL, 0, method_test_HID_return, NULL);
}

static fwts_framework_minor_test power_button_tests[] = {
	{ method_test_HID, "Test _HID (Hardware ID)." },
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

FWTS_REGISTER("acpi_pwrb", &power_button_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
