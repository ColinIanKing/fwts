/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
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

#define EBDA_OFFSET	0x40e
#define BAD_ADDR	0

static unsigned long ebda_addr = BAD_ADDR;

static fwts_list *klog;

static int ebda_init(fwts_framework *fw)
{
	int fd;
	unsigned short addr;

	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Failed to read kernel log.");
		return FWTS_ERROR;
	}

	if ((fd = open("/dev/mem", O_RDONLY)) < 0) {
		fwts_log_error(fw, "Failed to open /dev/mem.");
		return FWTS_ERROR;
	}

	if (lseek(fd, EBDA_OFFSET, SEEK_SET) < 0) {
		fwts_log_error(fw, "Failed to seek to EBDA offset 0x%x.", EBDA_OFFSET);
		return FWTS_ERROR;
	}
	if (read(fd, &addr, sizeof(unsigned short)) <= 0) {
		fwts_log_error(fw, "Failed to read EBDA address.");
		return FWTS_ERROR;
	}
	close(fd);

	ebda_addr = ((unsigned long)addr) << 4;

	return FWTS_OK;
}

static int ebda_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return FWTS_OK;
}

static char *ebda_headline(void)
{
	return "Validate EBDA region is mapped and reserved in E820 table.";
}

static int ebda_test1(fwts_framework *fw)
{
	int passed = 0;
	fwts_list_link *item;

	if (klog == NULL)
		return FWTS_ERROR;

	fwts_log_info(fw, "The Extended BIOS Data Area (EBDA) is normally located at the end of the "
			  "low 640K region and is typically 2-4K in size. It should be reserved in "
			  "the E820 table.");

	fwts_list_foreach(item, klog) {
		char *tmp;

		if ((tmp = strstr(fwts_text_list_text(item), "BIOS-e820")) != NULL) {
			uint64_t start_addr = 0;
			uint64_t end_addr = 0;
			tmp = strstr(tmp,"BIOS-e820:");
			if (tmp) {
				tmp += 11;
				start_addr = strtoull(tmp + 11, NULL, 16);
				tmp = strstr(tmp + 11, " - ");
				if (tmp) {
					tmp += 3;
					end_addr   = strtoull(tmp, NULL, 16);
				}
				else {
					end_addr = 0;
				}
			}
			if (strstr(tmp, "(reserved)") || strstr(tmp, "ACPI")) {
				if (start_addr <= ebda_addr && end_addr > ebda_addr) {
					fwts_passed(fw, "EBDA region mapped at 0x%lx and reserved as a %lldK region in E820 table at 0x%llx..0x%llx.",
						ebda_addr, 
						(end_addr - start_addr) / 1024,
						start_addr, end_addr);
					passed = 1;
					break;
				}
			}
		}
	}

	if (!passed)
		fwts_failed(fw, "EBDA region mapped at 0x%lx but not reserved in E820 table.", ebda_addr);
		
	return FWTS_OK;
}

static fwts_framework_minor_test ebda_tests[] = {
	{ ebda_test1, "Check EBDA is reserved in E820 table." },
	{ NULL, NULL }
};

static fwts_framework_ops ebda_ops = {
	.headline    = ebda_headline,
	.init        = ebda_init,
	.deinit      = ebda_deinit,
	.minor_tests = ebda_tests
};

FWTS_REGISTER(ebda, &ebda_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
