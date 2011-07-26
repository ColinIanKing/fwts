/*
 * Copyright (C) 2010-2011 Canonical
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

#include "fwts.h"

static void acpi_table_check_ecdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_ecdt *ecdt = (fwts_acpi_table_ecdt*)table->data;

	if ((ecdt->ec_control.address_space_id != 0) &&
            (ecdt->ec_control.address_space_id != 1))
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "ECDTECCtrlAddrSpaceID", "ECDT EC_CONTROL address space id = %hhu, "
				"should be 0 or 1 (System I/O Space or System Memory Space)",
			ecdt->ec_control.address_space_id);

	if ((ecdt->ec_data.address_space_id != 0) &&
            (ecdt->ec_data.address_space_id != 1))
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "ECDTECDataAddrSpaceID", "ECDT EC_CONTROL address space id = %hhu, "
				"should be 0 or 1 (System I/O Space or System Memory Space)",
			ecdt->ec_data.address_space_id);
}

static void acpi_table_check_facs(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_facs *facs = (fwts_acpi_table_facs*)table->data;

	if (table->length < 24) {
		if ((facs->flags & 2) == 0) {
			if (facs->firmware_waking_vector == 0)
				fwts_failed(fw, LOG_LEVEL_MEDIUM, "FACS32BitExec", "FACS: flags indicate 32 bit execution environment "
						"but the 32 bit Firmware Waking Vector is set to zero.");
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FACS64BitExec", "FACS: flags indicate a 64 bit execution environment "
					"but the FACS table is not long enough to support 64 bit "
					"X Firmware Waking Vector.");
		}
	}
	else {
		if ((facs->flags & 2) && (facs->x_firmware_waking_vector == 0))
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FACT64BitExecWakingVectorZero", "FACS: Flags indicate a 64 bit execution environment "
					"but the 64 bit X Firmware Waking Vector is set to zero.");
	}
}

static void acpi_table_check_hpet(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_hpet *hpet = (fwts_acpi_table_hpet*)table->data;

	if (hpet->base_address.address == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "HPETBaseZero", "HPET base is 0x000000000000, which is invalid.");
	
	if (((hpet->event_timer_block_id >> 16) & 0xffff) == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "HPETVendorIdZero", "HPET PCI Vendor ID is 0x0000, which is invalid.");
}

static void acpi_table_check_fadt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_fadt *fadt = (fwts_acpi_table_fadt*)table->data;

	if (fadt->firmware_control == 0) {
		if (table->length >= 140) {
			if (fadt->x_firmware_ctrl == 0) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTFACSZero", "FADT 32 bit FIRMWARE_CONTROL and 64 bit X_FIRMWARE_CONTROL (FACS address) are null.");
			}
		} else
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADT32BitFACSNull", "FADT 32 bit FIRMWARE_CONTROL is null.");
	} else {
		if (table->length >= 140) {
			if (fadt->x_firmware_ctrl != 0) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADT32And64BothDefined", "FADT 32 bit FIRMWARE_CONTROL is non-zero, and X_FIRMWARE_CONTROL is also non-zero. "
						"Section 5.2.9 of the ACPI specification states that if the FIRMWARE_CONTROL is non-zero "
						"then X_FIRMWARE_CONTROL must be set to zero.");
				if (((uint64_t)fadt->firmware_control != fadt->x_firmware_ctrl)) {
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "FwCtrl32and64Differ", "FIRMWARE_CONTROL is 0x%x and differs from X_FIRMWARE_CONTROL 0x%llx",
						(unsigned int)fadt->firmware_control, (unsigned long long int)fadt->x_firmware_ctrl);
				}
			}
		}
	}

	if (fadt->dsdt == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTDSTNull", "FADT DSDT address is null.");
	if (table->length >= 148) {
		if (fadt->x_dsdt == 0)
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTXDSTKNull", "FADT X_DSDT address is null.");
		else if ((uint64_t)fadt->dsdt != fadt->x_dsdt)
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADT32And64Mismatch", "FADT 32 bit DSDT (0x%x) does not point to same physical address as 64 bit X_DSDT (0x%llx).",
				(unsigned int)fadt->dsdt, (unsigned long long int)fadt->x_dsdt);
	}

	
	if (fadt->sci_int == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTSCIIRQZero", "FADT SCI Interrupt is 0x00, should be defined.");
	if (fadt->smi_cmd == 0) {
		if ((fadt->acpi_enable == 0) &&
		    (fadt->acpi_disable == 0) &&
		    (fadt->s4bios_req == 0) &&
		    (fadt->pstate_cnt == 0) &&
		    (fadt->cst_cnt == 0))
			fwts_warning(fw, "FADT SMI_CMD is 0x00, system appears to not support System Management mode.");
		else
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTSMICMDZero",
					"FADT SMI_CMD is 0x00, however, one or more of ACPI_ENABLE, ACPI_DISABLE, "
					"S4BIOS_REQ, PSTATE_CNT and CST_CNT are defined which means SMI_CMD should be "
					"defined otherwise SMI commands cannot be sent.");
	}
	
	if (fadt->pm_tmr_len != 4)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadPMTMRLEN", "FADT PM_TMR_LEN is %hhu, should be 4.", fadt->pm_tmr_len);
	if (fadt->gpe0_blk_len & 1)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadGPEBLKLEN", "FADT GPE0_BLK_LEN is %hhu, should a multiple of 2.", (int)fadt->gpe0_blk_len);
	if (fadt->gpe1_blk_len & 1)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadGPE1BLKLEN", "FADT GPE1_BLK_LEN is %hhu, should a multiple of 2.", fadt->gpe1_blk_len);
	if (fadt->p_lvl2_lat > 100)
		fwts_warning(fw, "FADT P_LVL2_LAT is %hu, a value > 100 indicates a system not to support a C2 state.", fadt->p_lvl2_lat);
	if (fadt->p_lvl3_lat > 1000)
		fwts_warning(fw, "FADT P_LVL3_LAT is %hu, a value > 1000 indicates a system not to support a C3 state.", fadt->p_lvl3_lat);
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
		    (fadt->reset_reg.address_space_id != 2))
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "FADTBadRESETREG", "FADT RESET_REG was %hhu, must be System I/O space, System Memory space "
				"or PCI configuration spaces.",
				fadt->reset_reg.address_space_id);

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
	if (!passed)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "RSDPBadOEMId", "RSDP: oem_id does not contain any alpha numeric characters.");

	if (rsdp->revision > 2)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "RSDPBadRevisionId", "RSDP: revision is %hhu, expected value less than 2.", rsdp->revision);
}

static void acpi_table_check_rsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	int i;
	int n;
	fwts_acpi_table_rsdt *rsdt = (fwts_acpi_table_rsdt*)table->data;

	n = (table->length - sizeof(fwts_acpi_table_header)) / sizeof(uint32_t);
	for (i=0; i<n; i++)  {
		if (rsdt->entries[i] == 0)
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "RSDTEntryNull", "RSDT Entry %d is null, should not be non-zero.", i);
	}
}

static void acpi_table_check_sbst(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_sbst *sbst = (fwts_acpi_table_sbst*)table->data;
	
	if (sbst->critical_energy_level > sbst->low_energy_level)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SBSTEnergyLevel1", "SBST Critical Energy Level (%u) is greater than the Low Energy Level (%u).",
			sbst->critical_energy_level, sbst->low_energy_level);

	if (sbst->low_energy_level > sbst->warning_energy_level)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SBSTEnergeyLevel2", "SBST Low Energy Energy Level (%u) is greater than the Warning Energy Level (%u).",
			sbst->low_energy_level, sbst->warning_energy_level);

	if (sbst->warning_energy_level == 0)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SBSTEnergyLevelZero", "SBST Warning Energy Level is zero, which is probably too low.");
}

static void acpi_table_check_xsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	int i;
	int n;
	fwts_acpi_table_xsdt *xsdt = (fwts_acpi_table_xsdt*)table->data;

	n = (table->length - sizeof(fwts_acpi_table_header)) / sizeof(uint64_t);
	for (i=0; i<n; i++)  {
		if (xsdt->entries[i] == 0)
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "XSDTEntryNull", "XSDT Entry %d is null, should not be non-zero.", i);
	}
}

static void acpi_table_check_madt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_madt *madt = (fwts_acpi_table_madt*)table->data;
	const void *data = table->data;
	int length = table->length;
	int i = 0;
	
	if (madt->flags & 0xfffffffe)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTFlagsNonZero", "MADT flags field, bits 1..31 are reserved and should be zero, but are set as: %lx.\n",
			(unsigned long int)madt->flags);

	data += sizeof(fwts_acpi_table_madt);	
	length -= sizeof(fwts_acpi_table_madt);
	
	while (length > (int)sizeof(fwts_acpi_madt_sub_table_header)) {
		int skip = 0;
		i++;
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);
		
		switch (hdr->type) {
		case 0: {
				fwts_acpi_madt_processor_local_apic *lapic = (fwts_acpi_madt_processor_local_apic *)data;
				if (lapic->flags & 0xfffffffe)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTAPICFlagsNonZero", "MADT Local APIC flags field, bits 1..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)lapic->flags);
				skip = sizeof(fwts_acpi_madt_processor_local_apic);
			}
			break;
		case 1: {
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
		case 2: {
				fwts_acpi_madt_interrupt_override *int_override = (fwts_acpi_madt_interrupt_override*)data;
				if (int_override->bus != 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTIRQSrcISA", "MADT Interrupt Source Override Bus should be 0 for ISA bus.");
				if (int_override->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTIRQSrcFlags", "MADT Interrupt Source Override flags, bits 4..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)int_override->flags);
				skip = sizeof(fwts_acpi_madt_interrupt_override);
			}
			break;
		case 3: {
				fwts_acpi_madt_nmi *nmi = (fwts_acpi_madt_nmi*)data;
				if (nmi->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTNMISrcFlags", "MADT Non-Maskable Interrupt Source, flags, bits 4..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)nmi->flags);
				skip = sizeof(fwts_acpi_madt_nmi);
			}
			break;
		case 4: {
				fwts_acpi_madt_local_apic_nmi *nmi = (fwts_acpi_madt_local_apic_nmi*)data;
				if (nmi->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTLAPICNMIFlags", "MADT Local APIC NMI flags, bits 4..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)nmi->flags);
				skip = sizeof(fwts_acpi_madt_local_apic_nmi);
			}
			break;
		case 5: {
				//fwts_acpi_madt_local_apic_addr_override *override = (fwts_acpi_madt_local_apic_addr_override*)data;
				skip = sizeof(fwts_acpi_madt_local_apic_addr_override);
			}
			break;
		case 6: {
				fwts_acpi_madt_io_sapic *sapic = (fwts_acpi_madt_io_sapic*)data;
				if (sapic->address == 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADIOSPAICAddrZero", "MADT I/O SAPIC address is zero, appears not to be defined.");
				skip = sizeof(fwts_acpi_madt_io_sapic);
			}
			break;
		case 7: {
				fwts_acpi_madt_local_sapic *local_sapic = (fwts_acpi_madt_local_sapic*)data;
				skip = sizeof(fwts_acpi_madt_local_sapic) + strlen(local_sapic->uid_string) + 1;
			}
			break;
		case 8: {
				fwts_acpi_madt_platform_int_source *src = (fwts_acpi_madt_platform_int_source*)data;
				if (src->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQSrcFlags",
						"MADT Platform Interrupt Source, flags, bits 4..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)src->flags);
				if (src->type > 3)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQType",
						"MADT Platform Interrupt Source, type field is %hhu, should be 1..3.", src->type);
				if (src->io_sapic_vector == 0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQIOSAPICVector",
						"MADT Platform Interrupt Source, IO SAPIC Vector is zero, appears not to be defined.");
				if (src->pis_flags & 0xfffffffe)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADTPlatIRQSrcFlagsNonZero",
						"MADT Platform Interrupt Source, Platform Interrupt Source flag bits 1..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)src->pis_flags);
				skip = (sizeof(fwts_acpi_madt_platform_int_source));
			}
			break;
		case 9:
			skip = (sizeof(fwts_acpi_madt_local_x2apic));
			break;
		case 10: {
				fwts_acpi_madt_local_x2apic_nmi *nmi = (fwts_acpi_madt_local_x2apic_nmi*)data;
				if (nmi->flags & 0xfffffff0)
					fwts_failed(fw, LOG_LEVEL_MEDIUM, "MADLAPICX2APICNMIFlags",
						"MADT Local x2APIC NMI, flags, bits 4..31 are reserved and should be zero, but are set as: %lx.", (unsigned long int)nmi->flags);
				skip = (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			}
			break;
		default:
			skip = 0;
			break;
		}
		data   += skip;
		length -= skip;
	}
}

static void acpi_table_check_mcfg(fwts_framework *fw, fwts_acpi_table_info *table)
{
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
	{ "FACS", acpi_table_check_facs },
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
		int failed = fw->minor_tests.failed;
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

static fwts_framework_minor_test acpi_table_check_tests[] = {
	{ acpi_table_check_test1, "Check ACPI tables." },
	{ NULL, NULL }
};

static fwts_framework_ops acpi_table_check_ops = {
	.description = "ACPI table settings sanity checks.",
	.minor_tests = acpi_table_check_tests
};

FWTS_REGISTER(acpitables, &acpi_table_check_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
