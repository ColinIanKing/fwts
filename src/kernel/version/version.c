/*
 * Copyright (C) 2010-2012 Canonical
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

static int version_test1(fwts_framework *fw)
{
	char *str;

	/* Following is Ubuntu specific, so don't fail */
	if ((str = fwts_get("/proc/version_signature")) == NULL)
		fwts_warning(fw,
			"Cannot get version signature info from /proc/version_signature (This is Ubuntu specific, and not necessarily a failure).");
	else {
		fwts_chop_newline(str);
		fwts_passed(fw, "Signature: %s", str);
		free(str);
	}
	return FWTS_OK;
}

static int version_test2(fwts_framework *fw)
{
	char *str;


	if ((str = fwts_get("/proc/version")) == NULL)
		fwts_failed(fw, LOG_LEVEL_LOW,
			"KernelProcVersion",
			"Cannot get version info from /proc/version");
	else {
		fwts_chop_newline(str);
		fwts_passed(fw, "Kernel Version: %s", str);
		free(str);
	}
	return FWTS_OK;
}

static int version_test3(fwts_framework *fw)
{
	char *str;

        if (((str = fwts_get("/sys/module/acpi/parameters/acpica_version")) == NULL) &&
	    ((str = fwts_get("/proc/acpi/info")) == NULL))
		fwts_failed(fw, LOG_LEVEL_LOW,
			"KernelACPIVersion",
			"Cannot get ACPI version info from /sys/module/acpi/parameters/acpica_version or /proc/acpi/info");
	else {
		fwts_chop_newline(str);
		fwts_passed(fw, "ACPI Version: %s", str);
		free(str);
	}
	return FWTS_OK;
}

static fwts_framework_minor_test version_tests[] = {
	{ version_test1, "Gather kernel signature." },
	{ version_test2, "Gather kernel system information." },
	{ version_test3, "Gather APCI driver version." },
	{ NULL, NULL },
};

static fwts_framework_ops version_ops = {
	.description = "Gather kernel system information.",
	.minor_tests = version_tests
};

FWTS_REGISTER(version, &version_ops, FWTS_TEST_FIRST, FWTS_FLAG_BATCH);
