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
		return FWTS_ERROR;
	return FWTS_OK;
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

typedef void (*fwts_scan_callback_t)(fwts_framework *fw, char *line, char *prevline, void *private, int *errors);

int fwts_klog_scan(fwts_framework *fw, fwts_list *klog, fwts_scan_callback_t callback, void *private, int *errors)
{
	*errors = 0;
	char *prev;
	fwts_list_element *item;

	if (!klog)
		return FWTS_ERROR;

	prev = "";

	for (item = klog->head; item != NULL; item=item->next) {
		char *ptr = (char *)item->data;

		if ((ptr[0] == '<') && (ptr[2] == '>'))
			ptr += 3;

		callback(fw, ptr, prev, private, errors);
		prev = ptr;
	}
	return FWTS_OK;
}

#define FW_BUG	"[Firmware Bug]: "

/* List of common errors and warnings */

static fwts_klog_pattern common_error_warning_patterns[] = {
	{
		LOG_LEVEL_HIGH,
		"Temperature above threshold, cpu clock throttled",	
		NULL,
		"Test caused CPU temperature above critical threshold. Insufficient cooling?"
	},
	{
		LOG_LEVEL_HIGH,
		"ACPI: Handling Garbled _PRT entry",
		NULL,
		"BIOS has a garbled _PRT entry; source_name and source_index swapped."
	},
	{
		LOG_LEVEL_MEDIUM,
		"BIOS never enumerated boot CPU",
		NULL,
		"The boot processor is not enumerated!"
	},
	{
		LOG_LEVEL_HIGH,
		"*** Error: Return object type is incorrect",
		NULL,
		"Return object type is not the correct type, this is an AML error in the DSDT or SSDT"
	},
	{
		LOG_LEVEL_HIGH,
		"acpi_shpchprm",
		"_HPP fail",
		"Hotplug _HPP method failed"
	},
	{
		LOG_LEVEL_HIGH,
		"shpchp: acpi_pciehprm",
		"OSHP fail",
		"ACPI Hotplug OSHP method failed"
	},
	{
		LOG_LEVEL_HIGH,
		"shpchp: acpi_shpchprm:",
		"evaluate _BBN fail",
		"Hotplug _BBN method is missing"
	},
	{
		LOG_LEVEL_HIGH,
		"[PBST] Namespace lookup failure, AE_NOT_FOUND",
		NULL,
		"ACPI Namespace lookup failure reported"
	},
	{
		LOG_LEVEL_CRITICAL,
		"*** Error: Method reached maximum reentrancy limit",
		NULL,
		"ACPI method has reached reentrancy limit, this is a recursion bug in the AML"
	},
	{
		LOG_LEVEL_HIGH,
		"Error while parsing _PSD domain information",
		NULL,
		"_PSD domain information is corrupt!"
	},
	{
		LOG_LEVEL_CRITICAL,
		"Wrong _BBN value, reboot and use option 'pci=noacpi'",
		NULL,
		"The BIOS has wrong _BBN value, which will make PCI root bridge have wrong bus number"
	},
	{
		LOG_LEVEL_HIGH,
		"ACPI: lapic on CPU ",
		" stops in C2[C2]",
		"The local apic timer incorrectly stops during C2 idle state."
		"The ACPI specification forbids this and Linux needs the local "
		"APIC timer to work. The most likely cause of this is that the "
		"firmware uses a hardware C3 or C4 state that is mapped to "
		"the ACPI C2 state."
	},
	{
		LOG_LEVEL_MEDIUM,
		"Invalid _PCT data",
		NULL,
		"The ACPI _PCT data is invalid."
	},
	{
		LOG_LEVEL_HIGH,
		"*** Error: Method execution failed",
		NULL,
		"Execution of an ACPI AML method failed."
	},
	{
		LOG_LEVEL_HIGH,
		"Method parse/execution failed",
		"AE_NOT_FOUND",
		"Method parsing/execution failed."
	},
	{
		LOG_LEVEL_CRITICAL,
		"*** Error: Method execution failed",
		"AE_AML_METHOD_LIMIT",
		"ACPI method reached maximum reentrancy limit - infinite recursion in AML in DSTD or SSDT",
	},
	{
		LOG_LEVEL_HIGH,
		"Method execution failed",
		"AE_OWNER_ID_LIMIT",
		"Method failed to allocate owner ID."
	},
	{		
		LOG_LEVEL_HIGH,
		"Method execution failed",
		"AE_AML_BUFFER_LIMIT",
		"Method failed: ResourceSourceIndex is present but ResourceSource is not."
	},
	{
		LOG_LEVEL_CRITICAL,
		"Disabling IRQ",
		NULL,
		"The kernel detected an irq storm. This is most probably an IRQ routing bug."
	}
};

/* List of errors and warnings */
static fwts_klog_pattern firmware_error_warning_patterns[] = {
	{ 
		LOG_LEVEL_HIGH,	
		"ACPI Warning ", 
		NULL,
		"ACPI AML intepreter has found some non-conforming AML code. "
		"This should be investigated and fixed." 
	},
	{ 
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI: Invalid physical address in GAR", 
		NULL,
		"ACPI Generic Address is invalid" 
	},
	{
		LOG_LEVEL_MEDIUM,	
		FW_BUG "ACPI: Invalid bit width in GAR", 
		NULL,
		"ACPI Generic Address width must be 8, 16, 32 or 64" 
	},
	{
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI: Invalid address space type in GAR", 
		NULL,
		"ACPI Generic Address space type must be system memory or system IO space."
	},
	{
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI: no secondary bus range in _CRS", 
		NULL,
		"_CRS Method should return a secondary bus address for the "
		"status/command port. The kernel is having to guess this "
		"based on the _BBN or assume it's 0x00-0xff." 
	},
	{
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI: Invalid BIOS _PSS frequency", 
		NULL,
		"_PSS (Performance Supported States) package has an incorrectly "
		"define core frequency (first DWORD entry in the _PSS package)." 
	},
	{
		LOG_LEVEL_HIGH,
		FW_BUG "BIOS needs update for CPU frequency support", 
		NULL,
		"Having _PPC but missing frequencies (_PSS, _PCT) is a good hint "
		"that the BIOS is older than the CPU and does not know the CPU "
		"frequencies."
	},
	{
		LOG_LEVEL_CRITICAL,
		FW_BUG "Invalid critical threshold",
		NULL,
		"ACPI _CRT (Critical Trip Point) is returning a threshold "
		"lower than zero degrees Celsius which is clearly incorrect."
	},
	{
		LOG_LEVEL_CRITICAL,	
		FW_BUG "No valid trip found",
		NULL,
		"No valud ACPI _CRT (Critical Trip Point) was found."
	},
	{
		LOG_LEVEL_HIGH,
		FW_BUG "_BCQ is usef instead of _BQC",
		NULL,
		"ACPI Method _BCQ was defined (typo) instead of _BQC - this should be fixed."
		"however the kernel has detected this and is working around this typo."
	},
	{
		LOG_LEVEL_HIGH,
		"defines _DOD but not _DOS", 
		"ACPI Method _DOD (Enumerate all devices attached to display adapter) "
		"is defined but we should also have _DOS (Enable/Disable output switching) "
		"defined but it's been omitted. This can cause display switching issues."
	},
	{
		LOG_LEVEL_MEDIUM,
		FW_BUG "Duplicate ACPI video bus",
		NULL,
		"Try video module parameter video.allow_duplicates=1 if the current driver does't work."
	},
	{
		LOG_LEVEL_HIGH,
		"[Firmware Bug]:",
		NULL,
		"The kernel has detected a bug in the BIOS or ACPI which needs investigating and fixing."
	},
	{
		LOG_LEVEL_MEDIUM,
		"PCI: BIOS Bug:",
		NULL,
		NULL
	},
	{
		LOG_LEVEL_HIGH,
		"ACPI Error ",
		NULL,
		"The kernel has most probably detected an error while exeucting ACPI AML "
		"The error lists the ACPI driver module and the line number where the "
		"bug has been caught."
	},
	{
		0,
		NULL,
		NULL,			
		NULL
	}
};

static fwts_klog_pattern pm_error_warning_patterns[] = {
        {
		LOG_LEVEL_HIGH,
		"PM: Failed to prepare device",
		NULL,
		"dpm_prepare() failed to prepare all non-sys devices for "
		"a system PM transition. The device should be listed in the "
		"error message."
	},
        {
		LOG_LEVEL_HIGH,
		"PM: Some devices failed to power down",
		NULL,
		"dpm_suspend_noirq failed because some devices did not power down "
	},
        {
		LOG_LEVEL_HIGH,
		"PM: Some system devices failed to power down",
		NULL,
		"sysdev_suspend failed because some system devices did not power down."
	},
        {
		LOG_LEVEL_HIGH,
		"PM: Error",
		NULL,
		NULL
	},
        {
		LOG_LEVEL_HIGH,
		"PM: Some devices failed to power down",
		NULL,
		NULL
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Restore failed, recovering", 
		NULL,
		"A resume from hibernate failed when calling hibernation_restore()"
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Resume from disk failed",
		NULL,
		NULL
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Not enough free memory",
		NULL,
		"There was not enough physical memory to be able to "
		"generate a hibernation image before dumping it to disc."
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Memory allocation failed",
		NULL,
		"swusp_alloc() failed trying to allocate highmem and failing "
		"that non-highmem pages for the suspend image. There is "
		"probably just not enough free physcial memory available."
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Image mismatch",
		NULL,
		"Mismatch in kernel version, system type, kernel release "
		"version or machine id between suspended kernel and resumed "
		"kernel."
	},
        { 
		LOG_LEVEL_CRITICAL,
	  	"PM: Some devices failed to power down",
		NULL,
		NULL,
	},
        { 
		LOG_LEVEL_CRITICAL,
		"PM: Some devices failed to suspend", 
		NULL,
		NULL,
	},
        { 
		LOG_LEVEL_MEDIUM,
		"PM: can't read",
		NULL,
		"Testing suspend cannot read RTC"
	},
        {
		LOG_LEVEL_MEDIUM,
		"PM: can't set",
		NULL,
		"Testing suspend cannot set RTC"
	},
        {
		LOG_LEVEL_HIGH,
		"PM: suspend test failed, error",
		NULL,
		NULL
	},
        {	
		LOG_LEVEL_HIGH,
		"PM: can't test ", 						
		NULL,
		NULL
	},
        {
		LOG_LEVEL_MEDIUM,
		"PM: no wakealarm-capable RTC driver is ready",
		NULL,
		NULL
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Adding page to bio failed at",
		NULL,
		NULL
	},
        {	
		LOG_LEVEL_CRITICAL,
		"PM: Swap header not found", 
		NULL,
		NULL
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Cannot find swap device", 
		NULL,
		NULL,
	},
        {
		LOG_LEVEL_CRITICAL,
		"PM: Not enough free swap",
		NULL,
		"Hibernate failed because the swap parition was probably too small."
	},
        {
		LOG_LEVEL_HIGH,
		"PM: Image device not initialised",
		NULL,
		NULL
	},
        {
		LOG_LEVEL_HIGH,
		"PM: Please power down manually",
		NULL,
		NULL
	},
	{
		LOG_LEVEL_HIGH,
		"check_for_bios_corruption", 
		NULL,
		"The BIOS seems to be corrupting the first 64K of memory "
		"when doing suspend/resume. Setting bios_corruption_check=0 "
		"will disable this check."
	},
	{	
		LOG_LEVEL_HIGH,
		"WARNING: at",
		"hpet_next_event+",
		"Possibly an Intel I/O controller hub HPET Write Timing issue: "
		"A read transaction that immediately follows a write transaction "
		"to the HPET TIMn_COMP Timer 0 (108h), HPET MAIN_CNT (0F0h), or "
		"TIMn_CONF.bit 6 (100h) may return an incorrect value.  This is "
		"known to cause issues when coming out of S3."
	},
	{
		LOG_LEVEL_HIGH,
		"BUG: soft lockup",
		"stuck for 0s!",
		"Softlock errors that occur when coming out of S3 may be tripped "
		"by TSC warping.  It may be worth trying the notsc kernel parameter "
		"and repeating S3 tests to see if this solves the problem."
	},
	{	
		0,
		NULL,
		NULL,
		NULL
	}

};

void fwts_klog_scan_patterns(fwts_framework *fw, char *line, char *prevline, void *private, int *errors)
{
	int i;

	fwts_klog_pattern *patterns = (fwts_klog_pattern *)private;

	for (i=0;patterns[i].pat1 != NULL;i++) {
		int match = 0;

		if (patterns[i].pat2 == NULL)
			match = (strstr(line, patterns[i].pat1) != NULL);
		else
			match = (strstr(line, patterns[i].pat1) != NULL) &&
				(strstr(line, patterns[i].pat2) != NULL);
		
		if (match) {
			fwts_failed_level(fw, patterns[i].level, "Kernel message: %s", line);
			(*errors)++;
			if (patterns[i].advice != NULL) {
				fwts_log_advice(fw, "Advice:\n%s", patterns[i].advice);
				fwts_log_nl(fw);
			}
		}
	}
}

int fwts_klog_firmware_check(fwts_framework *fw, fwts_list *klog, int *errors)
{	
	return fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, firmware_error_warning_patterns, errors);
}

int fwts_klog_pm_check(fwts_framework *fw, fwts_list *klog, int *errors)
{
	return fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, pm_error_warning_patterns, errors);
}

int fwts_klog_common_check(fwts_framework *fw, fwts_list *klog, int *errors)
{
	return fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, common_error_warning_patterns, errors);
}
