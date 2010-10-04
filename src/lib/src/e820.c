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
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "fwts.h"

typedef struct {
	uint64_t	start_address;
	uint64_t	end_address;
	int	type;
} e820_entry;

static int fwts_e820_entry_compare(void *data1, void *data2)
{
        e820_entry *entry1 = (e820_entry *)data1;
        e820_entry *entry2 = (e820_entry *)data2;

	if (entry1->start_address < entry2->start_address)
		return -1;
	else if (entry1->start_address < entry2->start_address)
		return 1;
	else
		return 0;
}

static int fwts_e820_str_to_type(const char *str)
{
	/* Strings from /sys/firmware/memmap/x/type */

	if (strstr(str, "System RAM"))
		return E820_USABLE;
	if (strstr(str, "reserved"))
		return E820_RESERVED;
	if (strstr(str, "ACPI Non-volatile Storage"))
		return E820_ACPI;

	/* Strings from kernel log */

	if (strstr(str, "(usable)"))
		return E820_USABLE;
	if (strstr(str, "(reserved)"))
		return E820_RESERVED;
	if (strstr(str, "ACPI"))
		return E820_ACPI;

	return E820_UNKNOWN;
}

static char *fwts_e820_type_to_str(int type)
{
	switch (type) {
	case E820_RESERVED:
		return "(reserved)";
	case E820_ACPI:
		return "(ACPI Non-volatile Storage)";
	case E820_USABLE:
		return "(System RAM)";
	default:
		return "(UNKNOWN)";
	}
}

static int fwts_register_e820_line(fwts_list *e820_list, const uint64_t start, const uint64_t end, const int type)
{
	e820_entry *entry;

	if ((entry = calloc(1, sizeof(e820_entry))) == NULL)
		return FWTS_ERROR;
	
	entry->start_address = start;
	entry->end_address   = end;
	entry->type          = type;

	if (fwts_list_add_ordered(e820_list, entry, fwts_e820_entry_compare) == NULL)
		return FWTS_ERROR;

	return FWTS_OK;
}

int fwts_e820_type(fwts_list *e820_list, const uint64_t memory)
{
	e820_entry *entry;
	fwts_list_link *item;

	for (item = e820_list->head; item != NULL; item = item->next) {
		entry = (e820_entry*)item->data;
		if (entry->start_address <= memory && entry->end_address > memory)
			return entry->type;
	}

	return E820_UNKNOWN;
}

fwts_bool fwts_e820_is_reserved(fwts_list *e820_list, const uint64_t memory)
{
	int result = E820_UNKNOWN;

	/* when we don't have E820 info, assume all is fair */
	if (e820_list == NULL)
		return FWTS_TRUE; 
	
	/* bios data area is always reserved */
	if ((memory >= 640 * 1024) && (memory <= 1024*1024))
		return FWTS_TRUE;

	result = fwts_e820_type(e820_list, memory);

	if (result == E820_RESERVED)
		return FWTS_TRUE;
	if (result == E820_ACPI)
		return FWTS_TRUE;

	return FWTS_FALSE;
}

/* checks dmesg for useful e820 info */
static void fwts_e820_dmesg_info(void *data, void *private)
{
	char *str;
	char *line = (char *)data;
	fwts_list *e820_list = (fwts_list *)private;

	if ((str = strstr(line,"BIOS-e820:")) != NULL) {
		uint64_t start;
		uint64_t end;

		start = strtoull(str+10, NULL, 16);
		str = strstr(line," - ");
		if (str) 
			str += 3;
		end   = strtoull(str, NULL, 16) - 1;

		fwts_register_e820_line(e820_list, start, end, fwts_e820_str_to_type(line));
	}
}

static void fwts_e820_dump_info(void *data, void *private)
{
	e820_entry *entry = (e820_entry *)data;
	fwts_framework *fw = (fwts_framework *)private;

	fwts_log_info(fw, "%016llx - %016llx  %s", entry->start_address, entry->end_address, fwts_e820_type_to_str(entry->type));
}

void fwts_e820_table_dump(fwts_framework *fw, fwts_list *e820_list)
{
	fwts_log_info(fw, "E820 memory layout");
	fwts_log_info(fw, "------------------");

	fwts_list_foreach(e820_list, fwts_e820_dump_info, fw);
}

fwts_list *fwts_e820_table_load_from_klog(fwts_framework *fw)
{
	fwts_list *klog;
	fwts_list *e820_list;

	if ((klog = fwts_klog_read()) == NULL)
		return NULL;

	if ((e820_list = fwts_list_init()) == NULL)
		return NULL;
	
	fwts_list_foreach(klog, fwts_e820_dmesg_info, e820_list);
	fwts_klog_free(klog);


	return e820_list;
}

static e820_entry *fwts_e820_table_read_entry(const char *which)
{
	char path[PATH_MAX];
	char *data;
	e820_entry *entry;

	if ((entry = calloc(1, sizeof(e820_entry))) == NULL)
		return NULL;

	snprintf(path, sizeof(path), "/sys/firmware/memmap/%s/start", which);
	if ((data = fwts_get(path)) == NULL) {
		free(entry);
		return NULL;
	}
	sscanf(data, "0x%llx", &entry->start_address);
	free(data);

	snprintf(path, sizeof(path), "/sys/firmware/memmap/%s/end", which);
	if ((data = fwts_get(path)) == NULL) {
		free(entry);
		return NULL;
	}
	sscanf(data, "0x%llx", &entry->end_address);
	free(data);
	
	snprintf(path, sizeof(path), "/sys/firmware/memmap/%s/type", which);
	if ((data = fwts_get(path)) == NULL) {
		free(entry);
		return NULL;
	}
	entry->type = fwts_e820_str_to_type(data);
	free(data);

	return entry;
}

fwts_list *fwts_e820_table_load(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *directory;
	fwts_list *e820_list;

	/* Try to load from /sys/firmware/memmap, but if we fail, try
	   scanning the kernel log as a fallback */
	if ((dir = opendir("/sys/firmware/memmap/")) == NULL)
		return fwts_e820_table_load_from_klog(fw);

	if ((e820_list = fwts_list_init()) == NULL) {
		closedir(dir);
		return NULL;
	}

	while ((directory = readdir(dir)) != NULL) {
		if (strncmp(directory->d_name, ".", 1)) {
			e820_entry *entry = fwts_e820_table_read_entry(directory->d_name);
			fwts_list_add_ordered(e820_list, entry, fwts_e820_entry_compare);
		}
	}
	closedir(dir);

	return e820_list;
}

void fwts_e820_table_free(fwts_list *e820_list)
{
	fwts_list_free(e820_list, free);
}
