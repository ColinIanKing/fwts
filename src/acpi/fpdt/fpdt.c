/*
 * Copyright (C) 2015-2024 Canonical
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
#include <ctype.h>

static fwts_acpi_table_info *table;
acpi_table_init(FPDT, &table)

static void fpdt_rec_header_dump(
	fwts_framework *fw,
	const char *type_name,
	fwts_acpi_table_fpdt_header *header)
{
	fwts_log_info_verbatim(fw, "  %s:", type_name);
	fwts_log_info_simp_int(fw, "    Perf Rec Type:	", header->type);
	fwts_log_info_simp_int(fw, "    Rec Length:	", header->length);
	fwts_log_info_simp_int(fw, "    Revision:	", header->revision);
}

static void fpdt_dump_raw_data(
	fwts_framework *fw,
	const uint8_t *data,
	const size_t length,
	const size_t offset)
{
        size_t n;

	fwts_log_info_verbatim(fw, "    Data:");
	for (n = 0; n < length; n += 16) {
		int left = length - n;
		char buffer[128];
		fwts_dump_raw_data(buffer, sizeof(buffer), data + n, n + offset, left > 16 ? 16 : left);
		fwts_log_info_verbatim(fw, "%s", buffer);
        }
}

static int fpdt_test1(fwts_framework *fw)
{
	uint8_t *data = (uint8_t *)table->data, *ptr = data;
	fwts_acpi_table_header *header= (fwts_acpi_table_header *)table->data;
	bool passed = true;
	const size_t fpdt_hdr_len = sizeof(fwts_acpi_table_fpdt_header);

	/* Size sanity, got enough table to get initial header */
	if (!fwts_acpi_table_length(fw, "FPDT", table->length, sizeof(fwts_acpi_table_header))) {
		passed = false;
		goto done;
	}

	fwts_acpi_revision_check("FPDT", header->revision, 1, &passed);

	ptr += sizeof(fwts_acpi_table_header);
	while (ptr < data + table->length) {
		fwts_acpi_table_fpdt_header *fpdt = (fwts_acpi_table_fpdt_header *)ptr;
		fwts_acpi_table_fpdt_basic_boot_perf_ptr *fbbpr = (fwts_acpi_table_fpdt_basic_boot_perf_ptr *)ptr;
		fwts_acpi_table_fpdt_s3_perf_ptr *s3ptpr = (fwts_acpi_table_fpdt_s3_perf_ptr *)ptr;

		/* checking if the fpdt header fully available */
		if (ptr + sizeof(fwts_acpi_table_fpdt_header) > (data + table->length)) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"FPDTPerformanceRecordOutsideTable",
				"FPDT performance record header is not fully"
				"available, outside the FPDT table.");
			goto done;
		}

		switch (fpdt->type) {
		case 0x0000:	/* Firmware Basic Boot Performance Pointer Record */
			if (fbbpr->fpdt.length != sizeof(fwts_acpi_table_fpdt_basic_boot_perf_ptr)) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"FPDTFWBootPerfPrtRecBadLength",
					"FPDT Firmware Basic Boot Performance Pointer Record is %" PRIu32
					" bytes and should be %zu bytes in size",
					fbbpr->fpdt.length, sizeof(fwts_acpi_table_fpdt_basic_boot_perf_ptr));
			} else {
				fpdt_rec_header_dump(fw, "  Firmware Basic Boot Performance Pointer Record", fpdt);
				fwts_log_info_simp_int(fw, "    Reserved:	", fbbpr->reserved);
				fwts_log_info_simp_int(fw, "    FBPT Pointer:	", fbbpr->fbpt_addr);

				fwts_acpi_reserved_zero("FPDT", "Reserved", fbbpr->reserved, &passed);

				/*
				 * For the moment, only dump the 64-bit processor-relative physical address
				 * of the Firmware Basic Boot Performance Table, should also get and check
				 * the table from the address
				 */
				fwts_log_info(fw, "Note: currently fwts does not check FBPT validity and the associated data");
			}

			fwts_acpi_fixed_value(fw, LOG_LEVEL_MEDIUM, "FPDT", "FBPT Revision", fbbpr->fpdt.revision, 1, &passed);

			/* Spec doesn't mention Reserved field should be 0 or not, skip checking the reserved field */
			break;
		case 0x0001:	/* S3 Performance Table Pointer Record */
			if (s3ptpr->fpdt.length != sizeof(fwts_acpi_table_fpdt_s3_perf_ptr)) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"FPDTS3PerfPrtRecBadLength",
					"FPDT S3 Performance Table Pointer Record is %" PRIu32
					" bytes and should be %zu bytes in size",
					s3ptpr->fpdt.length, sizeof(fwts_acpi_table_fpdt_s3_perf_ptr));
			} else {
				fpdt_rec_header_dump(fw, "S3 Performance Table Pointer Record", fpdt);
				fwts_log_info_simp_int(fw, "    Reserved:	", s3ptpr->reserved);
				fwts_log_info_simp_int(fw, "    S3PT Pointer:	", s3ptpr->s3pt_addr);

				fwts_acpi_reserved_zero("FPDT", "Reserved", s3ptpr->reserved, &passed);

				/*
				 * For the moment, only dump 64-bit processor-relative physical
				 * address of the S3 Performance Table, should also get and check
				 * the table from the address
				 */
				fwts_log_info(fw, "Note: currently fwts does not check S3PT validity and the associated data");
			}

			fwts_acpi_fixed_value(fw, LOG_LEVEL_MEDIUM, "FPDT", "S3PT Revision", s3ptpr->fpdt.revision, 1, &passed);

			/* Spec doesn't mention Reserved field should be 0 or not, skip checking the reserved field */
			break;
		case 0x0002 ... 0x0fff:
			fpdt_rec_header_dump(fw, "Reserved for ACPI specification use", fpdt);
			fpdt_dump_raw_data(fw, ptr + fpdt_hdr_len, fpdt->length - fpdt_hdr_len, ptr + fpdt_hdr_len - data);
			break;
		case 0x1000 ... 0x1fff:
			fpdt_rec_header_dump(fw, "Reserved for Platform Vendor use", fpdt);
			fpdt_dump_raw_data(fw, ptr + fpdt_hdr_len, fpdt->length - fpdt_hdr_len, ptr + fpdt_hdr_len - data);
			break;
		case 0x2000 ... 0x2fff:
			fpdt_rec_header_dump(fw, "Reserved for Hardware Vendor use", fpdt);
			fpdt_dump_raw_data(fw, ptr + fpdt_hdr_len, fpdt->length - fpdt_hdr_len, ptr + fpdt_hdr_len - data);
			break;
		case 0x3000 ... 0x3fff:
			fpdt_rec_header_dump(fw, "Reserved for BIOS Vendor use", fpdt);
			fpdt_dump_raw_data(fw, ptr + fpdt_hdr_len, fpdt->length - fpdt_hdr_len, ptr + fpdt_hdr_len - data);
			break;
		default:	/* For future use */
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"FPDTPerformanceRecordTypeInvalid",
				"FPDT Performance Record Type is 0x%4.4" PRIx16
				" which is a reserved type for future use only",
				fpdt->type);
			break;
		}
		ptr += fpdt->length;

		/* terminate loop for buggy BIOS */
		if (fpdt->length == 0)
			break;
	}

done:
	if (passed)
		fwts_passed(fw, "No issues found in FPDT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test fpdt_tests[] = {
	{ fpdt_test1, "FPDT Firmware Performance Data Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops fpdt_ops = {
	.description = "FPDT Firmware Performance Data Table test.",
	.init        = FPDT_init,
	.minor_tests = fpdt_tests
};

FWTS_REGISTER("fpdt", &fpdt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
