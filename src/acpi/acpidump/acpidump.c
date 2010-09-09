/*
 * Copyright (C) 2010 Canonical
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <unistd.h> 
#include <sys/io.h> 

#include "fwts.h"

static int acpidump_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	return FWTS_OK;
}


static char *acpidump_headline(void)
{
	return "Check ACPI table acpidump.";
}

static void acpi_dump_raw_data(fwts_framework *fw, const uint8 *data, const int offset, const int bytes)
{
        int i;
	int n = 0;
	char buffer[128];

	n = snprintf(buffer, sizeof(buffer), "  %4.4x: ", offset);

        for (i=0;i<bytes;i++)
                n += snprintf(buffer + n, sizeof(buffer) - n, "%2.2x ", data[i]);

        for (;i<16;i++)
                n += snprintf(buffer + n, sizeof(buffer) -n , "   ");

        n += snprintf(buffer + n, sizeof(buffer) -n , " ");

        for (i=0;i<bytes;i++)
		buffer[n++] = (data[i] < 32 || data[i] > 126) ? '.' : data[i];
	buffer[n] = 0;

	fwts_log_info_verbatum(fw, "%s", buffer);
}

static void acpi_dump_raw_table(fwts_framework *fw, uint8 *data, int length)
{
        int n;

	fwts_log_nl(fw);

        for (n = 0; n < length; n+=16) {
                int left = length - n;
                acpi_dump_raw_data(fw, data + n, n, left > 16 ? 16 : left);
        }
}


static void acpi_dump_gas(fwts_framework *fw, const char *str, const fwts_acpi_gas *gas)
{	
	char *txt;
	fwts_log_info_verbatum(fw, "%s:", str);

	switch (gas->address_space_id) {
	case 0x00:	
		txt = "System Memory";
		break;
	case 0x01:
		txt = "System I/O";
		break;
	case 0x02:
		txt = "PCI Configuration Space";
		break;
	case 0x03:
		txt = "Embedded Controller";
		break;
	case 0x04:
		txt = "SMBus";
		break;
	case 0x05 ... 0x7e:
	case 0x80 ... 0xbf:
		txt = "Reserved";
		break;
	case 0x7f:
		txt = "Functional Fixed Hardware";
		break;
	case 0xc0 ... 0xff:
		txt = "OEM Defined";
		break;
	default:
		txt = "Unknown";
		break;
	}
	fwts_log_info_verbatum(fw, "  addr_space_id:  0x%x (%s)", gas->address_space_id, txt);
	fwts_log_info_verbatum(fw, "  reg_bit_width:  0x%x", gas->register_bit_width);
	fwts_log_info_verbatum(fw, "  reg_bit_offset: 0x%x", gas->register_bit_offset);
	switch (gas->access_width) {
	case 0x00:
		txt = "Undefined (legacy reasons)";
		break;
	case 0x01:
		txt = "Byte Access";
		break;
	case 0x02:
		txt = "Word Access";
		break;
	case 0x03:
		txt = "DWord Access";
		break;
	case 0x04:
		txt = "QWord Access";
		break;
	default:
		txt = "Unknown";
		break;
	}
	fwts_log_info_verbatum(fw, "  access_width:   0x%x (%s)", gas->access_width, txt);
	fwts_log_info_verbatum(fw, "  address:        0x%llx", gas->address);
}

static void acpidump_hdr(fwts_framework *fw, fwts_acpi_table_header *hdr)
{
	fwts_log_info_verbatum(fw, "Signature:        \"%4.4s\"", hdr->signature);
	fwts_log_info_verbatum(fw, "Length:           0x%lx (%lu)", hdr->length, hdr->length);
	fwts_log_info_verbatum(fw, "Revision:         0x%x (%d)", hdr->revision, hdr->revision);
	fwts_log_info_verbatum(fw, "Checksum:         0x%x (%d)", hdr->checksum, hdr->checksum);
	fwts_log_info_verbatum(fw, "OEM ID:           \"%6.6s\"", hdr->oem_id);
	fwts_log_info_verbatum(fw, "OEM Table ID:     \"%6.6s\"", hdr->oem_tbl_id);
	fwts_log_info_verbatum(fw, "OEM Revision:     0x%lx (%lu)", hdr->oem_revision, hdr->oem_revision);
	fwts_log_info_verbatum(fw, "Creator ID:       \"%4.4s\"", hdr->creator_id);
	fwts_log_info_verbatum(fw, "Creator Revision: 0x%lx (%lu)", hdr->creator_revision, hdr->creator_revision);
}

static void acpidump_boot(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_boot *boot = (fwts_acpi_table_boot*)data;
	uint8 cmos_data;

	if (length < (sizeof(fwts_acpi_table_header) + 4)) {
		fwts_log_info(fw, "Boot table too short\n");
		return;
	}

	fwts_log_info_verbatum(fw, "CMOS offset: 0x%lx", boot->cmos_index);

	cmos_data = fwts_cmos_read(boot->cmos_index);

	fwts_log_info_verbatum(fw, "Boot Register: 0x%x", cmos_data);
	fwts_log_info_verbatum(fw, "  PNP-OS:   %x", (cmos_data & FTWS_BOOT_REGISTER_PNPOS) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Booting:  %x", (cmos_data & FWTS_BOOT_REGISTER_BOOTING) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Diag:     %x", (cmos_data & FWTS_BOOT_REGISTER_DIAG) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Suppress: %x", (cmos_data & FWTS_BOOT_REGISTER_SUPPRESS_BOOT_DISPLAY) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Parity:   %x", (cmos_data & FWTS_BOOT_REGISTER_PARITY) ? 1 : 0);
}

static void acpidump_bert(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_bert *bert = (fwts_acpi_table_bert*)data;
	static char *error_severity[] = {
		"Correctable",
		"Fatal",
		"Corrected",
		"None",
		"Uknown"
	};

	int i;
	int n = length - sizeof(fwts_acpi_table_bert);

	fwts_log_info_verbatum(fw, "Region Length:    0x%lx", bert->boot_error_region_length);
	fwts_log_info_verbatum(fw, "Region Addr:      0x%llx", bert->boot_error_region);
	fwts_log_info_verbatum(fw, "Boot Status:      0x%lx", bert->boot_status);
	fwts_log_info_verbatum(fw, "Raw Data Offset:  0x%lx", bert->raw_data_offset);
	fwts_log_info_verbatum(fw, "Raw Data Length:  0x%lx", bert->raw_data_length);
	fwts_log_info_verbatum(fw, "Error Severity:   0x%lx (%s)", bert->error_severity, 
		error_severity[bert->error_severity > 3 ? 4 : bert->error_severity]);
	fwts_log_info_verbatum(fw, "Generic Error Data:");
	for (i=0; i<n; i+= 16) {
		int left = length - n;
		acpi_dump_raw_data(fw, &bert->generic_error_data[i], i, left > 16 ? 16 : left);
	}
}

static void acpidump_cpep(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_cpep *cpep = (fwts_acpi_table_cpep*)data;
	int i;
	int n = (length - sizeof(fwts_acpi_table_bert)) / sizeof(fwts_acpi_cpep_processor_info);

	for (i=0; i<n; i++) {
		fwts_log_info_verbatum(fw, "CPEP #%d\n", i);
		fwts_log_info_verbatum(fw, "  Type:           0x%x", cpep->cpep_info[i].type);
		fwts_log_info_verbatum(fw, "  Length:         0x%x", cpep->cpep_info[i].length);
		fwts_log_info_verbatum(fw, "  Processor ID:   0x%x", cpep->cpep_info[i].processor_id);
		fwts_log_info_verbatum(fw, "  Processor EID:  0x%x", cpep->cpep_info[i].processor_eid);
		fwts_log_info_verbatum(fw, "  Poll Interval   0x%lx ms", cpep->cpep_info[i].polling_interval);
	}
}

static void acpidump_ecdt(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_ecdt *ecdt = (fwts_acpi_table_ecdt*)data;
	int i;
	int n = length - sizeof(fwts_acpi_table_ecdt);

	acpi_dump_gas(fw, "EC_CONTROL", &ecdt->ec_control);
	acpi_dump_gas(fw, "EC_DATA", &ecdt->ec_data);
	fwts_log_info_verbatum(fw, "UID:              0x%lx", ecdt->uid);
	fwts_log_info_verbatum(fw, "GPE_BIT:          0x%x", ecdt->uid);
	fwts_log_info_verbatum(fw, "EC_ID:");

	for (i=0; i<n; i+= 16) {
		int left = length - n;
		acpi_dump_raw_data(fw, &ecdt->ec_id[i], i, left > 16 ? 16 : left);
	}
}

static void acpidump_amlcode(fwts_framework *fw, uint8 *data, int length)
{
	fwts_log_info_verbatum(fw, "Contains 0x%x byes of AML byte code", length-sizeof(fwts_acpi_table_header));
}

static void acpidump_facs(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_facs *facs = (fwts_acpi_table_facs*)data;

	fwts_log_info_verbatum(fw, "Signature:        \"%4.4s\"", facs->signature);
	fwts_log_info_verbatum(fw, "Length:           0x%lx (%lu)", facs->length, facs->length);
	fwts_log_info_verbatum(fw, "H/W Signature:    0x%lx", facs->hardware_signature);
	fwts_log_info_verbatum(fw, "Waking Vector:    0x%lx", facs->firmware_waking_vector);
	fwts_log_info_verbatum(fw, "Global Lock       0x%lx", facs->global_lock);
	fwts_log_info_verbatum(fw, "Flags             0x%lx", facs->flags);
	fwts_log_info_verbatum(fw, "X Waking Vector:  0x%llx", facs->x_firmware_waking_vector);
	fwts_log_info_verbatum(fw, "Version:          0x%x (%u)", facs->version, facs->version);
	fwts_log_info_verbatum(fw, "OSPM Flags        0x%lx", facs->ospm_flags);
}

static void acpidump_hpet(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_hpet *hpet = (fwts_acpi_table_hpet*)data;

	static char *prot_attr[] = {
		"No guarantee for page protection",
		"4K page protected",
		"64K page protected",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved"
	};

	fwts_log_info_verbatum(fw, "Event Timer ID:   0x%lx", hpet->event_timer_block_id);
	fwts_log_info_verbatum(fw, "  Hardware Rev:   0x%x", hpet->event_timer_block_id & 0xff);
	fwts_log_info_verbatum(fw, "  Num Comparitors:0x%x", (hpet->event_timer_block_id >> 8) & 0x31);
	fwts_log_info_verbatum(fw, "  Count Size Cap: 0x%x", (hpet->event_timer_block_id >> 13) & 0x1);
	fwts_log_info_verbatum(fw, "  IRQ Routing Cap:0x%x", (hpet->event_timer_block_id >> 15) & 0x1);
	fwts_log_info_verbatum(fw, "  PCI Vendor ID:  0x%x", (hpet->event_timer_block_id >> 16) & 0xffff);
	acpi_dump_gas(fw, "Base Address", &hpet->base_address);
	fwts_log_info_verbatum(fw, "HPET Number:      0x%lx", hpet->hpet_number);
	fwts_log_info_verbatum(fw, "Main Counter Min: 0x%lx", hpet->main_counter_minimum);
	fwts_log_info_verbatum(fw, "Page Prot Attr:   0x%x", hpet->page_prot_and_oem_attribute);
	fwts_log_info_verbatum(fw, "  Page Protection:0x%x (%s)", hpet->page_prot_and_oem_attribute & 0xf,
		prot_attr[hpet->page_prot_and_oem_attribute & 0xf]);
	fwts_log_info_verbatum(fw, "  OEM Attr:      :0x%x", hpet->page_prot_and_oem_attribute >> 4);
	
}

static void acpidump_fadt(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_fadt *fadt = (fwts_acpi_table_fadt*)data;

	fwts_log_info_verbatum(fw, "Firmware Conrtrol:0x%lx", fadt->firmware_control);
	fwts_log_info_verbatum(fw, "DSDT:             0x%lx", fadt->dsdt);
	fwts_log_info_verbatum(fw, "Pref. PM Profile: 0x%x (%u) (%s)", fadt->preferred_pm_profile, fadt->preferred_pm_profile,
			FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(fadt->preferred_pm_profile) );
	fwts_log_info_verbatum(fw, "SCI_INT:          IRQ 0x%x (%u)", fadt->sci_int, fadt->sci_int);
	fwts_log_info_verbatum(fw, "SMI_CMD:          Port 0x%x (%u)", fadt->smi_cmd, fadt->smi_cmd);
	fwts_log_info_verbatum(fw, "ACPI_ENABLE:      0x%x (%u)", fadt->acpi_enable, fadt->acpi_enable);
	fwts_log_info_verbatum(fw, "ACPI_DISABLE:     0x%x (%u)", fadt->acpi_disable, fadt->acpi_disable);
	fwts_log_info_verbatum(fw, "S4BIOS_REQ:       0x%x (%u)", fadt->s4bios_req, fadt->s4bios_req);
	fwts_log_info_verbatum(fw, "PSTATE_CNT:       0x%x (%u)", fadt->pstate_cnt, fadt->pstate_cnt);
	fwts_log_info_verbatum(fw, "PM1a_EVT_BLK:     0x%x (%u)", fadt->pm1a_evt_blk, fadt->pm1a_evt_blk);
	fwts_log_info_verbatum(fw, "PM1b_EVT_BLK:     0x%x (%u)", fadt->pm1b_evt_blk, fadt->pm1b_evt_blk);
	fwts_log_info_verbatum(fw, "PM1a_CNT_BLK:     0x%x (%u)", fadt->pm1a_cnt_blk, fadt->pm1a_cnt_blk);
	fwts_log_info_verbatum(fw, "PM1b_CNT_BLK:     0x%x (%u)", fadt->pm1b_cnt_blk, fadt->pm1b_cnt_blk);
	fwts_log_info_verbatum(fw, "PM2_CNT_BLK:      0x%x (%u)", fadt->pm2_cnt_blk, fadt->pm2_cnt_blk);
	fwts_log_info_verbatum(fw, "PM_TMR_BLK:       0x%x (%u)", fadt->pm_tmr_blk, fadt->pm_tmr_blk);
	fwts_log_info_verbatum(fw, "GPE0_BLK:         0x%x (%u)", fadt->gpe0_blk, fadt->gpe0_blk);
	fwts_log_info_verbatum(fw, "GPE1_BLK:         0x%x (%u)", fadt->gpe1_blk, fadt->gpe1_blk);
	fwts_log_info_verbatum(fw, "PM1_EVT_LEN:      0x%x (%u)", fadt->pm1_evt_len, fadt->pm1_evt_len);
	fwts_log_info_verbatum(fw, "PM1_CNT_LEN:      0x%x (%u)", fadt->pm1_cnt_len, fadt->pm1_cnt_len);
	fwts_log_info_verbatum(fw, "PM2_CNT_LEN:      0x%x (%u)", fadt->pm2_cnt_len, fadt->pm2_cnt_len);
	fwts_log_info_verbatum(fw, "GPE0_BLK_LEN:     0x%x (%u)", fadt->gpe0_blk_len, fadt->gpe0_blk_len);
	fwts_log_info_verbatum(fw, "GPE1_BLK_LEN:     0x%x (%u)", fadt->gpe1_blk_len, fadt->gpe1_blk_len);
	fwts_log_info_verbatum(fw, "CST_CNT:          0x%x (%u)", fadt->cst_cnt, fadt->cst_cnt);
	fwts_log_info_verbatum(fw, "P_LVL2_LAT:       0x%x (%u)", fadt->p_lvl2_lat, fadt->p_lvl2_lat);
	fwts_log_info_verbatum(fw, "P_LVL3_LAT:       0x%x (%u)", fadt->p_lvl3_lat, fadt->p_lvl3_lat);
	fwts_log_info_verbatum(fw, "FLUSH_SIZE:       0x%x (%u)", fadt->flush_size, fadt->flush_size);
	fwts_log_info_verbatum(fw, "FLUSH_STRIDE:     0x%x (%u)", fadt->flush_stride, fadt->flush_stride);
	fwts_log_info_verbatum(fw, "DUTY_OFFSET:      0x%x (%u)", fadt->duty_offset, fadt->duty_offset);
	fwts_log_info_verbatum(fw, "DUTY_WIDTH:       0x%x (%u)", fadt->duty_width, fadt->duty_width);
	fwts_log_info_verbatum(fw, "DAY_ALRM:         0x%x (%u)", fadt->day_alrm, fadt->day_alrm);
	fwts_log_info_verbatum(fw, "MON_ALRM:         0x%x (%u)", fadt->mon_alrm, fadt->mon_alrm);
	fwts_log_info_verbatum(fw, "CENTURY:          0x%x (%u)", fadt->century, fadt->century);
	fwts_log_info_verbatum(fw, "IAPC_BOOT_ARCH:   0x%x (%u)", fadt->iapc_boot_arch, fadt->iapc_boot_arch);
	fwts_log_info_verbatum(fw, "Flags:            0x%x (%u)", fadt->flags, fadt->flags);
	fwts_log_info_verbatum(fw, "  WBINVD:                  0x%1.1x", (fadt->flags >> 0) & 1);
	fwts_log_info_verbatum(fw, "  WBINVD_FLUSH:            0x%1.1x", (fadt->flags >> 1) & 1);
	fwts_log_info_verbatum(fw, "  PROC_C1:                 0x%1.1x", (fadt->flags >> 2) & 1);
	fwts_log_info_verbatum(fw, "  P_LVL2_UP:               0x%1.1x", (fadt->flags >> 3) & 1);
	fwts_log_info_verbatum(fw, "  PWR_BUTTON:              0x%1.1x", (fadt->flags >> 4) & 1);
	fwts_log_info_verbatum(fw, "  SLP_BUTTON:              0x%1.1x", (fadt->flags >> 5) & 1);
	fwts_log_info_verbatum(fw, "  FIX_RTC:                 0x%1.1x", (fadt->flags >> 6) & 1);
	fwts_log_info_verbatum(fw, "  RTC_S4:                  0x%1.1x", (fadt->flags >> 7) & 1);
	fwts_log_info_verbatum(fw, "  TMR_VAL_EXT:             0x%1.1x", (fadt->flags >> 8) & 1);
	fwts_log_info_verbatum(fw, "  DCK_CAP:                 0x%1.1x", (fadt->flags >> 9) & 1);
	fwts_log_info_verbatum(fw, "  RESET_REG_SUP:           0x%1.1x", (fadt->flags >> 10) & 1);
	fwts_log_info_verbatum(fw, "  SEALED_CASE:             0x%1.1x", (fadt->flags >> 11) & 1);
	fwts_log_info_verbatum(fw, "  HEADLESS:                0x%1.1x", (fadt->flags >> 12) & 1);
	fwts_log_info_verbatum(fw, "  CPU_WS_SLP:              0x%1.1x", (fadt->flags >> 13) & 1);
	fwts_log_info_verbatum(fw, "  PCI_EXP_WAK:             0x%1.1x", (fadt->flags >> 14) & 1);
	fwts_log_info_verbatum(fw, "  USE_PLATFORM_CLOCK:      0x%1.1x", (fadt->flags >> 15) & 1);
	fwts_log_info_verbatum(fw, "  S4_RTC_STS_VALID:        0x%1.1x", (fadt->flags >> 16) & 1);
	fwts_log_info_verbatum(fw, "  REMOTE_POWER_ON_CAPABLE: 0x%1.1x", (fadt->flags >> 17) & 1);
	fwts_log_info_verbatum(fw, "  FORCE_APIC_CLUSTER_MODEL:0x%1.1x", (fadt->flags >> 18) & 1);
	fwts_log_info_verbatum(fw, "  FORCE_APIC_PYS_DEST_MODE:0x%1.1x", (fadt->flags >> 19) & 1);
	fwts_log_info_verbatum(fw, "  RESERVED:                0x%2.2x", (fadt->flags >> 20));
	fwts_log_info_verbatum(fw, "RESET_REG       : 0x%x (%u)", fadt->reset_reg, fadt->reset_reg);
	fwts_log_info_verbatum(fw, "RESET_VALUE     : 0x%x (%u)", fadt->reset_value, fadt->reset_value);
	fwts_log_info_verbatum(fw, "X_FIRMWARE_CTRL : 0x%llx (%llu)", fadt->x_firmware_ctrl, fadt->x_firmware_ctrl);
	fwts_log_info_verbatum(fw, "X_DSDT          : 0x%llx (%llu)", fadt->x_dsdt, fadt->x_dsdt);
	acpi_dump_gas(fw, "X_PM1a_EVT_BLK", &fadt->x_pm1a_evt_blk);
	acpi_dump_gas(fw, "X_PM1b_EVT_BLK", &fadt->x_pm1b_evt_blk);
	acpi_dump_gas(fw, "X_PM1a_CNT_BLK", &fadt->x_pm1a_cnt_blk);
	acpi_dump_gas(fw, "X_PM1b_CNT_BLK", &fadt->x_pm1b_cnt_blk);
	acpi_dump_gas(fw, "X_PM2_CNT_BLK", &fadt->x_pm2_cnt_blk);
	acpi_dump_gas(fw, "X_PM_TMR_BLK", &fadt->x_pm_tmr_blk);
	acpi_dump_gas(fw, "X_GPE0_BLK", &fadt->x_gpe0_blk);
	acpi_dump_gas(fw, "X_GPE1_BLK", &fadt->x_gpe1_blk);
}

static void acpidump_rsdp(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_rsdp *rsdp = (fwts_acpi_table_rsdp*)data;

	fwts_log_info_verbatum(fw, "Signature:        \"%8.8s\"", rsdp->signature);
	fwts_log_info_verbatum(fw, "Checksum:         0x%x (%d)", rsdp->checksum, rsdp->checksum);
	fwts_log_info_verbatum(fw, "OEM ID:           \"%6.6s\"", rsdp->oem_id);
	fwts_log_info_verbatum(fw, "Revision:         0x%lx (%lu)", rsdp->revision, rsdp->revision);
	fwts_log_info_verbatum(fw, "RsdtAddress:      0x%lx", rsdp->rsdt_address);
	fwts_log_info_verbatum(fw, "Length:           0x%lx (%lu)", rsdp->length, rsdp->length);
	fwts_log_info_verbatum(fw, "XsdtAddress:      0x%lx", rsdp->xsdt_address);
	fwts_log_info_verbatum(fw, "Extended Checksum:0x%x (%d)", rsdp->extended_checksum, rsdp->extended_checksum);
	fwts_log_info_verbatum(fw, "Reserved:         0x%2.2x 0x%2.2x 0x%2.2x", 
		rsdp->reserved[0], rsdp->reserved[1], rsdp->reserved[2]);
}

static void acpidump_rsdt(fwts_framework *fw, uint8 *data, int length)
{
	int i;
	int n;
	fwts_acpi_table_rsdt *rsdt = (fwts_acpi_table_rsdt*)data;

	n = (length - sizeof(fwts_acpi_table_header)) / sizeof(uint32);
	for (i=0; i<n; i++)  {
		fwts_acpi_table_info *table = fwts_acpi_find_table_by_addr((uint64)rsdt->entries[i]);
		char *name = table == NULL ? "unknown" : table->name;
		fwts_log_info_verbatum(fw, "Entry %2.2d          0x%8.8lx (%s)", i, rsdt->entries[i], name);
	}
}

static void acpidump_sbst(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_sbst *sbst = (fwts_acpi_table_sbst*)data;

	fwts_log_info_verbatum(fw, "Warn. Energy Lvl: 0x%lx", sbst->warning_energy_level);
	fwts_log_info_verbatum(fw, "Low  Energy Lvl:  0x%lx", sbst->low_energy_level);
	fwts_log_info_verbatum(fw, "Crit. Energy Lvl: 0x%lx", sbst->critical_energy_level);
}

static void acpidump_xsdt(fwts_framework *fw, uint8 *data, int length)
{
	int i;
	int n;
	fwts_acpi_table_xsdt *xsdt = (fwts_acpi_table_xsdt*)data;

	n = (length - sizeof(fwts_acpi_table_header)) / sizeof(uint64);
	for (i=0; i<n; i++)  {
		fwts_acpi_table_info *table = fwts_acpi_find_table_by_addr(xsdt->entries[i]);
		char *name = table == NULL ? "unknown" : table->name;
		fwts_log_info_verbatum(fw, "Entry %2.2d          0x%16.16llx (%s)", i, xsdt->entries[i], name);
	}
}

static void acpidump_madt(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_madt *madt = (fwts_acpi_table_madt*)data;
	int i = 0;
	int n;

	fwts_log_info_verbatum(fw, "Local APIC Addr:  0x%lx", madt->lapic_address);
	fwts_log_info_verbatum(fw, "Flags:            0x%lx", madt->flags);
	fwts_log_info_verbatum(fw, "  PCAT_COMPAT     0x%1.1x", madt->flags & 1);
	
	data += sizeof(fwts_acpi_table_madt);	
	length -= sizeof(fwts_acpi_table_madt);

	while (length > sizeof(fwts_acpi_madt_sub_table_header)) {
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;
		fwts_acpi_madt_processor_local_apic *proc_local_apic;
		fwts_acpi_madt_io_apic              *io_apic;
		fwts_acpi_madt_interrupt_override   *interrupt_override;
		fwts_acpi_madt_nmi                  *nmi;
		fwts_acpi_madt_local_apic_nmi       *local_apic_nmi;
		fwts_acpi_madt_local_apic_addr_override *local_apic_addr_override;
		fwts_acpi_madt_io_sapic 	    *io_sapic;
		fwts_acpi_madt_local_sapic	    *local_sapic;
		fwts_acpi_madt_platform_int_source  *platform_int_source;
		fwts_acpi_madt_local_x2apic         *local_x2apic;
		fwts_acpi_madt_local_x2apic_nmi	    *local_x2apic_nmi;
		

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);
		
		fwts_log_info_verbatum(fw, "APIC Structure #%d:", ++i);
		/*
		fwts_log_info_verbatum(fw, " Type:            0x%2.2x", hdr->type);
		fwts_log_info_verbatum(fw, " Size:            0x%2.2x", hdr->length);
		*/
		switch (hdr->type) {
		case 0: 
			proc_local_apic = (fwts_acpi_madt_processor_local_apic*)data;
			fwts_log_info_verbatum(fw, " Processor Local APIC:");
			fwts_log_info_verbatum(fw, "  ACPI CPU ID:    0x%2.2x", proc_local_apic->acpi_processor_id);
			fwts_log_info_verbatum(fw, "  APIC ID:        0x%2.2x", proc_local_apic->apic_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", proc_local_apic->flags);
			fwts_log_info_verbatum(fw, "   Enabled:       0x%x", proc_local_apic->flags & 1);
			data += sizeof(fwts_acpi_madt_processor_local_apic);
			length -= sizeof(fwts_acpi_madt_processor_local_apic);
			break;
		case 1:
			io_apic = (fwts_acpi_madt_io_apic*)data;
			fwts_log_info_verbatum(fw, " I/O APIC:");
			fwts_log_info_verbatum(fw, "  I/O APIC ID:    0x%2.2x", io_apic->io_apic_id);
			fwts_log_info_verbatum(fw, "  I/O APIC Addr:  0x%lx", io_apic->io_apic_phys_address);
			fwts_log_info_verbatum(fw, "  Global IRQ Base:0x%lx", io_apic->io_apic_phys_address);
			data += sizeof(fwts_acpi_madt_io_apic);
			length -= sizeof(fwts_acpi_madt_io_apic);
			break;
		case 2:
			interrupt_override = (fwts_acpi_madt_interrupt_override*)data;
			fwts_log_info_verbatum(fw, " Interrupt Source Override:");
			fwts_log_info_verbatum(fw, "  Bus:            0x%2.2x", interrupt_override->bus);
			fwts_log_info_verbatum(fw, "  Source:         0x%2.2x", interrupt_override->source);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", interrupt_override->gsi);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", interrupt_override->flags);
			break;
		case 3:
			nmi = (fwts_acpi_madt_nmi*)data;
			fwts_log_info_verbatum(fw, " Non-maskable Interrupt Source (NMI):");
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", nmi->flags);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", nmi->gsi);
			data += sizeof(fwts_acpi_madt_nmi);
			length -= sizeof(fwts_acpi_madt_nmi);
			break;
		case 4:
			local_apic_nmi = (fwts_acpi_madt_local_apic_nmi *)data;
			fwts_log_info_verbatum(fw, " Local APIC NMI:");
			fwts_log_info_verbatum(fw, "  ACPI CPU ID:    0x%x", local_apic_nmi->acpi_processor_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", local_apic_nmi->flags);
			fwts_log_info_verbatum(fw, "  Local APIC LINT:0x%x", local_apic_nmi->local_apic_lint);
	
			data += sizeof(fwts_acpi_madt_local_apic_nmi);
			length -= sizeof(fwts_acpi_madt_local_apic_nmi);
			break;
		case 5:
			local_apic_addr_override = (fwts_acpi_madt_local_apic_addr_override*)data;
			fwts_log_info_verbatum(fw, " Local APIC Address Override:");
			fwts_log_info_verbatum(fw, "  Local APIC Addr:0x%x", local_apic_addr_override->address);
			data += sizeof(fwts_acpi_madt_local_apic_addr_override);
			length -= sizeof(fwts_acpi_madt_local_apic_addr_override);
			
			break;
		case 6:
			io_sapic = (fwts_acpi_madt_io_sapic *)data;
			fwts_log_info_verbatum(fw, " I/O SAPIC:");
			fwts_log_info_verbatum(fw, "  I/O SAPIC ID:   0x%x", io_sapic->io_sapic_id);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", io_sapic->gsi);
			fwts_log_info_verbatum(fw, "  I/O SAPIC Addr: 0x%llx", io_sapic->address);
			data += sizeof(fwts_acpi_madt_io_sapic);
			length -= sizeof(fwts_acpi_madt_io_sapic);
			break;
		case 7:
			local_sapic = (fwts_acpi_madt_local_sapic*)data;
			fwts_log_info_verbatum(fw, " Local SAPIC:");
			fwts_log_info_verbatum(fw, "  ACPI CPU ID:    0x%x", local_sapic->acpi_processor_id);
			fwts_log_info_verbatum(fw, "  Local SAPIC ID: 0x%x", local_sapic->local_sapic_id);
			fwts_log_info_verbatum(fw, "  Local SAPIC EID:0x%x", local_sapic->local_sapic_eid);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", local_sapic->flags);
			fwts_log_info_verbatum(fw, "  UID Value:      0x%x", local_sapic->uid_value);
		
			fwts_log_info_verbatum(fw, "  UID String:     %s", local_sapic->uid_string);
			
			n = strlen(local_sapic->uid_string) + 1;
			data += (sizeof(fwts_acpi_madt_io_sapic) + n);
			length -= (sizeof(fwts_acpi_madt_io_sapic) + n);
			break;
		case 8:
			platform_int_source = (fwts_acpi_madt_platform_int_source*)data;
			fwts_log_info_verbatum(fw, " Platform Interrupt Sources:");
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", platform_int_source->flags);
			fwts_log_info_verbatum(fw, "  Type:           0x%x", platform_int_source->type);
			fwts_log_info_verbatum(fw, "  Processor ID:   0x%x", platform_int_source->processor_id);
			fwts_log_info_verbatum(fw, "  Processor EID:  0x%x", platform_int_source->processor_eid);
			fwts_log_info_verbatum(fw, "  IO SAPIC Vector:0x%lx", platform_int_source->io_sapic_vector);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", platform_int_source->gsi);
			fwts_log_info_verbatum(fw, "  PIS Flags:      0x%x", platform_int_source->pis_flags);
			data += (sizeof(fwts_acpi_madt_platform_int_source));
			length += (sizeof(fwts_acpi_madt_platform_int_source));
			break;
		case 9:
			local_x2apic = (fwts_acpi_madt_local_x2apic*)data;
			fwts_log_info_verbatum(fw, " Processor Local x2APIC:");
			fwts_log_info_verbatum(fw, "  x2APIC ID       0x%lx", local_x2apic->x2apic_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", local_x2apic->flags);
			fwts_log_info_verbatum(fw, "  Processor UID:  0x%lx", local_x2apic->processor_uid);
			data += (sizeof(fwts_acpi_madt_local_x2apic));
			length += (sizeof(fwts_acpi_madt_local_x2apic));
			break;
		case 10:
			local_x2apic_nmi = (fwts_acpi_madt_local_x2apic_nmi*)data;
			fwts_log_info_verbatum(fw, " Local x2APIC NMI:");
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", local_x2apic_nmi->flags);
			fwts_log_info_verbatum(fw, "  Processor UID:  0x%lx", local_x2apic_nmi->processor_uid);
			fwts_log_info_verbatum(fw, "  LINT#           :0x%lx", local_x2apic_nmi->local_x2apic_lint);
			data += (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			length += (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			break;
		default:
			fwts_log_info_verbatum(fw, " Reserved for OEM use:");
			break;
		}
	}
}

static void acpidump_mcfg(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_mcfg *mcfg = (fwts_acpi_table_mcfg*)data;
	int n;
	int i;

	fwts_log_info_verbatum(fw, "Base Address:     0x%lx", mcfg->base_address);
	fwts_log_info_verbatum(fw, "Base Reserved:    0x%lx", mcfg->base_reserved);

	n = length - sizeof(fwts_acpi_table_mcfg);

	for (i=0; i<n/sizeof(fwts_acpi_mcfg_configuration); i++) {
		fwts_log_info_verbatum(fw, "Configuration #%d", i+1);
		fwts_log_info_verbatum(fw, "  Base Address:   0x%lx", mcfg[i].configuration->base_address);
		fwts_log_info_verbatum(fw, "  Base Reserved:  0x%lx", mcfg[i].configuration->base_reserved);
		fwts_log_info_verbatum(fw, "  PCI Seg Grp Num:0x%lx", mcfg[i].configuration->pci_segment_group_number);
		fwts_log_info_verbatum(fw, "  Start Bus Num:  0x%lx", mcfg[i].configuration->start_bus_number);
		fwts_log_info_verbatum(fw, "  End Bus Num:    0x%lx", mcfg[i].configuration->end_bus_number);
	}
}

typedef struct {
	char *name;
	void (*func)(fwts_framework *fw, uint8 *data, int length);
	int  standard_header;
} acpidump_table_vec;


/* To be implemented */
#define acpidump_einj		acpi_dump_raw_table
#define acpidump_erst		acpi_dump_raw_table
#define acpidump_hest		acpi_dump_raw_table
#define acpidump_msct		acpi_dump_raw_table
#define acpidump_psdt		acpi_dump_raw_table
#define acpidump_slit		acpi_dump_raw_table
#define acpidump_srat		acpi_dump_raw_table


acpidump_table_vec table_vec[] = {
	{ "APIC", 	acpidump_madt, 	1 },
	{ "BERT", 	acpidump_bert, 	1 },
	{ "BOOT", 	acpidump_boot, 	1 },
	{ "CPEP", 	acpidump_cpep, 	1 },
	{ "DSDT", 	acpidump_amlcode, 1 },
	{ "ECDT", 	acpidump_ecdt, 	1 },
	{ "EINJ", 	acpidump_einj, 	1 },
	{ "ERST", 	acpidump_erst, 	1 },
	{ "FACP", 	acpidump_fadt, 	1 },
	{ "FACS", 	acpidump_facs, 	0 },
	{ "HEST", 	acpidump_hest, 	1 },
	{ "HPET", 	acpidump_hpet, 	1 },
	{ "MCFG", 	acpidump_mcfg, 	1 },
	{ "MSCT", 	acpidump_msct, 	1 },
	{ "PSDT", 	acpidump_psdt, 	1 },
	{ "RSDT", 	acpidump_rsdt, 	1 },
	{ "RSD PTR ", 	acpidump_rsdp, 	0 },
	{ "SBST", 	acpidump_sbst,  1 },
	{ "SSDT", 	acpidump_amlcode, 1 },
	{ "SLIT", 	acpidump_slit,  1 },
	{ "SRAT", 	acpidump_srat,  1 },
	{ "XSDT", 	acpidump_xsdt, 	1 },
	{ NULL,		NULL,		0 },
};

static int acpidump_table(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8 *data;
	fwts_acpi_table_header hdr;
	int length;
	int i;

	data = table->data;
	length = table->length;

	for (i=0; table_vec[i].name != NULL; i++) {
		if (strncmp(table_vec[i].name, (char*)data, strlen(table_vec[i].name)) == 0) {
			if (table_vec[i].standard_header) {
				fwts_acpi_table_get_header(&hdr, data);
				acpidump_hdr(fw, &hdr);
			}
			table_vec[i].func(fw, data, length);
			return FWTS_OK;
		}
	}

	/* Cannot find, assume standard table header */
	fwts_acpi_table_get_header(&hdr, data);
	acpidump_hdr(fw, &hdr);
	acpi_dump_raw_table(fw, data, length);

	return FWTS_OK;
}

static int acpidump_test1(fwts_framework *fw)
{
	int i;

	fwts_acpi_table_info *table;

	for (i=0; (table = fwts_acpi_get_table(i)) !=NULL; i++) {
		fwts_log_info_verbatum(fw, "%s @ %16.16llx", table->name, (uint64)table->addr);
		fwts_log_info_verbatum(fw, "-----------------------");
		acpidump_table(fw, table);
		fwts_log_nl(fw);
	}

	return FWTS_OK;
}


static fwts_framework_tests acpidump_tests[] = {
	acpidump_test1,
	NULL
};

static fwts_framework_ops acpidump_ops = {
	acpidump_headline,
	acpidump_init,
	NULL,
	acpidump_tests
};

FWTS_REGISTER(acpidump, &acpidump_ops, FWTS_TEST_ANYTIME, FWTS_BATCH_EXPERIMENTAL);
