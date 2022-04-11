/*
 * Copyright (C) 2016-2022 Canonical
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

#include "fwts_acpi_object_eval.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

static fwts_acpi_table_info *table;
acpi_table_init(MSCT, &table)

/*
 *  MSCT Maximum System Characteristics Table
 */
static int msct_test1(fwts_framework *fw)
{
	fwts_acpi_table_msct *msct = (fwts_acpi_table_msct*) table->data;
	fwts_acpi_msct_proximity *proximity;
	uint32_t num_proximity = 0, offset_proximity = 0;
	uint32_t i;
	bool passed = true;

	fwts_log_info_verbatim(fw, "MSCT Max System Characteristics Table:");
	fwts_log_info_simp_int(fw, "  Proximity Offset:      ",	msct->proximity_offset);
	fwts_log_info_simp_int(fw, "  Max Proximity Domains: ",	msct->max_proximity_domains);
	fwts_log_info_simp_int(fw, "  Max Clock Domains:     ",	msct->max_clock_domains);
	fwts_log_info_simp_int(fw, "  Max Physical Address:  ",	msct->max_address);

	if (msct->proximity_offset < 0x38) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"MSCTBadProximityOffset",
			"Proximity Domain Information Structures need to be at "
			"Offset 0x38 or after, but Proximity Offsetis is 0x%"
			PRIx32, msct->proximity_offset);
		offset_proximity = 0x38;
		passed = false;
	} else
		offset_proximity = msct->proximity_offset;

	if (offset_proximity + sizeof(fwts_acpi_msct_proximity) *
		msct->max_proximity_domains > msct->header.length) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"MSCTBadTableLength",
			"MSCT Table is too small to include maximum number of "
			"Proximity Domains");
		passed = false;
	}

	num_proximity = (msct->header.length - offset_proximity) /
			sizeof(fwts_acpi_msct_proximity);

	if (num_proximity > msct->max_proximity_domains + 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"MSCTBadProimityDomains",
			"MSCT's max number of proximity domains is 0x%" PRIx32
			", but it has 0x%" PRIx32 " Proximity Domain "
			"Information Structures",
			msct->max_proximity_domains, num_proximity);
		passed = false;
	}

	fwts_log_nl(fw);

	proximity = (fwts_acpi_msct_proximity *)((char *) msct + offset_proximity);
	for (i = 0; i < num_proximity; i++, proximity++) {
		fwts_log_info_verbatim(fw, "  Proximity Domain %2.2" PRIu8, i);
		fwts_log_info_verbatim(fw, "    Revision:               0x%2.2"
				PRIx8, proximity->revision);
		fwts_log_info_verbatim(fw, "    Length:                 0x%2.2"
				PRIx8,	proximity->length);
		fwts_log_info_verbatim(fw, "    Domain Range (low):     0x%8.8"
				PRIx32, proximity->range_start);
		fwts_log_info_verbatim(fw, "    Domain Range (high):    0x%8.8"
				PRIx32, proximity->range_end);
		fwts_log_info_verbatim(fw, "    Max Processor Capacity: 0x%8.8"
				PRIx32, proximity->processor_capacity);
		fwts_log_info_verbatim(fw, "    Max Memory Capacity:    "
				"0x%16.16" PRIx64, proximity->memory_capacity);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in MSCT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test msct_tests[] = {
	{ msct_test1, "MSCT Maximum System Characteristics Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops msct_ops = {
	.description = "MSCT Maximum System Characteristics Table test.",
	.init        = MSCT_init,
	.minor_tests = msct_tests
};

FWTS_REGISTER("msct", &msct_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH |
	      FWTS_FLAG_ACPI)

#endif
