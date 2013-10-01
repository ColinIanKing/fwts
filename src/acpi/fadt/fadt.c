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

#define FADT_RESET_SUPPORTED		(1 << 10)

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

	/*  Not having a FADT is not a failure */
	if (fadt_size == 0) {
		fwts_log_info(fw, "FADT does not exist, this is not necessarily a failure, skipping tests.");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static int fadt_test1(fwts_framework *fw)
{
	uint32_t port, width, val32;
	int ret = FWTS_OK;

	fwts_log_info(fw, "FADT Preferred PM Profile: %hhu (%s)\n",
		fadt->preferred_pm_profile,
		FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(fadt->preferred_pm_profile));

	port = fadt->pm1a_cnt_blk;
	width = fadt->pm1_cnt_len * 8;	/* In bits */

	if ((fadt->header.revision > 1) || (fadt->header.length >= 244)) {
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
		if (ioperm(port, width/8, 1) < 0)
			ret = FWTS_ERROR;
		else {
			uint8_t val8;

			ret = fwts_inb(port, &val8);
			val32 = val8;
			ioperm(port, width/8, 0);
		}
		break;
	case 16:
		if (ioperm(port, width/8, 1) < 0)
			ret = FWTS_ERROR;
		else {
			uint16_t val16;

			ret = fwts_inw(port, &val16);
			val32 = val16;
			ioperm(port, width/8, 0);
		}
		break;
	case 32:
		if (ioperm(port, width/8, 1) < 0)
			ret = FWTS_ERROR;
		else {
			ret = fwts_inl(port, &val32);
			ioperm(port, width/8, 0);
		}
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_HIGH, "FADTPM1AInvalidWidth",
			"FADT pm1a register has invalid bit width of %d.",
			width);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_BAD_LENGTH);
		return FWTS_OK;
	}

	if (ret != FWTS_OK) {
		fwts_log_error(fw, "Cannot read FADT PM1A_CNT_BLK port 0x%" PRIx32 ".", port);
		return FWTS_ERROR;
	}

	if (val32 & 0x01)
		fwts_passed(fw, "SCI_EN bit in PM1a Control Register Block is enabled.");
	else {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SCI_ENNotEnabled",
			"SCI_EN bit in PM1a Control Register Block is not enabled.");
		fwts_tag_failed(fw, FWTS_TAG_POWER_MANAGEMENT);
	}

	return FWTS_OK;
}

static int fadt_test2(fwts_framework *fw)
{
	if ((fadt->header.revision == 1) || (fadt->header.length < 244)) {
		fwts_skipped(fw, "Header size indicates an ACPI 1.0 FADT, skipping test.");
		return FWTS_SKIP;
	}
	/*
 	 * Sanity check Reset reg, c.f. ACPICA
	 * commit 57bdfbee675cd2c0618c66882d68a6bdf9f8efc4
	 */
	if (fadt->flags & FADT_RESET_SUPPORTED) {
		if (fadt->reset_reg.register_bit_width != 8) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "FadtResetRegWidth",
				"FADT reset register is not 8 bits wide, got instead: %" PRIi8 ".",
				fadt->reset_reg.register_bit_width);
			fwts_advice(fw,
				"Section 5.2.9 Fixed ACPI Description Table (Table 5-34) of the ACPI "
				"specification states that the FADT register width should "
				"be 8 bits wide. However, recent versions of ACPICA will ignore this "
				"and default to 8 bits.");
		} else
			fwts_passed(fw, "FADT reset register width is 8 bits wide as expected.");

		if (fadt->reset_reg.register_bit_offset != 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "FadtResetRegBitOffset",
				"FADT reset register bit offset is not 0, got instead: %" PRIi8 ".",
				fadt->reset_reg.register_bit_offset);
			fwts_advice(fw,
				"Section 5.2.9 Fixed ACPI Description Table (Table 5-34) of the ACPI "
				"specification states that the FADT register bit offset should be 0.");
		} else
			fwts_passed(fw, "FADT register bit offset is 0 as expected.");
	} else {
		fwts_skipped(fw, "FADT flags indicates reset register not supported, skipping test.");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static fwts_framework_minor_test fadt_tests[] = {
	{ fadt_test1, "Check FADT SCI_EN bit is enabled." },
	{ fadt_test2, "Check FADT reset register." },
	{ NULL, NULL }
};

static fwts_framework_ops fadt_ops = {
	.description = "FADT SCI_EN enabled check.",
	.init        = fadt_init,
	.minor_tests = fadt_tests
};

FWTS_REGISTER("fadt", &fadt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);

#endif
