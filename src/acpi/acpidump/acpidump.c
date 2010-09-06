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

static void acpi_dump_gas(fwts_framework *fw, const char *str, const fwts_gas *gas)
{
	fwts_log_info_verbatum(fw, "%s:", str);
	fwts_log_info_verbatum(fw, "  addr_space_id:  0x%x", gas->address_space_id);
	fwts_log_info_verbatum(fw, "  reg_bit_width:  0x%x", gas->register_bit_width);
	fwts_log_info_verbatum(fw, "  reg_bit_offset: 0x%x", gas->register_bit_offset);
	fwts_log_info_verbatum(fw, "  access_width:   0x%x", gas->access_width);
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

static void acpidump_xsdt(fwts_framework *fw, uint8 *data, int length)
{
	int n;
	int i;
	uint32 *entry;

	if (length < (sizeof(fwts_acpi_table_header))) {
		fwts_log_info(fw, "Boot table too short\n");
		return;
	}
	n = (length - sizeof(fwts_acpi_table_header)) / 4;
	entry = (uint32*)(data + sizeof(fwts_acpi_table_header));

	for (i=0; i<n; i++) {
		fwts_log_info_verbatum(fw, "Description Hdr:  0x%x", *entry++);
	}
}


static int acpidump_table(fwts_framework *fw, const char *filename, const char *basename)
{
	int fd;
	uint8 *data;
	fwts_acpi_table_header hdr;
	int length;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		fwts_failed_low(fw, "Cannot read table %s.", basename);
		return FWTS_ERROR;
	}

	if ((data = fwts_acpi_table_read(fd, &length)) == NULL) {
		fwts_failed_low(fw, "Cannot read table %s.", basename);
		return FWTS_ERROR;
	}

	if (strncmp((char *)data, "RSD PTR ", 8) == 0) {
		acpidump_rsdp(fw, data, length);
		return FWTS_OK;
	}
	if (strncmp((char *)data, "FACS", 4) == 0) {
		acpidump_facs(fw, data, length);
		return FWTS_OK;
	}
	
	fwts_acpi_table_get_header(&hdr, data);
	acpidump_hdr(fw, &hdr);

	if (strncmp(hdr.signature, "BOOT", 4) == 0)
		acpidump_boot(fw, data, length);
	if (strncmp(hdr.signature, "FACP", 4) == 0)
		acpidump_fadt(fw, data, length);
	if (strncmp(hdr.signature, "HPET", 4) == 0)
		acpidump_hpet(fw, data, length);
	if (strncmp(hdr.signature, "MCFG", 4) == 0)
		acpidump_mcfg(fw, data, length);
	if (strncmp(hdr.signature, "XSDT", 4) == 0)
		acpidump_xsdt(fw, data, length);

	return FWTS_OK;
}

static int acpidump_all_tables(fwts_framework *fw, const char *path)
{
	DIR *dir;
	struct dirent *entry;

	if ((dir = opendir(path)) == NULL) 
		return FWTS_ERROR;

	while ((entry = readdir(dir)) != NULL) {
		if (strlen(entry->d_name) > 2) {
			struct stat buf;
			char pathname[PATH_MAX];

			snprintf(pathname, sizeof(pathname), "%s/%s", path, entry->d_name);
			if (stat(pathname, &buf) != 0)
				continue;
			if (S_ISDIR(buf.st_mode))
				acpidump_all_tables(fw, pathname);
			else {
				acpidump_table(fw, pathname, entry->d_name);
				fwts_log_nl(fw);
			}
		}
	}
	return FWTS_OK;
}

static int acpidump_test1(fwts_framework *fw)
{
	char *path = (fw->acpi_table_path == NULL) ? 
		FWTS_ACPI_TABLES_PATH : fw->acpi_table_path;

	acpidump_all_tables(fw, path);

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
