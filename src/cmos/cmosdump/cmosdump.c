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

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

/* Some CMOS information from http://www-ivs.cs.uni-magdeburg.de/~zbrog/asm/cmos.html */

static int cmosdump_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	return FWTS_OK;
}

static char *cmosdump_headline(void)
{
	return "Dump CMOS Memory.";
}

static int cmosdump_test1(fwts_framework *fw)
{
	int i;
	unsigned long tmp;

	static char *cmos_shutdown_status[] = {
		"Power on or soft reset",
		"Memory size pass",
		"Memory test pass",
		"Memory test fail",

		"INT 19h reboot",		
		"Flush keyboard and jmp via 40h:67h",
		"Protected mode tests pass",
		"Protected mode tests fail",

		"Used by POST during protected-mode RAM test",
		"Int 15h (block move)",
		"Jmp via 40h:67h",
		"Used by 80386",
	};

	static char *floppy_disk[] = {
		"None",
		"360KB 5.25\" Drive",
		"1.2MB 5.25\" Drive",
		"720KB 3.5\" Drive",
		"1.44MB 3.5\" Drive",
		"2.88MB 3.5\" Drive",
		"Unknown",
		"Unknown"
	};

	static char *hard_disk[] = {
		"None",
		"Type 1",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Unknown",
		"Type 14",
		"Type 16-47"
	};

	static char *primary_display[] = {
		"BIOS selected",
		"CGA 40 column",
		"CGA 80 column",
		"Monochrome"
	};

	static char *divider[8] = {
		"unknown",
		"unknown",
		"32.768 KHz (default)",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
	};

	static char *rate_selection[16] = {
		"none",
		"unknown",
		"unknown",
		"122 microseconds (minimum)",
		"unknown",
		"unknown",
		"976.562 microseconds (default)",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"500 milliseconds"
	};

	unsigned char data[0x80];

	/* Read CMOS Data */
	for (i=0;i<(int)sizeof(data); i++) {
		if (fwts_cmos_read(i, &data[i]) != FWTS_OK) {
			fwts_log_error(fw, "Cannot get read/write permission on I/O ports.");
			return FWTS_ERROR;
		}
	}

	fwts_log_info_verbatum(fw, "CMOS Memory Dump:");
	for (i=0;i<(int)sizeof(data); i+= 8) {
		char buffer[128];
		fwts_dump_raw_data(buffer, sizeof(buffer), data + i, i, 16);
		fwts_log_info_verbatum(fw, "%s", buffer);
	}
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "RTC Current Time: (CMOS 0x00..0x09)");
	fwts_log_info_verbatum(fw, "  RTC seconds:            %2.2x", data[0]);
	fwts_log_info_verbatum(fw, "  RTC minutes:            %2.2x", data[2]);
	fwts_log_info_verbatum(fw, "  RTC hours:              %2.2x", data[4]);
	fwts_log_info_verbatum(fw, "  RTC day of week:        %2.2x", data[6]);
	fwts_log_info_verbatum(fw, "  RTC date day:           %2.2x", data[7]);
	fwts_log_info_verbatum(fw, "  RTC date month:         %2.2x", data[8]);
	fwts_log_info_verbatum(fw, "  RTC date year:          %2.2x", data[9]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "RTC Alarm:");
	fwts_log_info_verbatum(fw, "  RTC seconds:            %2.2x", data[1]);
	fwts_log_info_verbatum(fw, "  RTC minutes:            %2.2x", data[3]);
	fwts_log_info_verbatum(fw, "  RTC hours:              %2.2x", data[5]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Status Register A: (CMOS 0x0a): 0x%2.2x", data[10]);
	fwts_log_info_verbatum(fw, "  Rate freq:              %1.1x (%s)", data[10] & 0xf, rate_selection[data[10] & 0xf]);
	fwts_log_info_verbatum(fw, "  Timer freq divider:     %1.1x (%s)", (data[10] >> 4) & 0x7, divider[(data[10] >> 4) & 0x7]);
	fwts_log_info_verbatum(fw, "  Update in progress:     %1.1x", (data[10] >> 7) & 1);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Status Register B: (CMOS 0x0b): 0x%2.2x", data[11]);
	fwts_log_info_verbatum(fw, "  Daylight savings:       %1.1x (%s)", (data[11] >> 1) & 1, (data[11] >> 1) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  24 Hour Clock:          %1.1x (%s)", (data[11] >> 2) & 1, (data[11] >> 1) & 1 ? "12 Hour" : "24 Hour");
	fwts_log_info_verbatum(fw, "  Square Wave:            %1.1x (%s)", (data[11] >> 3) & 1, (data[11] >> 2) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  Update ended IRQ:       %1.1x (%s)", (data[11] >> 4) & 1, (data[11] >> 3) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  Alarm IRQ:              %1.1x (%s)", (data[11] >> 5) & 1, (data[11] >> 5) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  Periodic IRQ:           %1.1x (%s)", (data[11] >> 6) & 1, (data[11] >> 6) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  Clock update cycle:     %1.1x (%s)", (data[11] >> 7) & 1, (data[11] >> 7) & 1 ? "Abort update in progress" : "Update normally");
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Status Register C: (CMOS 0x0c): 0x%2.2x", data[12]);
	fwts_log_info_verbatum(fw, "  UF flag:                0x%1.1x", (data[12] >> 4) & 1);
	fwts_log_info_verbatum(fw, "  AF flag:                0x%1.1x", (data[12] >> 5) & 1);
	fwts_log_info_verbatum(fw, "  PF flag:                0x%1.1x", (data[12] >> 6) & 1);
	fwts_log_info_verbatum(fw, "  IRQF flag:              0x%1.1x", (data[12] >> 7) & 1);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Status Register D: (CMOS 0x0d): 0x%2.2x", data[13]);
	fwts_log_info_verbatum(fw, "  Valid CMOS RAM flag:    0x%1.1x (%s)", (data[13] >> 7) & 1, (data[13] >> 7) & 1 ? "Battery Good": "Battery Dead");
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Diagnostic Status: (CMOS 0x0e): 0x%2.2x", data[14]);
	fwts_log_info_verbatum(fw, "  CMOS time status:       0x%1.1x (%s)", (data[14] >> 2) & 1, (data[14] >> 2) & 1 ? "Invalid": "Valid");
	fwts_log_info_verbatum(fw, "  Fixed disk init:        0x%1.1x (%s)", (data[14] >> 3) & 1, (data[14] >> 3) & 1 ? "Bad": "Good");
	fwts_log_info_verbatum(fw, "  Memory size check:      0x%1.1x (%s)", (data[14] >> 4) & 1, (data[14] >> 4) & 1 ? "Bad": "Good");
	fwts_log_info_verbatum(fw, "  Config info status:     0x%1.1x (%s)", (data[14] >> 5) & 1, (data[14] >> 5) & 1 ? "Invalid": "Valid");
	fwts_log_info_verbatum(fw, "  CMOS checksum status:   0x%1.1x (%s)", (data[14] >> 6) & 1, (data[14] >> 6) & 1 ? "Bad": "Good");
	fwts_log_info_verbatum(fw, "  CMOS power loss:        0x%1.1x (%s)", (data[14] >> 7) & 1, (data[14] >> 7) & 1 ? "Lost power": "Not lost power");
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "CMOS Shutdown Status: (CMOS 0x0f): 0x%2.2x (%s)", data[15],
			data[15] < 0xb ? cmos_shutdown_status[data[15]] : "Perform power-on reset");
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Floppy Disk Type: (CMOS 0x10): 0x%2.2x", data[16]);
	fwts_log_info_verbatum(fw, "  Drive 0: %s", floppy_disk[((data[16] >> 4) & 0xf)]);
	fwts_log_info_verbatum(fw, "  Drive 1: %s", floppy_disk[((data[16] >> 0) & 0xf)]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Hard Disk Type: (CMOS 0x12, Obsolete): 0x%2.2x", data[18]);
	fwts_log_info_verbatum(fw, "  Drive 0: %s", hard_disk[((data[18] >> 4) & 0xf)]);
	fwts_log_info_verbatum(fw, "  Drive 1: %s", hard_disk[((data[18] >> 0) & 0xf)]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Installed H/W: (CMOS 0x14): 0x%2.2x", data[20]);
	fwts_log_info_verbatum(fw, "  Maths Coprocessor:      0x%1.1x (%s)", (data[20] >> 1) & 1, (data[20] >> 1) & 1 ? "Installed": "Not Installed");
	fwts_log_info_verbatum(fw, "  Keyboard:               0x%1.1x (%s)", (data[20] >> 2) & 1, (data[20] >> 2) & 1 ? "Installed": "Not Installed");
	fwts_log_info_verbatum(fw, "  Display Adaptor:        0x%1.1x (%s)", (data[20] >> 3) & 1, (data[20] >> 3) & 1 ? "Installed": "Not Installed");
	fwts_log_info_verbatum(fw, "  Primary Display:        0x%1.1x (%s)", (data[20] >> 4) & 3, primary_display[(data[20] >> 4) & 3]);
	fwts_log_info_verbatum(fw, "  Floppy Drives:          0x%2.2x (%d drives)", (data[20] >> 6) & 3, ((data[20] >> 6) & 3) + 1);
	fwts_log_nl(fw);

	tmp = ((data[22] << 8) | (data[21]));
	fwts_log_info_verbatum(fw, "Base Mem: (CMOS 0x16):");
	fwts_log_info_verbatum(fw, "  0x%2.2x%2.2x (%luK)", data[22], data[21], tmp);
	fwts_log_nl(fw);

	tmp = ((data[24] << 8) | (data[25]));
	fwts_log_info_verbatum(fw, "Extended Mem: (CMOS 0x18):");
	fwts_log_info_verbatum(fw, "  0x%2.2x%2.2x (%luK) %s", data[24], data[23], tmp, tmp > (16 * 1024) ? "[untrustworthy]" : "");
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Hard Disk Extended Types (CMOS 0x19, 0x1a):");
	fwts_log_info_verbatum(fw, "  Hard Disk 0:            0x%2.2x", data[25]);
	fwts_log_info_verbatum(fw, "  Hard Disk 1:            0x%2.2x", data[26]);
	fwts_log_nl(fw);
	
	/*
	fwts_log_info_verbatum(fw, "Drive C: (CMOS 0x");
	fwts_log_info_verbatum(fw, "  Number of Cylinders:    0x%2.2x%2.2x", data[28], data[27]);
	fwts_log_info_verbatum(fw, "  Number of Heads:        0x%2.2x", data[29]);
	fwts_log_info_verbatum(fw, "  Number of Sectors:      0x%2.2x", data[35]);
	fwts_log_info_verbatum(fw, "  Write precomp cylinder: 0x%2.2x%2.2x", data[31], data[30]);
	fwts_log_info_verbatum(fw, "  Control Byte:           0x%2.2x", data[32]);
	fwts_log_info_verbatum(fw, "  Landing Zone            0x%2.2x%2.2x", data[34], data[33]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Drive D:");
	fwts_log_info_verbatum(fw, "  Number of Cylinders:    0x%2.2x%2.2x", data[37], data[36]);
	fwts_log_info_verbatum(fw, "  Number of Heads:        0x%2.2x", data[38]);
	fwts_log_info_verbatum(fw, "  Number of Sectors:      0x%2.2x", data[39]);
	fwts_log_info_verbatum(fw, "  Write precomp cylinder: 0x%2.2x%2.2x", data[41], data[40]);
	fwts_log_info_verbatum(fw, "  Control Byte:           0x%2.2x", data[41]);
	fwts_log_info_verbatum(fw, "  Landing Zone            0x%2.2x%2.2x", data[44], data[43]);
	fwts_log_nl(fw);
	*/
	
	/* AMI only
	fwts_log_info_verbatum(fw, "System Operational Flags:");
	fwts_log_info_verbatum("  Turbo switch:           0x%1.1x (%s)", (data[45] >> 0) & 1, (data[45] >> 0) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  Fast gate A20 operation 0x%1.1x (%s)", (data[45] >> 1) & 1, (data[45] >> 1) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  Internal Cache:         0x%1.1x (%s)", (data[45] >> 2) & 1, (data[45] >> 2) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  External Cache:         0x%1.1x (%s)", (data[45] >> 3) & 1, (data[45] >> 3) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  Boot CPU speed:         0x%1.1x (%s)", (data[45] >> 4) & 1, (data[45] >> 4) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  Boot Sequence:          0x%1.1x (%s)", (data[45] >> 5) & 1, (data[45] >> 5) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  Floppy boot seek:       0x%1.1x (%s)", (data[45] >> 6) & 1, (data[45] >> 6) & 1 ? "Yes" : "No");
	fwts_log_info_verbatum(fw, "  Weitek CPU:             0x%1.1x (%s)", (data[45] >> 7) & 1, (data[45] >> 7) & 1 ? "Yes" : "No");
	fwts_log_nl(fw);
	*/

	fwts_log_info_verbatum(fw, "CMOS Checksum:(CMOS 0x2e):0x%2.2x%2.2x", data[47], data[46]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Extended Mem: (CMOS 0x30):0x%2.2x%2.2x", data[49], data[48]);
	fwts_log_nl(fw);

	fwts_log_info_verbatum(fw, "Century Date: (CMOS 0x32):%2.2x", data[50]);
	fwts_log_nl(fw);
	fwts_log_info_verbatum(fw, "POST Information Flag (CMOS 0x33):");
	fwts_log_info_verbatum(fw, "  POST cache test:        0x%1.1x %s", (data[51] >> 0) & 1, (data[51] >> 0) & 1 ? "Failed" : "Passed");
	fwts_log_info_verbatum(fw, "  BIOS size:              0x%1.1x %s", (data[51] >> 7) & 1, (data[51] >> 7) & 1 ? "128KB" : "64KB");
	fwts_log_nl(fw);

	/*
	fwts_log_info_verbatum(fw, "BIOS and Shadow Option flags:");
	fwts_log_info_verbatum(fw, "  Password checking:      0x%1.1x %s", (data[52] >> 6) & 1, (data[52] >> 6) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  Boot sector virus prot: 0x%1.1x %s", (data[52] >> 7) & 1, (data[52] >> 7) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  Numeric CPU test:       0x%1.1x %s", (data[53] >> 0) & 1, (data[53] >> 0) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xC000:      0x%1.1x %s", (data[53] >> 2) & 1, (data[53] >> 2) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xC400:      0x%1.1x %s", (data[53] >> 1) & 1, (data[53] >> 1) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xC800:      0x%1.1x %s", (data[52] >> 5) & 1, (data[52] >> 5) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xCC00:      0x%1.1x %s", (data[52] >> 4) & 1, (data[52] >> 4) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xD000:      0x%1.1x %s", (data[52] >> 3) & 1, (data[52] >> 3) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xD400:      0x%1.1x %s", (data[52] >> 2) & 1, (data[52] >> 2) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xD800:      0x%1.1x %s", (data[52] >> 1) & 1, (data[52] >> 1) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xDC00:      0x%1.1x %s", (data[52] >> 0) & 1, (data[52] >> 0) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xE000:      0x%1.1x %s", (data[53] >> 7) & 1, (data[53] >> 7) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xE400:      0x%1.1x %s", (data[53] >> 6) & 1, (data[53] >> 6) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xE800:      0x%1.1x %s", (data[53] >> 5) & 1, (data[53] >> 5) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xEC00:      0x%1.1x %s", (data[53] >> 4) & 1, (data[53] >> 4) & 1 ? "Enabled" : "Disabled");
	fwts_log_info_verbatum(fw, "  ROM shadow 0xF000:      0x%1.1x %s", (data[53] >> 3) & 1, (data[53] >> 3) & 1 ? "Enabled" : "Disabled");
	fwts_log_nl(fw);
	*/

	/* AMI
	fwts_log_info_verbatum(fw, "Encrypted Password:       0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
			data[56], data[57], data[58], data[59], data[60], data[61]);
	fwts_log_info_verbatum(fw, "Extended CMOS checksum:   0x%2.2x%2.2x", data[62], data[63]);

	fwts_log_info_verbatum(fw, "Model Number:             0x%2.2x", data[64]);
	fwts_log_info_verbatum(fw, "Serial Number:            0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
			data[65], data[66], data[67], data[68], data[69], data[70]);
	fwts_log_info_verbatum(fw, "CRC byte:                 0x%2.2x", data[71]);
	fwts_log_info_verbatum(fw, "Century Byte:             0x%2.2x", data[72]);
	fwts_log_info_verbatum(fw, "Date Alarm:               0x%2.2x", data[73]);
	fwts_log_info_verbatum(fw, "RTC Address:              0x%2.2x%2.2x", data[79], data[78]);
	fwts_log_info_verbatum(fw, "Extended RAM address:     0x%2.2x%2.2x", data[81], data[80]);
	fwts_log_nl(fw);
	*/
	
	return FWTS_OK;
}


static fwts_framework_minor_test cmosdump_tests[] = {
	{ cmosdump_test1, "Dump CMOS Memory." },
	{ NULL, NULL }
};

static fwts_framework_ops cmosdump_ops = {
	.headline    = cmosdump_headline,
	.init        = cmosdump_init,
	.minor_tests = cmosdump_tests
};

FWTS_REGISTER(cmosdump, &cmosdump_ops, FWTS_TEST_ANYTIME, FWTS_UTILS);

#endif
