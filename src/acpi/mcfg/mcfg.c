/*
 * Copyright (C) 2010-2012 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

static fwts_list *memory_map_list;
static fwts_acpi_table_info *mcfg_table;

static void compare_config_space(fwts_framework *fw, int segment, int device, unsigned char *space)
{
	fwts_list *lspci_output;
	fwts_list_link *item;

	char command[PATH_MAX];
	char compare_line[1024];

	snprintf(compare_line, sizeof(compare_line),
		"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		space[0],  space[1],  space[2],  space[3],
		space[4],  space[5],  space[6],  space[7],
		space[8],  space[9],  space[10], space[11],
		space[12], space[13], space[14], space[15]);

	snprintf(command, sizeof(command), "%s -vxxx -s %i:%i",
		fw->lspci, segment, device);

	if (fwts_pipe_exec(command, &lspci_output) == FWTS_EXEC_ERROR) {
		fwts_log_warning(fw, "Could not execute %s", command);
		return;
	}

	fwts_list_foreach(item, lspci_output) {
		char *line = fwts_text_list_text(item);

		fwts_chop_newline(line);

		if (strncmp(line, "00: ",4)==0) {
			if (strcmp(&line[4], compare_line)) {
				fwts_log_info(fw, "%s is read from MMCONFIG, but traditional gives :\n-%s-\n", &line[4], compare_line);
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"PCIConfigSpaceBad",
					"PCI config space appears to not work");
			} else
				fwts_passed(fw, "PCI config space verified");

			fwts_text_list_free(lspci_output);
			return ;
		}
	}
	fwts_text_list_free(lspci_output);
	fwts_log_warning(fw, "Possible PCI space config space defect?");
}

static int mcfg_init(fwts_framework *fw)
{
	if (fwts_check_executable(fw, fw->lspci, "lspci"))
		return FWTS_ERROR;

	if (fwts_acpi_find_table(fw, "MCFG", 0, &mcfg_table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (mcfg_table == NULL) {
		fwts_log_error(fw, "ACPI table MCFG not found. This table is required to check for PCI Express*");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int mcfg_deinit(fwts_framework *fw)
{
	if (memory_map_list)
		fwts_memory_map_table_free(memory_map_list);

	return FWTS_OK;
}

static int mcfg_test1(fwts_framework *fw)
{
	int nr, i;
	void *mapped_table_page;
	fwts_acpi_table_mcfg *mcfg = (fwts_acpi_table_mcfg*)mcfg_table->data;
	fwts_acpi_mcfg_configuration *config;
	int failed = 0;
	ssize_t mcfg_size;
	const char *memory_map_name;
	int page_size;

	page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1)
		page_size = 4096;

	memory_map_name = fwts_memory_map_name(fw->firmware_type);

	fwts_log_info(fw,
		"This test tries to validate the MCFG table by comparing the first "
		"16 bytes in the MMIO mapped config space with the 'traditional' config "
		"space of the first PCI device (root bridge). The MCFG data is only "
		"trusted if it is marked reserved in the %s",
		memory_map_name);
	fwts_log_nl(fw);

	if ((memory_map_list = fwts_memory_map_table_load(fw)) == NULL) {
		/* Not fatal, just means test will be less comprehensive */
		fwts_log_warning(fw, "No memory map table found");
	} else {
		fwts_memory_map_table_dump(fw, memory_map_list);
		fwts_log_nl(fw);
	}

	mcfg_size = mcfg_table->length;
	mcfg_size -= sizeof(fwts_acpi_table_mcfg);

	if (mcfg_size < 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "MCFGInvalidSize",
			"Invalid MCFG ACPI table size: got %zd bytes expecting more",
			mcfg_size + sizeof(fwts_acpi_table_mcfg));
		fwts_tag_failed(fw, FWTS_TAG_ACPI_INVALID_TABLE);
		fwts_advice(fw,
			"MCFG table must be least %d bytes (header size) with "
			"multiples of %d bytes for each MCFG entry.",
			36+8, (int)sizeof(fwts_acpi_mcfg_configuration));
		return FWTS_ERROR;
	}
	nr = mcfg_size / sizeof(fwts_acpi_mcfg_configuration);

	if (!nr) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MCFGNoEntries",
			"No MCFG ACPI table entries");
		return FWTS_ERROR;
	}

	if (mcfg_size != (ssize_t)(nr * sizeof(fwts_acpi_mcfg_configuration))) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "MCFGInvalidSize2",
			"MCFG table is not a multiple of record size");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_INVALID_TABLE);
		return FWTS_ERROR;
	}

	fwts_log_info(fw, "MCFG table found, size is %zd bytes (excluding header) (%i entries).",
			mcfg_size, nr);

	if (mcfg == NULL) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "MCFGInvalidTable",
			"Invalid MCFG ACPI table");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_INVALID_TABLE);
		return FWTS_ERROR;
	}

	if (memory_map_list == NULL)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MMapUnreadable",
			"Cannot check MCFG mmio space against memory map table: could not read memory map table.");

	config = &mcfg->configuration[0];
	for (i = 0; i < nr; i++, config++) {
		fwts_log_info(fw, "Configuration Entry #%d address : 0x%" PRIx64, i, config->base_address);

		if ((memory_map_list != NULL) &&
		    (!fwts_memory_map_is_reserved(memory_map_list, config->base_address))) {

			fwts_failed(fw, LOG_LEVEL_MEDIUM, "MCFGMMIONotReserved",
				"MCFG mmio config space at 0x%" PRIx64
				" is not reserved in the memory map table",
				config->base_address);
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			fwts_advice(fw,
				"The PCI Express specification states that the PCI Express configuration space should "
				"be defined in the MCFG table and *maybe* optionally defined in the %s "
				"if ACPI MCFG is present. "
				"Linux checks if the region is reserved in the memory map table and will reject the "
				"MMCONFIG if there is a discrepency between MCFG and the memory map table for the "
				"PCI Express region.  [See arch/x86/pci/mmconfig-shared.c pci_mmcfg_reject_broken()]. "
				"It is recommended that this is defined in the %s table for Linux.",
				memory_map_name, memory_map_name);
			failed++;
		}

		fwts_log_info_verbatum(fw, "Segment       : %" PRIu8, config->pci_segment_group_number);
		fwts_log_info_verbatum(fw, "Start bus     : %" PRIu8, config->start_bus_number);
		fwts_log_info_verbatum(fw, "End bus       : %" PRIu8, config->end_bus_number);
	}
	if (!failed)
		fwts_passed(fw, "MCFG mmio config space is reserved in memory map table.");

	/* Sanity check on first config */
	config = &mcfg->configuration[0];

	if ((mapped_table_page = fwts_mmap(config->base_address, page_size)) == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap table at 0x%" PRIx64 ".", config->base_address);
		return FWTS_ERROR;
	}

	compare_config_space(fw, config->pci_segment_group_number,
		0, (unsigned char *)mapped_table_page);

	fwts_munmap(mapped_table_page, page_size);

	return FWTS_OK;
}

static fwts_framework_minor_test mcfg_tests[] = {
	{ mcfg_test1, "Validate MCFG table." },
	{ NULL, NULL }
};

static fwts_framework_ops mcfg_ops = {
	.description = "MCFG PCI Express* memory mapped config space.",
	.init        = mcfg_init,
	.deinit      = mcfg_deinit,
	.minor_tests = mcfg_tests
};

FWTS_REGISTER(mcfg, &mcfg_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);

#endif
