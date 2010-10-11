/*
 * Copyright (C) 2010 Canonical
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "fwts.h"

#define BIOS_START	(0x000e0000)
#define BIOS_END  	(0x000fffff)
#define BIOS_LENGTH	(BIOS_END - BIOS_START)
#define PAGE_SIZE	(4096)

#define ACPI_MAX_TABLES	(128)

static fwts_acpi_table_info	tables[ACPI_MAX_TABLES];
static int acpi_tables_loaded = 0;

/*
 *  Get RSDP address from EFI if possible
 */
static uint32_t fwts_acpi_find_rsdp_efi(void)
{
	FILE *systab;
	char text[1024];
	unsigned long rsdp = 0;

	if ((systab = fopen("/sys/firmware/efi/systab", "r")) == NULL)
		return 0;

	while (fgets(text, sizeof(text), systab) != NULL) {
		if (sscanf(text, "ACPI20=0x%lx", &rsdp) == 1)
			break;
	}
	fclose(systab);

	return rsdp;
}

/*
 *  RSDP checksum
 */
static uint8_t fwts_acpi_rsdp_checksum(uint8_t *data, const int length)
{
	int i;
	uint8_t checksum = 0;

	for (i=0; i<length; i++) {
		checksum += *data++;
	}
	return checksum;
}

static void *fwts_acpi_mmap(off_t start, size_t size)
{
	int fd;
	void *mem;
	off_t offset = ((size_t)start) & (PAGE_SIZE-1);
	size_t length = (size_t)size + offset;

	if ((fd = open("/dev/mem", O_RDONLY)) < 0)
		return MAP_FAILED;

	if ((mem = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, start - offset)) == MAP_FAILED) {
		close(fd);
		return MAP_FAILED;
	}
	close(fd);

	return (mem + offset);
}

static void fwts_acpi_munmap(void *mem, unsigned long size)
{
	unsigned long offset = ((unsigned long)(mem)) & (PAGE_SIZE-1);

	munmap(mem - offset, size + offset);
}

/*
 *  Find RSDP address by scanning BIOS memory
 */
static uint32_t fwts_acpi_find_rsdp_bios(void)
{
	uint8_t *bios;
	uint8_t *ptr;
	fwts_acpi_table_rsdp *rsdp;
	unsigned long addr = 0;

	if ((bios = fwts_acpi_mmap(BIOS_START, BIOS_LENGTH)) == MAP_FAILED)
		return 0;

	/* Scan BIOS for RSDP, ACPI spec states it is aligned on 16 byte intervals */
	for (ptr = bios; ptr < (bios+BIOS_LENGTH); ptr += 16) {
		rsdp = (fwts_acpi_table_rsdp*)ptr;
		if (strncmp(rsdp->signature, "RSD PTR ",8) == 0) {
			int length = (rsdp->revision < 2) ? 20 : 36;
			if (fwts_acpi_rsdp_checksum(ptr, length) == 0) {
				addr = BIOS_START+(ptr - bios);
				break;
			}
		}
	}
	fwts_acpi_munmap(bios, BIOS_LENGTH);

	return addr;
}

static fwts_acpi_table_rsdp *fwts_acpi_get_rsdp(uint32_t addr)
{
	uint8_t *mem;
	fwts_acpi_table_rsdp *rsdp;

	if ((rsdp = (fwts_acpi_table_rsdp*)malloc(sizeof(fwts_acpi_table_rsdp))) == NULL) 
		return NULL;

	if ((mem = fwts_acpi_mmap(addr, sizeof(fwts_acpi_table_rsdp))) == MAP_FAILED)
		return 0;

	memcpy(rsdp, mem, sizeof(fwts_acpi_table_rsdp));
	fwts_acpi_munmap(mem, sizeof(fwts_acpi_table_rsdp));

	return rsdp;
}

static void *fwts_acpi_load_table(off_t addr)
{
	fwts_acpi_table_header *hdr;
	void *mem;
	void *table;
	int len;
	
	if ((hdr = fwts_acpi_mmap((off_t)addr, sizeof(fwts_acpi_table_header))) == MAP_FAILED)
		return NULL;

	len = hdr->length;
	fwts_acpi_munmap(hdr, sizeof(fwts_acpi_table_header));

	if ((table = malloc(len)) == NULL)
		return NULL;

	if ((mem = fwts_acpi_mmap((off_t)addr, len)) == MAP_FAILED)
		return NULL;

	memcpy(table, mem, len);
	fwts_acpi_munmap(mem, len);
	
	return table;
}

static void fwts_acpi_add_table(char *name, void *table, uint64_t addr, int length)
{
	int i;
	int which = 0;

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (addr && tables[i].addr == addr) {
			/* We don't need it, it's a duplicate, so free and return */
			free(table);
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
			return;
		}
	}
}

void fwts_acpi_free_tables(void)
{
	int i;

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (tables[i].data) {
			free(tables[i].data);
			memset(&tables[i], 0, sizeof(fwts_acpi_table_info));
		}
	}
}

static void fwts_acpi_handle_fadt_tables(fwts_acpi_table_fadt *fadt, uint32_t *addr32, uint64_t *addr64)
{
	off_t addr;
	fwts_acpi_table_header *header;

	if ((addr64 != 0) && (fadt->header.length >= 140)) 
		addr = (off_t)*addr64;
	else if ((addr32 !=0) && (fadt->header.length >= 44)) 
		addr = (off_t)*addr32;
	else addr = 0;

	if (addr) {
		header = fwts_acpi_load_table(addr);
		fwts_acpi_add_table(header->signature, header, (uint64_t)addr, header->length);
	}
}

static void fwts_acpi_handle_fadt(fwts_acpi_table_fadt *fadt)
{
	fwts_acpi_handle_fadt_tables(fadt, &fadt->dsdt, &fadt->x_dsdt);
	fwts_acpi_handle_fadt_tables(fadt, &fadt->firmware_control, &fadt->x_firmware_ctrl);
}

/*
 *  Load up cached copies of all the ACPI tables
 */
static void fwts_acpi_load_tables_from_firmware(void)
{
	fwts_acpi_table_rsdp *rsdp;
	fwts_acpi_table_xsdt *xsdt;
	fwts_acpi_table_rsdt *rsdt;
	fwts_acpi_table_header *header;

	uint64_t	rsdp_addr;
	int num_entries;
	int i;

	if ((rsdp_addr = fwts_acpi_find_rsdp_efi()) == 0)
		if ((rsdp_addr = fwts_acpi_find_rsdp_bios()) == 0) 
			return;

	rsdp = fwts_acpi_get_rsdp(rsdp_addr);
	fwts_acpi_add_table("RSDP", rsdp, (uint64_t)rsdp_addr, sizeof(fwts_acpi_table_rsdp));

	if (rsdp->rsdt_address) {
		rsdt = fwts_acpi_load_table((off_t)rsdp->rsdt_address);
		fwts_acpi_add_table("RSDT", rsdt, (uint64_t)rsdp->rsdt_address, rsdt->header.length);
		num_entries = (rsdt->header.length - sizeof(fwts_acpi_table_header)) / 4;
		for (i=0; i<num_entries; i++) {
			if (rsdt->entries[i]) {
				header = fwts_acpi_load_table((off_t)rsdt->entries[i]);
				if (strncmp("FACP", header->signature, 4) == 0)
					fwts_acpi_handle_fadt((fwts_acpi_table_fadt*)header);
				fwts_acpi_add_table(header->signature, header, (uint64_t)rsdt->entries[i], header->length);
			}
		}
	}

	if (rsdp->xsdt_address) {
		xsdt = fwts_acpi_load_table((off_t)rsdp->xsdt_address);
		fwts_acpi_add_table("XSDT", xsdt, (uint64_t)rsdp->xsdt_address, xsdt->header.length);
		num_entries = (xsdt->header.length - sizeof(fwts_acpi_table_header)) / 8;
		for (i=0; i<num_entries; i++) {
			if (xsdt->entries[i]) {
				header = fwts_acpi_load_table((off_t)xsdt->entries[i]);
				if (strncmp("FACP", header->signature, 4) == 0)
					fwts_acpi_handle_fadt((fwts_acpi_table_fadt*)header);
				fwts_acpi_add_table(header->signature, header, xsdt->entries[i], header->length);
			}
		}
	}
}

static uint8_t *fwts_acpi_load_table_from_file(int fd, int *length)
{
	uint8_t *ptr = NULL;
	int n;
	int size = 0;
	char buffer[4096];	

	*length = 0;

	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				free(ptr);
				return NULL;
			}
		}
		else {
			ptr = (uint8_t*)realloc(ptr, size + n + 1);
			memcpy(ptr + size, buffer, n);
			size += n;
		}
	}
	*length = size;
	return ptr;
}

static void fwts_acpi_load_tables_from_file(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *direntry;
	int count = 0;

	if ((dir = opendir(fw->acpi_table_path)) == NULL) {
		fwts_log_error(fw, "Cannot open directory '%s' to read ACPI tables.", 
			fw->acpi_table_path);
		return;
	}

	while ((direntry = readdir(dir)) != NULL) {
		if (strstr(direntry->d_name, ".dat")) {
			char path[PATH_MAX];
			int fd;
printf("LOAD %s\n", direntry->d_name);
			snprintf(path, sizeof(path), "%s/%s",
				fw->acpi_table_path, direntry->d_name);
			if ((fd = open(path, O_RDONLY)) >= 0) {
				uint8_t *table;
				int length;
				char name[PATH_MAX];

				count++;
				strcpy(name, direntry->d_name);
				name[strlen(name)-4] = '\0';
				if ((table = fwts_acpi_load_table_from_file(fd, &length)) != NULL) 
					fwts_acpi_add_table(name, table, (uint64_t)0, length);
				close(fd);
			} else
				fwts_log_error(fw, "Cannot load ACPI table from file '%s'\n", path);
		}
	}
	if (count == 0)
		fwts_log_error(fw, "Could not find any APCI tables in directory '%s'.\n", fw->acpi_table_path);

	closedir(dir);
}

void fwts_acpi_load_tables(fwts_framework *fw)
{
	/* Load from firmware or from files in a specified directory */
	if (fw->acpi_table_path == NULL)
		fwts_acpi_load_tables_from_firmware();
	else
		fwts_acpi_load_tables_from_file(fw);

	acpi_tables_loaded = 1;
}

/*
 *  Search for an ACPI table. There may be more than one, so
 *  specify the one using which.
 */
fwts_acpi_table_info *fwts_acpi_find_table(fwts_framework *fw, const char *name, const int which)
{
	int i;

	if (!acpi_tables_loaded)
		fwts_acpi_load_tables(fw);

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (tables[i].data == NULL)
			break;
		if ((strcmp(tables[i].name, name) == 0) && 
	            (tables[i].which == which))
			return &tables[i];
	}
	return NULL;
}

/*
 *  Search for an ACPI table by address.
 */
fwts_acpi_table_info *fwts_acpi_find_table_by_addr(fwts_framework *fw, const uint64_t addr)
{
	int i;
	
	if (!acpi_tables_loaded)
		fwts_acpi_load_tables(fw);

	for (i=0;i<ACPI_MAX_TABLES;i++) {
		if (tables[i].data == NULL)
			break;
		if (tables[i].addr == addr)
			return &tables[i];
	}
	return NULL;
}

/*
 *  Get an ACPI table.
 */
fwts_acpi_table_info *fwts_acpi_get_table(fwts_framework *fw, const int index)
{
	if ((index < 0) || (index >= ACPI_MAX_TABLES))
		return NULL;

	if (!acpi_tables_loaded)
		fwts_acpi_load_tables(fw);

	if (tables[index].data == NULL)
		return NULL;

	return &tables[index];
}
