/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010-2011 Canonical
 *
 * This file was original part of the Linux-ready Firmware Developer Kit
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* 
 * This test checks if MaxReadReq is set > 128 for non-internal stuff
 * A too low value hurts performance
 */

static fwts_list *lspci_text;

static int maxreadreq_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	if (fwts_check_executable(fw, fw->lspci, "lspci"))
		return FWTS_ERROR;

	if (fwts_pipe_exec("lspci -vvv", &lspci_text)) {
		fwts_log_error(fw, "Failed to execute lspci -vvv.");
		return FWTS_ERROR;
	}

	if (lspci_text == NULL) {
		fwts_log_error(fw, "Unexpected empty output from lspci -vvv.");
		return FWTS_ERROR;
	}
	
	return FWTS_OK;
}

static int maxreadreq_deinit(fwts_framework *fw)
{
	if (lspci_text)
		fwts_text_list_free(lspci_text);

	return FWTS_OK;
}

static int maxreadreq_test1(fwts_framework *fw)
{	
	int warnings = 0;
	char current_type[512];
	char current_device[512];

	fwts_list_link *item;

	if (lspci_text == NULL)
		return FWTS_ERROR;

	fwts_list_foreach(item, lspci_text) {
		char *line = fwts_text_list_text(item);
		int val = 0;
		char *c;

		if (line[0]!=' ' && line[0] != '\t' && strlen(line)>8) {
			if (strlen(line) > 500){
				fwts_log_warning(fw, "Too big pci string would overflow "
					    "current_device buffer Internal plugin, "
					    "not a firmware bug.");
				break;
			}
			snprintf(current_device, sizeof(current_device), "pci://00:%s", line);
			strncpy(current_type, line+8, 511);
			c = strchr(current_type, ':');
			if (c) 
				*c='\0';
		}
		/* chipset devices are exempt from this check */
		if (strcmp(current_type, "PCI bridge")==0)
			continue;		
		if (strcmp(current_type, "Host bridge")==0)
			continue;		
		if (strcmp(current_type, "System peripheral")==0)
			continue;		

		c = strstr(line, "MaxReadReq ");
		if (c) {
			c += 11;
			val = strtoul(c, NULL, 10);
			if (val <= 128) {
				fwts_log_warning(fw, "MaxReadReq for %s is low (%d) [%s].", current_device, val, current_type);
				warnings++;
			}
		}
	}

	if (warnings > 0) {
		fwts_failed_low(fw, "%d devices have low MaxReadReq settings. " 
				"Firmware may have configured these too low.",
				 warnings);
		fwts_tag_failed(fw, FWTS_TAG_BIOS);
	} else
		fwts_passed(fw, "All devices have MaxReadReq set > 128.");

	return FWTS_OK;
}

static fwts_framework_minor_test maxreadreq_tests[] = {
	{ maxreadreq_test1, "Check firmware settings MaxReadReq for PCI Express devices." },
	{ NULL, NULL },
};

static fwts_framework_ops maxreadreq_ops = {
	.description = "Checks firmware has set PCI Express MaxReadReq to a higher value on non-motherboard devices.",
	.init        = maxreadreq_init,	
	.deinit      = maxreadreq_deinit,
	.minor_tests = maxreadreq_tests
};

FWTS_REGISTER(maxreadreq, &maxreadreq_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);

#endif
