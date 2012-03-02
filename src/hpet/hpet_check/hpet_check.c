/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2012 Canonical
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
#include <string.h>

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

static fwts_list *klog;

#define HPET_REG_SIZE  (0x400)
#define MAX_CLK_PERIOD (100000000)

static uint64_t	hpet_base_p = 0;
static void     *hpet_base_v = 0;

#if 0
/* check_hpet_base_hpet() -- used to parse the HPET Table for HPET base info */
static void check_hpet_base_hpet(void)
{
        unsigned long address = 0;
        unsigned long size = 0;
	struct hpet_table *table;
	char *table_ptr;

	if (locate_acpi_table("HPET", &address, &size))
		return;

        if (address == 0 || address == -1)
                return;

        table = (struct hpet_table *) address;

	hpet_base_p = table->base_address.address;
	free((void *) address);
}
#endif

static void hpet_parse_check_base(fwts_framework *fw,
	const char *table, fwts_list_link *item)
{
	char *val, *idx;

	if ((val = strstr(fwts_text_list_text(item), "0x")) != NULL) {
		uint64_t address_base;
		idx = index(val, ',');
		if (idx)
			*idx = '\0';

		address_base = strtoul(val, NULL, 0x10);

		if (hpet_base_p != 0) {
			if (hpet_base_p != address_base)
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"HPETBaseMismatch",
					"Mismatched HPET base between %s (%lx) "
					"and the kernel (%lx).",
					table,
					(unsigned long)hpet_base_p,
					(unsigned long)address_base);
			else
				fwts_passed(fw,
					"HPET base matches that between %s and "
					"the kernel (%lx).",
					table,
					(unsigned long)hpet_base_p);
		}
	}
}

static void hpet_parse_device_hpet(fwts_framework *fw,
	const char *table, fwts_list_link *item)
{
	for (;item != NULL; item = item->next) {
		char *str = fwts_text_list_text(item);
		if ((strstr(str, "Name") != NULL) &&
			(strstr(str, "ResourceTemplate") != NULL)) {
			fwts_list_link *tmp_item = item->next;
			for (; tmp_item != NULL; tmp_item = tmp_item->next) {
				if (strstr(fwts_text_list_text(tmp_item),
					"Memory32Fixed") != NULL) {
					tmp_item = tmp_item->next;
					if (tmp_item != NULL) {
						hpet_parse_check_base(fw, table, tmp_item);
						return;
					}
				}
				if (strstr(fwts_text_list_text(tmp_item),
					"DWordMemory") != NULL) {
					tmp_item = tmp_item->next;
					if (tmp_item != NULL) {
						tmp_item = tmp_item->next;
						if (tmp_item != NULL) {
							/* HPET section is found, get base */
							hpet_parse_check_base(fw, table, tmp_item);
							return;
						}
					}
				}
			}
		}
	}
}

/*
 *  check_hpet_base_dsdt()
 *	used to parse the DSDT for HPET base info
 */
static void hpet_check_base_acpi_table(fwts_framework *fw,
	const char *table, const int which)
{
	fwts_list *output;
	fwts_list_link *item;

	if (fwts_iasl_disassemble(fw, table, which, &output) != FWTS_OK)
		return;
	if (output == NULL)
		return;

	fwts_list_foreach(item, output)
		if (strstr(fwts_text_list_text(item), "Device (HPET)") != NULL)
			hpet_parse_device_hpet(fw, table, item);

	fwts_text_list_free(output);
}


static int hpet_check_init(fwts_framework *fw)
{
	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Cannot read kernel log.");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static int hpet_check_deinit(fwts_framework *fw)
{
	if (klog)
		fwts_text_list_free(klog);

	return FWTS_OK;
}

static int hpet_check_test1(fwts_framework *fw)
{
	if (klog == NULL)
		return FWTS_ERROR;

	fwts_list_link *item;

	fwts_log_info(fw,
		"This test checks the HPET PCI BAR for each timer block "
		"in the timer. The base address is passed by the firmware "
		"via an ACPI table. IRQ routing and initialization is also "
		"verified by the test.");

	fwts_list_foreach(item, klog) {
		char *text = fwts_text_list_text(item);
		/* Old format */
		if (strstr(text, "ACPI: HPET id:") != NULL) {
			char *str = strstr(text, "base: ");
			if (str) {
				hpet_base_p = strtoul(str+6,  NULL, 0x10);
				fwts_passed(fw,
					"Found HPET base %x in kernel log.",
					(uint32_t)hpet_base_p);
				break;
			}
		}
		/* New format */
		/* [    0.277934] hpet0: at MMIO 0xfed00000, IRQs 2, 8, 0 */
		if ((strstr(text, "hpet") != NULL) &&
		    (strstr(text, "IRQs") != NULL)) {
			char *str = strstr(text, "at MMIO ");
			if (str) {
				hpet_base_p = strtoul(str+8,  NULL, 0x10);
				fwts_passed(fw,
					"Found HPET base %x in kernel log.",
					(uint32_t)hpet_base_p);
				break;
			}
		}
	}

	if (hpet_base_p == 0) {
		fwts_log_info(fw, "No base address found for HPET.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int hpet_check_test2(fwts_framework *fw)
{
	uint64_t hpet_id;
	uint32_t vendor_id;
	uint32_t clk_period;

	if (hpet_base_p == 0) {
		fwts_log_info(fw, "Test skipped because previous test failed.");
		return FWTS_SKIP;
	}

	hpet_base_v = fwts_mmap(hpet_base_p, HPET_REG_SIZE);
	if (hpet_base_v == NULL) {
		fwts_log_error(fw, "Cannot mmap to /dev/mem.");
		return FWTS_ERROR;
	}

	hpet_id = *(uint64_t*) hpet_base_v;
	vendor_id = (hpet_id & 0xffff0000) >> 16;

	if (vendor_id == 0xffff)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "HPETVendorId",
			"Invalid Vendor ID: %04x - this should be configured.",
			vendor_id);
	else
		fwts_passed(fw, "Vendor ID looks sane: %04x.", vendor_id);

	clk_period = hpet_id >> 32;
	if ((clk_period > MAX_CLK_PERIOD) || (clk_period == 0))
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "HPETClockPeriod",
			"Invalid clock period %u, must be non-zero and "
			"less than 10^8.", clk_period);
	else
		fwts_passed(fw, "Valid clock period %u.", clk_period);

	(void)fwts_munmap(hpet_base_v, HPET_REG_SIZE);

	return FWTS_OK;
}

static int hpet_check_test3(fwts_framework *fw)
{
	int i;

	hpet_check_base_acpi_table(fw, "DSDT", 0);
	for (i=0;i<11;i++) {
		hpet_check_base_acpi_table(fw, "SSDT", i);
	}
	return FWTS_OK;
}

static fwts_framework_minor_test hpet_check_tests[] = {
	{ hpet_check_test1, "Check HPET base in kernel log." },
	{ hpet_check_test2, "Sanity check HPET configuration." },
	{ hpet_check_test3, "Check HPET base in DSDT and/or SSDT. "},
	{ NULL, NULL }
};

static fwts_framework_ops hpet_check_ops = {
	.description = "HPET configuration test.",
	.init        = hpet_check_init,
	.deinit      = hpet_check_deinit,
	.minor_tests = hpet_check_tests
};

FWTS_REGISTER(hpet_check, &hpet_check_ops, FWTS_TEST_ANYTIME,
	FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
