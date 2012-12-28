/*
 * Copyright (C) 2010-2013 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static const fwts_acpi_table_fadt *fadt;
static int fadt_size;

static int fadt_init(fwts_framework *fw)
{
	fwts_acpi_table_info *table;

	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI table FACP.");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI table FACP does not exist!");
		return FWTS_ERROR;
	}
	fadt = (const fwts_acpi_table_fadt*)table->data;
	fadt_size = table->length;

	return FWTS_OK;
}

static int fadt_test1(fwts_framework *fw)
{
	uint32_t port, width, value;

	/*  Not having a FADT is not a failure */
	if (fadt_size == 0) {
		fwts_log_info(fw, "FADT does not exist, this is not necessarily a failure.");
		return FWTS_OK;
	}

	fwts_log_info(fw, "FADT Preferred PM Profile: %hhu (%s)\n",
		fadt->preferred_pm_profile,
		FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(fadt->preferred_pm_profile));

	port = fadt->pm1a_cnt_blk;
	width = fadt->pm1_cnt_len * 8;	/* In bits */

	/* Punt at 244 byte FADT is V2 */
	if (fadt->header.length == 244) {
		/*  Sanity check sizes with extended address variants */
		fwts_log_info(fw, "FADT is greater than ACPI version 1.0");
		if ((uint64_t)port != fadt->x_pm1a_cnt_blk.address) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTPM1CNTAddrMismatch",
				"32 and 64 bit versions of FADT pm1_cnt address do not match (0x%8.8x vs 0x%16.16" PRIx64 ").",
				port, fadt->x_pm1a_cnt_blk.address);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_BAD_ADDRESS);
		}
		if (width != fadt->x_pm1a_cnt_blk.register_bit_width) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTPM1CNTSizeMismatch",
				"32 and 64 bit versions of FADT pm1_cnt size do not match (0x%x vs 0x%x).",
				width, fadt->x_pm1a_cnt_blk.register_bit_width);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_BAD_ADDRESS);
		}

		port = fadt->x_pm1a_cnt_blk.address;
		width = fadt->x_pm1a_cnt_blk.register_bit_width;
	}

	switch (width) {
	case 8:
		ioperm(port, width/8, 1);
		value = inb(fadt->pm1a_cnt_blk);
		ioperm(port, width/8, 0);
		break;
	case 16:
		ioperm(port, width/8, 1);
		value = inw(fadt->pm1a_cnt_blk);
		ioperm(port, width/8, 0);
		break;
	case 32:
		ioperm(port, width/8, 1);
		value = inl(fadt->pm1a_cnt_blk);
		ioperm(port, width/8, 0);
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_HIGH, "FADTPM1AInvalidWidth",
			"FADT pm1a register has invalid bit width of %d.",
			width);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_BAD_LENGTH);
		return FWTS_OK;
	}

	if (value & 0x01)
		fwts_passed(fw, "SCI_EN bit in PM1a Control Register Block is enabled.");
	else {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SCI_ENNotEnabled",
			"SCI_EN bit in PM1a Control Register Block is not enabled.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	return FWTS_OK;
}

static fwts_framework_minor_test fadt_tests[] = {
	{ fadt_test1, "Check FADT SCI_EN bit is enabled." },
	{ NULL, NULL }
};

static fwts_framework_ops fadt_ops = {
	.description = "FADT SCI_EN enabled check.",
	.init        = fadt_init,
	.minor_tests = fadt_tests
};

FWTS_REGISTER(fadt, &fadt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);

#endif
