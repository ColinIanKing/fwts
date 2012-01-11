/*
 * Copyright (C) 2011 Canonical
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/pci.h>

#include "fwts.h"

int fwts_facp_get_aspm_control(fwts_framework *fw, int *aspm)
{
	fwts_acpi_table_info *table;
	fwts_acpi_table_fadt *fadt;

	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK) {
		return FWTS_ERROR;
	}
	fadt = (fwts_acpi_table_fadt*)table->data;

	if ((fadt->iapc_boot_arch & FWTS_FACP_IAPC_BOOT_ARCH_PCIE_ASPM_CONTROLS) == 0) {
		*aspm = 1;
		fwts_log_info(fw, "PCIE ASPM is controlled by Linux kernel.");
	} else {
		*aspm = 0;
		fwts_log_info(fw, "PCIE ASPM is not controlled by Linux kernel.");
	}

	return FWTS_OK;
}

int aspm_check_configuration(fwts_framework *fw)
{
	int ret;
	int aspm_facp;

	ret = fwts_facp_get_aspm_control(fw, &aspm_facp);
	if (ret == FWTS_ERROR) {
		fwts_log_info(fw, "No valid FACP information present: cannot test aspm.");
		return FWTS_ERROR;
	}

	return ret;
}


static fwts_framework_minor_test aspm_tests[] = {
	{ aspm_check_configuration, "PCIe ASPM configuration test." },
	{ NULL, NULL }
};

static fwts_framework_ops aspm_ops = {
	.description = "PCIe ASPM check.",
	.minor_tests = aspm_tests
};

FWTS_REGISTER(aspm, &aspm_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

