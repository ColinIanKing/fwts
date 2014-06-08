/*
 * Copyright (C) 2014 Canonical
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
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>

/* acpica headers */
#include "acpi.h"
#include "aclocal.h"
#include "acobject.h"
#include "acstruct.h"
#include "acglobal.h"
#include "fwts_acpi_object_eval.h"

/*
 *  gpedump_init()
 *	initialize ACPI
 */
static int gpedump_init(fwts_framework *fw)
{
	if (fwts_acpi_init(fw) != FWTS_OK) {
		fwts_log_error(fw, "Cannot initialise ACPI.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

/*
 *  gpedump_deinit
 *	de-intialize ACPI
 */
static int gpedump_deinit(fwts_framework *fw)
{
	return fwts_acpi_deinit(fw);
}

static void gpu_dump_gpes(
	fwts_framework *fw,
	uint32_t reg,
	ACPI_GPE_BLOCK_INFO *gpe_block_info)
{
	uint32_t i, gpe = ACPI_GPE_REGISTER_WIDTH * reg;

	for (i = 0; i < ACPI_GPE_REGISTER_WIDTH; i++, gpe++) {
		char buf[80];
		uint32_t n;
		ACPI_GPE_NOTIFY_INFO *notify_info;
		ACPI_GPE_EVENT_INFO *GpeEventInfo = &gpe_block_info->EventInfo[gpe];

		if ((GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) == ACPI_GPE_DISPATCH_NONE)
			continue;

		switch (GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) {
		case ACPI_GPE_DISPATCH_NONE:
			strncpy(buf, "none", sizeof(buf));
			break;
		case ACPI_GPE_DISPATCH_HANDLER:
			strncpy(buf, "handler", sizeof(buf));
			break;
		case ACPI_GPE_DISPATCH_METHOD:
			strncpy(buf, "method", sizeof(buf));
			break;
		case ACPI_GPE_DISPATCH_NOTIFY:
			for (n = 0, notify_info = GpeEventInfo->Dispatch.NotifyList; notify_info; notify_info = notify_info->Next)
				n++;
			snprintf(buf, sizeof(buf), "notifes %" PRIu32 " devices", n);
			break;
		}
		fwts_log_info_verbatum(fw, "      GPE 0x%2.2" PRIx8 ":",
			gpe_block_info->BlockBaseNumber + gpe);
		fwts_log_info_verbatum(fw, "        Flags:    0x%2.2" PRIx8 " (trigger: %s, wake: %s)",
			GpeEventInfo->Flags,
			GpeEventInfo->Flags & ACPI_GPE_LEVEL_TRIGGERED ? "level" : "edge",
			GpeEventInfo->Flags & ACPI_GPE_CAN_WAKE ? "can wake" : "run only");
		fwts_log_info_verbatum(fw, "        Dispatch: %s", buf);
	}
}

static void gpedump_block(
	fwts_framework *fw,
	ACPI_GPE_XRUPT_INFO	*gpe_xrupt_info,
	ACPI_GPE_BLOCK_INFO	*gpe_block_info,
	uint32_t block)
{
	ACPI_BUFFER	buffer;
	ACPI_STATUS	Status;
	uint32_t	reg;
	char 		name[128];

	buffer.Length = sizeof(name);
	buffer.Pointer = name;

	Status = AcpiGetName (gpe_block_info->Node, ACPI_FULL_PATHNAME, &buffer);
	if (ACPI_FAILURE (Status))
		strncpy(name, "unknown", sizeof(name));

	fwts_log_nl(fw);
	fwts_log_info_verbatum(fw, "Block %" PRIu32": DeviceNode %s (%s)\n",
		block, name,
		gpe_block_info->Node == AcpiGbl_FadtGpeDevice ?
			"FADT Defined GPE Block" :
			"GPE Block Device");

	fwts_log_info_verbatum(fw, "  GPE range: 0x%2.2" PRIx8 " to 0x%2.2" PRIx8 " on interrupt %" PRIu32 "\n",
		(uint8_t)gpe_block_info->BlockBaseNumber,
		(uint8_t)gpe_block_info->BlockBaseNumber + (gpe_block_info->GpeCount - 1),
		(uint32_t)gpe_xrupt_info->InterruptNumber);
	fwts_log_info_verbatum(fw, "  GPE count: %" PRIu32 " (%" PRIu32 " registers)\n",
		(uint32_t)gpe_block_info->GpeCount,
		(uint32_t)gpe_block_info->RegisterCount);

	for (reg = 0; reg < gpe_block_info->RegisterCount; reg++) {
		ACPI_GPE_REGISTER_INFO *gpe_reg_info = &gpe_block_info->RegisterInfo[reg];

		fwts_log_nl(fw);
		fwts_log_info_verbatum(fw,
			"    Register %" PRIu32 ": (GPE 0x%2.2" PRIx8 "-0x%2.2" PRIx8 ")",
			reg,
			gpe_reg_info->BaseGpeNumber,
			gpe_reg_info->BaseGpeNumber + (ACPI_GPE_REGISTER_WIDTH - 1));
		fwts_log_info_verbatum(fw, "      Run Enable:     0x%2.2" PRIx8,
			gpe_reg_info->EnableForRun);
		fwts_log_info_verbatum(fw, "      Wake Enable:    0x%2.2" PRIx8,
			gpe_reg_info->EnableForWake);
		fwts_log_info_verbatum(fw, "      Status Address: 0x%8.8" PRIx64,
			(uint64_t)gpe_reg_info->StatusAddress.Address);
		fwts_log_info_verbatum(fw, "      Enable Address: 0x%8.8" PRIx64,
			(uint64_t)gpe_reg_info->EnableAddress.Address);

		gpu_dump_gpes(fw, reg, gpe_block_info);
	}
}

static int gpedump_test1(fwts_framework *fw)
{
	ACPI_GPE_BLOCK_INFO     *gpe_block_info;
	ACPI_GPE_XRUPT_INFO     *gpe_xrupt_info;
	uint32_t		block = 0;

	gpe_xrupt_info = AcpiGbl_GpeXruptListHead;

	for (gpe_xrupt_info = AcpiGbl_GpeXruptListHead; gpe_xrupt_info; gpe_xrupt_info = gpe_xrupt_info->Next) {
		for (gpe_block_info = gpe_xrupt_info->GpeBlockListHead; gpe_block_info; gpe_block_info = gpe_block_info->Next) {
			gpedump_block(fw, gpe_xrupt_info, gpe_block_info, block);
			block++;
		}
	}
	return FWTS_OK;
}

static fwts_framework_minor_test gpedump_tests[] = {
	{ gpedump_test1, "Dump GPEs." },
	{ NULL, NULL }
};

static fwts_framework_ops gpedump_ops = {
	.description = "Dump GPEs.",
	.init        = gpedump_init,
	.deinit      = gpedump_deinit,
	.minor_tests = gpedump_tests
};

FWTS_REGISTER("gpedump", &gpedump_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UTILS)
