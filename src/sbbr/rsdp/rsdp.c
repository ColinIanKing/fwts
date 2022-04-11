/*
 * Copyright (C) 2015-2022 Canonical
 * Copyright (C) 2017-2021 ARM Ltd
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

#if defined(FWTS_HAS_ACPI) && (FWTS_ARCH_AARCH64)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#define CHECKSUM_BYTES		20
#define SBBR_RSDP_REVISION	2
#define SBBR_RSDP_LENGTH	36
#define EXT_CHECKSUM_BYTES	36

static fwts_acpi_table_info *table;

static int rsdp_sbbr_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "RSDP", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (!table) {
		fwts_log_error(fw,
			       "ACPI RSDP is required for the "
			       "%s target architecture.",
			       fwts_arch_get_name(fw->target_arch));
		return FWTS_ERROR;
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
static int rsdp_sbbr_test1(fwts_framework *fw)
{
	fwts_acpi_table_rsdp *rsdp = (fwts_acpi_table_rsdp *)table->data;
	uint8_t checksum;

	/*
	 * This includes only the first 20 bytes of this table, bytes
	 * 0 to 19, including the checksum field. These bytes must sum to
	 * zero.
	 */
	static const char RSDP_SIGNATURE[] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};
	bool signature_pass;
	bool checksum_pass      = false;
	bool rsdp_revision_pass = false;
	bool rsdp_length_pass   = false;
	bool ext_checksum_pass  = false;
	bool xsdt_address_pass  = false;

	fwts_log_info(fw, "RSDP Signature = %.8s", rsdp->signature);
	signature_pass = strncmp(rsdp->signature, RSDP_SIGNATURE,
			sizeof(rsdp->signature))? false : true;

	/* verify first checksum */
	checksum = fwts_checksum(table->data, CHECKSUM_BYTES);
	fwts_log_info(fw, "RSDP Checksum = 0x%x", checksum);
	checksum_pass = (checksum == 0) ? true : false;

	fwts_log_info(fw, "RSDP Revision = 0x%x", rsdp->revision);
	rsdp_revision_pass = (rsdp->revision >= SBBR_RSDP_REVISION) ? true : false;

	fwts_log_info(fw, "RSDP Length = 0x%x", rsdp->length);
	rsdp_length_pass = (rsdp->length == SBBR_RSDP_LENGTH) ? true : false;

	checksum = fwts_checksum(table->data, EXT_CHECKSUM_BYTES);
	fwts_log_info(fw, "RSDP Extended Checksum = 0x%x", checksum);
	ext_checksum_pass = (checksum == 0) ? true : false;

	if ((rsdp->xsdt_address != 0) && (rsdp->rsdt_address == 0))
		xsdt_address_pass = true;

	if ((signature_pass == true) && (checksum_pass == true) &&
	(rsdp_revision_pass == true) && (rsdp_length_pass == true) &&
	(ext_checksum_pass == true) && (xsdt_address_pass == true)) {
		fwts_passed(fw,
			"SBBR RSDP: Structure of RSDP Table is consistent with "
			"ACPI 6.0 or later and uses 64 bit xsdt addresses.");
	} else {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"SBBR RSDP:",
			"Structure of RSDP Table is not consistent with ACPI 6.0 "
			"or later and/or does not use 64 bit xsdt addresses.");
	}

	return FWTS_OK;
}

static fwts_framework_minor_test rsdp_sbbr_tests[] = {
	{ rsdp_sbbr_test1, "RSDP Root System Description Pointer test." },
	{ NULL, NULL }
};

static fwts_framework_ops rsdp_sbbr_ops = {
	.description = "SBBR RSDP Root System Description Pointer tests.",
	.init        = rsdp_sbbr_init,
	.minor_tests = rsdp_sbbr_tests
};

FWTS_REGISTER("rsdp_sbbr", &rsdp_sbbr_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_SBBR)

#endif
