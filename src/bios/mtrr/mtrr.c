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
#include <fcntl.h>
#include <unistd.h>

#include "fwts.h"

static fwts_list *klog;
static fwts_list *mtrr_list;

#define UNCACHED	1
#define	WRITEBACK	2
#define	WRITECOMBINING	4
#define WRITETHROUGH	8
#define DEFAULT		16

struct mtrr_entry {
	uint64 start;
	uint64 end;
	uint8  type;
};

char *mtrr_resource = NULL;

static char *cache_to_string(int type)
{
	char str[1024];
	memset(str, 0, 1024);

	if (type & UNCACHED || type==0)
		strcat(str,"uncached ");
	if (type & WRITECOMBINING)
		strcat(str,"write-combining ");
	if (type & WRITEBACK)
		strcat(str,"write-back ");
	if (type & WRITETHROUGH)
		strcat(str,"write-through ");
	if (type & DEFAULT)
		strcat(str,"default ");
	return strdup(str);
}

static int get_mtrrs(void)
{
	struct mtrr_entry *entry;
	FILE *file;
	char line[4096];
	
	memset(line, 0, 4096);

	mtrr_list = fwts_list_init();

	if ((file = fopen("/proc/mtrr", "r")) == NULL)
		return 1;

	while (!feof(file)) {
		char *c, *c2;
		if (fgets(line, 4095, file)==NULL)
			break;

		entry = malloc(sizeof(struct mtrr_entry));
		if (!entry)
			break;
		memset(entry, 0, sizeof(struct mtrr_entry));

		c = strstr(line, "base=0x");
		if (c==NULL)
			continue;
		c+=5;
		entry->start = strtoull(c, NULL, 16);
		c = strstr(line, "size=");
		if (!c)
			continue;
		c+=5;
		entry->end = strtoull(c, &c2, 10);
		if (c2 && *c2=='M')
			entry->end = entry->end * 1024 * 1024;
		if (c2 && *c2=='K')
			entry->end = entry->end * 1024;
		if (c2 && *c2=='m')
			entry->end = entry->end * 1024 * 1024;
		if (c2 && *c2=='k')
			entry->end = entry->end * 1024;
		
		entry->end += entry->start;

		if (strstr(line, "write-back"))
			entry->type = WRITEBACK;
		if (strstr(line, "uncachable"))
			entry->type = UNCACHED;
		if (strstr(line, "write-through"))
			entry->type = WRITETHROUGH;
		if (strstr(line, "write-combining"))
			entry->type = WRITECOMBINING;

		fwts_list_append(mtrr_list, entry);		
	}
	fclose(file);

	return 0;
}

static int cache_types(uint64 start, uint64 end)
{
	fwts_list_element *list;
	struct mtrr_entry *entry;
	int type = 0;
	
	for (list = mtrr_list->head; list != NULL; list = list->next) {
		entry = (struct mtrr_entry*)list->data;

		if (entry->end > start && entry->start < end)
			type |= entry->type;
	}


	/* now to see if there is any part of the range that isn't covered by an mtrr,
	   since it's UNCACHED if so */
restart:
	for (list = mtrr_list->head; list != NULL; list = list->next) {
		entry = (struct mtrr_entry*)list->data;

		if (entry->end >= end && entry->start < end) {
			end = entry->start;
			if (end<start)
				end = start;
			else 
				goto restart;
		}
	}
	
	/* if there is no full coverage it's also uncached */
	if (start != end)
		type |= DEFAULT;
	return type;
}

static int is_prefetchable(char *device, uint64 address)
{
	int pref = 0;
	char line[4096];
	char *lspci_cmd;
	fwts_list *lspci_output;
	fwts_list_element *item;

	memset(line,0,4096);
	
	lspci_cmd = "/usr/bin/lspci";
	sprintf(line,"%s -v -s %s", lspci_cmd, device);
	fwts_pipe_exec(line, &lspci_output);
	if (lspci_output == NULL)
		return pref;

	for (item=lspci_output->head; item != NULL; item = item->next) {
		char *str = strstr(fwts_text_list_text(item), "Memory at ");
		if (str) {
			uint64 addr;
			str += 10;
			addr = strtoull(str, NULL, 16);
			if (addr == address) {
				if (strstr(line,"non-prefetchable")) 
					pref = 0;
				else if (strstr(line,"(prefetchable"))
					pref = 1;
				else if (strstr(line,", prefetchable"))
					pref = 1;
			}				
		}
	}
	fwts_list_free(lspci_output, free);

	return pref;
}

static void guess_cache_type(char *string, int *must, int *mustnot, uint64 address)
{
	*must = 0;
	*mustnot = 0;

	if (strstr(string, "System RAM")) {
		*must = WRITEBACK;
		*mustnot = ~*must;
		return;
	}
	/* if it's PCI mmio -> uncached typically except for video */
	if (strstr(string, "0000:")) {
		if (is_prefetchable(string, address)) {
			*must = 0;
			*mustnot = WRITEBACK | UNCACHED;
		} else {
			*must = UNCACHED;
			*mustnot = (~*must) & ~DEFAULT;
		}
	}
}

static int validate_iomem(fwts_framework *fw)
{
	FILE *file;
	char buffer[4096];
	int pcidepth = 0;
	memset(buffer, 0, 4096);
	int failed = 0;

	if ((file = fopen("/proc/iomem", "r")) == NULL)
		return 1;

	while (!feof(file)) {
		uint64 start;
		uint64 end;
		int type, type_must, type_mustnot;
		char *c, *c2;
		int i;
		int skiperror = 0;

		if (fgets(buffer, 4095, file)==NULL)
			break;

		fwts_chop_newline(buffer);

		/* for pci bridges, we note the increased depth and otherwise skip the entry */
		if (strstr(buffer, ": PCI Bus #")) {
			pcidepth++;
			continue;
		}

		/* then: check the pci depth */		
		for (i=0; i<pcidepth*2; i++) {
			if (buffer[i]!=' ') {
				pcidepth = i/2;
				break;
			}
		}
		c = &buffer[pcidepth*2];
		/* sub entry to a main entry -> skip */
		if (*c==' ')
			continue;
		
		start = strtoull(c, NULL, 16);
		c2 = strchr(c, '-');
		if (!c2) 
			continue;
		c2++;
		end = strtoull(c2, NULL, 16);
		

		c2 = strstr(c, " : ");
		if (!c2)
			continue;
		c2+=3;

		/* exception: 640K - 1Mb range we ignore */
		if (start >= 640*1024 && end <= 1024*1024)
			continue;
		
		type = cache_types(start, end);

		guess_cache_type(c2, &type_must, &type_mustnot, start);

		if ((type & type_mustnot)!=0) {
			failed++;
			fwts_framework_failed(fw, "Memory range 0x%llx to 0x%llx (%s) has incorrect attribute %s", start, end, c2,
				cache_to_string(type & type_mustnot));
			if (type_must == UNCACHED)
				skiperror = 1;
		}

		if (type & DEFAULT) {
			type |= UNCACHED;
			type &= ~DEFAULT;
		}
		if ((type & type_must)!=type_must && skiperror==0) {
			failed++;
			fwts_framework_failed(fw, "Memory range 0x%llx to 0x%llx (%s) is lacking attribute %s", start, end, c2,
				cache_to_string( (type & type_must) ^ type_must));
		}
		
	}
	fclose(file);

	if (!failed) {
		fwts_framework_passed(fw, "Memory ranges seem to have correct attributes");
	}

	return 0;
}

static void do_mtrr_resource(fwts_framework *fw)
{
	fwts_list_element *list;
	struct mtrr_entry *entry;

	fwts_log_info(fw,"MTRR overview\n-------------\n");

	for (list = mtrr_list->head; list != NULL; list = list->next) {
		entry = (struct mtrr_entry*)list->data;
		fwts_log_info(fw, "0x%08llx - 0x%08llx   %s \n", entry->start, entry->end, cache_to_string(entry->type));
	}
	fwts_log_info(fw,"\n");
}


static void check_line(void *data, void *private)
{
	char *line = (char *)data;
	fwts_framework *fw = (fwts_framework *)private;

	if (strstr(line, "mtrr: probably your BIOS does not setup all CPUs."))
		fwts_framework_failed(fw, "Not all processors have the MTRR set up");
}

static int mtrr_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return 1;

	if (get_mtrrs())
		fwts_log_error(fw, "Failed to read /proc/mtrr");

	if (access("/proc/mtrr", R_OK))
		return 1;

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Failed to read kernel log");
		return 1;
	}

	do_mtrr_resource(fw);

	return 0;
}

static int mtrr_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);

	return 0;
}

static char *mtrr_headline(void)
{
	return "MTRR validation";
}

static int mtrr_test1(fwts_framework *fw)
{
	fwts_log_info(fw, 
		"This test validates the MTRR iomem setup");

	return validate_iomem(fw);
}

static int mtrr_test2(fwts_framework *fw)
{
	fwts_log_info(fw, 
		"This test validates the MTRR setup across all processors");

	if (klog != NULL) {
		fwts_list_foreach(klog, check_line, fw);
		if (!fw->tests_failed)
			fwts_framework_passed(fw, "All processors have the MTRR setup");
	} else
		fwts_log_error(fw, "No boot dmesg found.\n");
		
	return 0;
}

static fwts_framework_tests mtrr_tests[] = {
	mtrr_test1,
	mtrr_test2,
	NULL
};

static fwts_framework_ops mtrr_ops = {
	mtrr_headline,
	mtrr_init,
	mtrr_deinit,
	mtrr_tests
};

FRAMEWORK(mtrr, &mtrr_ops, TEST_ANYTIME);
