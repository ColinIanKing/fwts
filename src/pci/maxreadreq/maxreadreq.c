/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "framework.h"

/* 
 * This test checks if MaxReadReq is set > 128 for non-internal stuff
 * A too low value hurts performance
 */


char *maxreadreq_headline(void)
{
	return "Checks f/w has set PCI Express MaxReadReq to a higher value on non-montherboard devices";
}

static char *lspci = "/usr/bin/lspci";
static char *lspci_text;


int maxreadreq_init(log *results, framework *fw)
{
	struct stat buffer;

	if (check_root_euid(results))
		return 1;

	if (stat(lspci, &buffer)) {
		log_error(results, "Cannot find %s", lspci);	
		return 1;
	}

	if (pipe_exec("lspci -vvv", &lspci_text)) {
		log_error(results, "Failed to execute lspci -vvv");
		return 1;
	}

	if (lspci_text == NULL) {
		log_error(results, "Unexpected empty output from lspci -vvv");
		return 1;
	}
	
	return 0;
}

int maxreadreq_deinit(log *results, framework *fw)
{
	if (lspci_text)
		free(lspci_text);

	return 0;
}

static void getaline(char **text, char *buffer, int buffer_len)
{
	char *ptr1 = *text;
	char *ptr2 = buffer;


	if (ptr1 == NULL) {
		*buffer = '\0';
		return;
	}

	while (*ptr1 && *ptr1 != '\n' && (ptr2 < buffer+buffer_len-1))
		*ptr2++ = *ptr1++;
	
	*ptr2 = '\0';
	
	/* Gobble up any left overs if we hit the end of buffer */

	while (*ptr1 && *ptr1 != '\n')
		ptr1++;

	if (*ptr1 == '\n')
		ptr1++;

	*text = ptr1;
}

static int maxreadreq_test1(log *results, framework *fw)
{	
	int warnings = 0;
	char current_type[512];
	char current_device[512];
	
	while (lspci_text && *lspci_text) {
		char line[4096];
		int val = 0;
		char *c;

		getaline(&lspci_text, line, sizeof(line));

		if (line[0]!=' ' && line[0] != '\t' && strlen(line)>8) {
			if (strlen(line) > 500){
				log_warning(results, "Too big pci string would overflow"
					    "current_device buffer Internal plugin,"
					    " not a firmware" " bug");
				break;
			}
			sprintf(current_device, "pci://00:%s", line);
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
				log_warning(results, "MaxReadReq for %s is low (%d) [%s]", current_device, val, current_type);
				warnings++;
			}
		}
	}

	if (warnings > 0)
		framework_failed(fw, "%d devices have low MaxReadReq settings.\n" 
				     "Firmware may have configured these too low.",
				     warnings);
	else
		framework_passed(fw, "All devices have MaxReadReq set > 128");

	return 0;
}

framework_tests maxreadreq_tests[] = {
	maxreadreq_test1,
	NULL
};

framework_ops maxreadreq_ops = {
	maxreadreq_headline,
	maxreadreq_init,	
	NULL,
	maxreadreq_tests
};

FRAMEWORK(maxreadreq, &maxreadreq_ops, TEST_ANYTIME);
