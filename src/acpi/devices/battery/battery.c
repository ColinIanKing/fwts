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

#define FWTS_ACPI_BATTERY_HID "PNP0C0A"

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

static int acpi_battery_init(fwts_framework *fw)
{
	ACPI_STATUS status;

	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	status = AcpiGetDevices(FWTS_ACPI_BATTERY_HID, get_device_handle, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		fwts_log_error(fw, "Cannot find the ACPI device");
		return FWTS_ERROR;
	}

	if (!device) {
		fwts_log_error(fw, "ACPI Battery device does not exist, skipping test");
		fwts_acpica_deinit();
		return FWTS_SKIP;
	} else {
		ACPI_BUFFER buffer;
		char full_name[128];

		buffer.Length = sizeof(full_name);
		buffer.Pointer = full_name;

		status = AcpiGetName(device, ACPI_FULL_PATHNAME, &buffer);
		if (ACPI_SUCCESS(status)) {
			fwts_log_info_verbatim(fw, "ACPI Battery Device: %s", full_name);
			fwts_log_nl(fw);
		}
	}

	return FWTS_OK;
}

static void method_test_BIF_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;

	static const fwts_package_element elements[] = {
		{ ACPI_TYPE_INTEGER,	"Power Unit" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity" },
		{ ACPI_TYPE_INTEGER,	"Last Full Charge Capacity" },
		{ ACPI_TYPE_INTEGER,	"Battery Technology" },
		{ ACPI_TYPE_INTEGER,	"Design Voltage" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity of Warning" },
		{ ACPI_TYPE_INTEGER,	"Design Capactty of Low" },
		{ ACPI_TYPE_INTEGER,	"Battery Capacity Granularity 1" },
		{ ACPI_TYPE_INTEGER,	"Battery Capacity Granularity 2" },
		{ ACPI_TYPE_STRING,	"Model Number" },
		{ ACPI_TYPE_STRING,	"Serial Number" },
		{ ACPI_TYPE_STRING,	"Battery Type" },
		{ ACPI_TYPE_STRING,	"OEM Information" }
	};

	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_count_equal(fw, name, "_BIF", obj, 13) != FWTS_OK)
		return;

	if (fwts_method_package_elements_type(fw, name, "_BIF", obj, elements, 13) != FWTS_OK)
		return;

	/* Sanity check each field */
	/* Power Unit */
	if (obj->Package.Elements[0].Integer.Value > 0x00000002) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIFBadUnits",
			"%s: Expected Power Unit (Element 0) to be "
			"0 (mWh) or 1 (mAh), got 0x%8.8" PRIx64 ".",
			name, (uint64_t)obj->Package.Elements[0].Integer.Value);
		failed = true;
	}
#ifdef FWTS_METHOD_PEDANDTIC
	/*
	 * Since this information may be evaluated by communicating with
	 * the EC we skip these checks as we can't do this from userspace
	 */
	/* Design Capacity */
	if (obj->Package.Elements[1].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIFBadCapacity",
			"%s: Design Capacity (Element 1) is "
			"unknown: 0x%8.8" PRIx64 ".",
			name, obj->Package.Elements[1].Integer.Value);
		failed = true;
	}
	/* Last Full Charge Capacity */
	if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIFChargeCapacity",
			"%s: Last Full Charge Capacity (Element 2) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, obj->Package.Elements[2].Integer.Value);
		failed = true;
	}
#endif
	/* Battery Technology */
	if (obj->Package.Elements[3].Integer.Value > 0x00000002) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_BIFBatTechUnit",
			"%s: Expected Battery Technology Unit "
			"(Element 3) to be 0 (Primary) or 1 "
			"(Secondary), got 0x%8.8" PRIx64 ".",
			name, (uint64_t)obj->Package.Elements[3].Integer.Value);
		failed = true;
	}
#ifdef FWTS_METHOD_PEDANDTIC
	/*
	 * Since this information may be evaluated by communicating with
	 * the EC we skip these checks as we can't do this from userspace
	 */
	/* Design Voltage */
	if (obj->Package.Elements[4].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIFDesignVoltage",
			"%s: Design Voltage (Element 4) is "
			"unknown: 0x%8.8" PRIx64 ".",
			name, obj->Package.Elements[4].Integer.Value);
		failed = true;
	}
	/* Design capacity warning */
	if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIFDesignCapacityE5",
			"%s: Design Capacity Warning (Element 5) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, obj->Package.Elements[5].Integer.Value);
		failed = true;
	}
	/* Design capacity low */
	if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIFDesignCapacityE6",
			"%s: Design Capacity Warning (Element 6) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, obj->Package.Elements[6].Integer.Value);
		failed = true;
	}
#endif
	if (failed)
		fwts_advice(fw,
			"Battery %s package contains errors. It is "
			"worth running the firmware test suite "
			"interactive 'battery' test to see if this "
			"is problematic.  This is a bug an needs to "
			"be fixed.", name);
	else
		fwts_method_passed_sane(fw, name, "package");
}

static int method_test_BIF(fwts_framework *fw)
{
	ACPI_STATUS status;
	ACPI_HANDLE	method;

	/* _BIF can be superseded by _BIX */
	status = AcpiGetHandle (device, "_BIX", &method);
	if (ACPI_SUCCESS(status))
		return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
			"_BIF", NULL, 0, method_test_BIF_return, NULL);
	else
		return fwts_evaluate_method(fw, METHOD_MANDATORY, &device,
			"_BIF", NULL, 0, method_test_BIF_return, NULL);
}

static void method_test_BIX_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;
	uint64_t revision = 0;

	/* Revision 0 in ACPI 5.x */
	static const fwts_package_element elements[] = {
		{ ACPI_TYPE_INTEGER,	"Revision" },
		{ ACPI_TYPE_INTEGER,	"Power Unit" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity" },
		{ ACPI_TYPE_INTEGER,	"Last Full Charge Capacity" },
		{ ACPI_TYPE_INTEGER,	"Battery Technology" },
		{ ACPI_TYPE_INTEGER,	"Design Voltage" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity of Warning" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity of Low" },
		{ ACPI_TYPE_INTEGER,	"Cycle Count" },
		{ ACPI_TYPE_INTEGER,	"Measurement Accuracy" },
		{ ACPI_TYPE_INTEGER,	"Max Sampling Time" },
		{ ACPI_TYPE_INTEGER,	"Min Sampling Time" },
		{ ACPI_TYPE_INTEGER,	"Max Averaging Interval" },
		{ ACPI_TYPE_INTEGER,	"Min Averaging Interval" },
		{ ACPI_TYPE_INTEGER,	"Battery Capacity Granularity 1" },
		{ ACPI_TYPE_INTEGER,	"Battery Capacity Granularity 2" },
		{ ACPI_TYPE_STRING,	"Model Number" },
		{ ACPI_TYPE_STRING,	"Serial Number" },
		{ ACPI_TYPE_STRING,	"Battery Type" },
		{ ACPI_TYPE_STRING,	"OEM Information" }
	};

	/* Revision 1 in ACPI 6.x introduces swapping capability */
	static const fwts_package_element elements_v1[] = {
		{ ACPI_TYPE_INTEGER,	"Revision" },
		{ ACPI_TYPE_INTEGER,	"Power Unit" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity" },
		{ ACPI_TYPE_INTEGER,	"Last Full Charge Capacity" },
		{ ACPI_TYPE_INTEGER,	"Battery Technology" },
		{ ACPI_TYPE_INTEGER,	"Design Voltage" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity of Warning" },
		{ ACPI_TYPE_INTEGER,	"Design Capacity of Low" },
		{ ACPI_TYPE_INTEGER,	"Cycle Count" },
		{ ACPI_TYPE_INTEGER,	"Measurement Accuracy" },
		{ ACPI_TYPE_INTEGER,	"Max Sampling Time" },
		{ ACPI_TYPE_INTEGER,	"Min Sampling Time" },
		{ ACPI_TYPE_INTEGER,	"Max Averaging Interval" },
		{ ACPI_TYPE_INTEGER,	"Min Averaging Interval" },
		{ ACPI_TYPE_INTEGER,	"Battery Capacity Granularity 1" },
		{ ACPI_TYPE_INTEGER,	"Battery Capacity Granularity 2" },
		{ ACPI_TYPE_STRING,	"Model Number" },
		{ ACPI_TYPE_STRING,	"Serial Number" },
		{ ACPI_TYPE_STRING,	"Battery Type" },
		{ ACPI_TYPE_STRING,	"OEM Information" },
		{ ACPI_TYPE_INTEGER,	"Battery Swapping Capability" }
	};

	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (obj->Package.Count > 1 && obj->Package.Elements[0].Type == ACPI_TYPE_INTEGER)
		revision = obj->Package.Elements[0].Integer.Value;

	switch (revision) {
	case 0:
		if (fwts_method_package_count_equal(fw, name, "_BIX", obj, 20) != FWTS_OK)
			return;

		if (fwts_method_package_elements_type(fw, name, "_BIX", obj, elements, 20) != FWTS_OK)
			return;
		break;
	case 1:
		if (fwts_method_package_count_equal(fw, name, "_BIX", obj, 21) != FWTS_OK)
			return;

		if (fwts_method_package_elements_type(fw, name, "_BIX", obj, elements_v1, 21) != FWTS_OK)
			return;
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXRevision",
			"%s: Expected %s (Element 0) to be "
			"0 or 1, got 0x%8.8" PRIx64 ".",
			name, elements_v1[0].name,
			(uint64_t)obj->Package.Elements[0].Integer.Value);
		return;
	}

	/* Sanity check each field */
	/* Power Unit */
	if (obj->Package.Elements[1].Integer.Value > 0x00000002) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXPowerUnit",
			"%s: Expected %s (Element 1) to be "
			"0 (mWh) or 1 (mAh), got 0x%8.8" PRIx64 ".",
			name, elements[1].name,
			(uint64_t)obj->Package.Elements[1].Integer.Value);
		failed = true;
	}
#ifdef FWTS_METHOD_PEDANDTIC
	/*
	 * Since this information may be evaluated by communicating with
	 * the EC we skip these checks as we can't do this from userspace
	 */
	/* Design Capacity */
	if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXDesignCapacity",
			"%s: %s (Element 2) is "
			"unknown: 0x%8.8" PRIx64 ".",
			name, elements[2].name,
			obj->Package.Elements[2].Integer.Value);
		failed = true;
	}
	/* Last Full Charge Capacity */
	if (obj->Package.Elements[3].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXFullChargeCapacity",
			"%s: %s (Element 3) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, elements[3].name,
			obj->Package.Elements[3].Integer.Value);
		failed = true;
	}
#endif
	/* Battery Technology */
	if (obj->Package.Elements[4].Integer.Value > 0x00000002) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_BIXBatteryTechUnit",
			"%s: %s "
			"(Element 4) to be 0 (Primary) or 1 "
			"(Secondary), got 0x%8.8" PRIx64 ".",
			name, elements[4].name,
			(uint64_t)obj->Package.Elements[4].Integer.Value);
		failed = true;
	}
#ifdef FWTS_METHOD_PEDANDTIC
	/*
	 * Since this information may be evaluated by communicating with
	 * the EC we skip these checks as we can't do this from userspace
	 */
	/* Design Voltage */
	if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXDesignVoltage",
			"%s: %s (Element 5) is unknown: "
			"0x%8.8" PRIx64 ".",
			name, elements[5].name,
			obj->Package.Elements[5].Integer.Value);
		failed = true;
	}
	/* Design capacity warning */
	if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXDesignCapacityE6",
			"%s: %s (Element 6) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, elements[6].name,
			obj->Package.Elements[6].Integer.Value);
		failed = true;
	}
	/* Design capacity low */
	if (obj->Package.Elements[7].Integer.Value > 0x7fffffff) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXDesignCapacityE7",
			"%s: %s (Element 7) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, elements[7].name,
			obj->Package.Elements[7].Integer.Value);
		failed = true;
	}
	/* Cycle Count: value = 0 ~ 0xFFFFFFFE or 0xFFFFFFFF (unknown) */

	/* Measurement Accuracy */
	if (obj->Package.Elements[9].Integer.Value > 100000) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXMeasurementAccuracy",
			"%s: %s (Element 9) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, elements[9].name,
			obj->Package.Elements[9].Integer.Value);
		failed = true;
	}

#endif

	/* Swapping Capability */
	if (revision == 1 && obj->Package.Elements[20].Integer.Value > 2) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BIXSwappingCapability",
			"%s: %s (Element 20) "
			"is unknown: 0x%8.8" PRIx64 ".",
			name, elements_v1[20].name,
			obj->Package.Elements[20].Integer.Value);
		failed = true;
	}

	if (failed)
		fwts_advice(fw,
			"Battery %s package contains errors. It is "
			"worth running the firmware test suite "
			"interactive 'battery' test to see if this "
			"is problematic.  This is a bug an needs to "
			"be fixed.", name);
	else
		fwts_method_passed_sane(fw, name, "package");
}

static int method_test_BIX(fwts_framework *fw)
{
	ACPI_STATUS status;
	ACPI_HANDLE	method;

	/* _BIX may not supported by older firmware */
	status = AcpiGetHandle (device, "_BIF", &method);
	if (ACPI_SUCCESS(status))
		return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
			"_BIX", NULL, 0, method_test_BIX_return, NULL);
	else
		return fwts_evaluate_method(fw, METHOD_MANDATORY, &device,
			"_BIX", NULL, 0, method_test_BIX_return, NULL);
}

static int method_test_BMA(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
		"_BMA", arg, 1, fwts_method_test_integer_return, NULL);
}

static int method_test_BMS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
		"_BMS", arg, 1, fwts_method_test_integer_return, NULL);
}

static void method_test_BST_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;

	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_count_equal(fw, name, "_BST", obj, 4) != FWTS_OK)
		return;

	if (fwts_method_package_elements_all_type(fw, name, "_BST", obj, ACPI_TYPE_INTEGER) != FWTS_OK)
		return;

	/* Sanity check each field */
	/* Battery State */
	if ((obj->Package.Elements[0].Integer.Value) > 7) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BSTBadState",
			"%s: Expected Battery State (Element 0) to "
			"be 0..7, got 0x%8.8" PRIx64 ".",
			name, (uint64_t)obj->Package.Elements[0].Integer.Value);
		failed = true;
	}
	/* Ensure bits 0 (discharging) and 1 (charging) are not both set, see 10.2.2.6 */
	if (((obj->Package.Elements[0].Integer.Value) & 3) == 3) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"Method_BSTBadState",
			"%s: Battery State (Element 0) is "
			"indicating both charging and discharginng "
			"which is not allowed. Got value 0x%8.8" PRIx64 ".",
			name, (uint64_t)obj->Package.Elements[0].Integer.Value);
		failed = true;
	}
	/* Battery Present Rate - cannot check, pulled from EC */
	/* Battery Remaining Capacity - cannot check, pulled from EC */
	/* Battery Present Voltage - cannot check, pulled from EC */
	if (failed)
		fwts_advice(fw,
			"Battery %s package contains errors. It is "
			"worth running the firmware test suite "
			"interactive 'battery' test to see if this "
			"is problematic.  This is a bug an needs to "
			"be fixed.", name);
		else
			fwts_method_passed_sane(fw, name, "package");
}

static int method_test_BST(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_MANDATORY, &device,
		"_BST", NULL, 0, method_test_BST_return, NULL);
}

static int method_test_BTH(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	uint8_t i, ret;
	arg[0].Type = ACPI_TYPE_INTEGER;

	for (i = 0; i <= 100; i++) {
		arg[0].Integer.Value = i;
		ret = fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
			"_BTH", arg, 1, fwts_method_test_NULL_return, NULL);

		if (ret != FWTS_OK)
			break;
	}
	return ret;
}

static int method_test_BTP(fwts_framework *fw)
{
	static const int values[] = { 0, 1, 100, 200, 0x7fffffff };
	ACPI_STATUS status;
	uint8_t i;

	for (i = 0; i < sizeof(values) / sizeof(int); i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		status = fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
			"_BTP", arg, 1, fwts_method_test_NULL_return, NULL);
		if (ACPI_FAILURE(status))
			break;
	}
	return FWTS_OK;
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
	return fwts_evaluate_method(fw, METHOD_MANDATORY, &device,
		 "_PCL", NULL, 0, method_test_PCL_return, NULL);
}

static void method_test_STA_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;

	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) != FWTS_OK)
		return;

	if ((obj->Integer.Value & 3) == 2) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"Method_STAEnabledNotPresent",
			"%s indicates that the device is enabled "
			"but not present, which is impossible.", name);
		failed = true;
	}
	if ((obj->Integer.Value & ~0x1f) != 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"Method_STAReservedBitsSet",
			"%s is returning non-zero reserved "
			"bits 5-31. These should be zero.", name);
		failed = true;
	}

	if (!failed)
		fwts_method_passed_sane_uint64(fw, name, obj->Integer.Value);
}

static int method_test_STA(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
		"_STA", NULL, 0, method_test_STA_return, NULL);
}

static int method_test_BTM(fwts_framework *fw)
{
	static const int values[] = { 0, 1, 100, 200, 0x7fffffff };
	ACPI_STATUS status;
	uint8_t i;

	for (i = 0; i < sizeof(values) / sizeof(int); i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		status = fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
			"_BTM", arg, 1, fwts_method_test_NULL_return, NULL);
		if (ACPI_FAILURE(status))
			break;
	}
	return FWTS_OK;
}

static int method_test_BCT(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 50;	/* 50% */

	/*
	 * For now, just check that we get some integer back, values
	 * can be 0x00000000, 0x00000001-0xfffffffe and 0xffffffff,
	 * so anything is valid as long as it is an integer
	 */
	return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
		"_BCT", arg, 1, fwts_method_test_integer_return, NULL);
}

static void method_test_BMD_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (fwts_method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	if (fwts_method_package_count_equal(fw, name, "_BMD", obj, 5) != FWTS_OK)
		return;

	if (fwts_method_package_elements_all_type(fw, name, "_BMD", obj, ACPI_TYPE_INTEGER) != FWTS_OK)
		return;

	fwts_method_passed_sane(fw, name, "package");
}

static int method_test_BMD(fwts_framework *fw)
{
	return fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
		"_BMD", NULL, 0, method_test_BMD_return, NULL);
}

static int method_test_BMC(fwts_framework *fw)
{
	static const int values[] = { 0, 1, 2, 4 };
	ACPI_STATUS status;
	uint8_t i;

	for (i = 0; i < sizeof(values) / sizeof(int); i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		status = fwts_evaluate_method(fw, METHOD_OPTIONAL, &device,
			"_BMC", arg, 1, fwts_method_test_NULL_return, NULL);
		if (ACPI_FAILURE(status))
			break;
	}
	return FWTS_OK;
}

static fwts_framework_minor_test acpi_battery_tests[] = {
	{ method_test_BIF, "Test _BIF (Battery Information)." },
	{ method_test_BIX, "Test _BIX (Battery Information Extended)." },
	{ method_test_BMA, "Test _BMA (Battery Measurement Averaging)." },
	{ method_test_BMS, "Test _BMS (Battery Measurement Sampling Time)." },
	{ method_test_BST, "Test _BST (Battery Status)." },
	{ method_test_BTH, "Test _BTH (Battery Throttle Limit)." },
	{ method_test_BTP, "Test _BTP (Battery Trip Point)." },
	{ method_test_PCL, "Test _PCL (Power Consumer List)." },
	{ method_test_STA, "Test _STA (Status)." },
	{ method_test_BTM, "Test _BTM (Battery Time)." },
	{ method_test_BCT, "Test _BCT (Battery Charge Time)." },
	{ method_test_BMD, "Test _BMD (Battery Maintenance Data)." },
	{ method_test_BMC, "Test _BMC (Battery Maintenance Control)." },
	{ NULL, NULL }
};

static int acpi_battery_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);
	fwts_acpica_deinit();

	return FWTS_OK;
}

static fwts_framework_ops acpi_battery_ops = {
	.description = "ACPI battery device test",
	.init        = acpi_battery_init,
	.deinit      = acpi_battery_deinit,
	.minor_tests = acpi_battery_tests
};

FWTS_REGISTER("acpi_battery", &acpi_battery_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
