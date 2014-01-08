/*
 * Copyright (C) 2010-2014 Canonical
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "fwts.h"

#define BIOS_START	(0x000e0000)		/* Start of BIOS memory */
#define BIOS_END  	(0x000fffff)		/* End of BIOS memory */
#define BIOS_LENGTH	(BIOS_END - BIOS_START)	/* Length of BIOS memory */
#define PAGE_SIZE	(4096)

#define ACPI_MAX_TABLES	(64)			/* Max number of ACPI tables */

static fwts_acpi_table_info	tables[ACPI_MAX_TABLES];
static int acpi_tables_loaded = 0;

/*
 *  fwts_acpi_find_rsdp_efi()
 *  	Get RSDP address from EFI if possible
 */
static void *fwts_acpi_find_rsdp_efi(void)
{
	return fwts_scan_efi_systab("ACPI20");
}

/*
 *  fwts_acpi_find_rsdp_klog()
 *	Get RSDP by parsing kernel log
 */
static void *fwts_acpi_find_rsdp_klog(void)
{
	fwts_list *klog;
	fwts_list_link *item;
	void *rsdp = NULL;

	if ((klog = fwts_klog_read()) == NULL)
		return NULL;

	fwts_list_foreach(item, klog) {
		char *text = fwts_text_list_text(item);
		char *ptr = strstr(text, "ACPI: RSDP");

		if (ptr) {
			rsdp = (void *)strtoul(ptr + 11, NULL, 16);
			break;
		}
	}

	fwts_text_list_free(klog);

	return rsdp;
}

/*
 *  fwts_acpi_find_rsdp_bios()
 *	Find RSDP address by scanning BIOS memory
 */
static void *fwts_acpi_find_rsdp_bios(void)
{
#ifdef FWTS_ARCH_INTEL
	uint8_t *bios;
	uint8_t *ptr;
	fwts_acpi_table_rsdp *rsdp;
	void *addr = 0;

	if ((bios = fwts_mmap(BIOS_START, BIOS_LENGTH)) == FWTS_MAP_FAILED)
		return 0;

	/* Scan BIOS for RSDP, ACPI spec states it is aligned on 16 byte intervals */
	for (ptr = bios; ptr < (bios+BIOS_LENGTH); ptr += 16) {
		rsdp = (fwts_acpi_table_rsdp*)ptr;
		/* Look for RSD PTR string */
		if (strncmp(rsdp->signature, "RSD PTR ",8) == 0) {
			int length = (rsdp->revision < 1) ? 20 : 36;
			if (fwts_checksum(ptr, length) == 0) {
				addr = (void*)(BIOS_START+(ptr - bios));
				break;
			}
		}
	}
	(void)fwts_munmap(bios, BIOS_LENGTH);

	return addr;
#else
	return NULL;
#endif
}

/*
 *  fwts_acpi_get_rsdp()
 *	given the address of the rsdp, map in the region, copy it and
 *	return the rsdp table. Return NULL if fails.
 */
static fwts_acpi_table_rsdp *fwts_acpi_get_rsdp(void *addr, size_t *rsdp_len)
{
	uint8_t *mem;
	fwts_acpi_table_rsdp *rsdp = NULL;
	*rsdp_len = 0;

	if ((mem = fwts_mmap((off_t)addr, sizeof(fwts_acpi_table_rsdp))) == FWTS_MAP_FAILED)
		return NULL;

	rsdp = (fwts_acpi_table_rsdp*)mem;
	/* Determine original RSDP size from revision. */
	*rsdp_len = (rsdp->revision < 1) ? 20 : 36;

	/* Must have the correct signature */
	if (strncmp(rsdp->signature, "RSD PTR ", 8))
		goto out;

	/* Assume version 2.0 size, we don't care about a few bytes over allocation if it's version 1.0 */
	if ((rsdp = (fwts_acpi_table_rsdp*)fwts_low_calloc(1, sizeof(fwts_acpi_table_rsdp))) == NULL)
		goto out;

	memcpy(rsdp, mem, *rsdp_len);
out:
	(void)fwts_munmap(mem, sizeof(fwts_acpi_table_rsdp));

	return rsdp;
}

/*
 *  fwts_acpi_load_table()
 *	given the address of a ACPI table, map in firmware, find out size,
 *	copy it and return the copy. Returns NULL if fails.
 */
static void *fwts_acpi_load_table(const off_t addr)
{
	fwts_acpi_table_header *hdr;
	void *mem;
	void *table;
	int len;

	if ((hdr = fwts_mmap((off_t)addr, sizeof(fwts_acpi_table_header))) == FWTS_MAP_FAILED)
		return NULL;

	len = hdr->length;
	if (len < (int)sizeof(fwts_acpi_table_header))
		return NULL;

	(void)fwts_munmap(hdr, sizeof(fwts_acpi_table_header));

	if ((table = fwts_low_calloc(1, len)) == NULL)
		return NULL;

	if ((mem = fwts_mmap((off_t)addr, len)) == FWTS_MAP_FAILED)
		return NULL;

	memcpy(table, mem, len);
	(void)fwts_munmap(mem, len);

	return table;
}

/*
 *  fwts_acpi_add_table()
 *	Add a table to internal ACPI table cache. Ignore duplicates based on
 *	their address.
 */
static void fwts_acpi_add_table(
	const char *name,			/* Table Name */
	const void *table,			/* Table binary blob */
	const uint64_t addr,			/* Address of table */
	const size_t length,			/* Length of table */
	const fwts_acpi_table_provenance provenance)
						/* Where we got the table from */
{
	int i;
	int which = 0;

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (addr && tables[i].addr == addr) {
			/* We don't need it, it's a duplicate, so free and return */
			fwts_low_free(table);
			return;
		}
		if (strncmp(tables[i].name, name, 4) == 0)
			which++;
		if (tables[i].data == NULL) {
			strncpy(tables[i].name, name, 4);
			tables[i].name[4] = 0;
			tables[i].data = table;
			tables[i].addr = addr;
			tables[i].length = length;
			tables[i].which = which;
			tables[i].provenance = provenance;
			return;
		}
	}
}

/*
 *  fwts_acpi_free_tables()
 *	free up the cached copies of the ACPI tables
 */
int fwts_acpi_free_tables(void)
{
	int i;

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (tables[i].data) {
			fwts_low_free(tables[i].data);
			memset(&tables[i], 0, sizeof(fwts_acpi_table_info));
		}
	}
	return FWTS_OK;
}

/*
 *  fwts_acpi_handle_fadt_tables()
 *	depending on whether 32 or 64 bit address is usable, get the FADT table
 *	address and load the FADT.
 */
static void fwts_acpi_handle_fadt_tables(
	fwts_acpi_table_fadt *fadt,
	const uint32_t *addr32,
	const uint64_t *addr64,
	const fwts_acpi_table_provenance provenance)
{
	off_t addr;
	fwts_acpi_table_header *header;

	if ((addr64 != 0) && (fadt->header.length >= 140))
		addr = (off_t)*addr64;
	else if ((addr32 !=0) && (fadt->header.length >= 44))
		addr = (off_t)*addr32;
	else addr = 0;

	if (addr) {
		if ((header = fwts_acpi_load_table(addr)) != NULL)
			fwts_acpi_add_table(header->signature, header,
				(uint64_t)addr, header->length, provenance);
	}
}

static void fwts_acpi_handle_fadt(fwts_acpi_table_fadt *fadt, fwts_acpi_table_provenance provenance)
{
	fwts_acpi_handle_fadt_tables(fadt, &fadt->dsdt, &fadt->x_dsdt, provenance);
	fwts_acpi_handle_fadt_tables(fadt, &fadt->firmware_control, &fadt->x_firmware_ctrl, provenance);
}

/*
 *  fwts_acpi_load_tables_from_firmware()
 *  	Load up cached copies of all the ACPI tables
 */
static int fwts_acpi_load_tables_from_firmware(fwts_framework *fw)
{
	fwts_acpi_table_rsdp *rsdp;
	fwts_acpi_table_xsdt *xsdt;
	fwts_acpi_table_rsdt *rsdt;
	fwts_acpi_table_header *header;

	void *	rsdp_addr = NULL;
	size_t	rsdp_len;
	int 	num_entries;
	int 	i;

	/* Check for RSDP as EFI, then BIOS, parsed klog, if not found, give up */
	if (fw->rsdp)
		rsdp_addr = (void*)fw->rsdp;
	if (!rsdp_addr)
		rsdp_addr = fwts_acpi_find_rsdp_efi();
	if (!rsdp_addr)
		rsdp_addr = fwts_acpi_find_rsdp_bios();
	if (!rsdp_addr)
		rsdp_addr = fwts_acpi_find_rsdp_klog();
	if (!rsdp_addr)
		return FWTS_ERROR;

	/* Must be on a 16 byte boundary */
	if (((unsigned long)rsdp_addr & 0xf))
		return FWTS_ERROR;

	/* Load and save cached RSDP */
	if ((rsdp = fwts_acpi_get_rsdp(rsdp_addr, &rsdp_len)) == NULL)
		return FWTS_ERROR;
	fwts_acpi_add_table("RSDP", rsdp, (uint64_t)(off_t)rsdp_addr, rsdp_len, FWTS_ACPI_TABLE_FROM_FIRMWARE);

	/* Load any tables from XSDT if it's valid */
	if (rsdp->xsdt_address) {
		if ((xsdt = fwts_acpi_load_table((off_t)rsdp->xsdt_address)) != NULL) {
			fwts_acpi_add_table("XSDT", xsdt, (uint64_t)rsdp->xsdt_address,
				xsdt->header.length, FWTS_ACPI_TABLE_FROM_FIRMWARE);
			num_entries = (xsdt->header.length - sizeof(fwts_acpi_table_header)) / 8;
			for (i=0; i<num_entries; i++) {
				if (xsdt->entries[i]) {
					if ((header = fwts_acpi_load_table((off_t)xsdt->entries[i])) != NULL) {
						if (strncmp("FACP", header->signature, 4) == 0)
							fwts_acpi_handle_fadt((fwts_acpi_table_fadt*)header,
								FWTS_ACPI_TABLE_FROM_FIRMWARE);
						fwts_acpi_add_table(header->signature, header, xsdt->entries[i],
							header->length, FWTS_ACPI_TABLE_FROM_FIRMWARE);
					}
				}
			}
		}
	}

	/* Load any tables from RSDT if it's valid */
	if (rsdp->rsdt_address) {
		if ((rsdt = fwts_acpi_load_table((off_t)rsdp->rsdt_address)) != NULL) {
			fwts_acpi_add_table("RSDT", rsdt, (uint64_t)rsdp->rsdt_address,
				rsdt->header.length, FWTS_ACPI_TABLE_FROM_FIRMWARE);
			num_entries = (rsdt->header.length - sizeof(fwts_acpi_table_header)) / 4;
			for (i=0; i<num_entries; i++) {
				if (rsdt->entries[i]) {
					if ((header = fwts_acpi_load_table((off_t)rsdt->entries[i])) != NULL) {
						if (strncmp("FACP", header->signature, 4) == 0)
							fwts_acpi_handle_fadt((fwts_acpi_table_fadt*)header,
								FWTS_ACPI_TABLE_FROM_FIRMWARE);
						fwts_acpi_add_table(header->signature, header, (uint64_t)rsdt->entries[i],
							header->length, FWTS_ACPI_TABLE_FROM_FIRMWARE);
					}
				}
			}
		}
	}

	return FWTS_OK;
}

/*
 *  fwts_fake_physical_addr()
 *	Loading tables from file may result in data without an originating
 *	physical address of the table, so fake a unique 32 bit address for the table.
 */
static uint32_t fwts_fake_physical_addr(const size_t size)
{
	static uint32_t fake_phys_addr = 0xbff00000;
	uint32_t addr = fake_phys_addr;

	fake_phys_addr += (size + 16);

	return addr;
}

/*
 *  fwts_acpi_load_table_from_acpidump()
 *	Load an ACPI table from the output of acpidump or fwts --dump
 */
static uint8_t *fwts_acpi_load_table_from_acpidump(FILE *fp, char *name, uint64_t *addr, size_t *size)
{
	uint32_t offset;
	uint8_t  data[16];
	char buffer[128];
	uint8_t *table;
	uint8_t *tmp = NULL;
	char *ptr = buffer;
	size_t len = 0;
	unsigned long long table_addr;
	ptrdiff_t name_len;

	*size = 0;

	if (fgets(buffer, sizeof(buffer), fp) == NULL)
		return NULL;

	/*
	 * Parse tablename followed by address, e.g.
	 *   DSTD @ 0xbfa02344
	 *   SSDT4 @ 0xbfa0f230
	 */
	ptr = strstr(buffer, "@ 0x");
	if (ptr == NULL)
		return NULL; /* Can't find table name */

	name_len = ptr - buffer;
	/*
	 * We should have no more than the table name (4..5 chars)
	 * plus a space left between the start of the buffer and
	 * the @ sign.  If we have more then something is wrong with
	 * the data. So just ignore this garbage as we don't want to
	 * overflow the name on the following strcpy()
	 */
	if ((name_len > 6) || (name_len < 5))
		return NULL; /* Name way too long or too short */

	if (sscanf(ptr, "@ 0x%Lx\n", &table_addr) < 1)
		return NULL; /* Can't parse address */

	*(ptr-1) = '\0';
	strcpy(name, buffer);

	/* In fwts RSD PTR is known as the RSDP */
	if (strncmp(name, "RSD PTR", 7) == 0)
		strcpy(name, "RSDP");

	/* Pull in 16 bytes at a time */
	while (fgets(buffer, sizeof(buffer), fp) ) {
		uint8_t *new_tmp;
		int n;
		buffer[56] = '\0';	/* truncate */
		if ((n = sscanf(buffer,"  %x: %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
			&offset,
			&data[0], &data[1], &data[2], &data[3],
			&data[4], &data[5], &data[6], &data[7],
			&data[8], &data[9], &data[10], &data[11],
			&data[12], &data[13], &data[14], &data[15])) < 1)
			break;

		len += (n - 1);
		if ((new_tmp = realloc(tmp, len)) == NULL) {
			free(tmp);
			return NULL;
		} else
			tmp = new_tmp;

		memcpy(tmp + offset, data, n-1);
	}

	/* Allocate the table using low 32 bit memory */
	if ((table = fwts_low_malloc(len)) == NULL) {
		free(tmp);
		return NULL;
	}
	memcpy(table, tmp, len);
	free(tmp);

	if (table_addr == 0)
		table_addr = fwts_fake_physical_addr(len);

	/* We may need to fake a physical address if its null */
	*addr = (uint64_t)(table_addr == 0 ? fwts_fake_physical_addr(len) : table_addr);
	*size = len;

	return table;
}

/*
 *  fwts_acpi_load_tables_from_acpidump()
 *	Load in all ACPI tables from output of acpidump or fwts --dump
 */
static int fwts_acpi_load_tables_from_acpidump(fwts_framework *fw)
{
	FILE *fp;

	if ((fp = fopen(fw->acpi_table_acpidump_file, "r")) == NULL) {
		fwts_log_error(fw, "Cannot open '%s' to read ACPI tables.",
			fw->acpi_table_acpidump_file);
		return FWTS_ERROR;
	}

	while (!feof(fp)) {
		uint64_t addr;
		uint8_t *table;
		size_t length;
		char name[16];

		if ((table = fwts_acpi_load_table_from_acpidump(fp, name, &addr, &length)) != NULL)
			fwts_acpi_add_table(name, table, addr, length, FWTS_ACPI_TABLE_FROM_FILE);
	}

	fclose(fp);

	return FWTS_OK;
}

/*
 *  fwts_acpi_load_table_from_file()
 *	load table from a raw binary dump
 */
static uint8_t *fwts_acpi_load_table_from_file(const int fd, size_t *length)
{
	uint8_t *ptr = NULL;
	ssize_t n;
	size_t size = 0;
	char buffer[4096];

	*length = 0;

	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				fwts_low_free(ptr);
				return NULL;
			}
		}
		else {
			ptr = (uint8_t*)fwts_low_realloc(ptr, size + n + 1);
			if (ptr == NULL)
				return NULL;
			memcpy(ptr + size, buffer, n);
			size += n;
		}
	}
	*length = size;
	return ptr;
}

static int fwts_acpi_load_tables_from_file(fwts_framework *fw)
{
	struct dirent **dir_entries;
	int count = 0;
	int i;

	/*
 	 * Read in directory in alphabetical name sorted order
	 * to ensure the tables are always loaded into memory
	 * in some form of deterministic order
	 */
	if ((count = scandir(fw->acpi_table_path, &dir_entries, 0, alphasort)) < 0) {
		fwts_log_error(fw, "Cannot open directory '%s' to read ACPI tables.",
			fw->acpi_table_path);
		return FWTS_ERROR;
	}

	for (i = 0; i < count; i++) {
		if (strstr(dir_entries[i]->d_name, ".dat")) {
			char path[PATH_MAX];
			int fd;

			snprintf(path, sizeof(path), "%s/%s",
				fw->acpi_table_path, dir_entries[i]->d_name);
			if ((fd = open(path, O_RDONLY)) >= 0) {
				uint8_t *table;
				size_t length;

				if ((table = fwts_acpi_load_table_from_file(fd, &length)) != NULL) {
					char name[9];	/* "RSD PTR " or standard ACPI 4 letter name */

					fwts_acpi_table_rsdp *rsdp  = (fwts_acpi_table_rsdp *)table;

					/* Could be RSDP or a standard ACPI table, so check */

					if (!strncmp(rsdp->signature, "RSD PTR ", 8)) {
						/* In fwts, RSD PTR is tagged as the RSDP */
						strcpy(name, "RSDP");
					} else {
						/* Assume it is a standard ACPI table */
						fwts_acpi_table_header *hdr = (fwts_acpi_table_header *)table;

						strncpy(name, hdr->signature, 4);
						name[4] = '\0';
					}

					if (!strncmp(name, "XSDT", 4) || !strncmp(name, "RSDT", 4)) {
						/*
						 * For XSDT and RSDT we don't bother loading at this point.
						 * These tables point to the other tables, however, we can't
						 * figure out which table each pointer references because
						 * we are loading in raw table data and we don't know where
						 * these were located in the original machine.  So the best
						 * way forward is to ignore these tables and instead leave
						 * the fix up stage fwts_acpi_load_tables_fixup() to magically
						 * create faked XSDT and RSDT entries based on the tables
						 * we've loaded from file.
						 */
						fwts_low_free(table);
					} else {
						fwts_acpi_add_table(name, table,
							(uint64_t)fwts_fake_physical_addr(length), length,
							FWTS_ACPI_TABLE_FROM_FILE);
					}
				}
				close(fd);
			} else
				fwts_log_error(fw, "Cannot load ACPI table from file '%s'\n", path);
		}
		free(dir_entries[i]);
	}
	free(dir_entries);

	if (count == 0) {
		fwts_log_error(fw, "Could not find any APCI tables in directory '%s'.\n", fw->acpi_table_path);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

/*
 *  fwts_acpi_table_fixable()
 *	return true if a table can be put into RSDT or XSDT
 */
static bool fwts_acpi_table_fixable(fwts_acpi_table_info *table)
{
	fwts_acpi_table_header *header;

	if (table == NULL)
		return false;

	header = (fwts_acpi_table_header *)table->data;

	/* We don't want RSDT or XSDT in the RSDT and XSDT */
	if (strncmp(header->signature, "RSDT", 4) == 0)
		return false;
	if (strncmp(header->signature, "XSDT", 4) == 0)
		return false;

	return true;
}

/*
 *  fwts_acpi_load_tables_fixup()
 *	tables loaded from file sometimes do not contain the original
 *	physical address of the tables, so these need faking. Also, some
 *	poor ACPI dumping implementations fail to save the RSDP, RSDT and
 *	XSDT, so we need to create fake versions from scratch.
 */
static int fwts_acpi_load_tables_fixup(fwts_framework *fw)
{
	int i;
	int j;
	int count;
	char *oem_tbl_id = "FWTS    ";
	fwts_acpi_table_info *table;
	fwts_acpi_table_rsdp *rsdp = NULL;
	fwts_acpi_table_rsdt *rsdt = NULL;
	fwts_acpi_table_xsdt *xsdt = NULL;
	fwts_acpi_table_fadt *fadt = NULL;

	/* Fetch the OEM Table ID */
	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table) {
		fadt = (fwts_acpi_table_fadt *)table->data;
		oem_tbl_id = fadt->header.oem_tbl_id;
	}

	/* Get RSDP */
	if (fwts_acpi_find_table(fw, "RSDP", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table)
		rsdp = (fwts_acpi_table_rsdp *)table->data;

	/* Get RSDT */
	if (fwts_acpi_find_table(fw, "RSDT", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table)
		rsdt = (fwts_acpi_table_rsdt *)table->data;

	/* Get XSDT */
	if (fwts_acpi_find_table(fw, "XSDT", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table)
		xsdt = (fwts_acpi_table_xsdt *)table->data;

	/* Figure out how many tables we need to put into RSDT and XSDT */
	for (count=0,i=0;;i++) {
		if (fwts_acpi_get_table(fw, i, &table) != FWTS_OK)
			break;
		if (table == NULL)	/* No more tables */
			break;
		if (fwts_acpi_table_fixable(table))
			count++;
	}

	/* No RSDT? go and fake one */
	if (rsdt == NULL) {
		size_t size = sizeof(fwts_acpi_table_rsdt) + (count * sizeof(uint32_t));
		if ((rsdt = fwts_low_calloc(1, size)) == NULL)
			return FWTS_ERROR;

		for (i=0,j=0; j<count ;i++)
			if (fwts_acpi_get_table(fw, i, &table) == FWTS_OK)
				if (fwts_acpi_table_fixable(table))
					rsdt->entries[j++] = (uint32_t)table->addr;

		strncpy(rsdt->header.signature, "RSDT", 4);
		rsdt->header.length = size;
		rsdt->header.revision = 1;
		strncpy(rsdt->header.oem_id, "FWTS  ", 6);
		strncpy(rsdt->header.oem_tbl_id, oem_tbl_id, 8);
		rsdt->header.oem_revision = 1;
		strncpy(rsdt->header.creator_id, "FWTS", 4);
		rsdt->header.creator_revision = 1;
		rsdt->header.checksum = 256 - fwts_checksum((uint8_t*)rsdt, size);

		fwts_acpi_add_table("RSDT", rsdt, (uint64_t)fwts_fake_physical_addr(size),
			size, FWTS_ACPI_TABLE_FROM_FIXUP);
	}

	/* No XSDT? go and fake one */
	if (xsdt == NULL) {
		size_t size = sizeof(fwts_acpi_table_rsdt) + (count * sizeof(uint64_t));
		if ((xsdt = fwts_low_calloc(1, size)) == NULL)
			return FWTS_ERROR;

		for (i=0,j=0; j<count ;i++)
			if (fwts_acpi_get_table(fw, i, &table) == FWTS_OK)
				if (fwts_acpi_table_fixable(table))
					xsdt->entries[j++] = table->addr;

		strncpy(xsdt->header.signature, "XSDT", 4);
		xsdt->header.length = size;
		xsdt->header.revision = 2;
		strncpy(xsdt->header.oem_id, "FWTS  ", 6);
		strncpy(xsdt->header.oem_tbl_id, oem_tbl_id, 8);
		xsdt->header.oem_revision = 1;
		strncpy(xsdt->header.creator_id, "FWTS", 4);
		xsdt->header.creator_revision = 1;
		xsdt->header.checksum = 256 - fwts_checksum((uint8_t*)xsdt, size);

		fwts_acpi_add_table("XSDT", xsdt, (uint64_t)fwts_fake_physical_addr(size),
			size, FWTS_ACPI_TABLE_FROM_FIXUP);
	}

	/* No RSDP? go and fake one */
	if (rsdp == NULL) {
		size_t size = sizeof(fwts_acpi_table_rsdp);
		if ((rsdp = fwts_low_calloc(1, size)) == NULL)
			return FWTS_ERROR;

		strncpy(rsdp->signature, "RSD PTR ", 8);
		strncpy(rsdp->oem_id, "FWTS  ", 6);
		rsdp->revision = 2;
		rsdp->rsdt_address = (unsigned long)rsdt;
		rsdp->length = sizeof(fwts_acpi_table_rsdp);
		rsdp->xsdt_address = (uint64_t)(unsigned long)xsdt;
		rsdp->reserved[0] = 0;
		rsdp->reserved[1] = 0;
		rsdp->reserved[2] = 0;

		rsdp->checksum = 256 - fwts_checksum((uint8_t*)rsdp, 20);
		rsdp->extended_checksum = 256 - fwts_checksum((uint8_t*)rsdp, sizeof(fwts_acpi_table_rsdp));

		fwts_acpi_add_table("RSDP", rsdp, (uint64_t)fwts_fake_physical_addr(size),
			sizeof(fwts_acpi_table_rsdp), FWTS_ACPI_TABLE_FROM_FIXUP);
	}
	return FWTS_OK;
}

/*
 *  fwts_acpi_load_tables()
 *	Load from firmware or from files in a specified directory
 */
int fwts_acpi_load_tables(fwts_framework *fw)
{
	int ret = FWTS_ERROR;

	if (fw->acpi_table_path != NULL)
		ret = fwts_acpi_load_tables_from_file(fw);
	else if (fw->acpi_table_acpidump_file != NULL)
		ret = fwts_acpi_load_tables_from_acpidump(fw);
	else if (fwts_check_root_euid(fw, true) == FWTS_OK)
		ret = fwts_acpi_load_tables_from_firmware(fw);
	else
		ret = FWTS_ERROR_NO_PRIV;

	if (ret == FWTS_OK) {
		acpi_tables_loaded = 1;

		/* Loading from file may require table address fixups */
		if ((fw->acpi_table_path != NULL) || (fw->acpi_table_acpidump_file != NULL))
			fwts_acpi_load_tables_fixup(fw);
	}

	return ret;
}

/*
 *  fwts_acpi_find_table()
 *  	Search for an ACPI table. There may be more than one, so
 *  	specify the one using which.
 */
int fwts_acpi_find_table(fwts_framework *fw, const char *name, const int which, fwts_acpi_table_info **info)
{
	int i;
	int ret;

	if (info == NULL)
		return FWTS_NULL_POINTER;

	*info = NULL;

	if (!acpi_tables_loaded)
		if ((ret = fwts_acpi_load_tables(fw)) != FWTS_OK)
			return ret;

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (tables[i].data == NULL)
			break;
		if ((strcmp(tables[i].name, name) == 0) &&
	            (tables[i].which == which)) {
			*info = &tables[i];
			return FWTS_OK;
		}
	}
	return FWTS_OK;
}

/*
 *  fwts_acpi_find_table_by_addr()
 *  	Search for a cached ACPI table given it's original physical address
 */
int fwts_acpi_find_table_by_addr(fwts_framework *fw, const uint64_t addr, fwts_acpi_table_info **info)
{
	int i;
	int ret;

	if (info == NULL)
		return FWTS_NULL_POINTER;

	*info = NULL;

	if (!acpi_tables_loaded)
		if ((ret = fwts_acpi_load_tables(fw)) != FWTS_OK)
			return ret;

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (tables[i].data == NULL)
			break;
		if (tables[i].addr == addr) {
			*info = &tables[i];
			return FWTS_OK;
		}
	}
	return FWTS_OK;
}

/*
 *  fwts_acpi_get_table()
 *  	Get the Nth cached ACPI table.
 */
int fwts_acpi_get_table(fwts_framework *fw, const int index, fwts_acpi_table_info **info)
{
	int ret;

	if (info == NULL)
		return FWTS_NULL_POINTER;

	*info = NULL;

	if ((index < 0) || (index >= ACPI_MAX_TABLES))
		return FWTS_ERROR;

	if (!acpi_tables_loaded)
		if ((ret = fwts_acpi_load_tables(fw)) != FWTS_OK)
			return ret;

	if (tables[index].data == NULL)
		return FWTS_OK;

	*info = &tables[index];
	return FWTS_OK;
}
