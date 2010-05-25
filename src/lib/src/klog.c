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

#include "log.h"
#include "klog.h"

int klog_clear(void)
{
	if (klogctl(5, NULL, 0) < 0) {
		return 1;
	}
	return 0;
}

char *klog_read(void)
{
	int len;
	char *buffer;

	if ((len = klogctl(10, NULL, 0)) < 0) {
		return NULL;
	}

	if ((buffer = malloc(len)) < 0) {		
		return NULL;
	}
	
	if (klogctl(3, buffer, len) < 0) {
		return NULL;
	}

	return buffer;
}

#define KERN_WARNING		0x00000001
#define KERN_ERROR		0x00000002

typedef struct {
	char *pattern;
	int  type;
} pattern;

typedef void (*scan_callback_t)(log *log, char *line, char *prevline, void *private, int *warnings, int *errors);

int klog_scan(log *log, char *klog, scan_callback_t callback, void *private, int *warnings, int *errors)
{
	*warnings = 0;
	*errors = 0;
	char prev[4096];
	char buffer[4096];

	if (!klog)
		return 1;

	*prev = '\0';

	while (*klog) {
		int i;
		char *bufptr = buffer;

		if ((klog[0] == '<') && (klog[2] == '>'))
			klog += 3;

		while (*klog && *klog !='\n' && (bufptr-buffer < sizeof(buffer)))
			*bufptr++ = *klog++;
		*bufptr = '\0';
		if (*klog == '\n')
			klog++;

		callback(log, buffer, prev, private, warnings, errors);
		strcpy(prev, buffer);
	}
	return 0;
}

/* List of errors and warnings */
static pattern error_warning_patterns[] = {
	"ACPI Warning ",	KERN_WARNING,
	"[Firmware Bug]:",	KERN_ERROR,
	"PCI: BIOS Bug:",	KERN_ERROR,
	"ACPI Error ",		KERN_ERROR,
	NULL,			0,
};

void scan_patterns(log *log, char *line, char *prevline, void *private, int *warnings, int *errors)
{
	int i;

	pattern *patterns = (pattern *)private;

	for (i=0;patterns[i].pattern != NULL;i++) {
		if (strstr(line, patterns[i].pattern)) {
			log_info(log, "%s\n", line);
			if (patterns[i].type & KERN_WARNING)
				(*warnings)++;
			if (patterns[i].type & KERN_ERROR)
				(*errors)++;
		}
	}
}

int klog_check(log *log, char *klog, int *warnings, int *errors)
{	
	return klog_scan(log, klog, scan_patterns, error_warning_patterns, warnings, errors);
}
