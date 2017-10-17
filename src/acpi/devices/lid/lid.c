/*
 * Copyright (C) 2017 Canonical
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

#define FWTS_ACPI_LID_HID "PNP0C0D"

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

static int acpi_lid_init(fwts_framework *fw)
{
	ACPI_STATUS status;

	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	status = AcpiGetDevices(FWTS_ACPI_LID_HID, get_device_handle, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		fwts_log_error(fw, "Cannot find the ACPI device");
		return FWTS_ERROR;
	}

	if (!device) {
		fwts_log_error(fw, "ACPI lid device does not exist, skipping test");
		fwts_acpica_deinit();
		return FWTS_SKIP;
	} else {
		ACPI_BUFFER buffer;
		char full_name[128];

		buffer.Length = sizeof(full_name);
		buffer.Pointer = full_name;

		status = AcpiGetName(device, ACPI_FULL_PATHNAME, &buffer);
		if (ACPI_SUCCESS(status)) {
			fwts_log_info_verbatim(fw, "ACPI Lid Device: %s", full_name);
			fwts_log_nl(fw);
		}
	}

	return FWTS_OK;
}

static void method_test_LID_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_method_passed_sane_uint64(fw, name, obj->Integer.Value);
}

static int method_test_LID(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_MANDATORY,
		&device, "_LID", NULL, 0, method_test_LID_return, NULL);
}

static void method_test_PRW_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;
	bool failed = false;

	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_count_min(fw, name, "_PRW", obj, 2) != FWTS_OK)
		return;

	if (obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER &&
	    obj->Package.Elements[0].Type != ACPI_TYPE_PACKAGE) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_PRWBadPackageReturnType",
			"%s element 0 is not an integer or an package.", name);
		failed = true;
	}

	if (obj->Package.Elements[0].Type == ACPI_TYPE_PACKAGE) {
		ACPI_OBJECT *pkg;
		pkg = &obj->Package.Elements[0];
		if (pkg->Package.Count != 2) {
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"Method_PRWSubPackageElementCount",
				"%s sub-package 0  was expected to have 2"
				"elements, got %" PRIu32 " elements instead.",
				name, pkg->Package.Count);
			failed = true;
		}

		if (pkg->Package.Elements[0].Type != ACPI_TYPE_LOCAL_REFERENCE) {
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"Method_PRWBadSubPackageElementType",
				"%s sub-package 0 element 0 is not "
				"a reference.",name);
			failed = true;
		}

		if (pkg->Package.Elements[1].Type != ACPI_TYPE_INTEGER) {
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"Method_PRWBadSubPackageElementType",
				"%s sub-package 0 element 0 is not "
				"an integer.",name);
			failed = true;
		}
	}

	if (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_PRWBadPackageReturnType",
			"%s element 1 is not an integer.", name);
		failed = true;
	}

	for (i = 2; i < obj->Package.Count - 1; i++) {
		if (obj->Package.Elements[i].Type != ACPI_TYPE_LOCAL_REFERENCE) {
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"Method_PRWBadPackageReturnType",
				"%s package %" PRIu32
				" element 0 is not a reference.",
				name, i);
			failed = true;
		}
	}

	if (!failed)
		fwts_method_passed_sane(fw, name, "package");
}

static int method_test_PRW(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_OPTIONAL,
		&device, "_PRW", NULL, 0, method_test_PRW_return, NULL);
}

static int method_test_PSW(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return fwts_evaluate_method(fw, METHOD_OPTIONAL,
		&device, "_PSW", arg, 1, fwts_method_test_NULL_return, NULL);
}

static fwts_framework_minor_test acpi_lid_tests[] = {
	{ method_test_LID, "Test _LID (Lid Status)." },
	{ method_test_PRW, "Test _PRW (Power Resources for Wake)." },
	{ method_test_PSW, "Test _PSW (Power State Wake)." },
	{ NULL, NULL }
};

static int acpi_lid_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);
	fwts_acpica_deinit();

	return FWTS_OK;
}

static fwts_framework_ops acpi_lid_ops = {
	.description = "Lid device test",
	.init        = acpi_lid_init,
	.deinit      = acpi_lid_deinit,
	.minor_tests = acpi_lid_tests
};

FWTS_REGISTER("acpi_lid", &acpi_lid_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
