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

#include "framework.h"

static void acpiinfo_check(log *log, char *line, char *prevline, void *private, int *warnings, int *errors)
{
	framework *fw = (framework *)private;

	if (strstr(line, "ACPI: Subsystem revision")!=NULL) {
		char *version = strstr(line,"sion ");
		if (version) {
			version+=5;
			log_info(log, "Linux ACPI interpreter version %s", version);
		}
	}

	if (strstr(line, "ACPI: DSDT")!=NULL) {
		char *vendor = "Unknown";
		if (strstr(line,"MSFT"))
			vendor="Microsoft";
		if (strstr(line,"INTL"))
			vendor="Intel";
		log_info(log,"DSDT was compiled by the %s AML compiler", vendor);
	}

	if (strstr(line, "Disabling IRQ")!=NULL && prevline && strstr(prevline,"acpi_irq")) {
		framework_failed(fw, "ACPI interrupt got stuck: level triggered?");
		log_info(log,"%s", line);
	}

	if (prevline && strstr(prevline, "*** Error: Return object type is incorrect")) {
		framework_failed(fw, "Return object type is incorrect: %s", line);
		log_info(log,"%s", line);
	}

	if (strstr(line,"ACPI: acpi_ec_space_handler: bit_width is 32, should be")) {
		framework_failed(fw, "Embedded controller bit_width is incorrect: %s", line);
		log_info(log,"%s", line);
	}

	if (strstr(line,"acpi_ec_space_handler: bit_width should be")) {
		framework_failed(fw, "Embedded controller bit_width is incorrect: %s", line);
		log_info(log,"%s", line);
	}

	if (strstr(line,"Warning: acpi_table_parse(ACPI_SRAT) returned 0!")) {
		framework_failed(fw, "SRAT table cannot be found");
		log_info(log,"%s", line);
	}

	if (strstr(line,"Warning: acpi_table_parse(ACPI_SLIT) returned 0!")) {
		framework_failed(fw, "SLIT table cannot be found");
		log_info(log,"%s", line);
	}
		
	if (strstr(line, "WARNING: No sibling found for CPU")) {
		framework_failed(fw, "Hyperthreading CPU enumeration fails");
		log_info(log,"%s", line);
	}

	if (prevline && strstr(line, ">>> ERROR: Invalid checksum") && strlen(prevline)>11) {
		char tmp[4096];
		strncpy(tmp, prevline, sizeof(tmp));
		tmp[11] = '\0';
		framework_failed(fw, "ACPI table %s has an invalid checksum", tmp+6);
		log_info(log,"%s", line);
	}

	if (strstr(line, "MP-BIOS bug: 8254 timer not connected to IO-APIC")) {
		framework_failed(fw, "8254 timer not connected to IO-APIC: %s", line);
		log_info(log,"%s", line);
	}

	if (strstr(line, "ACPI: PCI Interrupt Link") && strstr(line, " disabled and referenced, BIOS bug.")) {
		framework_failed(fw, line);
		log_info(log,"%s", line);
	}

	if (strstr(line, "*** Warning Inconsistent FADT length") && strstr(line, "using FADT V1.0 portion of table")) {
		framework_failed(fw, "FADT table claims to be of higher revision than it is");
		log_info(log,"%s", line);
	}

	if (strstr(line, "thermal_get_trip_point: Invalid active threshold")) {
		framework_failed(fw, "_AC0 thermal trip point is invalid");
		log_info(log,"%s", line);
	}

	if (strstr(line, "MMCONFIG has no entries")) {
		framework_failed(fw, "The MCFG table has no entries!");
		log_info(log,"%s", line);
	}

	if (strstr(line, "MMCONFIG not in low 4GB of memory")) {
		framework_failed(fw, "The MCFG table entries are not in the lower 4Gb of ram");
		log_info(log,"%s", line);
	}

	if (strstr(line, "pcie_portdrv_probe->Dev") && strstr(line, "has invalid IRQ. Check vendor BIOS")) {
		framework_failed(fw, "PCI Express port driver reports an invalid IRQ");
		log_info(log,"%s", line);
	}
		
	if (strstr(line, "BIOS handoff failed (BIOS bug ?)")) {
		framework_failed(fw, "EHCI BIOS emulation handoff failed");
		log_info(log,"%s", line);
	}
}



void acpiinfo_headline(log *results)
{
	log_info(results, "General ACPI information check");
}

static char *klog;

int acpiinfo_init(log *results, framework *fw)
{
	if ((klog = klog_read()) == NULL) {
		log_error(results, "cannot read kernel log");
		return 1;
	}
	return 0;
}

int acpiinfo_deinit(log *results, framework *fw)
{
	free(klog);

	return 0;
}

int acpiinfo_test1(log *results, framework *fw)
{	
	char *test = "General ACPI information check";
	int warnings = 0;
	int errors = 0;

	log_info(results, "This test checks the output of the in-kernel ACPI CA against common\n"
		 "error messages that indicate a bad interaction with the bios, including\n"
		 "those that point at AML syntax errors.\n");

	if (klog_scan(results, klog, acpiinfo_check, fw, &warnings, &errors)) {
		log_error(results, "failed to scan kernel log");
		return 1;
	}

	if (warnings + errors > 0) {
		log_info(results, "Found %d errors, %d warnings in kernel log", errors, warnings);
		framework_failed(fw, test);
	}
	else
		framework_passed(fw, test);

	return 0;
}

framework_tests acpiinfo_tests[] = {
	acpiinfo_test1,
	NULL
};

framework_ops acpiinfo_ops = {
	acpiinfo_headline,
	acpiinfo_init,	
	acpiinfo_deinit,
	acpiinfo_tests
};

FRAMEWORK(acpiinfo, &acpiinfo_ops);
