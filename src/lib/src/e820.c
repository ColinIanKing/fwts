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

#include "fwts_types.h"
#include "fwts_list.h"
#include "fwts_log.h"
#include "fwts_klog.h"
#include "fwts_framework.h"
#include "fwts_e820.h"

unsigned long RSDP_ADDRESS;

typedef struct {
	uint64	start_address;
	uint64	end_address;
	int	type;
} e820_entry;

static int fwts_register_e820_line(fwts_list *e820_list, uint64 start, uint64 end, int type)
{
	e820_entry *entry;

	if ((entry = calloc(1, sizeof(e820_entry))) == NULL)
		return FWTS_ERROR;
	
	entry->start_address = start;
	entry->end_address   = end;
	entry->type          = type;

	if (fwts_list_append(e820_list, entry) == NULL)
		return FWTS_ERROR;

	return FWTS_OK;
}


int fwts_e820_type(fwts_list *e820_list, uint64 memory)
{
	int result = E820_UNKNOWN;
	e820_entry *entry;
	fwts_list_link *item;

	for (item = e820_list->head; item != NULL; item = item->next) {
		entry = (e820_entry*)item->data;

		if (entry->start_address <= memory && entry->end_address > memory) {
			result = entry->type;
			break;
		}
	}

	return result;
}

fwts_bool fwts_e820_is_reserved(fwts_list *e820_list, uint64 memory)
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

	if (strstr(line, "ACPI") && strstr(line,"RSDP")) {
		char *c;
		c = strstr(line, "@ 0x");
		if (c) 
			RSDP_ADDRESS = strtoul(c+4, NULL, 16);
	}

	if ((str = strstr(line,"BIOS-e820:")) != NULL) {
		uint64 start;
		uint64 end;
		int type = E820_UNKNOWN;

		start = strtoull(str+10, NULL, 16);
		str = strstr(line," - ");
		if (str) 
			str += 3;
		end   = strtoull(str, NULL, 16);
		if (strstr(line, "(usable)"))
			type = E820_USABLE;
		if (strstr(line, "(reserved)"))
			type = E820_RESERVED;
		if (strstr(line, "ACPI"))
			type = E820_ACPI;

		fwts_register_e820_line(e820_list, start, end, type);
	}
}


static void fwts_e820_dump_info(void *data, void *private)
{
	e820_entry *entry = (e820_entry *)data;
	fwts_framework *fw = (fwts_framework *)private;

	char *type;

	switch (entry->type) {
	case E820_RESERVED:
		type = "(reserved)";
		break;
	case E820_ACPI:
		type = "(ACPI)";
		break;
	case E820_USABLE:
		type = "RAM used by OS";
		break;
	default:
		type = "(UNKNOWN)";
		break;
	}

	fwts_log_info(fw, "%016llx - %016llx  %s", entry->start_address, entry->end_address, type);
}

fwts_list *fwts_e820_table_load(fwts_framework *fw)
{
	fwts_list *klog;
	fwts_list *e820_list;

	if ((klog = fwts_klog_read()) == NULL)
		return NULL;

	if ((e820_list = fwts_list_init()) == NULL)
		return NULL;
	
	fwts_list_foreach(klog, fwts_e820_dmesg_info, e820_list);
	fwts_klog_free(klog);

	fwts_log_info(fw, "E820 memory layout");
	fwts_log_info(fw, "------------------");

	fwts_list_foreach(e820_list, fwts_e820_dump_info, fw);

	return e820_list;
}

void fwts_e820_table_free(fwts_list *e820_list)
{
	fwts_list_free(e820_list, free);
}

