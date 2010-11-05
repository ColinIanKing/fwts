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

#include <stdlib.h>
#include <string.h>

#include "fwts.h"

static char *fwts_tag_strings[] = {
	"",
	"error_critical",
	"error_high",
	"error_medium",
	"error_low",
	"bios",
	"bios_thermal",
	"bios_irq",
	"bios_amd_powernow",
	"bios_mmconfig",
	"acpi",
	"acpi_io_port",
	"acpi_invalid_table",
	"acpi_buffer_overflow",
	"acpi_aml_opcode",
	"acpi_namespace_lookup",
	"acpi_pci_express"
	"acpi_bad_result",
	"acpi_no_handler",
	"acpi_package_list",
	"acpi_parse_exec_fail",
	"acpi_eval",
	"acpi_bad_length",
	"acpi_bad_address",
	"acpi_method_return",
	"acpi_brightness",
	"acpi_button",
	"acpi_event",
	"acpi_parameter",
	"acpi_throttling",
	"acpi_exception",
	"acpi_package",
	"acpi_apic",
	"acpi_display",
	"acpi_multiple_facs",
	"acpi_pointer_mismatch",
	"acpi_table_checksum",
	"acpi_hotplug",
	"acpi_rsdp",
	"acpi_mutex",
	"acpi_pci_express",
	"acpi_thermal",
	"embedded_controller",
	"power_management",
	NULL,
};

char *fwts_tag_to_str(fwts_tag tag)
{
	return fwts_tag_strings[tag];
}

static int fwts_tag_compare(void *data1, void *data2)
{
	return strcmp((char *)data1, (char*)data2);
}

void fwts_tag_add(fwts_list *taglist, const char *tag)
{
	fwts_list_link	*item;
	char *str;

	fwts_list_foreach(item, taglist)
		if (strcmp((char*)item->data, tag) == 0)
			return;

	str = strdup(tag);
	if (str)
		fwts_list_add_ordered(taglist, str, fwts_tag_compare);
}

char *fwts_tag_list_to_str(fwts_list *taglist)
{
	fwts_list_link	*item;
	char *str = NULL;
	int len = 0;

	fwts_list_foreach(item, taglist) {
		char *tag = (char *)item->data;
		int taglen = strlen(tag);
		len += taglen + 1;

		if (str) {
			str = realloc(str, len);
			strcat(str, " ");
			strcat(str, tag);
		} else {
			str = malloc(len);
			strcpy(str, tag);
		}
	}
	return str;
}
