/* 
 * Copyright (C) 2010 Canonical
 *
 * from ideas in check-bios-nx
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
#include <string.h>

#include "fwts.h"

static char *nx_headline(void)
{
	return "Test if CPU NX is disabled by the BIOS.";
}

static int nx_test1(fwts_framework *fw)
{
	fwts_cpuinfo_x86 *fwts_nx_cpuinfo;

	if ((fwts_nx_cpuinfo = fwts_cpu_get_info(0)) == NULL) {
		fwts_log_error(fw, "Cannot get CPU info");
		return FWTS_ERROR;
	}

	if (strstr(fwts_nx_cpuinfo->flags," nx")) {
		fwts_passed(fw, "CPU has NX flags, BIOS is not disabling it.");
		fwts_cpu_free_info(fwts_nx_cpuinfo);
		return FWTS_OK;
	}
	
	if (strstr(fwts_nx_cpuinfo->flags," pae") == NULL) {
		fwts_passed(fw, "CPU is not PAE capable, so it does not have NX.");
		fwts_cpu_free_info(fwts_nx_cpuinfo);
		return FWTS_OK;
	}

	if ((fwts_nx_cpuinfo->x86 == -1) || 
	    (fwts_nx_cpuinfo->x86_model == -1)) {
		fwts_failed_high(fw, "No model or family found for this CPU. Please check /proc/cpuinfo.");
		fwts_cpu_free_info(fwts_nx_cpuinfo);
		return FWTS_OK;
	}

	if (((fwts_nx_cpuinfo->x86 == 6)  && (fwts_nx_cpuinfo->x86_model >= 14)) ||
	    ((fwts_nx_cpuinfo->x86 == 15) && (fwts_nx_cpuinfo->x86_model >= 3))  ||
	    (fwts_nx_cpuinfo->x86 > 15)) {
		fwts_failed_high(fw,
			"The CPU is family %d, model %d and has NX capabilities but they are not enabled.",
			fwts_nx_cpuinfo->x86, fwts_nx_cpuinfo->x86_model);
		fwts_advice(fw,
			"The system cannot use the protection features provided by NX because "
			"the BIOS is configured to disable this capability. Please ensure this "
			"is enabled in the BIOS. For more information please consult "
			"https://wiki.ubuntu.com/Security/CPUFeatures");
	} else
		fwts_passed(fw, 
			"The CPU is family %d, model %s and does not have NX capabilities.",
			fwts_nx_cpuinfo->x86, fwts_nx_cpuinfo->x86_model);

	fwts_cpu_free_info(fwts_nx_cpuinfo);
	return FWTS_OK;
}

static int nx_test2(fwts_framework *fw)
{
	fwts_cpuinfo_x86 *fwts_nx_cpuinfo;
	int i;
	int n;
	int cpu0_has_nx;
	int failed = 0;

	fwts_log_info(fw, 
		"This test verifies that all CPUs have the same NX flag setting. "
		"Although rare, BIOS may set the NX flag differently "
		"per CPU. ");

	if ((n = fwts_cpu_enumerate()) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot determine number of CPUs");
		return FWTS_ERROR;
	}

	if (n == 1) {
		fwts_log_info(fw, "Only one CPU, no need to run test.");
		return FWTS_OK;
	}

	if ((fwts_nx_cpuinfo = fwts_cpu_get_info(0)) == NULL) {
		fwts_log_error(fw, "Cannot get CPU info");
		return FWTS_ERROR;
	}
	cpu0_has_nx = (strstr(fwts_nx_cpuinfo->flags," nx") != NULL);
	fwts_cpu_free_info(fwts_nx_cpuinfo);

	for (i=1; i<n; i++) {
		if ((fwts_nx_cpuinfo = fwts_cpu_get_info(0)) == NULL) {
			fwts_failed(fw, "Cannot get CPU%d info", i);
			fwts_cpu_free_info(fwts_nx_cpuinfo);
			return FWTS_ERROR;
		}
		if (cpu0_has_nx != (strstr(fwts_nx_cpuinfo->flags," nx") != NULL)) {
			fwts_failed(fw, "CPU%d has different NX flags to CPU0.");
			failed++;
		}
		fwts_cpu_free_info(fwts_nx_cpuinfo);
	}

	if (!failed) 
		fwts_passed(fw, "All %d CPUs have the same NX flag.", n);

	return FWTS_OK;
}

static fwts_framework_tests nx_tests[] = {
	nx_test1,
	nx_test2,
	NULL
};

static fwts_framework_ops nx_ops = {
	nx_headline,
	NULL,
	NULL,
	nx_tests
};
 
FWTS_REGISTER(nx, &nx_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
