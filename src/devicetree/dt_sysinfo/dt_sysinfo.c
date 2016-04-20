/*
 * Copyright (C) 2014-2016 Jeremy Kerr <jk@ozlabs.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>

#include <libfdt.h>

#include "fwts.h"

static int check_property_printable(fwts_framework *fw, const char *name,
		const char *buf, size_t len)
{
	bool printable = true;
	unsigned int i;
	int rc;

	/* we need at least one character plus a nul */
	if (len < 2) {
		fwts_failed(fw, LOG_LEVEL_LOW, "DTPrintablePropertyShort",
				"property %s is too short", name);
		rc = FWTS_ERROR;
		goto out;
	}

	/* check all characters are printable */
	for (i = 0; i < len - 1; i++) {
		printable = printable && isprint(buf[i]);
		if (!printable)
			break;
	}

	if (!printable) {
		fwts_failed(fw, LOG_LEVEL_LOW, "DTPrintablePropertyInvalid",
				"property %s contains unprintable characters",
				name);
		rc = FWTS_ERROR;
		goto out;
	}

	/* check for a trailing nul */
	if (buf[len-1] != '\0') {
		fwts_failed(fw, LOG_LEVEL_LOW, "DTPrintablePropertyNoNul",
				"property %s isn't nul-terminated", name);
		rc = FWTS_ERROR;
	}

	rc = FWTS_OK;

out:
	return rc;
}

static int dt_sysinfo_check_root_property(fwts_framework *fw, const char *name)
{
	int rc, node, len;
	const char *buf;

	if (!fw->fdt)
		return FWTS_ABORTED;

	node = fdt_path_offset(fw->fdt, "/");
	if (node < 0) {
		fwts_failed(fw, LOG_LEVEL_LOW, "DTRootNodeMissing",
				"root device tree node is missing");
		return FWTS_ERROR;
	}

	buf = fdt_getprop(fw->fdt, node, name, &len);
	if (buf == NULL) {
		fwts_failed(fw, LOG_LEVEL_LOW, "DTSyinfoPropertyMissing",
				"can't read property %s: %s",
				name, fdt_strerror(len));
		return FWTS_ERROR;
	}

	rc = check_property_printable(fw, name, buf, len);

	if (rc == FWTS_OK)
		fwts_passed(fw, "sysinfo property %s is valid", name);

	return rc;
}

static int dt_sysinfo_check_model(fwts_framework *fw)
{
	return dt_sysinfo_check_root_property(fw, "model");
}

static int dt_sysinfo_check_system_id(fwts_framework *fw)
{
	return dt_sysinfo_check_root_property(fw, "system-id");
}

static fwts_framework_minor_test dt_sysinfo_tests[] = {
	{ dt_sysinfo_check_model,	"Check model property" },
	{ dt_sysinfo_check_system_id,	"Check system-id property" },
	{ NULL, NULL },
};

static fwts_framework_ops dt_sysinfo_ops = {
	.description	= "Device tree system information test",
	.minor_tests	= dt_sysinfo_tests,
};

FWTS_REGISTER_FEATURES("dt_sysinfo", &dt_sysinfo_ops, FWTS_TEST_ANYTIME,
		FWTS_FLAG_BATCH, FWTS_FW_FEATURE_DEVICETREE);
