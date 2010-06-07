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

static char *dmesg_parse_brackets(char *line)
{
	char buffer[4096];
	char *cptr;

	memset(buffer, 0, 4096);
	strcpy(buffer, line);		

	cptr = strchr(buffer, '[');
	if (cptr)
		cptr = strchr(cptr+1, '[');
	if (cptr) {
		char *cptr2;

		cptr++;
		cptr2 = strchr(cptr,']');
		if (cptr2)
			*cptr2=0;
	}

	return cptr;
}

static void dmesg_common_check(fwts_framework *fw, char *line, char *prevline, void *private, int *warnings, int *errors)
{
	if (strstr(line, "Temperature above threshold, cpu clock throttled")) {
		fwts_framework_failed(fw,"Test caused overtemperature. Insufficient cooling?");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "ACPI: Handling Garbled _PRT entry")) {
		fwts_framework_failed(fw,"Bios has a garbled _PRT entry; source_name and source_index swapped");
		fwts_log_info(fw, "%s", line);
	}
	
	if (strstr(line, "BIOS never enumerated boot CPU")) {
		fwts_framework_failed(fw,"The boot processor is not enumerated!"); 
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "*** Error: Return object type is incorrect")) {
		fwts_framework_failed(fw,"Return object type is incorrect: %s", line);
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "shpchp: acpi_shpchprm") && strstr(line,"_HPP fail")) {
		fwts_framework_failed(fw,"Hotplug _HPP method fails!");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "shpchp: acpi_pciehprm") && strstr(line,"OSHP fail")) {
		fwts_framework_failed(fw,"Hotplug OSHP method fails!");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "shpchp: acpi_shpchprm:") && strstr(line,"evaluate _BBN fail")) {
		fwts_framework_failed(fw,"Hotplug _BBN method is missing");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "[PBST] Namespace lookup failure, AE_NOT_FOUND")) {
		fwts_framework_failed(fw,"ACPI Namespace lookup failure reported");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "*** Error: Method reached maximum reentrancy limit")) {
		fwts_framework_failed(fw,"ACPI method has reached reentrancy limit");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "Error while parsing _PSD domain information")) {
		fwts_framework_failed(fw,"_PSD domain information is corrupt!");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "Wrong _BBN value, reboot and use option 'pci=noacpi'")) {
		fwts_framework_failed(fw,"The BIOS has wrong _BBN value, "
			"which will make PCI root bridge have wrong bus number"); 
		fwts_log_info(fw, "%s", line);
	}
			
	if (strstr(line,"ACPI: lapic on CPU ") && strstr(line, " stops in C2[C2]")) {
		fwts_framework_failed(fw,"The local apic timer incorrectly stops during C2 idle state\n",
				"The local apic timer incorrectly stops during C2 idle state.\n"
				"The ACPI specification forbids this and Linux needs the local\n"
				"apic timer to work. The most likely cause of this is that the\n"
				"firmware uses a hardware C3 or C4 state that is mapped to\n"
				"the ACPI C2 state.");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "Invalid _PCT data")!=NULL) {
		char *ptr = dmesg_parse_brackets(line);
		if (ptr)
			fwts_framework_failed(fw,"The _PCT data of %s is invalid", ptr);
		else 
			fwts_framework_failed(fw, "The _PCT data is invalid");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "*** Error: Method execution failed")) {
		char *ptr = dmesg_parse_brackets(line);
		if (ptr)
			fwts_framework_failed(fw,"Method execution of %s failed", ptr);
		else
			fwts_framework_failed(fw,"Method execution of failed: %s", line);
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "Method parse/execution failed") && strstr(line, "AE_NOT_FOUND")) {
		char *ptr = dmesg_parse_brackets(line);
		if (ptr) 
			fwts_framework_failed(fw,"Method parsing/execution of %s failed", ptr);
		else
			fwts_framework_failed(fw,"Method parsing/execution failed");
		fwts_log_info(fw, "%s", line);
		
	}

	if (strstr(line, "*** Error: Method execution failed") && strstr(line, "AE_AML_METHOD_LIMIT")) {
		char *ptr = dmesg_parse_brackets(line);
		if (ptr) 
			fwts_framework_failed(fw,"Method %s reached maximum reentrancy limit", ptr);
		else
			fwts_framework_failed(fw,"Method reached maximum reentrancy limit");
		fwts_log_info(fw, "%s", line);
	}
		
	if (strstr(line, "Method execution failed") && strstr(line, "AE_OWNER_ID_LIMIT")) {
		char *ptr = dmesg_parse_brackets(line);
		if (ptr) 
			fwts_framework_failed(fw,"Method %s failed to allocate owner ID", ptr);
		else
			fwts_framework_failed(fw,"Method failed to allocate owner ID");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "Method execution failed") && strstr(line, "AE_AML_BUFFER_LIMIT")) {
		char *ptr = dmesg_parse_brackets(line);
		if (ptr) 
			fwts_framework_failed(fw,"Method %s failed: ResourceSourceIndex is present but ResourceSource is not", ptr);
		else
			fwts_framework_failed(fw,"Method failed: ResourceSourceIndex is present but ResourceSource is not");
		fwts_log_info(fw, "%s", line);
	}

	if (strstr(line, "Disabling IRQ")!=NULL) {
		fwts_framework_failed(fw,"The kernel detected an irq storm. IRQ routing bug?");
		fwts_log_info(fw, "%s", line);
	}
}

static char *dmesg_common_headline(void)
{
	return "General dmesg common errors check";
}

static fwts_list *klog;

static int dmesg_common_init(fwts_framework *fw)
{
	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "cannot read kernel log");
		return 1;
	}
	return 0;
}

static int dmesg_common_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return 0;
}

static int dmesg_common_test1(fwts_framework *fw)
{	
	char *test = "General dmesg common errors check";
	int warnings = 0;
	int errors = 0;

	fwts_log_info(fw, "This checks for common errors found in the kernel message log");

	if (fwts_klog_scan(fw, klog, dmesg_common_check, NULL, &warnings, &errors)) {
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

static fwts_framework_tests dmesg_common_tests[] = {
	dmesg_common_test1,
	NULL
};

static fwts_framework_ops dmesg_common_ops = {
	dmesg_common_headline,
	dmesg_common_init,	
	dmesg_common_deinit,
	dmesg_common_tests
};

FRAMEWORK(dmesg_common, &dmesg_common_ops, TEST_ANYTIME);
