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
#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/mtrr.h>

static fwts_list *klog;
static fwts_list *mtrr_list;
static fwts_cpuinfo_x86 *fwts_virt_cpuinfo;



#define UNCACHED	0x0001
#define	WRITE_BACK	0x0002
#define	WRITE_COMBINING	0x0004
#define WRITE_THROUGH	0x0008
#define WRITE_PROTECT	0x0010
#define DEFAULT		0x0020
#define DISABLED	0x0040
#define UNKNOWN		0x0080

struct mtrr_entry {
	uint8_t  reg;
	uint64_t start;
	uint64_t end;
	uint64_t size;
	uint8_t  type;
};

char *mtrr_resource = NULL;

static char *cache_to_string(int type)
{
	static char str[1024];
	memset(str, 0, 1024);

	if (type & UNCACHED || type==0)
		strcat(str," Uncached");
	if (type & WRITE_BACK)
		strcat(str," Write-Back");
	if (type & WRITE_COMBINING)
		strcat(str," Write-Combining");
	if (type & WRITE_THROUGH)
		strcat(str," Write-Through");
	if (type & WRITE_PROTECT)
		strcat(str," Write-Protect");
	if (type & DEFAULT)
		strcat(str," Default");
	if (type & UNKNOWN)
		strcat(str," Unknown");
	return str;
}

static int get_mtrrs(void)
{
	struct mtrr_gentry gentry;
	struct mtrr_entry *entry;
	int fd;
	char line[4096];
	
	memset(line, 0, 4096);

	mtrr_list = fwts_list_init();

	if ((fd = open("/proc/mtrr", O_RDONLY, 0)) < 0)
		return FWTS_ERROR;

	for (gentry.regnum = 0;
		ioctl (fd, MTRRIOC_GET_ENTRY, &gentry) == 0;
         	++gentry.regnum) {
		if ((entry = calloc(1, sizeof(struct mtrr_entry))) != NULL) {
			entry->reg = gentry.regnum;
			if (gentry.size > 0) {
				entry->start = gentry.base;
				entry->size  = gentry.size;
				entry->end   = gentry.base + gentry.size;
				switch (gentry.type) {
				case 0:  /* uncachable */
					entry->type = UNCACHED;
					break;
				case 1: /* write combining */
					entry->type = WRITE_COMBINING;
					break;
				case 4: /* write through */
					entry->type = WRITE_THROUGH;
					break;
				case 5: /* write protect */
					entry->type = WRITE_PROTECT;
					break;
				case 6: /* write combining */
					entry->type = WRITE_BACK;
					break;
				default: /* unknown */
					entry->type = UNKNOWN;
					break;
				}
			}
			else {
				entry->type = DISABLED;
			}

			fwts_list_append(mtrr_list, entry);		
		}
	}
	close(fd);

	return FWTS_OK;
}

static int cache_types(uint64_t start, uint64_t end)
{
	fwts_list_link *item;
	struct mtrr_entry *entry;
	int type = 0;
	
	fwts_list_foreach(item, mtrr_list) {
		entry = fwts_list_data(struct mtrr_entry*, item);

		if (entry->end > start && entry->start < end)
			type |= entry->type;
	}

	/* now to see if there is any part of the range that isn't covered by an mtrr,
	   since it's UNCACHED if so */
restart:
	fwts_list_foreach(item, mtrr_list) {
		entry = fwts_list_data(struct mtrr_entry*, item);

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

static fwts_list *get_klog_bios_mtrr(void)
{
	fwts_list *mtrr_bios_list;
	fwts_list_link *item;
	int scan = 0;

	if ((mtrr_bios_list = fwts_list_init()) == NULL)
		return NULL;

	fwts_list_foreach(item, klog) {
		char *str = fwts_text_list_text(item);

		if (strstr(str, "MTRR variable ranges enabled")) {
			scan = 1;
			continue;
		}
	
		if (scan) {
			char *base = strstr(str, "base");
			char *disabled = strstr(str, "disabled");

			if ((base == NULL) && (disabled == NULL))
				scan = 0;
			
			if (base) {	
				uint64_t start = strtoull(base+6, NULL, 16);
				str = strstr(base, "mask");
				if (str) {
					struct mtrr_entry *mtrr;

					mtrr = calloc(1, sizeof(struct mtrr_entry));
					mtrr->type = 0;

					uint64_t mask = strtoull(str+5, NULL, 16);
					uint64_t pat = 0x8000000000000000ULL;
					while ((mask & pat) == 0) {
						mask |= pat;
						pat >>= 1;
					}

					mtrr->start = start;
					mtrr->end = start + ~mask;
				
					fwts_list_append(mtrr_bios_list, mtrr);
				}
			}
		}
	}

	return mtrr_bios_list;
}

static int check_vga_controller_address(fwts_framework *fw)
{
	char line[4096];
	fwts_list *lspci_output;
	fwts_list_link *item;
	fwts_list *mtrr_bios_list;
	int vga = 0;
	int found = 0;

	memset(line,0,4096);

	if ((mtrr_bios_list = get_klog_bios_mtrr()) == NULL)
		return FWTS_ERROR;
	
	snprintf(line, sizeof(line), "%s -v", fw->lspci);
	fwts_pipe_exec(line, &lspci_output);
	if (lspci_output == NULL)
		return FWTS_ERROR;

	for (item=lspci_output->head; (item != NULL) && !found; item = item->next) {
		char *str = fwts_text_list_text(item);
		if (strstr(str, "VGA compatible controller"))
			vga = 1;
		if (*str == '\0')
			vga = 0;
		if (vga) {
			if ((str = strstr(str, "Memory at ")) != NULL) {
				fwts_list_link *item;
				struct mtrr_entry *mtrr;
				uint64_t start = strtoull(str+10, NULL, 16);
				uint64_t size = 0;
#if 0
				int pref = 0;
				if (strstr(str, "non-prefetchable"))
					pref = 0;
				else if (strstr(str, "(prefetchable"))
					pref = 1;
				else if (strstr(str, ", prefetchable"))
					pref = 1;
#endif
				if ((str = strstr(str + 10, "size=")) != NULL) {
					size = strtoull(str+5, NULL, 10);
					if (strstr(str + 5, "K]"))
						size *= 1024;
					if (strstr(str + 5, "M]"))
						size *= (1024*1024);
					size--;
				}

				if (size > 1024*1024) {
					for (item = mtrr_bios_list->head; item != NULL; item = item->next) {
						mtrr = fwts_list_data(struct mtrr_entry *, item);
						if (start >= mtrr->start && (start+size)<= mtrr->end) {
							found = 1;
							fwts_passed(fw, "Found VGA memory region in BIOS initialised MTRR space: %llx - %llx\n",
								(unsigned long long int)mtrr->start,
								(unsigned long long int)mtrr->end);
							break;
						}
					}
				}
			}
		}
	}

	if (!found) {
		fwts_failed_low(fw, "Did not find a BIOS configured MTRR for VGA memory region. ");
		fwts_advice(fw, "The VGA memory region does not have a MTRR configured by the BIOS. "
				"This means that bootloaders rendering to a framebuffer will be rendering slowly "
				"and this will slow the boot speed. "
				"It is probably worth asking the BIOS vendor to map in the VGA write-combining region.\n");
	}
	fwts_list_free(mtrr_bios_list, free);
	fwts_list_free(lspci_output, free);

	return FWTS_OK;
}

static int is_prefetchable(fwts_framework *fw, char *device, uint64_t address)
{
	int pref = 0;
	char line[4096];
	fwts_list *lspci_output;
	fwts_list_link *item;

	memset(line,0,4096);
	
	snprintf(line, sizeof(line), "%s -v -s %s", fw->lspci, device);
	fwts_pipe_exec(line, &lspci_output);
	if (lspci_output == NULL)
		return pref;

	fwts_list_foreach(item, lspci_output) {
		char *str = strstr(fwts_text_list_text(item), "Memory at ");
		if (str && strtoull(str+10, NULL, 16) == address) {
			if (strstr(str, "Non-Prefetchable"))
				pref = 0;
			else if (strstr(str, "(Prefetchable"))
				pref = 1;
			else if (strstr(str, ", Prefetchable"))
				pref = 1;
		}
	}
	fwts_list_free(lspci_output, free);

	return pref;
}

static void guess_cache_type(fwts_framework *fw, char *string, int *must, int *mustnot, uint64_t address)
{
	*must = 0;
	*mustnot = 0;

	if (strstr(string, "System RAM")) {
		*must = WRITE_BACK;
		*mustnot = ~WRITE_BACK;
		return;
	}
	/* if it's PCI mmio -> uncached typically except for video */
	if (strstr(string, "0000:")) {
		if (is_prefetchable(fw, string, address)) {
			*must = 0;
			*mustnot = WRITE_BACK | UNCACHED;
		} else {
			*must = UNCACHED;
			*mustnot = (~UNCACHED) & (~DEFAULT);
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
		return FWTS_ERROR;

	while (!feof(file)) {
		uint64_t start;
		uint64_t end;
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

		guess_cache_type(fw, c2, &type_must, &type_mustnot, start);

		if ((type & type_mustnot)!=0) {
			failed++;
			fwts_failed(fw, "Memory range 0x%llx to 0x%llx (%s) has incorrect attribute%s.",
					(unsigned long long int)start,
					(unsigned long long int)end,
					c2, cache_to_string(type & type_mustnot));
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			if (type_must == UNCACHED)
				skiperror = 1;
		}

		if (type & DEFAULT) {
			type |= UNCACHED;
			type &= ~DEFAULT;
		}
		if ((type & type_must)!=type_must && skiperror==0) {
			failed++;
			fwts_failed(fw, "Memory range 0x%llx to 0x%llx (%s) is lacking attribute%s.",
					(unsigned long long int)start,
					(unsigned long long int)end,
					c2, cache_to_string( (type & type_must) ^ type_must));
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
		}
		
	}
	fclose(file);

	if (!failed)
		fwts_passed(fw, "Memory ranges seem to have correct attributes.");

	return FWTS_OK;
}

static void do_mtrr_resource(fwts_framework *fw)
{
	fwts_list_link *item;
	struct mtrr_entry *entry;

	fwts_log_info(fw,"MTRR overview");
	fwts_log_info(fw,"-------------");

	fwts_list_foreach(item, mtrr_list) {
		entry = fwts_list_data(struct mtrr_entry *, item);
		if (entry->type & DISABLED)
			fwts_log_info_verbatum(fw, "Reg %d: disabled\n", entry->reg);
		else
			fwts_log_info_verbatum(fw, "Reg %d: 0x%08llx - 0x%08llx (%6lld %cB)  %s \n", entry->reg,
				(unsigned long long int)entry->start,
				(unsigned long long int)entry->end,
				(unsigned long long int)(entry->size >= (1024*1024) ? entry->size / (1024*1024) : (entry->size / 1024)),
				entry->size >= (1024*1024) ? 'M' : 'K', cache_to_string(entry->type));
	}
	fwts_log_info(fw,"\n");
}

static int mtrr_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	if (fwts_check_executable(fw, fw->lspci, "lspci"))
		return FWTS_ERROR;

	if (get_mtrrs())
		fwts_log_error(fw, "Failed to read /proc/mtrr.");

	if (access("/proc/mtrr", R_OK))
		return FWTS_ERROR;

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Failed to read kernel log.");
		return FWTS_ERROR;
	}

	if ((fwts_virt_cpuinfo = fwts_cpu_get_info(0)) == NULL) {
		fwts_log_error(fw, "Cannot get CPU info");
		return FWTS_ERROR;
	}

	do_mtrr_resource(fw);

	return FWTS_OK;
}

static int mtrr_deinit(fwts_framework *fw)
{
	fwts_klog_free(klog);
	fwts_list_free(mtrr_list, free);
	if (fwts_virt_cpuinfo)
		fwts_cpu_free_info(fwts_virt_cpuinfo);

	return FWTS_OK;
}

static char *mtrr_headline(void)
{
	return "MTRR validation.";
}

static int mtrr_test1(fwts_framework *fw)
{
	return validate_iomem(fw);
}

static int mtrr_test2(fwts_framework *fw)
{
	if (klog != NULL) {
		int failed = 0;

		if (fwts_klog_regex_find(fw, klog, "mtrr: your CPUs had inconsistent fixed MTRR settings") > 0) {
			fwts_log_info(fw, "Detected CPUs with inconsitent fixed MTRR settings which the kernel fixed.");
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			failed = 1;
		}
		if (fwts_klog_regex_find(fw, klog, "mtrr: your CPUs had inconsistent variable MTRR settings") > 0) {
			fwts_log_info(fw, "Detected CPUs with inconsitent variable MTRR settings which the kernel fixed.");
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			failed = 1;
		}
		if (fwts_klog_regex_find(fw, klog, "mtrr: your CPUs had inconsistent MTRRdefType") > 0) {
			fwts_log_info(fw, "Detected CPUs with inconsitent variable MTRR settings which the kernel fixed.");
			fwts_tag_failed(fw, FWTS_TAG_BIOS);
			failed = 1;
		}

		if (failed)
			fwts_failed(fw, "It is probable that the BIOS does not set up all the CPUs correctly and "
					"the kernel has now corrected this misconfiguration.");
		else
			fwts_passed(fw, "All processors have the a consistent MTRR setup.");
	} else
		fwts_log_error(fw, "No boot dmesg found.");
		
	return FWTS_OK;
}

static int mtrr_test3(fwts_framework *fw)
{
	if (strstr(fwts_virt_cpuinfo->vendor_id, "AMD")) {
		if (klog != NULL) {
			if (fwts_klog_regex_find(fw, klog, "SYSCFG[MtrrFixDramModEn] not cleared by BIOS, clearing this bit") > 0) {
				fwts_failed_medium(fw, "The BIOS is expected to clear MtrrFixDramModEn bit, see for example "
 						"\"BIOS and Kernel Developer's Guide for the AMD Athlon 64 and AMD "
 						"Opteron Processors\" (26094 Rev. 3.30 February 2006), section "
 						"\"13.2.1.2 SYSCFG Register\": \"The MtrrFixDramModEn bit should be set "
 						"to 1 during BIOS initalization of the fixed MTRRs, then cleared to "
 						"0 for operation.\"");
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
			}
			else {
				fwts_passed(fw, "No MtrrFixDramModEn error detected.");
			}
		}
	} else
		fwts_skipped(fw, "CPU is not an AMD, cannot test.");

	return FWTS_OK;
}

static int mtrr_test4(fwts_framework *fw)
{
	return check_vga_controller_address(fw);
}

static fwts_framework_minor_test mtrr_tests[] = {
	{ mtrr_test1, "Validate the kernel MTRR IOMEM setup." },
	{ mtrr_test2, "Validate the MTRR setup across all processors." },
	{ mtrr_test3, "Check for AMD MtrrFixDramModEn being cleared by the BIOS." },
	{ mtrr_test4, "Validate the BIOS providided boot time MTRR IOMEM setup." },
	{ NULL, NULL }
};

static fwts_framework_ops mtrr_ops = {
	.headline    = mtrr_headline,
	.init        = mtrr_init,
	.deinit      = mtrr_deinit,
	.minor_tests = mtrr_tests
};

FWTS_REGISTER(mtrr, &mtrr_ops, FWTS_TEST_EARLY, FWTS_BATCH);

#endif
