/*
 * Copyright (C) 2010-2015 Canonical
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

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#define FADT_RESET_SUPPORTED		(1 << 10)

static const fwts_acpi_table_fadt *fadt;
static int fadt_size;

static int fadt_init(fwts_framework *fw)
{
	fwts_acpi_table_info *table;

	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI table FACP.");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI table FACP does not exist!");
		return FWTS_ERROR;
	}
	fadt = (const fwts_acpi_table_fadt*)table->data;
	fadt_size = table->length;

	/*  Not having a FADT is not a failure */
	if (fadt_size == 0) {
		fwts_log_info(fw, "FADT does not exist, this is not necessarily a failure, skipping tests.");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static void acpi_table_check_fadt_firmware_control(
	fwts_framework *fw,
	const fwts_acpi_table_fadt *fadt,
	bool *passed)
{
	if (fadt->firmware_control == 0) {
		if (fadt->header.length >= 140) {
			if ((fadt->x_firmware_ctrl == 0) &&
			    !(fwts_acpi_is_reduced_hardware(fadt))) {
				*passed = false;
				fwts_failed(fw, LOG_LEVEL_CRITICAL,
					"FADTFACSZero",
					"FADT 32 bit FIRMWARE_CONTROL and "
					"64 bit X_FIRMWARE_CONTROL (FACS "
					"address) are null.");
				fwts_advice(fw,
					"The 32 bit FIRMWARE_CTRL or 64 "
					"bit X_FIRMWARE_CTRL should point "
					"to a valid Firmware ACPI Control "
					"Structure (FACS) when ACPI hardware "
					"reduced mode is not set. ");
			}
		} else {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADT32BitFACSNull",
				"FADT 32 bit FIRMWARE_CONTROL is null.");
			fwts_advice(fw,
				"The ACPI version 1.0 FADT has a NULL "
				"FIRMWARE_CTRL and it needs to be defined "
				"to point to a valid Firmware ACPI Control "
				"Structure (FACS).");
		}
	} else {
		if (fadt->header.length >= 140) {
			if (fadt->x_firmware_ctrl != 0) {
				if (((uint64_t)fadt->firmware_control != fadt->x_firmware_ctrl)) {
					*passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"FwCtrl32and64Differ",
						"FIRMWARE_CONTROL is 0x%" PRIx32
						" and differs from X_FIRMWARE_CONTROL "
						"0x%" PRIx64,
						fadt->firmware_control,
						fadt->x_firmware_ctrl);
					fwts_advice(fw,
						"One would expect the 32 bit FIRMWARE_CTRL "
						"and 64 bit X_FIRMWARE_CTRL pointers to "
						"point to the same FACS, however they don't "
						"which is clearly ambiguous and wrong. "
						"The kernel works around this by using the "
						"64 bit X_FIRMWARE_CTRL pointer to the FACS. ");
				}
			}
		}
	}
}

static void acpi_table_check_fadt_dsdt(
	fwts_framework *fw,
	const fwts_acpi_table_fadt *fadt,
	bool *passed)
{

	if (fadt->header.length < 148) {
		if (fadt->dsdt == 0) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTDSDTNull",
				"FADT DSDT address is null.");
		}
	} else {
		if (fadt->x_dsdt == 0) {
			if (fadt->dsdt == 0) {
				*passed = false;
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"FADTXDSDTNull",
					"FADT X_DSDT and DSDT address are null.");
			} else {
				*passed = false;
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"FADTXDSDTNull",
					"FADT X_DSDT address is null.");
				fwts_advice(fw,
					"An ACPI 2.0 FADT is being used however "
					"the 64 bit X_DSDT is null."
					"The kernel will fall back to using "
					"the 32 bit DSDT pointer instead.");
			}
		} else if ((uint64_t)fadt->dsdt != fadt->x_dsdt && fadt->dsdt != 0) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADT32And64Mismatch",
				"FADT 32 bit DSDT (0x%" PRIx32 ") "
				"does not point to same "
				"physical address as 64 bit X_DSDT "
				"(0x%" PRIx64 ").",
				fadt->dsdt, fadt->x_dsdt);
			fwts_advice(fw,
				"One would expect the 32 bit DSDT and "
				"64 bit X_DSDT pointers to point to the "
				"same DSDT, however they don't which is "
				"clearly ambiguous and wrong. "
				"The kernel works around this by using the "
				"64 bit X_DSDT pointer to the DSDT. ");
		}
	}
}


static void acpi_table_check_fadt_smi(
	fwts_framework *fw,
	const fwts_acpi_table_fadt *fadt,
	bool *passed)
{
	if (fwts_acpi_is_reduced_hardware(fadt)) {
		if (fadt->smi_cmd != 0) {
			fwts_warning(fw, "FADT SMI_CMD is not zero "
				"but should be in reduced hardware mode.");
		}
		return;
	}

	/*
	 *  Section 5.2.9 (Fixed ACPI Description Table) of the ACPI 5.0
	 *  specification states that if SMI_CMD is zero then it is
	 *  a system that does not support System Management Mode, so
	 *  in that case, don't check SCI_INT being valid.
	 */
	if (fadt->smi_cmd != 0) {
		if (fadt->sci_int == 0) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTSCIIRQZero",
				"FADT SCI Interrupt is 0x00, should be defined.");
		}
	} else {
		if ((fadt->acpi_enable == 0) &&
		    (fadt->acpi_disable == 0) &&
		    (fadt->s4bios_req == 0) &&
		    (fadt->pstate_cnt == 0) &&
		    (fadt->cst_cnt == 0)) {
			/* Not an error, but intentional, but feedback this finding anyhow */
			fwts_log_info(fw, "The FADT SMI_CMD is zero, system "
				"does not support System Management Mode.");
		}
		else {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTSMICMDZero",
				"FADT SMI_CMD is 0x00, however, one or more of "
				"ACPI_ENABLE, ACPI_DISABLE, S4BIOS_REQ, PSTATE_CNT "
				"and CST_CNT are defined which means SMI_CMD should "
				"be defined otherwise SMI commands cannot be sent.");
			fwts_advice(fw,
				"The configuration seems to suggest that SMI command "
				"should be defined to allow the kernel to trigger "
				"system management interrupts via the SMD_CMD port. "
				"The fact that SMD_CMD is zero which is invalid means "
				"that SMIs are not possible through the normal ACPI "
				"mechanisms. This means some firmware based machine "
				"specific functions will not work.");
		}
	}
}

static void acpi_table_check_fadt_pm_tmr(
	fwts_framework *fw,
	const fwts_acpi_table_fadt *fadt,
	bool *passed)
{
	if (fwts_acpi_is_reduced_hardware(fadt)) {
		if (fadt->pm_tmr_len != 0)
			fwts_warning(fw, "FADT PM_TMR_LEN is not zero "
				"but should be in reduced hardware mode.");
		return;
	}

	if (fadt->pm_tmr_len != 4) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"FADTBadPMTMRLEN",
			"FADT PM_TMR_LEN is %" PRIu8
			", should be 4.", fadt->pm_tmr_len);
		fwts_advice(fw,
			"FADT field PM_TMR_LEN defines the number "
			"of bytes decoded by PM_TMR_BLK. "
			"This fields value must be 4. If it is not "
			"the correct size then the kernel will not "
			"request a region for the pm timer block. ");
	}
}

static void acpi_table_check_fadt_gpe(
	fwts_framework *fw,
	const fwts_acpi_table_fadt *fadt,
	bool *passed)
{
	if (fwts_acpi_is_reduced_hardware(fadt)) {
		if (fadt->gpe0_blk_len != 0) {
			fwts_warning(fw, "FADT GPE0_BLK_LEN is not zero "
				"but should be in reduced hardware mode.");
		}
		if (fadt->gpe1_blk_len != 0) {
			fwts_warning(fw, "FADT GPE1_BLK_LEN is not zero but "
				"should be in reduced hardware mode.");
		}
		return;
	}

	if (fadt->gpe0_blk_len & 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"FADTBadGPEBLKLEN",
			"FADT GPE0_BLK_LEN is %" PRIu8
			", should a multiple of 2.",
			fadt->gpe0_blk_len);
		fwts_advice(fw,
			"The FADT GPE_BLK_LEN should be a multiple of 2. "
			"Because it isn't, the ACPI driver will not map in "
			"the GPE0 region. This could mean that General "
			"Purpose Events will not function correctly (for "
			"example lid or ac-power events).");
	}
	if (fadt->gpe1_blk_len & 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"FADTBadGPE1BLKLEN",
			"FADT GPE1_BLK_LEN is %" PRIu8
			", should a multiple of 2.",
			fadt->gpe1_blk_len);
		fwts_advice(fw,
			"The FADT GPE1_BLK_LEN should be a multiple of 2. "
			"Because it isn't, the ACPI driver will not map in "
			"the GPE1 region. This could mean that General "
			"Purpose Events will not function correctly (for "
			"example lid or ac-power events).");
	}
}

static void acpi_table_check_fadt_reset(
	fwts_framework *fw,
	const fwts_acpi_table_fadt *fadt,
	bool *passed)
{
	if (fadt->header.length>=129) {
		if ((fadt->reset_reg.address_space_id != 0) &&
		    (fadt->reset_reg.address_space_id != 1) &&
		    (fadt->reset_reg.address_space_id != 2)) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTBadRESETREG",
				"FADT RESET_REG address space ID was %"
				PRIu8 ", must be System Memory space (0), "
				"System I/O space (1), or PCI configuration "
				"space (2).",
				fadt->reset_reg.address_space_id);
			fwts_advice(fw,
				"If the FADT RESET_REG address space ID is "
				"not set correctly then ACPI writes "
				"to this register *may* nor work correctly, "
				"meaning a reboot via this mechanism may not work.");
		}
		if ((fadt->reset_value == 0) &&
		    (fadt->reset_reg.address != 0))
			fwts_warning(fw, "FADT RESET_VALUE is zero, which "
				"may be incorrect, it is usually non-zero.");
	}
}

static int fadt_test1(fwts_framework *fw)
{
	bool passed = true;

	acpi_table_check_fadt_firmware_control(fw, fadt, &passed);
	acpi_table_check_fadt_dsdt(fw, fadt, &passed);
	acpi_table_check_fadt_smi(fw, fadt, &passed);
	acpi_table_check_fadt_pm_tmr(fw, fadt, &passed);
	acpi_table_check_fadt_gpe(fw, fadt, &passed);

	/*
	 * Bug LP: #833644
	 *
	 *   Remove these tests, really need to put more intelligence into it
	 *   perhaps in the cstates test rather than here. For the moment we
 	 *   shall remove this warning as it's giving users false alarms
	 *   See: https://bugs.launchpad.net/ubuntu/+source/fwts/+bug/833644
	 */
	/*
	if (fadt->p_lvl2_lat > 100) {
		fwts_warning(fw, "FADT P_LVL2_LAT is %" PRIi16 ", a value > 100 indicates a "
			"system not to support a C2 state.", fadt->p_lvl2_lat);
		fwts_advice(fw, "The FADT P_LVL2_LAT setting specifies the C2 latency in microseconds. The ACPI specification "
				"states that a value > 100 indicates that C2 is not supported and hence the "
				"ACPI processor idle routine will not use C2 power states.");
	}
	if (fadt->p_lvl3_lat > 1000) {
		fwts_warning(fw, "FADT P_LVL3_LAT is %" PRIu16 ", a value > 1000 indicates a "
			"system not to support a C3 state.", fadt->p_lvl3_lat);
		fwts_advice(fw, "The FADT P_LVL2_LAT setting specifies the C3 latency in microseconds. The ACPI specification "
				"states that a value > 1000 indicates that C3 is not supported and hence the "
				"ACPI processor idle routine will not use C3 power states.");
	}
	*/
	acpi_table_check_fadt_reset(fw, fadt, &passed);

	if (passed)
		fwts_passed(fw, "No issues found in FADT table.");

	return FWTS_OK;
}


static int fadt_test2(fwts_framework *fw)
{
	uint32_t port, width, val32;
	int ret = FWTS_OK;

	fwts_log_info(fw, "FADT Preferred PM Profile: %hhu (%s)\n",
		fadt->preferred_pm_profile,
		FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(fadt->preferred_pm_profile));

	port = fadt->pm1a_cnt_blk;
	width = fadt->pm1_cnt_len * 8;	/* In bits */

	if ((fadt->header.revision > 1) || (fadt->header.length >= 244)) {
		/*  Sanity check sizes with extended address variants */
		fwts_log_info(fw, "FADT is greater than ACPI version 1.0");
		if ((uint64_t)port != fadt->x_pm1a_cnt_blk.address)
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTPM1CNTAddrMismatch",
				"32 and 64 bit versions of FADT pm1_cnt address do not match (0x%8.8x vs 0x%16.16" PRIx64 ").",
				port, fadt->x_pm1a_cnt_blk.address);
		if (width != fadt->x_pm1a_cnt_blk.register_bit_width)
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FADTPM1CNTSizeMismatch",
				"32 and 64 bit versions of FADT pm1_cnt size do not match (0x%x vs 0x%x).",
				width, fadt->x_pm1a_cnt_blk.register_bit_width);

		port = fadt->x_pm1a_cnt_blk.address;
		width = fadt->x_pm1a_cnt_blk.register_bit_width;
	}

	switch (width) {
	case 8:
		if (ioperm(port, width/8, 1) < 0)
			ret = FWTS_ERROR;
		else {
			uint8_t val8;

			ret = fwts_inb(port, &val8);
			val32 = val8;
			ioperm(port, width/8, 0);
		}
		break;
	case 16:
		if (ioperm(port, width/8, 1) < 0)
			ret = FWTS_ERROR;
		else {
			uint16_t val16;

			ret = fwts_inw(port, &val16);
			val32 = val16;
			ioperm(port, width/8, 0);
		}
		break;
	case 32:
		if (ioperm(port, width/8, 1) < 0)
			ret = FWTS_ERROR;
		else {
			ret = fwts_inl(port, &val32);
			ioperm(port, width/8, 0);
		}
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_HIGH, "FADTPM1AInvalidWidth",
			"FADT pm1a register has invalid bit width of %d.",
			width);
		return FWTS_OK;
	}

	if (ret != FWTS_OK) {
		fwts_log_error(fw, "Cannot read FADT PM1A_CNT_BLK port 0x%" PRIx32 ".", port);
		return FWTS_ERROR;
	}

	if (val32 & 0x01)
		fwts_passed(fw, "SCI_EN bit in PM1a Control Register Block is enabled.");
	else
		fwts_failed(fw, LOG_LEVEL_HIGH, "SCI_ENNotEnabled",
			"SCI_EN bit in PM1a Control Register Block is not enabled.");

	return FWTS_OK;
}

static int fadt_test3(fwts_framework *fw)
{
	if ((fadt->header.revision == 1) || (fadt->header.length < 244)) {
		fwts_skipped(fw, "Header size indicates an ACPI 1.0 FADT, skipping test.");
		return FWTS_SKIP;
	}
	/*
 	 * Sanity check Reset reg, c.f. ACPICA
	 * commit 57bdfbee675cd2c0618c66882d68a6bdf9f8efc4
	 */
	if (fadt->flags & FADT_RESET_SUPPORTED) {
		if (fadt->reset_reg.register_bit_width != 8) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "FadtResetRegWidth",
				"FADT reset register is not 8 bits wide, got instead: %" PRIi8 ".",
				fadt->reset_reg.register_bit_width);
			fwts_advice(fw,
				"Section 5.2.9 Fixed ACPI Description Table (Table 5-34) of the ACPI "
				"specification states that the FADT register width should "
				"be 8 bits wide. However, recent versions of ACPICA will ignore this "
				"and default to 8 bits.");
		} else
			fwts_passed(fw, "FADT reset register width is 8 bits wide as expected.");

		if (fadt->reset_reg.register_bit_offset != 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "FadtResetRegBitOffset",
				"FADT reset register bit offset is not 0, got instead: %" PRIi8 ".",
				fadt->reset_reg.register_bit_offset);
			fwts_advice(fw,
				"Section 5.2.9 Fixed ACPI Description Table (Table 5-34) of the ACPI "
				"specification states that the FADT register bit offset should be 0.");
		} else
			fwts_passed(fw, "FADT register bit offset is 0 as expected.");
	} else {
		fwts_skipped(fw, "FADT flags indicates reset register not supported, skipping test.");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static fwts_framework_minor_test fadt_tests[] = {
	{ fadt_test1, "Test FADT ACPI Description Table tests." },
	{ fadt_test2, "Test FADT SCI_EN bit is enabled." },
	{ fadt_test3, "Test FADT reset register." },
	{ NULL, NULL }
};

static fwts_framework_ops fadt_ops = {
	.description = "FADT Fixed ACPI Description Table tests.",
	.init        = fadt_init,
	.minor_tests = fadt_tests
};

FWTS_REGISTER("fadt", &fadt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV | FWTS_FLAG_TEST_ACPI)

#endif
