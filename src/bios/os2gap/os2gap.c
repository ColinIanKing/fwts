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

#define OS2_GAP_ADDRESS 	(15*1024*1024)
#define OS2_GAP_SIZE		(1024*1024)

static fwts_list *os2gap_memory_map_info;

static int os2gap_init(fwts_framework *fw)
{
	if ((os2gap_memory_map_info = fwts_memory_map_table_load(fw)) == NULL) {
		fwts_log_warning(fw, "No memory map table found");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static int os2gap_deinit(fwts_framework *fw)
{
	if (os2gap_memory_map_info)
		fwts_memory_map_table_free(os2gap_memory_map_info);

	return FWTS_OK;
}

static int os2gap_test1(fwts_framework *fw)
{
	if (fwts_memory_map_is_reserved(os2gap_memory_map_info, OS2_GAP_ADDRESS)) {
		fwts_failed(fw, LOG_LEVEL_HIGH, 
			"OS2Gap",
			"The memory map has OS/2 memory hole of %dK at %p..%p.",
			OS2_GAP_SIZE / 1024,
			(void*)OS2_GAP_ADDRESS,
			(void*)(OS2_GAP_ADDRESS + OS2_GAP_SIZE));
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
		fwts_log_nl(fw);
		fwts_memory_map_table_dump(fw, os2gap_memory_map_info);
	} else
		fwts_passed(fw, "No OS/2 memory hole found.");

	return FWTS_OK;
}

static fwts_framework_minor_test os2gap_tests[] = {
	{ os2gap_test1, "Check the OS/2 15Mb memory hole is absent." },
	{ NULL, NULL }
};

static fwts_framework_ops os2gap_ops = {
	.description = "OS/2 memory hole test.",
	.init        = os2gap_init,
	.deinit      = os2gap_deinit,
	.minor_tests = os2gap_tests
};

FWTS_REGISTER(os2gap, &os2gap_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
