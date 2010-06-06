/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 * 
 * This file was originally part of the Linux-ready Firmware Developer Kit
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "fwts.h"

static void acpiinfo_check(fwts_framework *fw, char *line, char *prevline, void *private, int *warnings, int *errors)
{
	if (strstr(line, "ACPI: Subsystem revision")!=NULL) {
		char *version = strstr(line,"sion ");
		if (version) {
			version+=5;
			fwts_log_info(fw, "Linux ACPI interpreter version %s", version);
		}
	}

	if (strstr(line, "ACPI: DSDT")!=NULL) {
		char *vendor = "Unknown";
		if (strstr(line,"MSFT"))
			vendor="Microsoft";
		if (strstr(line,"INTL"))
			vendor="Intel";
		fwts_log_info(fw,"DSDT was compiled by the %s AML compiler", vendor);
	}

	if (strstr(line, "Disabling IRQ")!=NULL && prevline && strstr(prevline,"acpi_irq")) {
		fwts_framework_failed(fw, "ACPI interrupt got stuck: level triggered?");
		fwts_log_info(fw,"%s", line);
	}

	if (prevline && strstr(prevline, "*** Error: Return object type is incorrect")) {
		fwts_framework_failed(fw, "Return object type is incorrect: %s", line);
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line,"ACPI: acpi_ec_space_handler: bit_width is 32, should be")) {
		fwts_framework_failed(fw, "Embedded controller bit_width is incorrect: %s", line);
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line,"acpi_ec_space_handler: bit_width should be")) {
		fwts_framework_failed(fw, "Embedded controller bit_width is incorrect: %s", line);
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line,"Warning: acpi_table_parse(ACPI_SRAT) returned 0!")) {
		fwts_framework_failed(fw, "SRAT table cannot be found");
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line,"Warning: acpi_table_parse(ACPI_SLIT) returned 0!")) {
		fwts_framework_failed(fw, "SLIT table cannot be found");
		fwts_log_info(fw,"%s", line);
	}
		
	if (strstr(line, "WARNING: No sibling found for CPU")) {
		fwts_framework_failed(fw, "Hyperthreading CPU enumeration fails");
		fwts_log_info(fw,"%s", line);
	}

	if (prevline && strstr(line, ">>> ERROR: Invalid checksum") && strlen(prevline)>11) {
		char tmp[4096];
		strncpy(tmp, prevline, sizeof(tmp));
		tmp[11] = '\0';
		fwts_framework_failed(fw, "ACPI table %s has an invalid checksum", tmp+6);
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "MP-BIOS bug: 8254 timer not connected to IO-APIC")) {
		fwts_framework_failed(fw, "8254 timer not connected to IO-APIC: %s", line);
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "ACPI: PCI Interrupt Link") && strstr(line, " disabled and referenced, BIOS bug.")) {
		fwts_framework_failed(fw, line);
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "*** Warning Inconsistent FADT length") && strstr(line, "using FADT V1.0 portion of table")) {
		fwts_framework_failed(fw, "FADT table claims to be of higher revision than it is");
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "thermal_get_trip_point: Invalid active threshold")) {
		fwts_framework_failed(fw, "_AC0 thermal trip point is invalid");
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "MMCONFIG has no entries")) {
		fwts_framework_failed(fw, "The MCFG table has no entries!");
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "MMCONFIG not in low 4GB of memory")) {
		fwts_framework_failed(fw, "The MCFG table entries are not in the lower 4Gb of ram");
		fwts_log_info(fw,"%s", line);
	}

	if (strstr(line, "pcie_portdrv_probe->Dev") && strstr(line, "has invalid IRQ. Check vendor BIOS")) {
		fwts_framework_failed(fw, "PCI Express port driver reports an invalid IRQ");
		fwts_log_info(fw,"%s", line);
	}
		
	if (strstr(line, "BIOS handoff failed (BIOS bug ?)")) {
		fwts_framework_failed(fw, "EHCI BIOS emulation handoff failed");
		fwts_log_info(fw,"%s", line);
	}
}



static char *acpiinfo_headline(void)
{
	return "General ACPI information check";
}

static fwts_text_list *klog;

static int acpiinfo_init(fwts_framework *fw)
{
	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "cannot read kernel log");
		return 1;
	}
	return 0;
}

static int acpiinfo_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return 0;
}

static int acpiinfo_test1(fwts_framework *fw)
{	
	char *test = "General ACPI information check";
	int warnings = 0;
	int errors = 0;

	fwts_log_info(fw, "This test checks the output of the in-kernel ACPI CA against common\n"
		 "error messages that indicate a bad interaction with the bios, including\n"
		 "those that point at AML syntax errors.\n");

	if (fwts_klog_scan(fw, klog, acpiinfo_check, NULL, &warnings, &errors)) {
		fwts_log_error(fw, "failed to scan kernel log");
		return 1;
	}

	if (warnings + errors > 0) {
		fwts_log_info(fw, "Found %d errors, %d warnings in kernel log", errors, warnings);
		fwts_framework_failed(fw, test);
	}
	else
		fwts_framework_passed(fw, test);

	return 0;
}

static fwts_framework_tests acpiinfo_tests[] = {
	acpiinfo_test1,
	NULL
};

static fwts_framework_ops acpiinfo_ops = {
	acpiinfo_headline,
	acpiinfo_init,	
	acpiinfo_deinit,
	acpiinfo_tests
};

FRAMEWORK(acpiinfo, &acpiinfo_ops, TEST_ANYTIME);
