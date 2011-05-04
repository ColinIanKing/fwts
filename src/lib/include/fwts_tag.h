/*
 * Copyright (C) 2010-2011 Canonical
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

#ifndef __FWTS_TAG__
#define __FWTS_TAG__

#include "fwts_list.h"
#include "fwts_framework.h"

typedef enum {
	FWTS_TAG_NONE = 1,
	FWTS_TAG_ERROR_CRITICAL,
	FWTS_TAG_ERROR_HIGH,
	FWTS_TAG_ERROR_MEDIUM,
	FWTS_TAG_ERROR_LOW,
	FWTS_TAG_BIOS,
	FWTS_TAG_BIOS_THERMAL,
	FWTS_TAG_BIOS_IRQ,
	FWTS_TAG_BIOS_AMD_POWERNOW,
	FWTS_TAG_BIOS_MMCONFIG,
	FWTS_TAG_BIOS_DMI,
	FWTS_TAG_ACPI,
	FWTS_TAG_ACPI_IO_PORT,
	FWTS_TAG_ACPI_INVALID_TABLE,
	FWTS_TAG_ACPI_BUFFER_OVERFLOW,
	FWTS_TAG_ACPI_AML_OPCODE,
	FWTS_TAG_ACPI_NAMESPACE_LOOKUP,
	FWTS_TAG_ACPI_PCI_EXPRESS,
	FWTS_TAG_ACPI_BAD_RESULT,
	FWTS_TAG_ACPI_NO_HANDLER,
	FWTS_TAG_ACPI_PACKAGE_LIST,
	FWTS_TAG_ACPI_PARSE_EXEC_FAIL,
	FWTS_TAG_ACPI_EVAL,
	FWTS_TAG_ACPI_BAD_LENGTH,
	FWTS_TAG_ACPI_BAD_ADDRESS,
	FWTS_TAG_ACPI_METHOD_RETURN,
	FWTS_TAG_ACPI_BRIGHTNESS,
	FWTS_TAG_ACPI_BUTTON,
	FWTS_TAG_ACPI_EVENT,
	FWTS_TAG_ACPI_PARAMETER,
	FWTS_TAG_ACPI_THROTTLING,
	FWTS_TAG_ACPI_EXCEPTION,
	FWTS_TAG_ACPI_PACKAGE,
	FWTS_TAG_ACPI_APIC,
	FWTS_TAG_ACPI_DISPLAY,
	FWTS_TAG_ACPI_MULTIPLE_FACS,
	FWTS_TAG_ACPI_POINTER_MISMATCH,
	FWTS_TAG_ACPI_TABLE_CHECKSUM,
	FWTS_TAG_ACPI_HOTPLUG,
	FWTS_TAG_ACPI_RSDP,
	FWTS_TAG_ACPI_MUTEX,
	FWTS_TAG_ACPI_THERMAL,
	FWTS_TAG_ACPI_LID,
	FWTS_TAG_ACPI_METHOD,
	FWTS_TAG_EMBEDDED_CONTROLLER,
	FWTS_TAG_POWER_MANAGEMENT,
} fwts_tag;

fwts_tag fwts_tag_id_str_to_tag(const char *tag);
const char *fwts_tag_to_str(const fwts_tag tag);
void fwts_tag_add(fwts_list *taglist, const char *tag);
char *fwts_tag_list_to_str(fwts_list *taglist);
void fwts_tag_report(fwts_framework *fw, const fwts_log_field field, fwts_list *taglist);
void fwts_tag_failed(fwts_framework *fw, const fwts_tag tag);

#endif
