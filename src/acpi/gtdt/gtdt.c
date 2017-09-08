/*
 * Copyright (C) 2015-2017 Canonical
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

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static fwts_acpi_table_info *table;

static int gtdt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "GTDT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		if (fw->flags & FWTS_FLAG_TEST_SBBR) {
			fwts_log_error(fw, "ACPI GTDT table does not exist");
			return FWTS_ERROR;
		} else {
			fwts_log_error(fw, "ACPI GTDT table does not exist, skipping test");
			return FWTS_SKIP;
		}
	}
	return FWTS_OK;
}

/*
 *  GTDT  Generic Timer Description Table
 */
static int gtdt_test1(fwts_framework *fw)
{
	bool passed = true;
	uint8_t *ptr, *end_ptr;
	uint32_t i = 0, n;
	const fwts_acpi_table_gtdt *gtdt = (const fwts_acpi_table_gtdt *)table->data;

	fwts_acpi_reserved_bits_check(fw, "GTDT", "Flags", gtdt->virtual_timer_flags, sizeof(gtdt->virtual_timer_flags), 3, 31, &passed);

	ptr = (uint8_t *)table->data + gtdt->platform_timer_offset;
	n = gtdt->platform_timer_count;
	end_ptr = (uint8_t *)table->data + table->length;

	while ((i < n) && (ptr < end_ptr)) {
		uint32_t len, j;
		fwts_acpi_table_gtdt_block *block;
		fwts_acpi_table_gtdt_block_timer *block_timer;
		fwts_acpi_table_gtdt_watchdog *watchdog;

		switch (*ptr) {
		case 0x00:
			/* GT Block Structure */
			block = (fwts_acpi_table_gtdt_block *)ptr;
			if (ptr + 20 > end_ptr) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTShortBlock",
					"GTDT block is too short");
				goto done;
			}
			if (block->length < 20) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTInvalidBlockLength",
					"GTDT block %" PRIu32 " length has in "
					"invalid length: %" PRIu32 " bytes",
					i, block->length);
				goto done;
			}
			if (block->reserved) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTInvalidBlockReserved",
					"GTDT block %" PRIu32 " reserved is "
					"non-zero, got 0x%" PRIx8,
					i, block->reserved);
			}
			if (block->block_timer_count > 8) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTInvalidBlockTimerCount",
					"GTDT block %" PRIu32 " timer count "
					"is too large, %" PRIu32 ", must be <= 8",
					i, block->block_timer_count);
				break;
			}
			len = (block->block_timer_count * 40) + 20;
			if (len != block->length) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTInvalidTimerCountOrLength",
					"GTDT block %" PRIu32 " timer count %"
					PRIu32 " and block length %" PRIu32 ", "
					"expected length of %" PRIu32,
					i, block->block_timer_count,
					block->length, len);
				/* length may be inconsistent, don't trust it so stop here */
				goto done;
			}
			block_timer = &block->block_timers[0];

			for (j = 0; j < block->block_timer_count; j++) {
				if (((uint8_t *)block_timer + sizeof(*block_timer)) > end_ptr)
					break;
				if (block_timer->frame_number > 7) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_HIGH,
						"GTDTInvalidGTFrameNumber",
						"GTDT block frame number is %" PRIu8
						", expecting 0..7",
						block_timer->frame_number);
				}
				if (block_timer->reserved[0] |
				    block_timer->reserved[1] |
				    block_timer->reserved[2]) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_HIGH,
						"GTDTBlockTimerReservedNotZero",
						"GTDT block %" PRIu32 " timer reserved "
						"space is not zero, got 0x"
						"%" PRIx8 "%" PRIx8 "%" PRIx8
						" instead", i,
						block_timer->reserved[0],
						block_timer->reserved[1],
						block_timer->reserved[2]);
				}
				if (block_timer->phys_timer_flags & ~0x3) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"GTDTFBlockPhysTimerFlagReservedNonZero",
						"GTDT block %" PRIu32 " physical timer "
						"flags reserved bits 2 to 31 are "
						"non-zero, instead got 0x%" PRIx32 ".",
						i, block_timer->phys_timer_flags);
				}
				if (block_timer->virt_timer_flags & ~0x3) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"GTDTFBlockVirtTimerFlagReservedNonZero",
						"GTDT block %" PRIu32 " virtual timer "
						"flags reserved bits 2 to 31 are "
						"non-zero, instead got 0x%" PRIx32 ".",
						i, block_timer->virt_timer_flags);
				}
				if (block_timer->common_flags & ~0x3) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"GTDTFBlockCommonFlagReservedNonZero",
						"GTDT block %" PRIu32 " common flags "
						"reserved bits 2 to 31 are "
						"non-zero, instead got 0x%" PRIx32 ".",
						i, block_timer->common_flags);
				}
			}
			ptr += block->length;
			break;
		case 0x01:
			/* SBSA Generic Watchdog Timer Structure */
			watchdog = (fwts_acpi_table_gtdt_watchdog *)ptr;
			if (ptr + 28 > end_ptr) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTShortWatchDogTimer",
					"GTDT SBSA generic watchdog timer %"
					PRIu32 " is too short", i);
				goto done;
			}
			if (watchdog->length != 28) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTInvalidWatchDogTimeLength",
					"GTDT SBSA generic watchdog timer %" PRIu32
					" length has in invalid length: %"
					PRIu32 " bytes", i, watchdog->length);
				goto done;
			}
			if (watchdog->reserved) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"GTDTInvalidWatchDogReserved",
					"GTDT SBSA generic watchdog timer %" PRIu32
					" reserved is non-zero, got 0x%" PRIx8,
					i, watchdog->reserved);
			}
			if (watchdog->watchdog_timer_flags & ~0x7) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_LOW,
					"GTDTWatchDogTimerFlagReservedNonZero",
					"GTDT SBSA generic watchdog timer %" PRIu32
					" flags reserved bits 3 to 31 are non-zero, "
					"instead got 0x%" PRIx8 ".",
					i, watchdog->watchdog_timer_flags);
			}
			ptr += watchdog->length;
			break;
		default:
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"GTDTInvalidType",
				"GTDT platform timer strucuture %" PRIu32
				" has an invalid type: 0x%" PRIx8, i, *ptr);
			/* Can't determine field length, so end of parsing */
			goto done;
		}
		i++;
	}

done:
	if (passed)
		fwts_passed(fw, "No issues found in GTDT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test gtdt_tests[] = {
	{ gtdt_test1, "GTDT Generic Timer Description Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops gtdt_ops = {
	.description = "GTDT Generic Timer Description Table test.",
	.init        = gtdt_init,
	.minor_tests = gtdt_tests
};

FWTS_REGISTER("gtdt", &gtdt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI | FWTS_FLAG_TEST_SBBR)

#endif
