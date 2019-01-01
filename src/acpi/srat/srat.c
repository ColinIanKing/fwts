/*
 * Copyright (C) 2015-2019 Canonical
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

static int srat_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "SRAT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI SRAT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static void srat_check_local_apic_sapic_affinity(
	fwts_framework *fw,
	ssize_t		*length,
	uint8_t		**data,
	bool		*passed)
{
	fwts_acpi_table_local_apic_sapic_affinity *affinity =
		(fwts_acpi_table_local_apic_sapic_affinity *)*data;

	if ((ssize_t)sizeof(fwts_acpi_table_local_apic_sapic_affinity) > *length) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATLocalApicSapicAffinityShort",
			"SRAT Local APIC/SPAIC Affinity structure too short, got "
			"%zu bytes, expecting %zu bytes",
			*length, sizeof(fwts_acpi_table_local_apic_sapic_affinity));
		*passed = false;
		goto done;
	}

	if (affinity->length != sizeof(fwts_acpi_table_local_apic_sapic_affinity)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATLocalApicSapicAffinityLength",
			"SRAT Local APIC/SPAIC Affinity Length incorrect, got "
			"%" PRIu8 ", expecting %zu",
			affinity->length, sizeof(fwts_acpi_table_local_apic_sapic_affinity));
		*passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "SRAT Local APIC/SAPIC Affinity Structure:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, affinity->type);
	fwts_log_info_verbatim(fw, "  Length:                   0x%2.2" PRIx8, affinity->length);
	fwts_log_info_verbatim(fw, "  Proximity Domain:   [7:0] 0x%2.2" PRIx8, affinity->proximity_domain_0);
	fwts_log_info_verbatim(fw, "  APIC ID:                  0x%2.2" PRIx8, affinity->apic_id);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, affinity->flags);
	fwts_log_info_verbatim(fw, "  Local SAPIC EID:          0x%2.2" PRIx8, affinity->local_sapic_eid);
	fwts_log_info_verbatim(fw, "  Proximity Domain:  [8:15] 0x%2.2" PRIx8, affinity->proximity_domain_1);
	fwts_log_info_verbatim(fw, "  Proximity Domain: [16:23] 0x%2.2" PRIx8, affinity->proximity_domain_2);
	fwts_log_info_verbatim(fw, "  Proximity Domain: [23:31] 0x%2.2" PRIx8, affinity->proximity_domain_3);
	fwts_log_info_verbatim(fw, "  Clock Domain              0x%8.8" PRIx32, affinity->clock_domain);
	fwts_log_nl(fw);

	fwts_acpi_reserved_bits_check(fw, "SRAT", "Local APIC/SPAIC Affinity Flags", affinity->flags, sizeof(affinity->flags), 1, 31, passed);

	/*
	 * Not clear of bits 0..7 of Proximity Domain are reserved or not
	 * so skip check for these
	 */

	/*
	 *  Clock domain probably needs deeper sanity checking, for now
	 *  skip this.
	 */
done:
	*length -= sizeof(fwts_acpi_table_local_apic_sapic_affinity);
	*data += sizeof(fwts_acpi_table_local_apic_sapic_affinity);
}

static void srat_check_memory_affinity(
	fwts_framework *fw,
	ssize_t		*length,
	uint8_t		**data,
	bool		*passed)
{
	fwts_acpi_table_memory_affinity *affinity =
		(fwts_acpi_table_memory_affinity *)*data;

	if ((ssize_t)sizeof(fwts_acpi_table_memory_affinity) > *length) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATMemoryAffinityShort",
			"SRAT Memory Affinity structure too short, got "
			"%zu bytes, expecting %zu bytes",
			*length, sizeof(fwts_acpi_table_memory_affinity));
		*passed = false;
		goto done;
	}

	if (affinity->length != sizeof(fwts_acpi_table_memory_affinity)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATMemoryAffinityLength",
			"SRAT Memory Affinity Length incorrect, got "
			"%" PRIu8 ", expecting %zu",
			affinity->length, sizeof(fwts_acpi_table_memory_affinity));
		*passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "SRAT Memory Affinity Structure:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, affinity->type);
	fwts_log_info_verbatim(fw, "  Length:                   0x%2.2" PRIx8, affinity->length);
	fwts_log_info_verbatim(fw, "  Proximity Domain:         0x%8.8" PRIx32, affinity->proximity_domain);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, affinity->reserved1);
	fwts_log_info_verbatim(fw, "  Base Address:             0x%8.8" PRIx32 "%8.8" PRIx32,
		affinity->base_addr_hi, affinity->base_addr_lo);
	fwts_log_info_verbatim(fw, "  Length:                   0x%8.8" PRIx32 "%8.8" PRIx32,
		affinity->length_hi, affinity->length_lo);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%8.8" PRIx32, affinity->reserved2);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, affinity->flags);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%16.16" PRIx64, affinity->reserved3);
	fwts_log_nl(fw);

	fwts_acpi_reserved_bits_check(fw, "SRAT", "Memory Affinity Flags", affinity->flags, sizeof(affinity->flags), 3, 31, passed);

done:
	*length -= sizeof(fwts_acpi_table_memory_affinity);
	*data += sizeof(fwts_acpi_table_memory_affinity);
}

static void srat_check_local_x2apic_affinity(
	fwts_framework *fw,
	ssize_t		*length,
	uint8_t		**data,
	bool		*passed)
{
	fwts_acpi_table_local_x2apic_affinity *affinity =
		(fwts_acpi_table_local_x2apic_affinity *)*data;

	if ((ssize_t)sizeof(fwts_acpi_table_local_x2apic_affinity) > *length) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATLocalx2apicAffinityShort",
			"SRAT Local x2APIC Affinity structure too short, got "
			"%zu bytes, expecting %zu bytes",
			*length, sizeof(fwts_acpi_table_local_x2apic_affinity));
		*passed = false;
		goto done;
	}

	if (affinity->length != sizeof(fwts_acpi_table_local_x2apic_affinity)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATLocalx2apicAffinityLength",
			"SRAT Local x2APIC Affinity Length incorrect, got "
			"%" PRIu8 ", expecting %zu",
			affinity->length, sizeof(fwts_acpi_table_local_x2apic_affinity));
		*passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "SRAT Local x2APIC Affinity Structure:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, affinity->type);
	fwts_log_info_verbatim(fw, "  Length:                   0x%2.2" PRIx8, affinity->length);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, affinity->reserved1);
	fwts_log_info_verbatim(fw, "  Proximity Domain:         0x%4.4" PRIx16, affinity->proximity_domain);
	fwts_log_info_verbatim(fw, "  X2APIC ID:                0x%8.8" PRIx32, affinity->x2apic_id);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, affinity->flags);
	fwts_log_info_verbatim(fw, "  Clock Domain              0x%8.8" PRIx32, affinity->clock_domain);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, affinity->reserved2);
	fwts_log_nl(fw);

	/* Spec states 1st reserved field MUST be zero */
	if (affinity->reserved1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATLocalx2apicReserved1",
			"SRAT Local x2APIC Affinity 1st Reserved field should be zero, got "
			"0x%" PRIx32,
			affinity->reserved1);
		*passed = false;
	}

	fwts_acpi_reserved_bits_check(fw, "SRAT", "Local x2APIC Affinity Flags", affinity->flags, sizeof(affinity->flags), 1, 31, passed);

	/*
	 *  Clock domain probably needs deeper sanity checking, for now
	 *  skip this.
	 */

done:
	*length -= sizeof(fwts_acpi_table_local_x2apic_affinity);
	*data += sizeof(fwts_acpi_table_local_x2apic_affinity);
}

static void srat_check_gicc_affinity(
	fwts_framework *fw,
	ssize_t		*length,
	uint8_t		**data,
	bool		*passed)
{
	fwts_acpi_table_gicc_affinity *affinity =
		(fwts_acpi_table_gicc_affinity *)*data;

	if ((ssize_t)sizeof(fwts_acpi_table_gicc_affinity) > *length) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATGICCAffinityShort",
			"SRAT GICC Affinity structure too short, got "
			"%zu bytes, expecting %zu bytes",
			*length, sizeof(fwts_acpi_table_gicc_affinity));
		*passed = false;
		goto done;
	}

	if (affinity->length != sizeof(fwts_acpi_table_gicc_affinity)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATGICCAffinityLength",
			"SRAT GICC Affinity Length incorrect, got "
			"%" PRIu8 ", expecting %zu",
			affinity->length, sizeof(fwts_acpi_table_gicc_affinity));
		*passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "SRAT GICC Affinity Structure:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, affinity->type);
	fwts_log_info_verbatim(fw, "  Length:                   0x%2.2" PRIx8, affinity->length);
	fwts_log_info_verbatim(fw, "  Proximity Domain:         0x%8.8" PRIx32, affinity->proximity_domain);
	fwts_log_info_verbatim(fw, "  ACPI Processor UID:       0x%8.8" PRIx32, affinity->acpi_processor_uid);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, affinity->flags);
	fwts_log_info_verbatim(fw, "  Clock Domain              0x%8.8" PRIx32, affinity->clock_domain);
	fwts_log_nl(fw);

	fwts_acpi_reserved_bits_check(fw, "SRAT", "GICC Affinity Flags", affinity->flags, sizeof(affinity->flags), 1, 31, passed);

	/*
	 *  Clock domain probably needs deeper sanity checking, for now
	 *  skip this.
	 */

done:
	*length -= sizeof(fwts_acpi_table_gicc_affinity);
	*data += sizeof(fwts_acpi_table_gicc_affinity);
}

static void srat_check_its_affinity(
	fwts_framework *fw,
	ssize_t		*length,
	uint8_t		**data,
	bool		*passed)
{
	fwts_acpi_table_its_affinity *affinity =
		(fwts_acpi_table_its_affinity *)*data;

	if ((ssize_t)sizeof(fwts_acpi_table_its_affinity) > *length) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATITSAffinityShort",
			"SRAT ITS Affinity structure too short, got "
			"%zu bytes, expecting %zu bytes",
			*length, sizeof(fwts_acpi_table_its_affinity));
		*passed = false;
		goto done;
	}

	if (affinity->length != sizeof(fwts_acpi_table_its_affinity)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATITSAffinityLength",
			"SRAT ITS Affinity Length incorrect, got "
			"%" PRIu8 ", expecting %zu",
			affinity->length, sizeof(fwts_acpi_table_its_affinity));
		*passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "SRAT ITS Affinity Structure:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, affinity->type);
	fwts_log_info_verbatim(fw, "  Length:                   0x%2.2" PRIx8, affinity->length);
	fwts_log_info_verbatim(fw, "  Proximity Domain:         0x%8.8" PRIx32, affinity->proximity_domain);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, affinity->reserved);
	fwts_log_info_verbatim(fw, "  ITS ID                    0x%8.8" PRIx32, affinity->its_id);
	fwts_log_nl(fw);

	fwts_acpi_reserved_zero_check(fw, "SRAT", "ITS Affinity Reserved", affinity->reserved, sizeof(affinity->reserved), passed);

done:
	*length -= sizeof(fwts_acpi_table_its_affinity);
	*data += sizeof(fwts_acpi_table_its_affinity);
}

/*
 *  See ACPI 6.0, Section 5.2.16
 */
static int srat_test1(fwts_framework *fw)
{
	const fwts_acpi_table_srat *srat = (const fwts_acpi_table_srat *)table->data;
	uint8_t *data = (uint8_t *)table->data;
	bool passed = true;
	ssize_t length = (ssize_t)srat->header.length;

	if (srat->reserved1 != 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SRATInterfaceReserved",
			"SRAT reserved field 1 should be 1, instead was "
			"0x%" PRIx32, srat->reserved1);
		passed = false;
	}

	data += sizeof(fwts_acpi_table_srat);
	length -= sizeof(fwts_acpi_table_srat);

	while (length > 0) {
		switch (*data) {
		case 0x00:
			srat_check_local_apic_sapic_affinity(fw, &length, &data, &passed);
			break;
		case 0x01:
			srat_check_memory_affinity(fw, &length, &data, &passed);
			break;
		case 0x02:
			srat_check_local_x2apic_affinity(fw, &length, &data, &passed);
			break;
		case 0x03:
			srat_check_gicc_affinity(fw, &length, &data, &passed);
			break;
		case 0x04:
			srat_check_its_affinity(fw, &length, &data, &passed);
			break;
		default:
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SRATInvalidType",
				"SRAT Affinity Structure Type 0x%" PRIx8
				" is an invalid type, expecting 0x00..0x02",
				*data);
			passed = false;
			length = 0;
			break;
		}
	}

	if (passed)
		fwts_passed(fw, "No issues found in SRAT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test srat_tests[] = {
	{ srat_test1, "SRAT System Resource Affinity Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops srat_ops = {
	.description = "SRAT System Resource Affinity Table test.",
	.init        = srat_init,
	.minor_tests = srat_tests
};

FWTS_REGISTER("srat", &srat_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
