/*
 * Copyright (C) 2010-2015 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>

#include "fwts.h"

static void acpi_table_check_ecdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_ecdt *ecdt = (fwts_acpi_table_ecdt*)table->data;

	if ((ecdt->ec_control.address_space_id != 0) &&
            (ecdt->ec_control.address_space_id != 1)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "ECDTECCtrlAddrSpaceID",
				"ECDT EC_CONTROL address space id = %" PRIu8
				", should be 0 or 1 (System I/O Space or System Memory Space)",
				ecdt->ec_control.address_space_id);
		fwts_advice(fw, "The ECDT EC_CONTROL address space id was invalid, however the kernel ACPI EC driver "
				"will just assume it an I/O port address.  This will not affect "
				"the system behaviour and can probably be ignored.");
	}

	if ((ecdt->ec_data.address_space_id != 0) &&
            (ecdt->ec_data.address_space_id != 1)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "ECDTECDataAddrSpaceID",
				"ECDT EC_CONTROL address space id = %" PRIu8
				", should be 0 or 1 (System I/O Space or System Memory Space)",
				ecdt->ec_data.address_space_id);
		fwts_advice(fw, "The ECDT EC_DATA address space id was invalid, however the kernel ACPI EC driver "
				"will just assume it an I/O port address.  This will not affect "
				"the system behaviour and can probably be ignored.");
	}
}

static void acpi_table_check_hpet(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_hpet *hpet = (fwts_acpi_table_hpet*)table->data;

	if (hpet->base_address.address == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "HPETBaseZero", "HPET base is 0x000000000000, which is invalid.");

	if (((hpet->event_timer_block_id >> 16) & 0xffff) == 0) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "HPETVendorIdZero", "HPET PCI Vendor ID is 0x0000, which is invalid.");
		fwts_advice(fw, "The HPET specification (http://www.intel.com/hardwaredesign/hpetspec_1.pdf)  describes "
				"the HPET table in section 3.2.4 'The ACPI 2.0 HPET Description Table (HPET)'. The top "
				"16 bits of the Event Timer Block ID specify the Vendor ID and this should not be zero. "
				"This won't affect the kernel behaviour, but should be fixed as it is an undefined ID value.");
	}

}

static void acpi_table_check_fadt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_fadt *fadt = (fwts_acpi_table_fadt*)table->data;

	if (fadt->firmware_control == 0) {
		if (table->length >= 140) {
			if ((fadt->x_firmware_ctrl == 0) && !(fwts_acpi_is_reduced_hardware(fadt))) {
				fwts_failed(fw, LOG_LEVEL_CRITICAL, "FADTFACSZero", "FADT 32 bit FIRMWARE_CONTROL and 64 bit X_FIRMWARE_CONTROL (FACS address) are null.");
				fwts_advice(fw, "The 32 bit FIRMWARE_CTRL or 64 bit X_FIRMWARE_CTRL should point to a valid "
						"Firmware ACPI Control Structure (FACS) when ACPI hardware reduced mode is not set. "
						"This is a firmware bug and needs to be fixed.");
			}
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADT32BitFACSNull", "FADT 32 bit FIRMWARE_CONTROL is null.");
			fwts_advice(fw, "The ACPI version 1.0 FADT has a NULL FIRMWARE_CTRL and it needs to be defined "
					"to point to a valid Firmware ACPI Control Structure (FACS). This is a firmware "
					"bug and needs to be fixed.");
		}
	} else {
		if (table->length >= 140) {
			if (fadt->x_firmware_ctrl != 0) {
				if (((uint64_t)fadt->firmware_control != fadt->x_firmware_ctrl)) {
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "FwCtrl32and64Differ",
						"FIRMWARE_CONTROL is 0x%" PRIx32 " and differs "
						"from X_FIRMWARE_CONTROL 0x%" PRIx64,
						fadt->firmware_control,
						fadt->x_firmware_ctrl);
					fwts_advice(fw, "One would expect the 32 bit FIRMWARE_CTRL and 64 bit X_FIRMWARE_CTRL "
							"pointers to point to the same FACS, however they don't which is clearly ambiguous and wrong. "
							"The kernel works around this by using the 64 bit X_FIRMWARE_CTRL pointer to the FACS. ");
				}
			}
		}
	}

	if (fadt->dsdt == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTDSTNull", "FADT DSDT address is null.");
	if (table->length >= 148) {
		if (fadt->x_dsdt == 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTXDSTDNull", "FADT X_DSDT address is null.");
			fwts_advice(fw, "An ACPI 2.0 FADT is being used however the 64 bit X_DSDT is null."
					"The kernel will fall back to using the 32 bit DSDT pointer instead.");
		} else if ((uint64_t)fadt->dsdt != fadt->x_dsdt) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADT32And64Mismatch",
				"FADT 32 bit DSDT (0x%" PRIx32 ") does not point to same "
				"physical address as 64 bit X_DSDT (0x%" PRIx64 ").",
				fadt->dsdt, fadt->x_dsdt);
			fwts_advice(fw, "One would expect the 32 bit DSDT and 64 bit X_DSDT "
					"pointers to point to the same DSDT, however they don't which is clearly ambiguous and wrong. "
					"The kernel works around this by using the 64 bit X_DSDT pointer to the DSDT. ");
		}
	}

	/*
	 *  Section 5.2.9 (Fixed ACPI Description Table) of the ACPI 5.0
	 *  specification states that if SMI_CMD is zero then it is
	 *  a system that does not support System Management Mode, so
	 *  in that case, don't check SCI_INT being valid.
	 */
	if (fadt->smi_cmd != 0) {
		if (fadt->sci_int == 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTSCIIRQZero", "FADT SCI Interrupt is 0x00, should be defined.");
		}
	} else {
		if ((fadt->acpi_enable == 0) &&
		    (fadt->acpi_disable == 0) &&
		    (fadt->s4bios_req == 0) &&
		    (fadt->pstate_cnt == 0) &&
		    (fadt->cst_cnt == 0)) {
			/* Not an error, but intentional, but feedback this finding anyhow */
			fwts_log_info(fw, "The FADT SMI_CMD is zero, system does not support System Management Mode.");
		}
		else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTSMICMDZero",
					"FADT SMI_CMD is 0x00, however, one or more of ACPI_ENABLE, ACPI_DISABLE, "
					"S4BIOS_REQ, PSTATE_CNT and CST_CNT are defined which means SMI_CMD should be "
					"defined otherwise SMI commands cannot be sent.");
			fwts_advice(fw, "The configuration seems to suggest that SMI command should be defined to "
					"allow the kernel to trigger system management interrupts via the SMD_CMD port. "
					"The fact that SMD_CMD is zero which is invalid means that SMIs are not possible "
					"through the normal ACPI mechanisms. This means some firmware based machine "
					"specific functions will not work.");
		}
	}

	if (fadt->pm_tmr_len != 4) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadPMTMRLEN",
			"FADT PM_TMR_LEN is %" PRIu8 ", should be 4.", fadt->pm_tmr_len);
		fwts_advice(fw, "FADT field PM_TMR_LEN defines the number of bytes decoded by PM_TMR_BLK. "
				"This fields value must be 4. If it is not the correct size then the kernel "
				"will not request a region for the pm timer block. ");
	}
	if (fadt->gpe0_blk_len & 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadGPEBLKLEN", "FADT GPE0_BLK_LEN is %" PRIu8
			", should a multiple of 2.", fadt->gpe0_blk_len);
		fwts_advice(fw, "The FADT GPE_BLK_LEN should be a multiple of 2. Because it isn't, the ACPI driver will "
				"not map in the GPE0 region. This could mean that General Purpose Events will not "
				"function correctly (for example lid or ac-power events).");
	}
	if (fadt->gpe1_blk_len & 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadGPE1BLKLEN", "FADT GPE1_BLK_LEN is %" PRIu8
			", should a multiple of 2.", fadt->gpe1_blk_len);
		fwts_advice(fw, "The FADT GPE_BLK_LEN should be a multiple of 2. Because it isn't, the ACPI driver will "
				"not map in the GPE1 region. This could mean that General Purpose Events will not "
				"function correctly (for example lid or ac-power events).");
	}
	/*
	 * Bug LP: /833644
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
	/*
	if (fadt->day_alrm == 0)
		fwts_warning(fw, "FADT DAY_ALRM is zero, OS will not be able to program day of month alarm.");
	if (fadt->mon_alrm == 0)
		fwts_warning(fw, "FADT MON_ALRM is zero, OS will not be able to program month of year alarm.");
	if (fadt->century == 0)
		fwts_warning(fw, "FADT CENTURY is zero, RTC does not support centenary feature is not supported.");
	*/

	if (table->length>=129) {
		if ((fadt->reset_reg.address_space_id != 0) &&
		    (fadt->reset_reg.address_space_id != 1) &&
		    (fadt->reset_reg.address_space_id != 2)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadRESETREG",
				"FADT RESET_REG address space ID was %" PRIu8 ", must be System Memory space (0), "
				"System I/O space (1), or PCI configuration space (2).",
				fadt->reset_reg.address_space_id);
			fwts_advice(fw, "If the FADT RESET_REG address space ID is not set correctly then ACPI writes "
					"to this register *may* nor work correctly, meaning a reboot via this mechanism may not work.");
		}
		if ((fadt->reset_value == 0) && (fadt->reset_reg.address != 0))
			fwts_warning(fw, "FADT RESET_VALUE is zero, which may be incorrect, it is usually non-zero.");
	}
}

static void acpi_table_check_rsdp(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_rsdp *rsdp = (fwts_acpi_table_rsdp*)table->data;
	int i;
	int passed;

	for (passed=0,i=0;i<6;i++) {
		if (isalnum(rsdp->oem_id[i]))
			passed++;
	}
	if (!passed) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "RSDPBadOEMId", "RSDP: oem_id does not contain any alpha numeric characters.");
		fwts_advice(fw, "The RSDP OEM Id is non-conforming, but this will not affect the system behaviour. However "
				"this should be fixed if possible to make the firmware ACPI complaint.");
	}

	if (rsdp->revision > 2) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "RSDPBadRevisionId", "RSDP: revision is %" PRIu8
			", expected value less than 2.", rsdp->revision);
		fwts_advice(fw, "A RSDP revision number greater than 2 probably won't cause any system problems.");
	}
}

static void acpi_table_check_rsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	int i;
	int n;
	fwts_acpi_table_rsdt *rsdt = (fwts_acpi_table_rsdt*)table->data;

	n = (table->length - sizeof(fwts_acpi_table_header)) / sizeof(uint32_t);
	for (i=0; i<n; i++)  {
		if (rsdt->entries[i] == 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "RSDTEntryNull", "RSDT Entry %d is null, should not be non-zero.", i);
			fwts_advice(fw, "A RSDT pointer is null and therefore erroneously points to an invalid 32 bit "
					"ACPI table header. At worse this will cause the kernel to oops, at best the kernel "
					"may ignore this.  However, it should be fixed where possible.");
		}
	}
}

static void acpi_table_check_sbst(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_sbst *sbst = (fwts_acpi_table_sbst*)table->data;

	if (sbst->critical_energy_level > sbst->low_energy_level) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SBSTEnergyLevel1",
			"SBST Critical Energy Level (%" PRIu32 ") "
			"is greater than the Low Energy Level (%" PRIu32 ").",
			sbst->critical_energy_level, sbst->low_energy_level);
		fwts_advice(fw, "This could affect system behaviour based on incorrect smart battery information. This should be fixed.");
	}

	if (sbst->low_energy_level > sbst->warning_energy_level) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SBSTEnergeyLevel2",
			"SBST Low Energy Energy Level (%" PRIu32 ") "
			"is greater than the Warning Energy Level (%" PRIu32 ").",
			sbst->low_energy_level, sbst->warning_energy_level);
		fwts_advice(fw, "This could affect system behaviour based on incorrect smart battery information. This should be fixed.");
	}

	if (sbst->warning_energy_level == 0) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SBSTEnergyLevelZero", "SBST Warning Energy Level is zero, which is probably too low.");
		fwts_advice(fw, "This could affect system behaviour based on incorrect smart battery information. This should be fixed.");
	}
}

static void acpi_table_check_xsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	int i;
	int n;
	fwts_acpi_table_xsdt *xsdt = (fwts_acpi_table_xsdt*)table->data;

	n = (table->length - sizeof(fwts_acpi_table_header)) / sizeof(uint64_t);
	for (i=0; i<n; i++)  {
		if (xsdt->entries[i] == 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "XSDTEntryNull", "XSDT Entry %d is null, should not be non-zero.", i);
			fwts_advice(fw, "A XSDT pointer is null and therefore erroneously points to an invalid 64 bit "
					"ACPI table header. At worse this will cause the kernel to oops, at best the kernel "
					"may ignore this.  However, it should be fixed where possible.");
		}
	}
}

static void acpi_table_check_madt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_madt *madt = (fwts_acpi_table_madt*)table->data;
	fwts_list msi_frame_ids;
	const uint8_t *data = table->data;
	size_t length = table->length;
	int i = 0;

	fwts_list_init(&msi_frame_ids);

	if (madt->flags & 0xfffffffe)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTFlagsNonZero",
			"MADT flags field, bits 1..31 are reserved and "
			"should be zero, but are set as: %" PRIx32 ".\n",
			madt->flags);

	data += sizeof(fwts_acpi_table_madt);
	length -= sizeof(fwts_acpi_table_madt);

	while (length > sizeof(fwts_acpi_madt_sub_table_header)) {
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;
		size_t skip = 0;
		i++;

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);

		switch (hdr->type) {
		case FWTS_ACPI_MADT_LOCAL_APIC: {
				fwts_acpi_madt_processor_local_apic *lapic = (fwts_acpi_madt_processor_local_apic *)data;
				if (lapic->flags & 0xfffffffe)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTAPICFlagsNonZero",
						"MADT Local APIC flags field, bits 1..31 are reserved and "
						"should be zero, but are set as: %" PRIx32 ".",
						lapic->flags);
				skip = sizeof(fwts_acpi_madt_processor_local_apic);
			}
			break;
		case FWTS_ACPI_MADT_IO_APIC: {
				fwts_acpi_madt_io_apic *ioapic = (fwts_acpi_madt_io_apic*)data;
				if (ioapic->io_apic_phys_address == 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTIOAPICAddrZero", "MADT IO APIC address is zero, appears not to be defined.");
				/*
				if (ioapic->global_irq_base == 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTIOAPICIRQZero", "MADT IO APIC global IRQ base is zero, appears not to be defined.");
				*/
				skip = sizeof(fwts_acpi_madt_io_apic);
			}
			break;
		case FWTS_ACPI_MADT_INTERRUPT_OVERRIDE: {
				fwts_acpi_madt_interrupt_override *int_override = (fwts_acpi_madt_interrupt_override*)data;
				if (int_override->bus != 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTIRQSrcISA", "MADT Interrupt Source Override Bus should be 0 for ISA bus.");
				if (int_override->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTIRQSrcFlags",
						"MADT Interrupt Source Override flags, bits 4..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						int_override->flags);
				skip = sizeof(fwts_acpi_madt_interrupt_override);
			}
			break;
		case FWTS_ACPI_MADT_NMI_SOURCE: {
				fwts_acpi_madt_nmi *nmi = (fwts_acpi_madt_nmi*)data;
				if (nmi->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTNMISrcFlags",
						"MADT Non-Maskable Interrupt Source, flags, bits 4..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						nmi->flags);
				skip = sizeof(fwts_acpi_madt_nmi);
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_APIC_NMI: {
				fwts_acpi_madt_local_apic_nmi *nmi = (fwts_acpi_madt_local_apic_nmi*)data;
				if (nmi->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTLAPICNMIFlags",
						"MADT Local APIC NMI flags, bits 4..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						nmi->flags);
				skip = sizeof(fwts_acpi_madt_local_apic_nmi);
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_APIC_OVERRIDE:
			skip = sizeof(fwts_acpi_madt_local_apic_addr_override);
			break;
		case FWTS_ACPI_MADT_IO_SAPIC: {
				fwts_acpi_madt_io_sapic *sapic = (fwts_acpi_madt_io_sapic*)data;
				if (sapic->address == 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADIOSPAICAddrZero", "MADT I/O SAPIC address is zero, appears not to be defined.");
				skip = sizeof(fwts_acpi_madt_io_sapic);
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_SAPIC: {
				fwts_acpi_madt_local_sapic *local_sapic = (fwts_acpi_madt_local_sapic*)data;
				skip = sizeof(fwts_acpi_madt_local_sapic) + strlen(local_sapic->uid_string) + 1;
			}
			break;
		case FWTS_ACPI_MADT_INTERRUPT_SOURCE: {
				fwts_acpi_madt_platform_int_source *src = (fwts_acpi_madt_platform_int_source*)data;
				if (src->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQSrcFlags",
						"MADT Platform Interrupt Source, flags, bits 4..31 are "
						"reserved and should be zero, but are set as: %" PRIx32 ".",
						src->flags);
				if (src->type > 3)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQType",
						"MADT Platform Interrupt Source, type field is %" PRIu8
						", should be 1..3.", src->type);
				if (src->io_sapic_vector == 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQIOSAPICVector",
						"MADT Platform Interrupt Source, IO SAPIC Vector is zero, appears not to be defined.");
				if (src->pis_flags & 0xfffffffe)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQSrcFlagsNonZero",
						"MADT Platform Interrupt Source, Platform Interrupt Source flag "
						"bits 1..31 are reserved and should be zero, but are "
						"set as: %" PRIx32 ".", src->pis_flags);
				skip = (sizeof(fwts_acpi_madt_platform_int_source));
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_X2APIC:
			skip = (sizeof(fwts_acpi_madt_local_x2apic));
			break;
		case FWTS_ACPI_MADT_LOCAL_X2APIC_NMI: {
				fwts_acpi_madt_local_x2apic_nmi *nmi = (fwts_acpi_madt_local_x2apic_nmi*)data;
				if (nmi->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTLAPICX2APICNMIFlags",
						"MADT Local x2APIC NMI, flags, bits 4..31 are reserved and "
						"should be zero, but are set as: %" PRIx32 ".",
						nmi->flags);
				skip = (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			}
			break;
		case FWTS_ACPI_MADT_GIC_C_CPU_INTERFACE: {
				fwts_acpi_madt_gic *gic = (fwts_acpi_madt_gic*)data;

				if (gic->reserved)
					fwts_failed(fw, LOG_LEVEL_LOW, "MADTGICCReservedNonZero",
						"MADT GIC C Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gic->reserved);

				if (gic->flags & 0xfffffffc)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTGICFLags",
						"MADT GIC, flags, bits 2..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						gic->flags & 0xfffffffc);

				skip = sizeof(fwts_acpi_madt_gic);
			}
			break;
		case FWTS_ACPI_MADT_GIC_D_GOC_DISTRIBUTOR: {
				fwts_acpi_madt_gicd *gicd = (fwts_acpi_madt_gicd*)data;

				if (gicd->reserved)
					fwts_failed(fw, LOG_LEVEL_LOW, "MADTGICDReservedNonZero",
						"MADT GIC Distributor Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gicd->reserved);
				if (gicd->reserved2)
					fwts_failed(fw, LOG_LEVEL_LOW, "MADTGICDReserved2NonZero",
						"MADT GIC Distributor Structure second reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gicd->reserved2);
			}
			skip = sizeof(fwts_acpi_madt_gicd);
			break;
		case FWTS_ACPI_MADT_GIC_V2M_MSI_FRAME: {
				fwts_acpi_madt_gic_msi *gic_msi = (fwts_acpi_madt_gic_msi*)data;
				fwts_list_link *item;

				if (gic_msi->reserved)
					fwts_failed(fw, LOG_LEVEL_LOW, "MADTGICMSIReservedNonZero",
						"MADT GIC MSI Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gic_msi->reserved);

				if (gic_msi->flags & 0xfffffffe) {
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTGICMSIFLags",
						"MADT GIC MSI Frame, flags, bits 1..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						gic_msi->flags);
				}

				/* Check against previously IDs to see if unique */
				fwts_list_foreach(item, &msi_frame_ids) {
					uint32_t *frame_id = fwts_list_data(uint32_t *, item);

					/* Frame ID must be unique according to the spec */
					if (*frame_id == gic_msi->frame_id) {
						fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTGICMSINonUniqueFrameId",
							"MADT GIC MSI Frame ID 0x%" PRIx32 " is not unique "
							"and has already be defined in a previous GIC MSI "
							"Frame.", gic_msi->frame_id);
					} else {
						fwts_list_append(&msi_frame_ids, &(gic_msi->frame_id));
					}
				}
			}
			skip = sizeof(fwts_acpi_madt_gic_msi);
			break;
		case FWTS_ACPI_MADT_GIC_R_REDISTRIBUTOR: {
				fwts_acpi_madt_gicr *gicr = (fwts_acpi_madt_gicr*)data;

				if (gicr->reserved)
					fwts_failed(fw, LOG_LEVEL_LOW, "MADTGICRReservedNonZero",
						"MADT GIC Redistributor Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gicr->reserved);
			}
			skip = sizeof(fwts_acpi_madt_gicr);
			break;
		default:
			skip = 0;
			break;
		}
		data   += skip;
		length -= skip;
	}
	fwts_list_free_items(&msi_frame_ids, NULL);
}

static void acpi_table_check_mcfg(fwts_framework *fw, fwts_acpi_table_info *table)
{
	FWTS_UNUSED(fw);
	FWTS_UNUSED(table);

	/* FIXME */
	/*fwts_acpi_table_mcfg *mcfg = (fwts_acpi_table_mcfg*)table->data;*/
}

typedef void (*check_func)(fwts_framework *fw, fwts_acpi_table_info *table);

typedef struct {
	char *name;
	check_func func;
} acpi_table_check_table;

static acpi_table_check_table check_table[] = {
	{ "APIC", acpi_table_check_madt },
	{ "ECDT", acpi_table_check_ecdt },
	{ "FACP", acpi_table_check_fadt },
	{ "HPET", acpi_table_check_hpet },
	{ "MCFG", acpi_table_check_mcfg },
	{ "RSDT", acpi_table_check_rsdt },
	{ "RSDP", acpi_table_check_rsdp },
	{ "SBST", acpi_table_check_sbst },
	{ "XSDT", acpi_table_check_xsdt },
	{ NULL  , NULL },
} ;

static int acpi_table_check_test1(fwts_framework *fw)
{
	int i;

	for (i=0; check_table[i].name != NULL; i++) {
		uint32_t failed = fw->minor_tests.failed;
		fwts_acpi_table_info *table;

		if (fwts_acpi_find_table(fw, check_table[i].name, 0, &table) != FWTS_OK) {
			fwts_aborted(fw, "Cannot load ACPI table %s.", check_table[i].name);
			/* If this fails, we cannot load any subsequent tables so abort */
			break;
		}

		if (table) {
			check_table[i].func(fw, table);
			if (failed == fw->minor_tests.failed)
				fwts_passed(fw, "Table %s passed.", check_table[i].name);
		} else {
			fwts_log_info(fw, "Table %s not present to check.", check_table[i].name);
		}
	}

	return FWTS_OK;
}

static bool acpi_table_check_field(const char *field, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (!isascii(field[i]))
			return false;

	return true;
}

static bool acpi_table_check_field_test(
	fwts_framework *fw,
	const char *table_name,
	const char *field_name,
	const char *field,
	const size_t len)
{
	if (!acpi_table_check_field(field, len)) {
		fwts_failed(fw, LOG_LEVEL_LOW, "ACPITableHdrInfo",
			"ACPI Table %s has non-ASCII characters in "
			"header field %s\n", table_name, field_name);
		return false;
	}
	return true;
}

static int acpi_table_check_test2(fwts_framework *fw)
{
	int i;
	bool checked = false;

	for (i = 0; ; i++) {
		fwts_acpi_table_info *info;
		fwts_acpi_table_header *hdr;
		char name[50];
		bool passed;

		if (fwts_acpi_get_table(fw, i, &info) != FWTS_OK)
			break;
		if (info == NULL)
			continue;

		checked = true;
		/* RSDP and FACS are special cases, skip these */
		if (!strcmp(info->name, "RSDP") ||
		    !strcmp(info->name, "FACS"))
			continue;

		hdr = (fwts_acpi_table_header *)info->data;
		if (acpi_table_check_field(hdr->signature, 4)) {
			snprintf(name, sizeof(name), "%4.4s", hdr->signature);
		} else {
			/* Table name not printable, so identify it by the address */
			snprintf(name, sizeof(name), "at address 0x%" PRIx64, info->addr);
		}

		/*
		 * Tables shouldn't be short, however, they do have at
		 * least 4 bytes with their signature else they would not
		 * have been loaded by this stage.
		 */
		if (hdr->length < sizeof(fwts_acpi_table_header)) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "ACPITableHdrShort",
				"ACPI Table %s is too short, only %d bytes long. Further "
				"header checks will be omitted.", name, hdr->length);
			continue;
		}
		/* Warn about empty tables */
		if (hdr->length == sizeof(fwts_acpi_table_header)) {
			fwts_warning(fw,
				"ACPI Table %s is empty and just contains a table header. Further "
				"header checks will be omitted.", name);
			continue;
		}

		passed = acpi_table_check_field_test(fw, name, "Signature", hdr->signature, 4) &
			 acpi_table_check_field_test(fw, name, "OEM ID", hdr->oem_id, 6) &
			 acpi_table_check_field_test(fw, name, "OEM Table ID", hdr->oem_tbl_id, 8) &
			acpi_table_check_field_test(fw, name, "OEM Creator ID", hdr->creator_id, 4);
		if (passed)
			fwts_passed(fw, "Table %s has valid signature and ID strings.", name);

	}
	if (!checked)
		fwts_aborted(fw, "Cannot find any ACPI tables.");

	return FWTS_OK;
}

static fwts_framework_minor_test acpi_table_check_tests[] = {
	{ acpi_table_check_test1, "Test ACPI tables." },
	{ acpi_table_check_test2, "Test ACPI headers." },
	{ NULL, NULL }
};

static fwts_framework_ops acpi_table_check_ops = {
	.description = "ACPI table settings sanity tests.",
	.minor_tests = acpi_table_check_tests
};

FWTS_REGISTER("acpitables", &acpi_table_check_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH)
