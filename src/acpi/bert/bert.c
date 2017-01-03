/*
 * Copyright (C) 2016-2017 Canonical
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

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static fwts_acpi_table_info *table;

static int bert_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "BERT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI BERT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  For BERT refer to 18.3.1 Boot Error Record Table
 */
static int bert_test1(fwts_framework *fw)
{
	bool passed = true;
	const fwts_acpi_table_bert *bert = (const fwts_acpi_table_bert *)table->data;

	fwts_log_info_verbatim(fw, "Boot Error Record Table:");
	fwts_log_info_verbatim(fw, "  Error Region Length       0x%8.8" PRIx32, bert->boot_error_region_length);
	fwts_log_info_verbatim(fw, "  Error Region              0x%16.16" PRIx64, bert->boot_error_region);
	fwts_log_nl(fw);

	/* Sanity check length */
	if (bert->boot_error_region_length < sizeof(fwts_acpi_table_boot_error_region)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"BERTBootErrorRegionDataLength",
			"BERT Boot Error Region Length %" PRIu32
			" is smaller than the minimum allowed size of %zu"
			" bytes",
			bert->boot_error_region_length,
			sizeof(fwts_acpi_table_boot_error_region));
		passed = false;
		goto done;
	}

	/*
	 * If the tables directly from the machine we're running on
	 * then we can check out the error region too.
	 */
	if (table->provenance != FWTS_ACPI_TABLE_FROM_FIRMWARE) {
		fwts_log_info(fw, "ACPI table loaded from file so fwts will not memory map "
			"BERT boot error region 0x%16.16" PRIx64
			", skipping boot error region sanity checks.",
			bert->boot_error_region);
	} else {
		void *mapping;

		mapping = fwts_mmap(bert->boot_error_region, (size_t)bert->boot_error_region_length);
		if (mapping == FWTS_MAP_FAILED) {
			fwts_log_info(fw, "Cannot memory map BERT boot error region 0x%16.16" PRIx64
				", skipping boot error region sanity checks.",
				bert->boot_error_region);
		} else {
			fwts_acpi_table_boot_error_region *region =
				(fwts_acpi_table_boot_error_region *)mapping;

			fwts_log_info_verbatim(fw, "Boot Error Region:");
			fwts_log_info_verbatim(fw, "  Block Status:  bit [0]    0x%" PRIx32 " (Uncorrectable Error Valid)",
				(region->block_status >> 0) & 1);
			fwts_log_info_verbatim(fw, "  Block Status:  bit [1]    0x%" PRIx32 " (Correctable Error Valid)",
				(region->block_status >> 1) & 1);
			fwts_log_info_verbatim(fw, "  Block Status:  bit [2]    0x%" PRIx32 " (Multiple Uncorrectable Errors)",
				(region->block_status >> 2) & 1);
			fwts_log_info_verbatim(fw, "  Block Status:  bit [3]    0x%" PRIx32 " (Multiple Correctable Errors)",
				(region->block_status >> 3) & 1);
			fwts_log_info_verbatim(fw, "  Block Status:  bit [13:4] 0x%" PRIx32 " (Error Data Entry Count)",
				(region->block_status >> 4) & 0x3ff);
			fwts_log_info_verbatim(fw, "  Raw Data Offset:          0x%8.8" PRIx32,
				region->raw_data_offset);
			fwts_log_info_verbatim(fw, "  Raw Data Length:          0x%8.8" PRIx32,
				region->raw_data_length);
			fwts_log_info_verbatim(fw, "  Data Length:              0x%8.8" PRIx32,
				region->data_length);
			fwts_log_info_verbatim(fw, "  Error Severity            0x%8.8" PRIx32,
				region->error_severity);

			/* Sanity check raw data fields */
			if (region->raw_data_offset >
				bert->boot_error_region_length) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"BERTBootErrorRegionRawDataOffset",
					"BERT Boot Error Region Raw Data Offset %" PRIx32
						" is larger than the region size of %" PRIu32
						" bytes",
						region->raw_data_offset,
						bert->boot_error_region_length);
				passed = false;
			}
			if (region->raw_data_offset <
				sizeof(fwts_acpi_table_boot_error_region) + region->data_length) {
				if (region->raw_data_length) {
					fwts_failed(fw, LOG_LEVEL_HIGH,
						"BERTBootErrorRegionRawDataOffset",
						"BERT Boot Error Region Raw Data Offset %" PRIu32
							" is smaller than end of the data region and"
							" BERT Boot Error Region Raw Data Length %" PRIu32
							" is non-zero.",
							region->raw_data_offset,
							region->raw_data_length);
					passed = false;
				} else {
					fwts_warning(fw, "BERT Boot Error Region Raw Data Offset %"
							PRIu32 " is smaller than end of the data"
							"region. BERT Boot Error Region Data Lenght "
							"is zero.",
							region->raw_data_offset);
					fwts_advice(fw,
						"If there is raw data in the BERT Boot Error Region, "
						"Raw Data Offset must be larger than the end of the "
						"data region if there is raw data. However, since "
						"BERT Boot Error Region Raw Data Lenght is zero, "
						"this may mean that there is no raw data.");
				}
			}
			if (region->raw_data_length + region->raw_data_offset > bert->boot_error_region_length) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"BERTBootErrorRegionRawDatalength",
					"BERT Boot Error Region Raw Data Length %" PRIu32
						" is larger than the region size less the raw data offset of %" PRIu32
						" bytes",
						region->raw_data_length,
						bert->boot_error_region_length - region->raw_data_offset);
				passed = false;
			}
			/* Sanity check data length */
			if (region->data_length + sizeof(fwts_acpi_table_boot_error_region) > bert->boot_error_region_length) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"BERTBootErrorRegionDatalength",
					"BERT Boot Error Region Data Length %" PRIu32
						" is larger than the region size less the boot error region header of %" PRIu32
						" bytes",
						region->data_length,
						bert->boot_error_region_length - (uint32_t)sizeof(fwts_acpi_table_boot_error_region));
				passed = false;
			}
			if (region->error_severity > 3) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"BERTBootErrorRegionDataLength",
					"BERT Boot Error Region Data Length %" PRIu32
						" is larger than the remaining region size of %" PRIu32
						" bytes",
						region->raw_data_length,
						bert->boot_error_region_length);
				passed = false;
			}
			fwts_munmap(mapping, (size_t)bert->boot_error_region_length);
		}
	}

done:
	if (passed)
		fwts_passed(fw, "No issues found in BERT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test bert_tests[] = {
	{ bert_test1, "BERT Boot Error Record Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops bert_ops = {
	.description = "BERT Boot Error Record Table test.",
	.init        = bert_init,
	.minor_tests = bert_tests
};

FWTS_REGISTER("bert", &bert_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
