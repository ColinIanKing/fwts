/*
 * Copyright (C) 2010 Canonical
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
#include <sys/io.h> 

#include "fwts.h"

static char *acpi_table_check_headline(void)
{
	return "ACPI table settings sanity checks.";
}

static void acpi_table_check_ecdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_ecdt *ecdt = (fwts_acpi_table_ecdt*)table->data;

	if ((ecdt->ec_control.address_space_id != 0) &&
            (ecdt->ec_control.address_space_id != 1)) 
		fwts_failed(fw, "ECDT EC_CONTROL address space id = %d, "
				"should be 0 or 1 (System I/O Space or System Memory Space)", 
			ecdt->ec_control.address_space_id);

	if ((ecdt->ec_data.address_space_id != 0) &&
            (ecdt->ec_data.address_space_id != 1)) 
		fwts_failed(fw, "ECDT EC_CONTROL address space id = %d, "
				"should be 0 or 1 (System I/O Space or System Memory Space", 
			ecdt->ec_data.address_space_id);
}

static void acpi_table_check_facs(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_facs *facs = (fwts_acpi_table_facs*)table->data;

	if (table->length < 24) {
		if ((facs->flags & 2) == 0) {
			if (facs->firmware_waking_vector == 0)
				fwts_failed(fw, "FACS: flags indicate 32 bit execution environment "
						"but the 32 bit Firmware Waking Vector is set to zero.");
		} else {
			fwts_failed(fw, "FACS: flags indicate a 64 bit execution environment "
					"but the FACS table is not long enough to support 64 bit "
					"X Firmware Waking Vector.");
		}
	}
	else {
		if ((facs->flags & 2) && (facs->x_firmware_waking_vector == 0))
			fwts_failed(fw, "FACS: Flags indicate a 64 bit execution environment "
					"but the 64 bit X Firmware Waking Vector is set to zero.");
	}
}

static void acpi_table_check_hpet(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_hpet *hpet = (fwts_acpi_table_hpet*)table->data;

	if (hpet->base_address.address == 0) 
		fwts_failed(fw, "HPET base is 0x000000000000, which is invalid.");
	
	if (((hpet->event_timer_block_id >> 16) & 0xffff) == 0)
		fwts_failed(fw, "HPET PCI Vendor ID is 0x0000, which is invalid.");
}

static void acpi_table_check_fadt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_fadt *fadt = (fwts_acpi_table_fadt*)table->data;

	if (fadt->firmware_control == 0) {
		if (table->length >= 140) {
			if (fadt->x_firmware_ctrl == 0) {
				fwts_failed(fw, "FADT 32 bit FIRMWARE_CONTROL and 64 bit X_FIRMWARE_CONTROL (FACS address) are null.");
			}
		} else
			fwts_failed(fw, "FADT 32 bit FIRMWARE_CONTROL is null.");
	} else {
		if (table->length >= 140) {
			if (fadt->x_firmware_ctrl != 0) {
				fwts_failed(fw, "FADT 32 bit FIRMWARE_CONTROL is non-zero, and X_FIRMWARE_CONTROL is also non-zero. "
						"Section 5.2.9 of the ACPI specification states that if the FIRMWARE_CONTROL is non-zero "
						"then X_FIRMWARE_CONTROL must be set to zero.");
				if (((uint64_t)fadt->firmware_control != fadt->x_firmware_ctrl)) {
					fwts_failed(fw, "FIRMWARE_CONTROL is 0x%lx and differs from X_FIRMWARE_CONTROL 0x%llx",
						fadt->firmware_control, fadt->x_firmware_ctrl);
				}
			}
		}
	}

	if (fadt->dsdt == 0)
		fwts_failed(fw, "FADT DSDT address is null.");
	if (table->length >= 148) {
		if (fadt->x_dsdt == 0) 
			fwts_failed(fw, "FADT X_DSDT address is null.");
		else if ((uint64_t)fadt->dsdt != fadt->x_dsdt) 
			fwts_failed(fw, "FADT 32 bit DSDT (0x%lx) does not point to same physical address as 64 bit X_DSDT (0x%llx).",
				fadt->dsdt, fadt->x_dsdt);
	}

	
	if (fadt->sci_int == 0)
		fwts_failed(fw, "FADT SCI Interrupt is 0x00, should be defined.");
	if (fadt->smi_cmd == 0) {
		if ((fadt->acpi_enable == 0) && 
		    (fadt->acpi_disable == 0) && 
		    (fadt->s4bios_req == 0) &&
		    (fadt->pstate_cnt == 0) &&
		    (fadt->cst_cnt == 0))
			fwts_warning(fw, "FADT SMI_CMD is 0x00, system appears to not support System Management mode.");
		else
			fwts_failed(fw, "FADT SMI_CMD is 0x00, however, one or more of ACPI_ENABLE, ACPI_DISABLE, "
					"S4BIOS_REQ, PSTATE_CNT and CST_CNT are defined which means SMI_CMD should be "
					"defined otherwise SMI commands cannot be sent.");
	}
	
	if (fadt->pm_tmr_len != 4)
		fwts_failed(fw, "FADT PM_TMR_LEN is %d, should be 4.", fadt->pm_tmr_len);
	if (fadt->gpe0_blk_len & 1)
		fwts_failed(fw, "FADT GPE0_BLK_LEN is %d, should a multiple of 2.", fadt->gpe0_blk_len);
	if (fadt->gpe1_blk_len & 1)
		fwts_failed(fw, "FADT GPE1_BLK_LEN is %d, should a multiple of 2.", fadt->gpe1_blk_len);
	if (fadt->p_lvl2_lat > 100)
		fwts_warning(fw, "FADT P_LVL2_LAT is %d, a value > 100 indicates a system not to support a C2 state.", fadt->p_lvl2_lat);
	if (fadt->p_lvl3_lat > 1000)
		fwts_warning(fw, "FADT P_LVL3_LAT is %d, a value > 1000 indicates a system not to support a C3 state.", fadt->p_lvl2_lat);
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
			fwts_failed(fw, "FADT RESET_REG was %d, must be System I/O space, System Memory space "
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
		fwts_failed(fw, "RSDP: oem_id does not contain any alpha numeric characters.");

	if (rsdp->revision > 2) 
		fwts_failed(fw, "RSDP: revision is %d, expected value less than 2.", rsdp->revision);
}

static void acpi_table_check_rsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	int i;
	int n;
	fwts_acpi_table_rsdt *rsdt = (fwts_acpi_table_rsdt*)table->data;

	n = (table->length - sizeof(fwts_acpi_table_header)) / sizeof(uint32_t);
	for (i=0; i<n; i++)  {
		if (rsdt->entries[i] == 0)
			fwts_failed(fw, "RSDT Entry %d is null, should not be non-zero.", i);
	}
}

static void acpi_table_check_sbst(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_acpi_table_sbst *sbst = (fwts_acpi_table_sbst*)table->data;
	
	if (sbst->critical_energy_level > sbst->low_energy_level) 
		fwts_failed(fw, "SBST Critical Energy Level (%d) is greater than the Low Energy Level (%d).",
			sbst->critical_energy_level, sbst->low_energy_level);

	if (sbst->low_energy_level > sbst->warning_energy_level) 
		fwts_failed(fw, "SBST Low Energy Energy Level (%d) is greater than the Warning Energy Level (%d).",
			sbst->low_energy_level, sbst->warning_energy_level);

	if (sbst->warning_energy_level == 0)
		fwts_failed(fw, "SBST Warning Energy Level is zero, which is probably too low.");
}

static void acpi_table_check_xsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	int i;
	int n;
	fwts_acpi_table_xsdt *xsdt = (fwts_acpi_table_xsdt*)table->data;

	n = (table->length - sizeof(fwts_acpi_table_header)) / sizeof(uint64_t);
	for (i=0; i<n; i++)  {
		if (xsdt->entries[i] == 0)
			fwts_failed(fw, "XSDT Entry %d is null, should not be non-zero.", i);
	}
}

static void acpi_table_check_madt(fwts_framework *fw, fwts_acpi_table_info *table)
{
#if 0
	/* FIXME, to be done later */

	int i = 0;
	int n;
	int offset = 0;

	fwts_acpi_table_check_field fields[] = {
		FIELD_UINT("Local APIC Address", 	fwts_acpi_table_madt, lapic_address),
		FIELD_UINT("Flags", 			fwts_acpi_table_madt, flags),
		FIELD_BITF("  PCAT_COMPAT", 		fwts_acpi_table_madt, flags, 1, 0),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
	
	data += sizeof(fwts_acpi_table_madt);	
	length -= sizeof(fwts_acpi_table_madt);

	while (length > sizeof(fwts_acpi_madt_sub_table_header)) {
		int skip = 0;
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);
		offset += sizeof(fwts_acpi_madt_sub_table_header);
		
		fwts_log_nl(fw);
		fwts_log_info_verbatum(fw, "APIC Structure #%d:", ++i);

		switch (hdr->type) {
		case 0: {
				fwts_acpi_table_check_field fields_processor_local_apic[] = {
					FIELD_UINT("  ACPI CPU ID", fwts_acpi_madt_processor_local_apic, acpi_processor_id),
					FIELD_UINT("  APIC ID",     fwts_acpi_madt_processor_local_apic, apic_id),
					FIELD_UINT("  Flags",       fwts_acpi_madt_processor_local_apic, flags),
					FIELD_BITF("   Enabled",    fwts_acpi_madt_processor_local_apic, flags, 1, 0),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Processor Local APIC:");
				__acpi_dump_table_fields(fw, data, fields_processor_local_apic, offset);
				skip = sizeof(fwts_acpi_madt_processor_local_apic);
			}
			break;
		case 1: {
				fwts_acpi_table_check_field fields_io_apic[] = {
					FIELD_UINT("  I/O APIC ID", 	fwts_acpi_madt_io_apic, io_apic_id),
					FIELD_UINT("  I/O APIC Addr", 	fwts_acpi_madt_io_apic, io_apic_phys_address),
					FIELD_UINT("  Global IRQ Base", fwts_acpi_madt_io_apic, io_apic_phys_address),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " I/O APIC:");
				__acpi_dump_table_fields(fw, data, fields_io_apic, offset);
				skip = sizeof(fwts_acpi_madt_io_apic);
			}
			break;
		case 2: {
				fwts_acpi_table_check_field fields_madt_interrupt_override[] = {
					FIELD_UINT("  Bus", 		fwts_acpi_madt_interrupt_override, bus),
					FIELD_UINT("  Source", 		fwts_acpi_madt_interrupt_override, source),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_interrupt_override, gsi),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_interrupt_override, flags),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Interrupt Source Override:");
				__acpi_dump_table_fields(fw, data, fields_madt_interrupt_override, offset);
				skip = sizeof(fwts_acpi_madt_interrupt_override);
			}
			break;
		case 3: {
				fwts_acpi_table_check_field fields_madt_nmi[] = {
					FIELD_UINT("  Flags", 		fwts_acpi_madt_nmi, flags),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_nmi, gsi),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Non-maskable Interrupt Source (NMI):");
				__acpi_dump_table_fields(fw, data, fields_madt_nmi, offset);
				skip = sizeof(fwts_acpi_madt_nmi);
			}
			break;
		case 4: {
				fwts_acpi_table_check_field fields_madt_local_apic_nmi[] = {
					FIELD_UINT("  ACPI CPU ID", 	fwts_acpi_madt_local_apic_nmi, acpi_processor_id),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_apic_nmi, flags),
					FIELD_UINT("  Local APIC LINT", fwts_acpi_madt_local_apic_nmi, local_apic_lint),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local APIC NMI:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_apic_nmi, offset);
				skip = sizeof(fwts_acpi_madt_local_apic_nmi);
			}
			break;
		case 5: {
				fwts_acpi_table_check_field fields_madt_local_apic_addr_override[] = {
					FIELD_UINT("  Local APIC Addr", fwts_acpi_madt_local_apic_addr_override, address),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local APIC Address Override:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_apic_addr_override, offset);
				skip = sizeof(fwts_acpi_madt_local_apic_addr_override);
			}
			break;
		case 6: {
				fwts_acpi_table_check_field fields_madt_io_sapic[] = {
					FIELD_UINT("  I/O SAPIC ID", 	fwts_acpi_madt_io_sapic, io_sapic_id),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_io_sapic, gsi),
					FIELD_UINT("  I/O SAPIC Addr", 	fwts_acpi_madt_io_sapic, address),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " I/O SAPIC:");
				__acpi_dump_table_fields(fw, data, fields_madt_io_sapic, offset);
				skip = sizeof(fwts_acpi_madt_io_sapic);
			}
			break;
		case 7: {
				fwts_acpi_madt_local_sapic *local_sapic = (fwts_acpi_madt_local_sapic*)data;
				fwts_acpi_table_check_field fields_madt_local_sapic[] = {
					FIELD_UINT("  ACPI CPU ID", 	fwts_acpi_madt_local_sapic, acpi_processor_id),
					FIELD_UINT("  Local SAPIC ID", 	fwts_acpi_madt_local_sapic, local_sapic_id),
					FIELD_UINT("  Local SAPIC EID",	fwts_acpi_madt_local_sapic, local_sapic_eid),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_sapic, flags),
					FIELD_UINT("  UID Value", 	fwts_acpi_madt_local_sapic, uid_value),
					FIELD_UINT("  UID String", 	fwts_acpi_madt_local_sapic, uid_string),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local SAPIC:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_sapic, offset);
				n = strlen(local_sapic->uid_string) + 1;
				skip = (sizeof(fwts_acpi_madt_local_sapic) + n);
			}
			break;
		case 8: {
				fwts_acpi_table_check_field fields_madt_local_sapic[] = {
					FIELD_UINT("  Flags", 		fwts_acpi_madt_platform_int_source, flags),
					FIELD_UINT("  Type", 		fwts_acpi_madt_platform_int_source, type),
					FIELD_UINT("  Processor ID", 	fwts_acpi_madt_platform_int_source, processor_id),
					FIELD_UINT("  Processor EID", 	fwts_acpi_madt_platform_int_source, processor_eid),
					FIELD_UINT("  IO SAPIC Vector", fwts_acpi_madt_platform_int_source, io_sapic_vector),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_platform_int_source, gsi),
					FIELD_UINT("  PIS Flags", 	fwts_acpi_madt_platform_int_source, pis_flags),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Platform Interrupt Sources:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_sapic, offset);
				skip = (sizeof(fwts_acpi_madt_platform_int_source));
			}
			break;
		case 9: {
				fwts_acpi_table_check_field fields_madt_local_x2apic[] = {
					FIELD_UINT("  x2APIC ID", 	fwts_acpi_madt_local_x2apic, x2apic_id),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_x2apic, flags),
					FIELD_UINT("  Processor UID", 	fwts_acpi_madt_local_x2apic, processor_uid),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Processor Local x2APIC:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_x2apic, offset);
				skip = (sizeof(fwts_acpi_madt_local_x2apic));
			}
			break;
		case 10: {
				fwts_acpi_table_check_field fields_madt_local_x2apic_nmi[] = {
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_x2apic_nmi, flags),
					FIELD_UINT("  Processor UID", 	fwts_acpi_madt_local_x2apic_nmi, processor_uid),
					FIELD_UINT("  LINT#", 		fwts_acpi_madt_local_x2apic_nmi, local_x2apic_lint),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local x2APIC NMI:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_x2apic_nmi, offset);
				skip = (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			}
			break;
		default:
			fwts_log_info_verbatum(fw, " Reserved for OEM use:");
			skip = 0;
			break;
		}
		data   += skip;
		offset += skip;
		length -= skip;
	}
#endif
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
			fwts_aborted(fw, "Cannot load ACPI tables.", check_table[i].name);
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
	.headline    = acpi_table_check_headline,
	.minor_tests = acpi_table_check_tests
};

FWTS_REGISTER(acpitables, &acpi_table_check_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
