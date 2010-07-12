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

#include "fwts.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASK_4K 	0xfff

/* See http://download.intel.com/technology/computing/vptech/Intel(r)_VT_for_Direct_IO.pdf */

/* DMA Remapping Reporting Table (DMAR) */
struct acpi_table_dmar {
	uint8 head[36];
	uint8 haw;
	uint8 flags;
	uint8 reserved[10];
} __attribute__((packed));

#define DMAR_HEADER_SIZE sizeof(struct acpi_table_dmar)

struct acpi_dmar_entry_header {
	uint16     type;
	uint16     length;
} __attribute__((packed));

enum acpi_dmar_entry_type {
	ACPI_DMAR_DRHD = 0,
	ACPI_DMAR_RMRR,
	ACPI_DMAR_ASTR,
	ACPI_DMAR_ENTRY_COUNT
};

struct acpi_table_drhd {
	struct acpi_dmar_entry_header header;
	uint8      flags;  	/* BIT0: INCLUDE_ALL */
	uint8      reserved;
	uint16     segment;
	uint64     address; 	/* register base address for this drhd */
} __attribute__ ((packed));

struct acpi_table_rmrr {
	struct acpi_dmar_entry_header header;
	uint16     reserved;
	uint16     segment;
	uint64     base_address;
	uint64     end_address;
} __attribute__ ((packed));

enum acpi_dev_scope_type {
	ACPI_DEV_ENDPOINT=0x01,
	ACPI_DEV_P2PBRIDGE,
	ACPI_DEV_IOAPIC,
	ACPI_DEV_HPET,
	ACPI_DEV_ENTRY_COUNT
};

struct acpi_dev_scope {
	uint8      dev_type;
	uint8      length;
	uint16     reserved;
	uint8      enumeration_id;
	uint8      start_bus;
} __attribute__((packed));

struct acpi_pci_path {
	uint8      dev;
	uint8      fn;
} __attribute__((packed));

#define MIN_SCOPE_LEN (sizeof(struct acpi_pci_path) + \
	sizeof(struct acpi_dev_scope))

/*
 * = -1, no such device
 * = 0, normal pci device
 * = 1, pci bridge, sec_bus gets set
 */
static int read_pci_device_secondary_bus_number(uint8 seg,
	uint8 bus, uint8 dev,
	uint8 fn, uint8 *sec_bus)
{
	FILE *file;
	char path[PATH_MAX];
	char configs[64];
	size_t count;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%04x:%02x:%02x.%d/config", seg, bus, dev, fn);
	if ((file = fopen(path, "r")) == NULL)
		return -1;

	count = fread(configs, sizeof(char), 64, file);
	fclose(file);

	if (count < 64)
		return -1;

	/* header type is at 0x0e */
	if ((configs[0xe] & 0x7f) != 1) /* not a pci bridge */
		return 0;
	*sec_bus = configs[0x19]; 	/* secondary bus number */
	return 1;
}

static int acpi_parse_one_dev_scope(fwts_framework *fw, struct acpi_dev_scope *scope, int seg)
{
	struct acpi_pci_path *path;
	int count;
	uint8 bus;
	uint8 sec_bus = 0;
	int dev_type;

	if (scope->length < MIN_SCOPE_LEN) {
		fwts_failed_medium(fw, "Invalid device scope entry.");
		return FWTS_ERROR;
	}

	if (scope->dev_type >= ACPI_DEV_ENTRY_COUNT) {
		fwts_warning(fw, "Unknown device scope type.");
		return FWTS_ERROR;
	}

	if (scope->dev_type > ACPI_DEV_P2PBRIDGE) {
		fwts_log_info(fw, "Unknown device scope type, the test case should be fixed.");
		return FWTS_ERROR;
	}

	bus = scope->start_bus;
	count = (scope->length - sizeof(struct acpi_dev_scope))
		/sizeof(struct acpi_pci_path);
	path = (struct acpi_pci_path *)(scope + 1);

	if (!count)
		goto error;
	dev_type = 1;
	while (count) {
		if (dev_type <= 0) /* last device isn't a pci bridge */
			goto error;
		dev_type = read_pci_device_secondary_bus_number(seg, bus,
			path->dev, path->fn, &sec_bus);
		if (dev_type < 0) /* no such device */
			goto error;
		path++;
		count--;
		bus = sec_bus;
	}

	if ((scope->dev_type == ACPI_DEV_ENDPOINT && dev_type > 0) ||
		(scope->dev_type == ACPI_DEV_P2PBRIDGE && dev_type == 0)) {
		fwts_failed(fw, "Device scope type not match.");
		return FWTS_ERROR;
	}
	return FWTS_OK;
error:
	fwts_failed(fw, "Device scope device not found.");
	return FWTS_ERROR;
}

static int acpi_parse_dev_scope(fwts_framework *fw, void *start, void *end, int seg)
{
	struct acpi_dev_scope *scope;
	int ret;

	while (start < end) {
		scope = start;
		ret = acpi_parse_one_dev_scope(fw, scope, seg);
		if (ret)
			return ret;
		start += scope->length;
	}
	return FWTS_OK;
}

static int acpi_parse_one_drhd(fwts_framework *fw, struct acpi_dmar_entry_header *header)
{
	static int include_all;
	struct acpi_table_drhd *drhd = (struct acpi_table_drhd*)header;

	if (drhd->address & MASK_4K) {
		fwts_failed(fw, "Invalid drhd register address.");
		return FWTS_ERROR;
	}
	if (drhd->flags & 1) {
		if (include_all == 1) {
			fwts_failed(fw, "Multiple drhds have include_all flag set.");
			return FWTS_ERROR;
		}
		include_all = 1;
	} else {
		return acpi_parse_dev_scope(fw, (void *)(drhd + 1),
			((void *)drhd) + header->length, drhd->segment);
	}
	return FWTS_OK;
}

static int acpi_parse_one_rmrr(fwts_framework *fw, struct acpi_dmar_entry_header *header)
{
	struct acpi_table_rmrr *rmrr = (struct acpi_table_rmrr *)header;

	if ((rmrr->base_address & MASK_4K)
		|| (rmrr->end_address < rmrr->base_address)
		|| ((rmrr->end_address - rmrr->base_address + 1) & MASK_4K)) {
		fwts_failed(fw, "Invalid rmrr range address.");
		return FWTS_ERROR;
	}
	return acpi_parse_dev_scope(fw, (void *)(rmrr + 1),
		((void*)rmrr) + header->length, rmrr->segment);
}

static int dmar_acpi_table_check(fwts_framework *fw)
{
	int size;
	uint8 *table_ptr;
	struct acpi_dmar_entry_header *header;
	int failed = 0;

	if ((table_ptr = fwts_acpi_table_load(fw, "DMAR", 0, &size)) == NULL) {
		fwts_log_info(fw, "Cannot load DMAR table. This is not necessarily a failure as most systems do not have this table.");
		return FWTS_ERROR;
	}
	if (size <= DMAR_HEADER_SIZE) {
		fwts_failed(fw, "Invalid DMAR ACPI table.");
		return FWTS_ERROR;
	}

	header = (struct acpi_dmar_entry_header *)(table_ptr+DMAR_HEADER_SIZE);
	while ((unsigned long)header < (unsigned long)(table_ptr + size)) {		
		if ((header->type == ACPI_DMAR_DRHD) && 
		    (acpi_parse_one_drhd(fw, header) != FWTS_OK)) {
			failed++;
			break;
		}
		if ((header->type == ACPI_DMAR_RMRR) && 
		    (acpi_parse_one_rmrr(fw, header) != FWTS_OK)) {
			failed++;
			break;
		}
		header = ((void *)header) + header->length;
	}
	free(table_ptr);

	if (!failed)
		fwts_passed(fw, "DMAR ACPI table has passed test.");

	return FWTS_OK;
}

static void acpiinfo_check(fwts_framework *fw, char *line, char *prevline, void *private, int *errors)
{
        if (strstr(line, "DMAR:[fault reason")) 
		fwts_failed(fw, "Found DMAR error: %s", line);
}

static char *dmar_headline(void)
{
	return "Check sane DMA Remapping (VT-d).";
}

static int dmar_test1(fwts_framework *fw)
{
	if (dmar_acpi_table_check(fw) == FWTS_OK) {
		fwts_list *klog;
		int errors = 0;

		if ((klog = fwts_klog_read()) == NULL) {
                	fwts_log_error(fw, "Cannot read kernel log.");
                	return FWTS_ERROR;
        	}
		if (fwts_klog_scan(fw, klog, acpiinfo_check, NULL, NULL, &errors)) {
                	fwts_log_error(fw, "Failed to scan kernel log.");
                	return FWTS_ERROR;
		}
		if (errors == 0)
			fwts_passed(fw, "Found no DMAR errors in kernel log.");
		fwts_klog_free(klog);
        }

	return FWTS_OK;
}

static fwts_framework_tests dmar_tests[] = {
	dmar_test1,
	NULL
};

static fwts_framework_ops dmar_ops = {
	dmar_headline,
	NULL,
	NULL,
	dmar_tests
};

FWTS_REGISTER(dmar, &dmar_ops, FWTS_TEST_ANYTIME, FWTS_EXPERIMENTAL);
