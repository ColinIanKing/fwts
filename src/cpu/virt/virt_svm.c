/* 
 * Copyright (C) 2006, Intel Corp 
 * Copyright (C) 2007, AMD Inc 
 * 
 * This file is part of the Linux-ready Firmware Developer Kit
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation;version 2.1 of the License.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301 USA
 */
 
 
/* 
 * This test checks if the SVM-setup is done correctly by the BIOS
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "fwts.h"

#define CPUID_FAM_REV 0x1
#define MSR_FEATURE_CONTROL	0xC0000080

static uint64 readmsr(int cpu, unsigned long offset)
{
	char buffer[PATH_MAX];
	FILE *file;
	int fd;
	uint64 msr_value=0xffffffff;
	unsigned char *msr_value_buf;
	int ret;

	msr_value_buf = (unsigned char *)&msr_value;
	snprintf(buffer, sizeof(buffer), "/dev/msr%i", cpu);
	file = fopen(buffer, "r");
	if (!file) {
		printf("Error: fopen failed \n");
		return -1;
	}
	fd = fileno(file);

	ret = pread(fd, msr_value_buf, 8, offset);	
	fclose(file);
	if (ret<0) {
		printf("Error: pread failed %d\n, errno=%d", ret, errno);
		return -2;
	}

	return msr_value;
}

int cpu_has_svm(void)
{
	int found = 0;
	fwts_list *cpuinfo;
	fwts_list_element *item;

	if ((cpuinfo = fwts_file_open_and_read("/proc/cpuinfo")) == NULL)
		return 0;

	for (item = cpuinfo->head; item != NULL; item = item->next) {
		char *line = fwts_text_list_text(item);
		if (strstr(line,"flags") && strstr(line," svm")) {
			found = 1;
			break;
		}
	}
	fwts_text_list_free(cpuinfo);

	return found;
}

static int can_lock_with_msr(void)
{
	int family = 0xF;
	cpu_registers regs;
	exec_cpuid(CURRENT_CPU, CPUID_FAM_REV, &regs);
	
	family = (regs.eax>>8 & 0xF) + (regs.eax>>20 & 0xFF);
	
	return (family & 0x10);
}

int vt_locked_by_bios(void)
{
	uint64 msr;
	
	if (!can_lock_with_msr())
		return 0;
	
	msr = readmsr(0, MSR_FEATURE_CONTROL);
	return (msr & 0x1000) == 1; /* SVM capable but locked by bios*/
}

void virt_check_svm(fwts_framework *fw)
{
	fwts_log_info(fw, "Check SVM Virtualization extensions are set up correctly");
		
	if (!cpu_has_svm()) 
		fwts_log_info(fw, "Processor does not support Virtualization extensions");
	else 
		if (vt_locked_by_bios())
			fwts_failed(fw, "Virtualization extensions supported but disabled by BIOS");
		else
			fwts_passed(fw, "Virtualization extensions supported and enabled by BIOS");
}
