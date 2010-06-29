/*
 * Copyright (C) 2010 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either bios_info 2
 * of the License, or (at your option) any later bios_info.
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
#include <string.h>
#include <limits.h>

#include "fwts.h"

static char *bios_info_headline(void)
{
	/* Return the name of the test scenario */
	return "Gather BIOS information.";
}

static int bios_info_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	if (fwts_check_executable(fw, fw->dmidecode, "dmidecode"))
		return FWTS_ERROR;

	return FWTS_OK;
}

static char *bios_fields[] = {
	"Vendor",
	"Version",
	"Release Date",
	"Address",
	"ROM Size",
	"BIOS Revision",
	NULL,
};

static int bios_info_test1(fwts_framework *fw)
{
	fwts_list *dmi_text;
	fwts_list_element *item;
	char buffer[PATH_MAX];
	int fields = 0;

	snprintf(buffer, sizeof(buffer), "%s -t 0", fw->dmidecode);

	if (fwts_pipe_exec(buffer, &dmi_text)) {
		fwts_log_error(fw, "Failed to execute dmidecode.");
		return FWTS_ERROR;
	}
	if (dmi_text == NULL) {
		fwts_log_error(fw, "Failed to read output from dmidecode.");
		return FWTS_ERROR;
	}	

	for (item = dmi_text->head; item != NULL; item = item->next) {
		char *text = fwts_text_list_text(item);
		char *str;
		int i;

		for (i=0;bios_fields[i]!=NULL;i++) {
			if ((str = strstr(text, bios_fields[i])) != NULL) {
				str += strlen(bios_fields[i]) + 2;
				fwts_log_info_verbatum(fw, "%-14.14s: %s", bios_fields[i],str);
				fields++;
				break;
			}
		}
	}

	if (fields == ((sizeof(bios_fields) / sizeof(char *))) - 1) 
		fwts_passed(fw, "Got all fields\n");

	fwts_text_list_free(dmi_text);
	return FWTS_OK;
}

static fwts_framework_tests bios_info_tests[] = {
	bios_info_test1,
	NULL
};

static fwts_framework_ops bios_info_ops = {
	bios_info_headline,
	bios_info_init,
	NULL,
	bios_info_tests
};

FWTS_REGISTER(bios_info, &bios_info_ops, FWTS_TEST_FIRST, FWTS_BATCH);
