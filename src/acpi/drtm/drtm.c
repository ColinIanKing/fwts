/*
 * Copyright (C) 2016-2018 Canonical
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

static int drtm_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "DRTM", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI DRTM table does not exist, skipping test");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 *  DRTM D-RTM Resources Table
 */
static int drtm_test1(fwts_framework *fw)
{
	fwts_acpi_table_drtm *drtm = (fwts_acpi_table_drtm*) table->data;
	fwts_acpi_table_drtm_vtl *drtm_vtl;
	fwts_acpi_table_drtm_rtl *drtm_rtl;
	fwts_acpi_table_drtm_dps *drtm_dps;
	bool passed = true;
	uint32_t offset;
	uint32_t i;

	fwts_log_info_verbatim(fw, "DRTM D-RTM Resources Table:");
	fwts_log_info_verbatim(fw, "  DL_Entry_Base:            0x%16.16" PRIx64, drtm->entry_base_address);
	fwts_log_info_verbatim(fw, "  DL_Entry_Length:          0x%16.16" PRIx64, drtm->entry_length);
	fwts_log_info_verbatim(fw, "  DL_Entry32:               0x%8.8" PRIx32, drtm->entry_address32);
	fwts_log_info_verbatim(fw, "  DL_Entry64:               0x%16.16" PRIx64, drtm->entry_address64);
	fwts_log_info_verbatim(fw, "  DLME_Exit:                0x%16.16" PRIx64, drtm->exit_address);
	fwts_log_info_verbatim(fw, "  Log_Area_Start:           0x%16.16" PRIx64, drtm->log_area_address);
	fwts_log_info_verbatim(fw, "  Log_Area_Length:          0x%8.8" PRIx32, drtm->log_area_length);
	fwts_log_info_verbatim(fw, "  Architecture_Dependent:   0x%16.16" PRIx64, drtm->arch_dependent_address);
	fwts_log_info_verbatim(fw, "  DRT_Flags:                0x%8.8" PRIx32, drtm->flags);

	fwts_acpi_reserved_bits_check(fw, "DRTM", "DRT_Flags", drtm->flags, sizeof(drtm->flags), 4, 31, &passed);
	fwts_log_nl(fw);

	offset = sizeof(fwts_acpi_table_drtm);
	drtm_vtl = (fwts_acpi_table_drtm_vtl *) (table->data + offset);
	fwts_log_info_verbatim(fw, "  VTL_Length:               0x%8.8" PRIx32, drtm_vtl->validated_table_count);
	offset += sizeof(drtm_vtl->validated_table_count);

	if (drtm->header.length < offset + sizeof(uint64_t) * drtm_vtl->validated_table_count) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"DRTMOutOfBound",
			"DRTM's length is too small to contain all fields");
		goto error;
	}

	for (i = 0; i < drtm_vtl->validated_table_count; i++) {
		fwts_log_info_verbatim(fw, "  Validated_Tables:         0x%16.16" PRIx64, drtm_vtl->validated_tables[i]);
		offset += sizeof(drtm_vtl->validated_tables[i]);
	}

	fwts_log_nl(fw);

	drtm_rtl = (fwts_acpi_table_drtm_rtl *) (table->data + offset);
	fwts_log_info_verbatim(fw, "  RL_Length:                0x%8.8" PRIx32, drtm_rtl->resource_count);
	offset += sizeof(drtm_rtl->resource_count);

	if (drtm->header.length < offset + sizeof(fwts_acpi_drtm_resource) * drtm_rtl->resource_count) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"DRTMOutOfBound",
			"DRTM's length is too small to contain all fields");
		goto error;
	}

	for (i = 0; i < drtm_rtl->resource_count; i++) {
		fwts_acpi_drtm_resource *resource = (fwts_acpi_drtm_resource *) (table->data + offset);
		uint64_t size;
		size = resource->size[0] + ((uint64_t) resource->size[1] << 8) +
					   ((uint64_t) resource->size[2] << 16) + ((uint64_t) resource->size[3] << 24) +
					   ((uint64_t) resource->size[4] << 32) + ((uint64_t) resource->size[5] << 40) +
					   ((uint64_t) resource->size[6] << 48);

		fwts_log_info_verbatim(fw, "  Resource Size:            0x%16.16" PRIx64, size);
		fwts_log_info_verbatim(fw, "  Resource Type:            0x%2.2" PRIx8, resource->type);
		fwts_log_info_verbatim(fw, "  Resource Address:         0x%16.16" PRIx64, resource->address);

		if (resource->type & 0x7C) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"DRTMBadResourceType",
				"DRTM Resource Type Bits [6:2] are reserved, got 0x%2.2" PRIx8
				" instead",	resource->type);
		}

		offset += sizeof(fwts_acpi_drtm_resource);
		fwts_log_nl(fw);
	}

	drtm_dps = (fwts_acpi_table_drtm_dps *) (table->data + offset);
	fwts_log_info_verbatim(fw, "  DPS_Length:               0x%8.8" PRIx32, drtm_dps->dps_id_length);

	if (drtm->header.length < offset + sizeof(fwts_acpi_table_drtm_dps)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"DRTMOutOfBound",
			"DRTM's length is too small to contain all fields");
		goto error;
	}

	for (i = 0; i < sizeof(drtm_dps->dps_id); i++) {
		fwts_log_info_verbatim(fw, "  DLME Platform Id:         0x%2.2" PRIx8, drtm_dps->dps_id[i]);
	}

	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in DRTM table.");

 error:
	return FWTS_OK;
}

static fwts_framework_minor_test drtm_tests[] = {
	{ drtm_test1, "DRTM D-RTM Resources Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops drtm_ops = {
	.description = "DRTM D-RTM Resources Table test.",
	.init        = drtm_init,
	.minor_tests = drtm_tests
};

FWTS_REGISTER("drtm", &drtm_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
