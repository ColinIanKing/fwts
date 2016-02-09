/*
 * Copyright (C) 2015-2016 Canonical
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
 */
#include "fwts.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

static fwts_acpi_table_info *table;

static int rsdp_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "RSDP", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (!table) {
		/*
		 * Really old x86 machines may not have an RSDP, but
		 * ia64 and arm64 are both required to be UEFI, so
		 * therefore must have an RSDP.
		 */
		if (fw->target_arch == FWTS_ARCH_X86)
			return FWTS_SKIP;
		else {
			fwts_log_error(fw,
				       "ACPI RSDP is required for the "
				       "%s target architecture.",
				       fwts_arch_get_name(fw->target_arch));
			return FWTS_ERROR;
		}
	}

	/* We know there is an RSDP now, so do a quick sanity check */
	if (table->length == 0) {
		fwts_log_error(fw,
			       "ACPI RSDP table has zero length");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

/*
 *  RSDP Root System Description Pointer
 */
static int rsdp_test1(fwts_framework *fw)
{
	fwts_acpi_table_rsdp *rsdp = (fwts_acpi_table_rsdp *)table->data;
	bool passed = true;
	size_t i;
	int value;
	uint8_t *ptr;
	uint8_t checksum;

	/* verify first checksum */
	checksum = fwts_checksum(table->data, 20);
	if (checksum == 0)
		fwts_passed(fw, "RSDP first checksum is correct");
	else
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			    "RSDPBadFirstChecksum",
			    "RSDP first checksum is incorrect: 0x%x", checksum);

	for (i = 0; i < 6; i++) {
		if (!isprint(rsdp->oem_id[i])) {
			passed = false;
			break;
		}
	}
	if (passed)
		fwts_passed(fw,
			    "RSDP: oem_id contains only printable "
			    "characters.");
	else {
		fwts_failed(fw, LOG_LEVEL_LOW,
			"RSDPBadOEMId",
			"RSDP: oem_id contains non-printable "
			"characters.");
		fwts_advice(fw,
			"The RSDP OEM Id is non-conforming, but this "
			"will not affect the system behaviour. However "
			"this should be fixed if possible to make the "
			"firmware ACPI complaint.");
	}

	if (rsdp->revision <= 2)
		fwts_passed(fw,
			    "RSDP: revision is %" PRIu8 ".", rsdp->revision);
	else
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"RSDPBadRevisionId",
			"RSDP: revision is %" PRIu8 ", expected "
			"value less than 2.", rsdp->revision);

	/* whether RSDT or XSDT depends arch */
	if (rsdp->rsdt_address == 0 && rsdp->xsdt_address == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			    "RSDPNoAddressesFound",
			    "RSDP: either the RsdtAddress must be non-zero "
			    "or the XsdtAddress must be non-zero.  Both "
			    "fields are zero.");
	else
		fwts_passed(fw,
			    "RSDP: at least one of RsdtAddress or XsdtAddress "
			    "is non-zero.");

	if (rsdp->rsdt_address != 0 && rsdp->xsdt_address != 0)
		if ((uint64_t)rsdp->rsdt_address == rsdp->xsdt_address) {
			fwts_warning(fw,
				     "Both RSDT and XSDT addresses are set. "
				     "Since they are the same address, this "
				     "is unambiguous to the OS.");
			fwts_advice(fw,
				    "Set only one of the 32-bit RSDT or the "
				    "64-bit XSDT addresses.  Recent versions "
				    "of the spec require that only one of "
				    "these be used but as a practical matter, "
				    "many vendors do use both.  If both "
				    "fields must be used, make sure they at "
				    "least contain the same value so that "
				    "the OS can unambiguously determine "
				    "which address is the correct one.");
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				    "RSDPBothAddressesFound",
				    "RSDP: only one of RsdtAddress or "
				    "XsdtAddress should be non-zero.  Both "
				    "fields are non-zero.");
		}
	else
		fwts_passed(fw,
			    "RSDP: only one of RsdtAddress or XsdtAddress "
			    "is non-zero.");

	passed = false;
	switch (fw->target_arch) {
	case FWTS_ARCH_X86:
		if (rsdp->rsdt_address != 0 || rsdp->xsdt_address != 0)
			passed = true;
		break;

	case FWTS_ARCH_ARM64:
		if (rsdp->xsdt_address != 0)
			passed = true;
		break;

	default:
		passed = true;
		fwts_log_advice(fw,
				"No clear rule for RSDT/XSDT address usage "
				"is provided for the %s architecture.",
				fwts_arch_get_name(fw->target_arch));
	}
	if (passed)
		fwts_passed(fw,
			    "RSDP: the correct RSDT/XSDT address is being "
			    "used.");
	else
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			    "RSDPBadAddresses",
			    "RSDP: the wrong RSDT/XSDT address is being "
			    "used for the %s architecture.",
			    fwts_arch_get_name(fw->target_arch));

	/* check the length field */
	if (rsdp->length == 36)
		fwts_passed(fw, "RSDP: the table is the correct length.");
	else
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			    "RSDPBadLength",
			    "RSDP: length is %d bytes but should be 36.",
			    rsdp->length);

	/* verify the extended checksum */
	checksum = fwts_checksum(table->data, 36);
	if (checksum == 0)
		fwts_passed(fw, "RSDP second checksum is correct");
	else
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			    "RSDPBadSecondChecksum",
			    "RSDP second checksum incorrect: 0x%x", checksum);

	/* the reserved field should be zero */
	value = 0;
	for (ptr = (uint8_t *)rsdp->reserved, i = 0; i < 3; i++)
		value += *ptr++;
	if (value)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			    "RSDPReserved",
			    "RSDP: reserved field is non-zero.");
	else
		fwts_passed(fw, "RSDP: the reserved field is zero.");

	return FWTS_OK;
}

static fwts_framework_minor_test rsdp_tests[] = {
	{ rsdp_test1, "RSDP Root System Description Pointer test." },
	{ NULL, NULL }
};

static fwts_framework_ops rsdp_ops = {
	.description = "RSDP Root System Description Pointer test.",
	.init        = rsdp_init,
	.minor_tests = rsdp_tests
};

FWTS_REGISTER("rsdp", &rsdp_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH |
	      FWTS_FLAG_TEST_ACPI | FWTS_FLAG_TEST_COMPLIANCE_ACPI)
