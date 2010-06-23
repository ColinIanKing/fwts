/* 
 * Copyright (C) 2006, Intel Corp
 * Copyright (C) 2007, AMD Inc
 * Copyright (C) 2010 Canonical
 *
 * This code was originally part of the Linux-ready Firmware Developer Kit
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
 #define _GNU_SOURCE
#include <sched.h>

#include <string.h>
#include <stdint.h>

#include "fwts.h"

typedef enum {
	CPU_UNKNOWN,
	CPU_AMD,
	CPU_INTEL,
} cpu_type;

static cpu_type cpu = CPU_UNKNOWN;
fwts_cpuinfo_x86 *fwts_virt_cpuinfo;

#define CPUID_NUM_FEATURES	0x00000000L

static int virt_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	if ((fwts_virt_cpuinfo = fwts_cpu_get_info()) == NULL) {
		fwts_log_error(fw, "Cannot get CPU info");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int virt_deinit(fwts_framework *fw)
{
	if (fwts_virt_cpuinfo)
		fwts_cpu_free_info(fwts_virt_cpuinfo);

	return FWTS_OK;
}

static char *virt_headline(void)
{
	return "Test CPU Virtualisation Configuration.";
}

static int virt_test1(fwts_framework *fw)
{
	fwts_log_info(fw, "Check if CPU is an AMD or Intel.");

	if (strstr(fwts_virt_cpuinfo->vendor_id, "AMD") != NULL) {
		cpu = CPU_AMD;
		fwts_passed(fw, "CPU is an AMD.");
	} else if (strstr(fwts_virt_cpuinfo->vendor_id, "Intel") != NULL) {
		cpu = CPU_INTEL;
		fwts_passed(fw, "CPU is an Intel.");
	} else {
		cpu = CPU_UNKNOWN;
		fwts_warning(fw, "CPU is unknown.");
	}

	return FWTS_OK;
}

static int virt_test2(fwts_framework *fw)
{
	extern void virt_check_svm(fwts_framework *);
	extern void virt_check_vmx(fwts_framework *);

	switch (cpu) {
	case CPU_AMD:
		virt_check_svm(fw);
		break;
	case CPU_INTEL:
		virt_check_vmx(fw);
		break;
	default:
		fwts_warning(fw, "Cannot test virtualisation extentions - unknown CPU.");
		break;
	}

	return FWTS_OK;
}
	

static fwts_framework_tests virt_tests[] = {
	virt_test1,
	virt_test2,
	NULL
};

static fwts_framework_ops virt_ops = {
	virt_headline,
	virt_init,
	virt_deinit,
	virt_tests
};
 
/* 
 * This test checks if the virtual machine setup is done correctly by the BIOS
 */
FWTS_REGISTER(virt, &virt_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
