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

/* 
 * This test sanity checks apic irq information
 * rule of thumb:
 * interrupts <16 should be EDGE
 * interrupts >16 should be LEVEL
 * acpi interrupt should be LEVEL
 */

static char *apicedge_headline(void)
{
	return "APIC Edge/Level Check";
}

static int apicedge_test1(fwts_framework *fw)
{	
	FILE *file;

	fwts_log_info(fw, "Checks if legacy interrupts are edge and PCI interrupts are level triggered");
		
	if ((file = fopen("/proc/interrupts", "r")) == NULL) {
		fwts_framework_failed(fw, "Could not open file /proc/interrupts");
		return 1;
	}

	while (!feof(file)) {
		char line[4096], *c;
		int edge = -1;
		int irq = 0;

		memset(line, 0, 4096);
		if (fgets(line, 4095, file) == NULL)
			break;
		
		if (! (line[0]==' ' || (line[0]>='0' && line[0]<='9')) )
			continue;

		irq = strtoul(line, &c, 10);
		if (c==line)
			continue;
		if (strstr(line, "IO-APIC-edge"))
			edge = 1;
		if (strstr(line, "IO-APIC-fasteoi"))
			edge = 0;
		if (strstr(line, "PCI-MSI-level"))
			edge = 0;
		if (strstr(line, "PCI-MSI-edge"))
			edge = 1;
		if (strstr(line, "IO-APIC-level"))
			edge = 0;
			
		if (strstr(line,"acpi")) {
			if (edge==1)
				fwts_framework_failed(fw,"ACPI Interrupt is incorrectly edge triggered");
			continue;	
		}
		if ((irq<15) && (edge == 0))
			fwts_framework_failed(fw,"Legacy interrupt %i is incorrectly level triggered", irq);
		if ((irq<15) && (edge == -1))
			fwts_framework_failed(fw,"Non-Legacy interrupt %i is incorrectly level triggered", irq);
	}
	fclose(file);	

	if (!fw->tests_failed)
		fwts_framework_passed(fw, "Legacy interrupts are edge and PCI interrupts are level triggered");

	return 0;
}

static fwts_framework_tests apicedge_tests[] = {
	apicedge_test1,
	NULL
};

static fwts_framework_ops apicedge_ops = {
	apicedge_headline,
	NULL,
	NULL,
	apicedge_tests
};

FRAMEWORK(apicedge, &apicedge_ops, TEST_ANYTIME);
