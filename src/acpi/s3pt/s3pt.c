/*
 * Copyright (C) 2024-2025 Canonical
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
 */
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

static fwts_acpi_table_info *table;
acpi_table_init(S3PT, &table)

static int s3pt_test1(fwts_framework *fw)
{

	fwts_acpi_s3pt_record_hdr *hdr;
	fwts_acpi_s3pt_resume *resume_record;
	fwts_acpi_s3pt_suspend *suspend_record;
	bool passed = true;
	uint32_t offset;
	uint16_t resume_type_count = 0;
	uint16_t suspend_type_count = 0;

	offset = sizeof(fwts_acpi_table_s3pt_header);
	fwts_log_info_verbatim(fw, "S3 Performance Table:");
	while (offset < table->length) {
		hdr = (fwts_acpi_s3pt_record_hdr *)(table->data + offset);
		if ((offset + hdr->record_len) > table->length) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"S3PTOutOfRangeOffset",
			"S3PT resume performance record offset is out of range.");
			passed = false;
			break;
		}
		switch(hdr->rt_perf_record_type) {
			case 0: /* Resume Performance Record Type */
				resume_record = (fwts_acpi_s3pt_resume *)(table->data + offset);
				fwts_log_info_verbatim(fw, "  Basic S3 Resume Performance Record:");
				fwts_log_info_simp_int(fw, "    Record Type: ", resume_record->header.rt_perf_record_type);
				fwts_log_info_simp_int(fw, "    Record Length: ", resume_record->header.record_len);
				fwts_log_info_simp_int(fw, "    Revision: ", resume_record->header.revision);
				fwts_log_info_simp_int(fw, "    Resume Count: ", resume_record->resume_count);
				fwts_log_info_simp_int(fw, "    FullResume: ", resume_record->full_resume);
				fwts_log_info_simp_int(fw, "    AverageResume: ", resume_record->average_resume);
				resume_type_count++;
				break;
			case 1: /* Suspend Performance Record */
				suspend_record = (fwts_acpi_s3pt_suspend *)(table->data + offset);
				fwts_log_info_verbatim(fw, "  Basic S3 Suspend Performance Record:");
				fwts_log_info_simp_int(fw, "    Record Type: ", suspend_record->header.rt_perf_record_type);
				fwts_log_info_simp_int(fw, "    Record Length: ", suspend_record->header.record_len);
				fwts_log_info_simp_int(fw, "    Revision: ", suspend_record->header.revision);
				fwts_log_info_simp_int(fw, "    SuspendStart: ", suspend_record->suspend_start);
				fwts_log_info_simp_int(fw, "    SuspendEnd: ", suspend_record->suspend_end);
				suspend_type_count++;
				break;
			default:
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"S3PTPerformanceRecordTypeInvalid",
					"S3PT Performance Record Type is %" PRIu16
					" which is invalid value.",
					hdr->rt_perf_record_type);
				break;
		}

		offset += hdr->record_len;
		fwts_log_nl(fw);
	}
	if (resume_type_count != 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
				"S3PTBadRecordTypeCount",
				"One of the basic S3 resume performance "
				"record type and only will be produced, but got %"
				PRIu16, resume_type_count);
	}

	if (suspend_type_count > 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
				"S3PTBadRecordTypeCount",
				"Zero to one of the basic S3 suspend performance "
				"record type will be produced, but got %" PRIu16,
				suspend_type_count);
	}

	if (passed)
		fwts_passed(fw, "No issues found in S3PT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test s3pt_tests[] = {
	{ s3pt_test1, "Validate S3PT table." },
	{ NULL, NULL }
};

static fwts_framework_ops s3pt_ops = {
	.description = "S3 Performance Table test.",
	.init        = S3PT_init,
	.minor_tests = s3pt_tests
};

FWTS_REGISTER("s3pt", &s3pt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
