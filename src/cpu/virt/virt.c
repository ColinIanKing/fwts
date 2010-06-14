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

cpu_type cpu = CPU_UNKNOWN;

#define CPUID_NUM_FEATURES	0x00000000L

static int is_AMD()
{
	char stramd[64];

	memset(stramd, 0, 64);

	cpu_registers regs;
	exec_cpuid(CURRENT_CPU, CPUID_NUM_FEATURES, &regs);
	
	memcpy(stramd, &regs.ebx, 4 );
	memcpy(stramd + 4, &regs.edx, 4);
	memcpy(stramd + 8, &regs.ecx, 4);

	return (!strcmp("AuthenticAMD", stramd));
}

static int is_Intel()
{
	char strintel[64];
	memset( strintel, 0, 64 );

	cpu_registers regs;
	exec_cpuid(CURRENT_CPU, CPUID_NUM_FEATURES, &regs);
	
	memcpy(strintel, &regs.ebx, 4);
	memcpy(strintel + 4, &regs.edx, 4);
	memcpy(strintel + 8, &regs.ecx, 4);

	return (!strcmp("GenuineIntel", strintel));
}

static int virt_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return 1;

	return 0;
}

static char *virt_headline(void)
{
	return "Test CPU Virtualisation Configuration.";
}

static int virt_test1(fwts_framework *fw)
{
	fwts_log_info(fw, "Check if CPU is an AMD or Intel.");
	if (is_AMD()) {
		cpu = CPU_AMD;
		fwts_passed(fw, "CPU is an AMD.");
	} else if (is_Intel()) {
		cpu = CPU_INTEL;
		fwts_passed(fw, "CPU is an Intel.");
	} else {
		cpu = CPU_UNKNOWN;
		fwts_warning(fw, "CPU is unknown.");
	}
	return 0;
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
	return 0;
}
	

static fwts_framework_tests virt_tests[] = {
	virt_test1,
	virt_test2,
	NULL
};

static fwts_framework_ops virt_ops = {
	virt_headline,
	virt_init,
	NULL,
	virt_tests
};

 
/* 
 * This test checks if the virtual machine setup is done correctly by the BIOS
 */
FRAMEWORK(virt, &virt_ops, TEST_ANYTIME);
