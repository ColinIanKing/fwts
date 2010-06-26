/*
 * Copyright (C) 2010 Canonical
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

static char *version_headline(void)
{
	/* Return the name of the test scenario */
	return "Gather kernel system information.";
}

static int version_test1(fwts_framework *fw)
{
	char *str;

	/* Following is Ubuntu specific, so don't fail */
	if ((str = fwts_get("/proc/version_signature")) == NULL)
		fwts_log_error(fw,"Cannot get version signature info from /proc/version_signature");
	else {
		fwts_log_info(fw, "Signature: %s", str);
		free(str);
	}

	if ((str = fwts_get("/proc/version")) == NULL) 
		fwts_failed_low(fw,"Cannot get version info from /proc/version");
	else {
		fwts_log_info(fw, "Version: %s", str);
		free(str);
		fwts_passed(fw, "Gathered kernel version info");
	}

	if ((str = fwts_get("/proc/acpi/info")) == NULL) 
		fwts_failed_low(fw,"Cannot get ACPI version info from /proc/acpi/info");
	else {
		fwts_log_info(fw, "ACPI Version: %s", str);
		free(str);
		fwts_passed(fw, "Gathered ACPI kernel version info");
	}

	return FWTS_OK;
}


/*
 *  Null terminated array of tests to run, in this
 *  scenario, we just have one test.
 */
static fwts_framework_tests version_tests[] = {
	version_test1,
	NULL
};

static fwts_framework_ops version_ops = {
	version_headline,
	NULL,
	NULL,
	version_tests
};

FWTS_REGISTER(version, &version_ops, FWTS_TEST_FIRST, FWTS_BATCH);
