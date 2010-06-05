/*
 * Copyright (C) 2010 Canonical
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h> 
#include <unistd.h>
#include <string.h>

#include "framework.h"
#include "acpi.h"

unsigned char *fadt_table;
unsigned long fadt_size;

/*
 *  From ACPI Spec, section 5.2.9 Fixed ACPI Description Field
 */
typedef struct {
	fwts_acpi_table_header	header;	
	u32			firmware_control;
	u32			dsdt;
	u8			reserved;
	u8			preferred_pm_profile;
	u16			sci_int;
	u16			smi_cmd;
	u8			acpi_enable;
	u8			acpi_disable;
	u8			s4bios_req;
	u8			pstate_cnt;
	u32			pm1a_evt_blk;
	u32			pm1b_evt_blk;
	u32			pm1a_cnt_blk;
	u32			pm1b_cnt_blk;
	u32			pm2_cnt_blk;
	u32			pm_tmr_blk;
	u32			gpe0_blk;
	u32			gpe1_blk;
	u8			pm1_evt_len;
	u8			pm1_cnt_len;
	u8			pm2_cnt_len;
	u8			pm_tmr_len;
	u8			gpe0_blk_len;
	u8			gpe1_blk_len;
	u8			cst_cnt;
	u16			p_lvl2_lat;
	u16			p_lvl3_lat;
	u16			flush_size;
	u16			flush_stride;
	u8			duty_offset;
	u8			duty_width;
	u8			day_alrm;
	u8			mon_alrm;
	u8			century;
	u16			iapc_boot_arch;
	u8			reserved1;
	u32			flags;
	fwts_gas		reset_reg;
	u8			reset_value;
	u8			reserved2;
	u8			reserved3;
	u8			reserved4;
	u64			x_firmware_control;
	u64			x_dsdt;
	fwts_gas		x_pm1a_evt_blk;
	fwts_gas		x_pm1b_evt_blk;
	fwts_gas		x_pm1a_cnt_blk;
	fwts_gas		x_pm1b_cnt_blk;
	fwts_gas		x_pm2_cnt_blk;
	fwts_gas		x_pm_tmr_blk;
	fwts_gas		x_gpe0_blk;
	fwts_gas		x_gpe1_blk;
} fwts_fadt_version_2;


static void fadt_get_header(unsigned char *fadt_data, int size, fwts_fadt_version_2 *hdr)
{
	u8 data[244];

	/* Copy to a V2 sized buffer incase we have read in a V1 smaller sized one */
	memset(data, 0, sizeof(data));
	memcpy(data, fadt_data, size);

	memcpy(&hdr->header, data, 36);
	GET_UINT32(hdr->firmware_control, data, 36);
	GET_UINT32(hdr->dsdt, data, 40);
	GET_UINT8 (hdr->reserved, data, 44);
	GET_UINT8 (hdr->preferred_pm_profile, data, 45);
	GET_UINT16(hdr->sci_int, data, 46);
	GET_UINT32(hdr->smi_cmd, data, 48);
	GET_UINT8 (hdr->acpi_enable, data, 52);
	GET_UINT8 (hdr->acpi_disable, data, 53);
	GET_UINT8 (hdr->s4bios_req, data, 54);
	GET_UINT8 (hdr->pstate_cnt, data, 55);
	GET_UINT32(hdr->pm1a_evt_blk, data, 56);
	GET_UINT32(hdr->pm1b_evt_blk, data, 60);
	GET_UINT32(hdr->pm1a_cnt_blk, data, 64);
	GET_UINT32(hdr->pm1b_cnt_blk, data, 68);
	GET_UINT32(hdr->pm2_cnt_blk, data, 72);
	GET_UINT32(hdr->pm_tmr_blk, data, 80);
	GET_UINT32(hdr->gpe0_blk, data, 84);
	GET_UINT32(hdr->gpe1_blk, data, 88);
	GET_UINT8 (hdr->pm1_evt_len, data, 89);
	GET_UINT8 (hdr->pm1_cnt_len, data, 90);
	GET_UINT8 (hdr->pm2_cnt_len, data, 91);
	GET_UINT8 (hdr->pm_tmr_len, data, 92);
	GET_UINT8 (hdr->gpe0_blk_len, data, 93);
	GET_UINT8 (hdr->gpe1_blk_len, data, 94);
	GET_UINT8 (hdr->cst_cnt, data, 95);
	GET_UINT16(hdr->p_lvl2_lat, data, 96);
	GET_UINT16(hdr->p_lvl3_lat, data, 98);
	GET_UINT16(hdr->flush_size, data, 100);
	GET_UINT16(hdr->flush_stride, data, 102);
	GET_UINT8 (hdr->duty_offset, data, 104);
	GET_UINT8 (hdr->duty_width, data, 105);
	GET_UINT8 (hdr->day_alrm, data, 106);
	GET_UINT8 (hdr->mon_alrm, data, 107);
	GET_UINT8 (hdr->century, data, 108);
	GET_UINT16(hdr->iapc_boot_arch, data, 109);
	GET_UINT8 (hdr->reserved1, data, 111);
	GET_UINT32(hdr->flags, data, 112);
	GET_GAS   (hdr->reset_reg, data, 116);
	GET_UINT8 (hdr->reset_value, data, 128);
	GET_UINT8 (hdr->reserved2, data, 129);
	GET_UINT8 (hdr->reserved3, data, 130);
	GET_UINT8 (hdr->reserved4, data, 131);
	GET_UINT64(hdr->x_firmware_control, data, 132);
	GET_UINT64(hdr->x_dsdt, data, 140);
	GET_GAS   (hdr->x_pm1a_evt_blk, data, 148);
	GET_GAS   (hdr->x_pm1b_evt_blk, data, 160);
	GET_GAS   (hdr->x_pm1a_cnt_blk, data, 172);
	GET_GAS   (hdr->x_pm1b_cnt_blk, data, 184);
	GET_GAS   (hdr->x_pm2_cnt_blk, data, 196);
	GET_GAS   (hdr->x_pm_tmr_blk, data, 208);
	GET_GAS   (hdr->x_gpe0_blk, data, 220);
	GET_GAS   (hdr->x_gpe1_blk, data, 232);
}

static int fadt_init(fwts_log *results, fwts_framework *fw)
{
	if (fwts_check_root_euid(results))
		return 1;

	if ((fadt_table = fwts_get_acpi_table(results, "FADT", &fadt_size)) == NULL) {
		fwts_log_error(results, "Failed to read ACPI FADT");
		return 1;
	}

	return 0;
}

static int fadt_deinit(fwts_log *results, fwts_framework *fw)
{
	if (fadt_table)
		free(fadt_table);

	return 0;
}

static char *fadt_headline(void)
{
	return "Check if FADT SCI_EN bit is enabled";
}

static int fadt_test1(fwts_log *results, fwts_framework *fw)
{
	char *test = "Check SCI_EN bit";
	fwts_fadt_version_2  fadt;
	u32 port, width, value;
	char *profile;

	fwts_log_info(results, test);

	/*  Not having a FADT is not a failure */
	if (fadt_size == 0) {
		fwts_log_info(results, "FADT does not exist, this is not necessarily a failure");
		return 0;
	}

	fadt_get_header(fadt_table, fadt_size, &fadt);

	/* Sanity check profile */
	switch (fadt.preferred_pm_profile) {
	case 0:
		profile = "Unspecified";
		break;
	case 1:
		profile = "Desktop";
		break;
	case 2:
		profile = "Mobile";
		break;
	case 3:
		profile = "Workstation";
		break;
	case 4:
		profile = "Enterprise Server";
		break;
	case 5:
		profile = "SOHO Server";
		break;
	case 6:
		profile = "Appliance PC";
		break;
	case 7:
		profile = "Performance Server";
		break;
	default:
		profile = "Reserved";
		fwts_log_warning(results, "FADT Preferred PM Profile is Reserved - this may be incorrect");
		break;
	}

	fwts_log_info(results, "FADT Preferred PM Profile: %d (%s)\n", 
		fadt.preferred_pm_profile, profile);
	
	port = fadt.pm1a_cnt_blk;
	width = fadt.pm1_cnt_len * 8;	/* In bits */

	/* Punt at244 byte FADT is V2 */
	if (fadt.header.length == 244) {
		/*  Sanity check sizes with extended address variants */
		fwts_log_info(results, "FADT is greater than ACPI version 1.0");
		if (port != fadt.x_pm1a_cnt_blk.address) 
			fwts_log_warning(results, "32 and 64 bit versions of FADT pm1_cnt address do not match");
		if (width != fadt.x_pm1a_cnt_blk.register_bit_width)
			fwts_log_warning(results, "32 and 64 bit versions of FADT pm1_cnt size do not match");

		port = fadt.x_pm1a_cnt_blk.address;	
		width = fadt.x_pm1a_cnt_blk.register_bit_width;
	}

	ioperm(port, width/8, 1);
	switch (width) {
	case 8:
		value = inb(fadt.pm1a_cnt_blk);
		ioperm(port, width/8, 0);
		break;
	case 16:
		value = inw(fadt.pm1a_cnt_blk);
		ioperm(port, width/8, 0);
		break;
	case 32:
		value = inl(fadt.pm1a_cnt_blk);
		ioperm(port, width/8, 0);
		break;
	default:
		ioperm(port, width/8, 0);
		fwts_framework_failed(fw, "FADT pm1a register has invalid bit width of %d", width);
		return 1;
	}

	if (value & 0x01)
		fwts_framework_passed(fw, "SCI_EN bit in PM1a Control Register Block is enabled");
	else
		fwts_framework_passed(fw, "SCI_EN bit in PM1a Control Register Block is not enabled");

	return 0;
}

static fwts_framework_tests fadt_tests[] = {
	fadt_test1,
	NULL
};

static fwts_framework_ops fadt_ops = {
	fadt_headline,
	fadt_init,
	fadt_deinit,
	fadt_tests
};

FRAMEWORK(fadt, &fadt_ops, TEST_ANYTIME);
