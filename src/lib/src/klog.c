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

/* List of errors and warnings */
static fwts_klog_pattern firmware_error_warning_patterns[] = {
	{ "ACPI Warning ",	KERN_WARNING },
	{ "[Firmware Bug]:",	KERN_ERROR },
	{ "PCI: BIOS Bug:",	KERN_ERROR },
	{ "ACPI Error ",	KERN_ERROR },
	{ NULL,			0 }
};

static fwts_klog_pattern pm_error_warning_patterns[] = {
        { "PM: Failed to prepare device", 		KERN_ERROR },
        { "PM: Some devices failed to power down", 	KERN_ERROR },
        { "PM: Some system devices failed to power down", KERN_ERROR },
        { "PM: Error", 					KERN_ERROR },
        { "PM: Some devices failed to power down", 	KERN_ERROR },
        { "PM: Restore failed, recovering", 		KERN_ERROR },
        { "PM: Resume from disk failed", 		KERN_ERROR },
        { "PM: Not enough free memory", 		KERN_ERROR },
        { "PM: Memory allocation failed", 		KERN_ERROR },
        { "PM: Image mismatch", 			KERN_ERROR },
        { "PM: Some devices failed to power down", 	KERN_ERROR },
        { "PM: Some devices failed to suspend", 	KERN_ERROR },
        { "PM: can't read", 				KERN_ERROR },
        { "PM: can't set", 				KERN_ERROR },
        { "PM: suspend test failed, error", 		KERN_ERROR },
        { "PM: can't test ", 				KERN_WARNING },
        { "PM: no wakealarm-capable RTC driver is ready", KERN_WARNING },
        { "PM: Adding page to bio failed at", 		KERN_ERROR },
        { "PM: Swap header not found", 			KERN_ERROR },
        { "PM: Cannot find swap device", 		KERN_ERROR },
        { "PM: Not enough free swap", 			KERN_ERROR },
        { "PM: Image device not initialised", 		KERN_ERROR },
        { "PM: Please power down manually", 		KERN_ERROR },
        { NULL,                   0, }
};

void fwts_klog_scan_patterns(fwts_framework *fw, char *line, char *prevline, void *private, int *warnings, int *errors)
{
	int i;

	fwts_klog_pattern *patterns = (fwts_klog_pattern *)private;

	for (i=0;patterns[i].pattern != NULL;i++) {
		if (strstr(line, patterns[i].pattern)) {
			fwts_log_info(fw, "%s\n", line);
			if (patterns[i].type & KERN_WARNING)
				(*warnings)++;
			if (patterns[i].type & KERN_ERROR)
				(*errors)++;
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
