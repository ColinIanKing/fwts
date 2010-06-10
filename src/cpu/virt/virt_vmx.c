/* 
 * Copyright (C) 2006, Intel Corporation
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
 * This test checks if the VT-setup is done correctly by the BIOS
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>
#include <sched.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "fwts.h"

#define MSR_FEATURE_CONTROL	0x03a

static uint64 readmsr(int cpu, unsigned long offset)
{
	char buffer[PATH_MAX];
	FILE *file;
	int fd;
	uint64 msr_value=0xffffffff;
	unsigned char *msr_value_buf;
	int ret;

	msr_value_buf = (unsigned char *)&msr_value;
	sprintf(buffer, "/dev/msr%i", cpu);
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

int cpu_has_vmx(void)
{
	int found = 0;
	fwts_list *cpuinfo;
	fwts_list_element *item;

	if ((cpuinfo = fwts_file_open_and_read("/proc/cpuinfo")) == NULL)
		return 0;

	for (item = cpuinfo->head; item != NULL; item = item->next) {
		char *line = fwts_text_list_text(item);
		if (strstr(line,"flags") && strstr(line," vmx")) {
			found = 1;
			break;
		}
	}
	fwts_text_list_free(cpuinfo);

	return found;
}

static int vt_locked_by_bios(void)
{
	uint64_t msr;

	msr = readmsr(0, MSR_FEATURE_CONTROL);
	return (msr & 5) == 1; /* VT capable but locked by bios*/
}

void virt_check_vmx(fwts_framework *fw)
{
	fwts_log_info(fw, "Check VT/VMX Virtualization extensions are set up correctly");

	if (!cpu_has_vmx()) 
		fwts_log_info(fw, "Processor does not support Virtualization extensions");
	else 
		if (vt_locked_by_bios())
			fwts_failed(fw, "Virtualization extensions supported but disabled by BIOS");
		else
			fwts_passed(fw, "Virtualization extensions supported and enabled by BIOS");
}
