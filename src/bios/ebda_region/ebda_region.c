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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "framework.h"
#include "checkeuid.h"
#include "klog.h"

#define EBDA_OFFSET	0x40e
#define BAD_ADDR	0

static unsigned long ebda_addr = BAD_ADDR;

static fwts_text_list *klog;

static int ebda_init(fwts_framework *fw)
{
	int fd;
	unsigned short addr;

	if (fwts_check_root_euid(fw))
		return 1;

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Failed to read kernel log");
		return 1;
	}

	if ((fd = open("/dev/mem", O_RDONLY)) < 0) {
		fwts_log_error(fw, "Failed to open /dev/mem");
		return 1;
	}

	if (lseek(fd, EBDA_OFFSET, SEEK_SET) < 0) {
		fwts_log_error(fw, "Failed to seek to EBDA offset 0x%x", EBDA_OFFSET);
		return 1;
	}
	if (read(fd, &addr, sizeof(unsigned short)) <= 0) {
		fwts_log_error(fw, "Failed to read EBDA address");
		return 1;
	}
	close(fd);

	ebda_addr = ((unsigned long)addr) << 4;

	return 0;
}

static int ebda_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return 0;
}

static char *ebda_headline(void)
{
	return "Validate EBDA region is mapped and reserved in E820 table";
}

static int ebda_test1(fwts_framework *fw)
{
	int passed = 0;
	fwts_text_list_element *item;

	if (klog == NULL)
		return 1;

	item = klog->head;

	for (item = klog->head; item != NULL; item = item->next) {
		char *tmp;

		if ((tmp = strstr(item->text, "BIOS-e820")) != NULL) {
			unsigned long long start_addr = 0;
			unsigned long long end_addr = 0;
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
					fwts_framework_passed(fw, "EBDA region mapped at %lx and reserved in E820 table in region 0x%llx..0x%llx",
						ebda_addr, start_addr, end_addr);
					passed = 1;
					break;
				}
			}
		}
	}

	if (!passed)
		fwts_framework_failed(fw, "EBDA region mapped at 0x%lx but not reserved in E820 table, ebda_addr");
		
	return 0;
}

static fwts_framework_tests ebda_tests[] = {
	ebda_test1,
	NULL
};

static fwts_framework_ops ebda_ops = {
	ebda_headline,
	ebda_init,
	ebda_deinit,
	ebda_tests
};

FRAMEWORK(ebda, &ebda_ops, TEST_ANYTIME);
