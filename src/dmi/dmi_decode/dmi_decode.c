/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 *
 * This code was originally part of the Linux-ready Firmware Developer Kit
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "framework.h"
#include "text_list.h"

typedef struct {
	char *pat1;
	char *pat2;
	char *message;
} dmi_pattern;

static char *dmi_types[] = {
	"BIOS",
	"System",
	"Base Board",
	"Chassis",
	"Processor",
	"Memory Controller",
	"Memory Module",
	"Cache",
	"Port Connector",
	"System Slots",
	"On Board Devices",
	"OEM Strings",
	"System Configuration Options",
	"BIOS Language",
	"Group Associations",
	"System Event Log",
	"Physical Memory Array",
	"Memory Device",
	"32-bit Memory Error",
	"Memory Array Mapped Address",
	"Memory Device Mapped Address",
	"Built-in Pointing Device",
	"Portable Battery",
	"System Reset",
	"Hardware Security",
	"System Power Controls",
	"Voltage Probe",
	"Cooling Device",
	"Temperature Probe",
	"Electrical Current Probe",
	"Out-of-band Remote Access",
	"Boot Integrity Services",
	"System Boot",
	"64-bit Memory Error",
	"Management Device",
	"Management Device Component",
	"Management Device Threshold Data",
	"Memory Channel",
	"IPMI Device",
	"Power Supply"
};

static dmi_pattern dmi_patterns[] = {
	{ "No SMBIOS nor DMI entry point found", NULL, "Check SMBIOS or DMI entry points" },
	{ "Wrong DMI structures count", NULL, "DMI structures count" },
	{ "Wrong DMI structures length",NULL, "DMI structures length" },
	{ "<OUT OF SPEC>", NULL, "Out of spec check" },
	{ "<BAD INDEX>", NULL, "Bad index check" },
	{ "Bad checksum! Please report.", "Bad checksum" },
	{ "Serial Number:", "0123456789", "Template Serial Number not updated" },
	{ "Asset Tag",  "1234567890", "Template Serial Number not updated" },
	{ "UUID:", "0A0A0A0A-0A0A-0A0A-0A0A-0A0A0A0A0A0A.", "UUID number not updated" },
	{ "To Be Filled By O.E.M.", "Value not updated" },
	{ NULL, NULL, NULL }
};

static char *dmidecode = "/usr/sbin/dmidecode";

static int dmi_decode_init(log *results, framework *fw)
{
	struct stat buffer;

	if (check_root_euid(results))
		return 1;

	if (fw->dmidecode)
		dmidecode = fw->dmidecode;

	if (stat(dmidecode, &buffer)) {
		log_error(results, "Cannot find %s, make sure dmidecode is installed", dmidecode);
		return 1;
	}
	return 0;
}

static char *dmi_decode_headline(void)
{
	return "Test DMI/SMBIOS tables for errors";
}

static int dmi_decode_test1(log *results, framework *fw)
{
	text_list *dmi_text;
	text_list_element *item;
	int type;

	for (type=0; type < 40; type++) {
		int dumped = 0;
		int failed = 0;
		char buffer[1024];

		sprintf(buffer, "%s -t %d", dmidecode, type);

		if (pipe_exec(buffer, &dmi_text)) {
			log_error(results, "Failed to execute dmidecode");
			return 1;
		}
		if (dmi_text == NULL) {
			log_error(results, "Failed to read output from dmidecode (out of memory)");
			return 1;
		}	

		for (item = dmi_text->head; item != NULL; item = item->next) {
			int i;

			for (i=0; dmi_patterns[i].pat1 != NULL; i++) {
				int match;
				if (dmi_patterns[i].pat2 == NULL) 
					match = (strstr(item->text, dmi_patterns[i].pat1) != NULL);
				else {
					match = (strstr(item->text, dmi_patterns[i].pat1) != NULL) &&
						(strstr(item->text, dmi_patterns[i].pat2) != NULL);
				}
				if (match) {		
					failed++;
					framework_failed(fw, "DMI type %s: %s", dmi_types[type],dmi_patterns[i].message);
					if (!dumped) {
						log_info(results, "DMI table dump:");
						text_list_element *dump;
						for (dump = dmi_text->head; dump != item->next; dump = dump->next)
							log_info(results, "%s", dump->text);
						dumped = 1;
					}
				}
			}
		}
		if (!failed)
			framework_passed(fw, "DMI type %s", dmi_types[type]);
		
		text_list_free(dmi_text);
	}
	return 0;
}

static framework_tests dmi_decode_tests[] = {
	dmi_decode_test1,
	NULL
};

static framework_ops dmi_decode_ops = {
	dmi_decode_headline,
	dmi_decode_init,
	NULL,
	dmi_decode_tests
};

FRAMEWORK(dmi_decode, &dmi_decode_ops, TEST_ANYTIME);
