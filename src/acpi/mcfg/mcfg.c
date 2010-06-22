/*
 * Copyright (C) 2010 Canonical
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h> 
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>

#include "fwts.h"

static fwts_list *e820_list;
static void *mcfg_table;
static int mcfg_size;

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
	fwts_list_element *item;

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

	for (item = lspci_output->head; item != NULL; item = item->next) {
		char *line = fwts_text_list_text(item);

		fwts_chop_newline(line);

		if (strncmp(line, "00: ",4)==0) {
			if (strcmp(&line[4], compare_line)) {
				fwts_log_info(fw, "%s is read from MMCONFIG, but traditional gives :\n-%s-\n", &line[4]);
				fwts_failed(fw, "PCI config space appears to not work");
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
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	if (fwts_check_executable(fw, fw->lspci, "lspci"))
		return FWTS_ERROR;

	if ((mcfg_table = fwts_acpi_table_load(fw, "MCFG", 0, &mcfg_size)) == NULL) {
		fwts_log_error(fw, "MCFG ACPI table not loaded. This table is required to check for PCI Express*");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int mcfg_deinit(fwts_framework *fw)
{
	if (mcfg_table)
		free(mcfg_table);

	if (e820_list)
		fwts_e820_table_free(e820_list);

	return FWTS_OK;
}

static char *mcfg_headline(void)
{
	return "MCFG PCI Express* memory mapped config space.";
}

static int mcfg_test1(fwts_framework *fw)
{
	int fd;
	int nr, i;
	uint8 *table_ptr, *table_page;
	struct mcfg_entry *table, firstentry;
	int failed = 0;
	
	fwts_log_info(fw,
		"This test tries to validate the MCFG table by comparing the first "
		"16 bytes in the MMIO mapped config space with the 'traditional' config "
		"space of the first PCI device (root bridge). The MCFG data is only "
		"trusted if it is marked reserved in the E820 table.");
	fwts_log_info(fw, "\n");

	if ((e820_list = fwts_e820_table_load(fw)) == NULL) {
		/* Not fatal, just means test will be less comprehensive */
		fwts_log_warning(fw, "No E820 table found");
	}
	fwts_log_info(fw, "\n");

	mcfg_size -= 36; /* general ACPI header */
	mcfg_size -= 8;  /* 8 bytes of padding */

	if ((int)mcfg_size<0) {
		fwts_failed(fw, "Invalid MCFG ACPI table size: got %d bytes expecting more", mcfg_size + 36 + 8);
		fwts_advice(fw, "MCFG table must be least %d bytes (header size) with multiples of %d"
				"bytes for each MCFG entry.", 36+8, sizeof(struct mcfg_entry));
		return FWTS_ERROR;
	}
	nr = mcfg_size / sizeof(struct mcfg_entry);

	if (!nr) {
		fwts_failed(fw, "No MCFG ACPI table entries");
		return FWTS_ERROR;
	}

	if ((nr * sizeof(struct mcfg_entry)) != mcfg_size) {
		fwts_failed(fw, "MCFG table is not a multiple of record size");
		return FWTS_ERROR;
	}

	fwts_log_info(fw, "MCFG table found, size is %i bytes (excluding header) (%i entries).", 
			mcfg_size, nr);

	table_page = table_ptr = mcfg_table;

	if (table_page==NULL) {
		fwts_failed(fw, "Invalid MCFG ACPI table");
		return FWTS_ERROR;
	}

	table_ptr += 36 + 8; /* 36 for the acpi header, 8 for padding */
        table = (struct mcfg_entry *) table_ptr;

	firstentry = *table;

	if (e820_list == NULL)
		fwts_failed(fw, "Cannot check MCFG mmio space against E820 table because E820 table could not load.");

	for (i = 0; i<nr; i++) {
		fwts_log_info(fw, "Entry address : %x\n", table->low_address);

		if ((e820_list != NULL) && (!fwts_e820_is_reserved(e820_list, table->low_address))) {
			fwts_failed_high(fw, "E820: MCFG mmio config space at 0x%x is not reserved in the E820 table", table->low_address);
			failed++;
		}

		fwts_log_info_verbatum(fw, "High  address : %x \n", table->high_address);
		fwts_log_info_verbatum(fw, "Segment       : %i \n", table->segment);
		fwts_log_info_verbatum(fw, "Start bus     : %i \n", table->start_bus);
		fwts_log_info_verbatum(fw, "End bus       : %i \n", table->end_bus);

		table++;
	}
	if (!failed)
		fwts_passed(fw, "MCFG mmio config space is reserved in E820 table.");

	if ((fd = open("/dev/mem", O_RDONLY)) < 0) {
		fwts_log_error(fw, "Cannot open /dev/mem.");
		return FWTS_ERROR;
	}

	if ((table_page = mmap(NULL, getpagesize(), PROT_READ, MAP_SHARED, fd, firstentry.low_address)) == NULL) {
		fwts_log_error(fw, "Cannot mmap onto /dev/mem.");
		return FWTS_ERROR;
	}

        if (table_page==(void*)-1) {
		fwts_log_error(fw, "Cannot mmap onto /dev/mem.");
		close(fd);
                return FWTS_ERROR;
        }

	compare_config_space(fw, firstentry.segment, 0, (unsigned char *)table_page);

	munmap(table_page, getpagesize());
	close(fd);
	
	return FWTS_OK;
}

static fwts_framework_tests mcfg_tests[] = {
	mcfg_test1,
	NULL
};

static fwts_framework_ops mcfg_ops = {
	mcfg_headline,
	mcfg_init,
	mcfg_deinit,
	mcfg_tests
};

FRAMEWORK(mcfg, &mcfg_ops, TEST_ANYTIME);
