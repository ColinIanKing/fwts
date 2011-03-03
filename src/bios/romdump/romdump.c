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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#define BIOS_ROM_REGION_START	(0x000c0000)
#define BIOS_ROM_REGION_END  	(0x000fffff)
#define BIOS_ROM_REGION_SIZE	(BIOS_ROM_REGION_END - BIOS_ROM_REGION_START)

#define BIOS_ROM_START		(0x000f0000)
#define BIOS_ROM_END		(0x000fffff)
#define BIOS_ROM_SIZE		(BIOS_ROM_END - BIOS_ROM_START)
#define BIOS_ROM_OFFSET		(BIOS_ROM_START - BIOS_ROM_REGION_START)

static int romdump_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	return FWTS_OK;
}

static char *romdump_headline(void)
{
	return "Dump ROM data.";
}

static void romdump_data(fwts_framework *fw, uint8_t *data, int offset, int length)
{
	char buffer[128];
	int i;

	for (i=0; i<length; i+=16) {
		fwts_dump_raw_data(buffer, sizeof(buffer), data+i, offset+i, 16);
		fwts_log_info_verbatum(fw, "%s", buffer);
	}
}

static int romdump_test1(fwts_framework *fw)
{
	uint8_t *mem;
	int i;

        if ((mem = fwts_mmap(BIOS_ROM_REGION_START, BIOS_ROM_REGION_SIZE)) == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap BIOS ROM region.");
		return FWTS_ERROR;
	}

	for (i=0; i<BIOS_ROM_REGION_SIZE; i+= 512) {
		if ((*(mem+i) == 0x55) && (*(mem+i+1) == 0xaa)) {
			int length = *(mem+i+2) << 9;
		
			fwts_log_info(fw, "Found ROM: %x..%x (%d bytes)", BIOS_ROM_REGION_START+i, BIOS_ROM_REGION_START+i+length, length);
			romdump_data(fw, mem+i, BIOS_ROM_REGION_START+i, length);
			fwts_log_nl(fw);
		}
	}

	fwts_log_info(fw, "BIOS ROM: %x..%x (%d bytes)", BIOS_ROM_START, BIOS_ROM_END, BIOS_ROM_SIZE);
	
	romdump_data(fw, mem+BIOS_ROM_OFFSET, BIOS_ROM_START, BIOS_ROM_SIZE);

	fwts_infoonly(fw);

        (void)fwts_munmap(mem, BIOS_ROM_REGION_SIZE);

	return FWTS_OK;
}

static fwts_framework_minor_test romdump_tests[] = {
	{ romdump_test1, "Dump ROM data." },
	{ NULL, NULL }
};

static fwts_framework_ops romdump_ops = {
	.headline    = romdump_headline,
	.init        = romdump_init,
	.minor_tests = romdump_tests
};

FWTS_REGISTER(romdump, &romdump_ops, FWTS_TEST_ANYTIME, FWTS_UTILS);

#endif
