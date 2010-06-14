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

#include <sys/klog.h>
#include <string.h>
#include <stdlib.h>

#include "fwts.h"

void fwts_klog_free(fwts_list *klog)
{
	fwts_text_list_free(klog);
}

int fwts_klog_clear(void)
{
	if (klogctl(5, NULL, 0) < 0)
		return 1;
	return 0;
}

fwts_list *fwts_klog_read(void)
{
	int len;
	char *buffer;
	fwts_list *list;

	if ((len = klogctl(10, NULL, 0)) < 0)
		return NULL;

	if ((buffer = malloc(len)) < 0)
		return NULL;
	
	if (klogctl(3, buffer, len) < 0)
		return NULL;

	list = fwts_list_from_text(buffer);
	free(buffer);
	
	return list;
}

typedef void (*fwts_scan_callback_t)(fwts_framework *fw, char *line, char *prevline, void *private, int *warnings, int *errors);

int fwts_klog_scan(fwts_framework *fw, fwts_list *klog, fwts_scan_callback_t callback, void *private, int *warnings, int *errors)
{
	*warnings = 0;
	*errors = 0;
	char *prev;
	fwts_list_element *item;

	if (!klog)
		return 1;

	prev = "";

	for (item = klog->head; item != NULL; item=item->next) {
		char *ptr = (char *)item->data;

		if ((ptr[0] == '<') && (ptr[2] == '>'))
			ptr += 3;

		callback(fw, ptr, prev, private, warnings, errors);
		prev = ptr;
	}
	return 0;
}

#define FW_BUG	"[Firmware Bug]: "

/* List of errors and warnings */
static fwts_klog_pattern firmware_error_warning_patterns[] = {
	{ "ACPI Warning ",					KERN_WARNING, 
		"ACPI AML intepreter has found some non-conforming AML code. "
		"This should be investigated and fixed." },
	{ FW_BUG "ACPI: Invalid physical address in GAR",   	KERN_ERROR, 
		"ACPI Generic Address is invalid" },
	{ FW_BUG "ACPI: Invalid bit width in GAR",   		KERN_ERROR, 
		"ACPI Generic Address width must be 8, 16, 32 or 64" },
	{ FW_BUG "ACPI: Invalid address space type in GAR",   	KERN_ERROR, 
		"ACPI Generic Address space type must be system memory or system IO space." },
	{ FW_BUG "ACPI: no secondary bus range in _CRS",   	KERN_WARNING,
		"_CRS Method should return a secondary bus address for the "
		"status/command port. The kernel is having to guess this "
		"based on the _BBN or assume it's 0x00-0xff." },
	{ FW_BUG "ACPI: Invalid BIOS _PSS frequency", 		KERN_ERROR,
		"_PSS (Performance Supported States) package has an incorrectly "
		"define core frequency (first DWORD entry in the _PSS package)." },
	{ FW_BUG "BIOS needs update for CPU frequency support", KERN_WARNING,
		"Having _PPC but missing frequencies (_PSS, _PCT) is a good hint "
		"that the BIOS is older than the CPU and does not know the CPU "
		"frequencies." },
	{ FW_BUG "Invalid critical threshold", 			KERN_WARNING, 
		"ACPI _CRT (Critical Trip Point) is returning a threshold "
		"lower than zero degrees Celsius which is clearly incorrect." },
	{ FW_BUG "No valid trip found", 			KERN_WARNING, 
		"No valud ACPI _CRT (Critical Trip Point) was found" },
	{ FW_BUG "_BCQ is usef instead of _BQC", 		KERN_WARNING,
		"ACPI Method _BCQ was defined (typo) instead of _BQC - this should be fixed."
		"however the kernel has detected this and is working around this typo." },
	{ "defines _DOD but not _DOS", 				KERN_WARNING,
		"ACPI Method _DOD (Enumerate all devices attached to display adapter) "
		"is defined but we should also have _DOS (Enable/Disable output switching) "
		"defined but it's been omitted. This can cause display switching issues." },
	{ FW_BUG "Duplicate ACPI video bus", 			KERN_WARNING,
		"Try video module parameter video.allow_duplicates=1 if "
		"the current driver does't work." },
	{ "[Firmware Bug]:",					KERN_ERROR, 
		"The kernel has detected a bug in the BIOS or ACPI which needs "
		"investigating and fixing." },
	{ "PCI: BIOS Bug:",					KERN_ERROR,   NULL },
	{ "ACPI Error ",					KERN_ERROR,
		"The kernel has most probably detected an error while exeucting ACPI AML "
		"The error lists the ACPI driver module and the line number where the "
		"bug has been caught." },
	{ NULL,			0,            NULL }
};

static fwts_klog_pattern pm_error_warning_patterns[] = {
        { "PM: Failed to prepare device", 			KERN_ERROR, 
		"dpm_prepare() failed to prepare all non-sys devices for "
		"a system PM transition. The device should be listed in the "
		"error message." },
        { "PM: Some devices failed to power down", 		KERN_ERROR,
		"dpm_suspend_noirq failed because some devices did not power down " },
        { "PM: Some system devices failed to power down", 	KERN_ERROR,
		"sysdev_suspend failed because some system devices did not power down." },
        { "PM: Error", 						KERN_ERROR, NULL },
        { "PM: Some devices failed to power down", 		KERN_ERROR, NULL },
        { "PM: Restore failed, recovering", 			KERN_ERROR, 
		"A resume from hibernate failed when calling hibernation_restore()" },
        { "PM: Resume from disk failed", 			KERN_ERROR, NULL },
        { "PM: Not enough free memory", 			KERN_ERROR, 
		"There was not enough physical memory to be able to "
		"generate a hibernation image before dumping it to disc." },
        { "PM: Memory allocation failed", 			KERN_ERROR,
		"swusp_alloc() failed trying to allocate highmem and failing "
		"that non-highmem pages for the suspend image. There is "
		"probably just not enough free physcial memory available." },
        { "PM: Image mismatch", 				KERN_ERROR,
		"Mismatch in kernel version, system type, kernel release "
		"version or machine id between suspended kernel and resumed "
		"kernel." },
        { "PM: Some devices failed to power down", 		KERN_ERROR, NULL },
        { "PM: Some devices failed to suspend", 		KERN_ERROR, NULL },
        { "PM: can't read", 					KERN_ERROR, 
		"Testing suspend cannot read RTC" },
        { "PM: can't set", 					KERN_ERROR,
		"Testing suspend cannot set RTC" },
        { "PM: suspend test failed, error", 			KERN_ERROR, NULL },
        { "PM: can't test ", 					KERN_WARNING, NULL },
        { "PM: no wakealarm-capable RTC driver is ready", KERN_WARNING, NULL },
        { "PM: Adding page to bio failed at", 			KERN_ERROR, NULL },
        { "PM: Swap header not found", 				KERN_ERROR, NULL },
        { "PM: Cannot find swap device", 			KERN_ERROR, NULL },
        { "PM: Not enough free swap", 				KERN_ERROR, 
		"Hibernate failed because the swap parition was probably too small." },
        { "PM: Image device not initialised", 			KERN_ERROR, NULL },
        { "PM: Please power down manually", 			KERN_ERROR, NULL },
	{ "check_for_bios_corruption", 				KERN_WARNING,
		"The BIOS seems to be corrupting the first 64K of memory "
		"when doing suspend/resume. Setting bios_corruption_check=0 "
		"will disable this check." },
        { NULL,                   0,  NULL }
};

void fwts_klog_scan_patterns(fwts_framework *fw, char *line, char *prevline, void *private, int *warnings, int *errors)
{
	int i;

	fwts_klog_pattern *patterns = (fwts_klog_pattern *)private;

	for (i=0;patterns[i].pattern != NULL;i++) {
		if (strstr(line, patterns[i].pattern)) {
			if (patterns[i].type & KERN_WARNING) {
				fwts_log_info_verbatum(fw, "Kernel warning: %s", line);
				(*warnings)++;
				if (patterns[i].advice != NULL) {
					fwts_log_advice(fw, "Advice:\n%s", patterns[i].advice);
					fwts_log_nl(fw);
				}
			}
			if (patterns[i].type & KERN_ERROR) {
				fwts_log_info_verbatum(fw, "Kernel error: %s", line);
				(*errors)++;
				if (patterns[i].advice != NULL) {
					fwts_log_advice(fw, "Advice:\n%s", patterns[i].advice);
					fwts_log_nl(fw);
				}
			}
		}
	}
}

int fwts_klog_firmware_check(fwts_framework *fw, fwts_list *klog, int *warnings, int *errors)
{	
	return fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, firmware_error_warning_patterns, warnings, errors);
}

int fwts_klog_pm_check(fwts_framework *fw, fwts_list *klog, int *warnings, int *errors)
{
	return fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, pm_error_warning_patterns, warnings, errors);
}
