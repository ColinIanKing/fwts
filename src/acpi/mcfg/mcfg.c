/*
 * Copyright (C) 2010-2011 Canonical
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

static fwts_list *memory_map_list;
static fwts_acpi_table_info *mcfg_table;

/* Defined in PCI Firmware Specification 3.0 */
struct mcfg_entry {
	unsigned int low_address;
	unsigned int high_address;
	unsigned short segment;
	unsigned char start_bus;
	unsigned char end_bus;
	unsigned char  reserved[4];
} __attribute__ ((packed));

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

	snprintf(command, sizeof(command), "%s -vxxx -s %i:%i", fw->lspci, segment, device);

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
				fwts_failed(fw, LOG_LEVEL_MEDIUM, "PCI config space appears to not work");
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
	const uint8_t *table_ptr;
	const uint8_t *table_page;	
	void *mapped_table_page;
	struct mcfg_entry *table, firstentry;
	int failed = 0;
	int mcfg_size;
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
	mcfg_size -= 36; /* general ACPI header */
	mcfg_size -= 8;  /* 8 bytes of padding */

	if ((int)mcfg_size<0) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "Invalid MCFG ACPI table size: got %d bytes expecting more", mcfg_size + 36 + 8);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_INVALID_TABLE);
		fwts_advice(fw, "MCFG table must be least %d bytes (header size) with multiples of %d"
				"bytes for each MCFG entry.", 36+8, (int)sizeof(struct mcfg_entry));
		return FWTS_ERROR;
	}
	nr = mcfg_size / sizeof(struct mcfg_entry);

	if (!nr) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "No MCFG ACPI table entries");
		return FWTS_ERROR;
	}

	if ((nr * sizeof(struct mcfg_entry)) != mcfg_size) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "MCFG table is not a multiple of record size");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_INVALID_TABLE);
		return FWTS_ERROR;
	}

	fwts_log_info(fw, "MCFG table found, size is %i bytes (excluding header) (%i entries).",
			mcfg_size, nr);

	table_page = table_ptr = (const uint8_t *)mcfg_table->data;

	if (table_page == NULL) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "Invalid MCFG ACPI table");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_INVALID_TABLE);
		return FWTS_ERROR;
	}

	table_ptr += 36 + 8; /* 36 for the acpi header, 8 for padding */
        table = (struct mcfg_entry *) table_ptr;

	firstentry = *table;

	if (memory_map_list == NULL)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Cannot check MCFG mmio space against memory map table: could not read memory map table.");

	for (i = 0; i<nr; i++) {
		fwts_log_info(fw, "Entry address : 0x%x\n", table->low_address);

		if ((memory_map_list != NULL) && (!fwts_memory_map_is_reserved(memory_map_list, table->low_address))) {
			
			fwts_failed_medium(fw, "MCFG mmio config space at 0x%x is not reserved in the memory map table", table->low_address);
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			fwts_advice(fw, "The PCI Express specification states that the PCI Express configuration space should "
					"be defined in the MCFG table and *maybe* optionally defined in the %s "
					"if ACPI MCFG is present. "
					"Linux checks if the region is reserved in the memory map table and will reject the "
					"MMCONFIG if there is a discrepency between MCFG and the memory map table for the "
					"PCI Express region.  [See arch/x86/pci/mmconfig-shared.c pci_mmcfg_reject_broken()]. "
					"It is recommended that this is defined in the %s table for Linux.",
					memory_map_name, memory_map_name);
			failed++;
		}

		fwts_log_info_verbatum(fw, "High  address : 0x%x \n", table->high_address);
		fwts_log_info_verbatum(fw, "Segment       : %i \n", table->segment);
		fwts_log_info_verbatum(fw, "Start bus     : %i \n", table->start_bus);
		fwts_log_info_verbatum(fw, "End bus       : %i \n", table->end_bus);

		table++;
	}
	if (!failed)
		fwts_passed(fw, "MCFG mmio config space is reserved in memory map table.");

	if ((mapped_table_page = fwts_mmap(firstentry.low_address, page_size)) == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap table at 0x%x.", firstentry.low_address);
		return FWTS_ERROR;
	}

	compare_config_space(fw, firstentry.segment, 0, (unsigned char *)mapped_table_page);

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

FWTS_REGISTER(mcfg, &mcfg_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
