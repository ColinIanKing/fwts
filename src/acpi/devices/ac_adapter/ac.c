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

#define FWTS_ACPI_AC_HID "ACPI0003"

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

static int acpi_ac_init(fwts_framework *fw)
{
	ACPI_STATUS status;

	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	status = AcpiGetDevices(FWTS_ACPI_AC_HID, get_device_handle, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		fwts_log_error(fw, "Cannot find the ACPI device");
		return FWTS_ERROR;
	}

	if (!device) {
		fwts_log_error(fw, "ACPI AC device does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static void method_test_PSR_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) != FWTS_OK)
		return;

	if (obj->Integer.Value > 2) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_PSRZeroOrOne",
			"%s returned 0x%8.8" PRIx64 ", expected 0 "
			"(offline) or 1 (online)",
			name, (uint64_t)obj->Integer.Value);
	} else
		fwts_method_passed_sane_uint64(fw, name, obj->Integer.Value);
}

static int method_test_PSR(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_MANDATORY,
		 &device, "_PSR", NULL, 0, method_test_PSR_return, NULL);
}

static void method_test_PCL_return(fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_elements_all_type(fw, name, "_PCL", obj, ACPI_TYPE_LOCAL_REFERENCE) != FWTS_OK)
		return;

	fwts_passed(fw,	"%s returned a sane package of %" PRIu32 " references.", name, obj->Package.Count);
}

static int method_test_PCL(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_MANDATORY,
		 &device, "_PCL", NULL, 0, method_test_PCL_return, NULL);
}

static void method_test_PIF_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	static const fwts_package_element elements[] = {
		{ ACPI_TYPE_INTEGER,	"Power Source State" },
		{ ACPI_TYPE_INTEGER,	"Maximum Output Power" },
		{ ACPI_TYPE_INTEGER,	"Maximum Input Power" },
		{ ACPI_TYPE_STRING,	"Model Number" },
		{ ACPI_TYPE_STRING,	"Serial Number" },
		{ ACPI_TYPE_STRING,	"OEM Information" }
	};

	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_count_equal(fw, name, "_PIF", obj, 6) != FWTS_OK)
		return;

	if (fwts_method_package_elements_type(fw, name, "_PIF", obj, elements, 6) != FWTS_OK)
		return;

	fwts_acpi_object_dump(fw, obj);

	fwts_method_passed_sane(fw, name, "package");
}

static int method_test_PIF(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_OPTIONAL,
		 &device, "_PIF", NULL, 0, method_test_PIF_return, NULL);
}

static void method_test_PRL_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_elements_all_type(fw, name, "_PRL", obj, ACPI_TYPE_LOCAL_REFERENCE) != FWTS_OK)
		return;

	fwts_method_passed_sane(fw, name, "package");
}

static int method_test_PRL(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_OPTIONAL,
		 &device, "_PRL", NULL, 0, method_test_PRL_return, NULL);
}

static fwts_framework_minor_test acpi_ac_tests[] = {
	{ method_test_PSR, "Test _PSR (Power Source)." },
	{ method_test_PCL, "Test _PCL (Power Consumer List)." },
	{ method_test_PIF, "Test _PIF (Power Source Information)." },
	{ method_test_PRL, "Test _PRL (Power Source Redundancy List)." },
	{ NULL, NULL }
};

static int acpi_ac_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);
	fwts_acpica_deinit();

	return FWTS_OK;
}

static fwts_framework_ops acpi_ac_ops = {
	.description = "AC adapter device test",
	.init        = acpi_ac_init,
	.deinit      = acpi_ac_deinit,
	.minor_tests = acpi_ac_tests
};

FWTS_REGISTER("acpi_ac", &acpi_ac_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
