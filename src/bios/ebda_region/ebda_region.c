/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2011 Canonical
 *
 * This code was originally part of the Linux-ready Firmware Developer Kit
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static off_t ebda_addr = FWTS_NO_EBDA;

static fwts_list *memory_map;

static int ebda_init(fwts_framework *fw)
{
	if (fw->firmware_type != FWTS_FIRMWARE_BIOS) {
		fwts_log_info(fw, "Machine is not using traditional BIOS firmware, skipping test.");
		return FWTS_SKIP;
	}

	if ((memory_map = fwts_memory_map_table_load(fw)) == NULL) {
		fwts_log_error(fw, "Failed to read memory map.");
		return FWTS_ERROR;
	}

	if ((ebda_addr = fwts_ebda_get()) == FWTS_NO_EBDA) {
		fwts_log_error(fw, "Failed to locate EBDA region.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int ebda_deinit(fwts_framework *fw)
{
	fwts_klog_free(memory_map);

	return FWTS_OK;
}

static int ebda_test1(fwts_framework *fw)
{
	const char *memory_map_name = fwts_memory_map_name(fw->firmware_type);
	fwts_memory_map_entry *entry;

	if (memory_map == NULL)
		return FWTS_ERROR;

	fwts_log_info(fw, "The Extended BIOS Data Area (EBDA) is normally located at the end of the "
			  "low 640K region and is typically 2-4K in size. It should be reserved in "
			  "the %s table.", memory_map_name);

	entry = fwts_memory_map_info(memory_map, (uint64_t)ebda_addr);
	if ((entry != NULL) &&
	    (entry->type == FWTS_MEMORY_MAP_RESERVED ||
	     entry->type == FWTS_MEMORY_MAP_ACPI)) {
		fwts_passed(fw, "EBDA region mapped at 0x%lx and reserved as a %lldK region in the %s table at 0x%llx..0x%llx.",
			ebda_addr,
			(unsigned long long int)(entry->end_address - entry->start_address) / 1024,
			memory_map_name,
			(unsigned long long int)entry->start_address,
			(unsigned long long int)entry->end_address);
	} else {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "EBDA region mapped at 0x%lx but not reserved in the %s table.", ebda_addr, memory_map_name);
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
	}
		
	return FWTS_OK;
}

static fwts_framework_minor_test ebda_tests[] = {
	{ ebda_test1, "Check EBDA is reserved in E820 table." },
	{ NULL, NULL }
};

static fwts_framework_ops ebda_ops = {
	.description = "Validate EBDA region is mapped and reserved in memory map table.",
	.init        = ebda_init,
	.deinit      = ebda_deinit,
	.minor_tests = ebda_tests
};

FWTS_REGISTER(ebda, &ebda_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
