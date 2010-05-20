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

char *read_klog(void)
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

/* List of errors and warnings */
static pattern error_warning_patterns[] = {
	"ACPI Warning ",	KERN_WARNING,
	"[Firmware Bug]:",	KERN_ERROR,
	"PCI: BIOS Bug:",	KERN_ERROR,
	"ACPI Error ",		KERN_ERROR,
	NULL,			0,
};

static int scan_klog(log *log, char *klog, pattern *patterns, int *warnings, int *errors)
{
	*warnings = 0;
	*errors = 0;

	char buffer[4096];

	if (!klog)
		return 1;

	while (*klog) {
		int i;
		char *bufptr = buffer;

		while (*klog && *klog !='\n' && (bufptr-buffer < sizeof(buffer)))
			*bufptr++ = *klog++;
		*bufptr = '\0';
		if (*klog == '\n')
			klog++;

		for (i=0;patterns[i].pattern != NULL;i++) {
			if (strstr(buffer, patterns[i].pattern)) {
				log_info(log, "%s\n", buffer);
				if (patterns[i].type & KERN_WARNING)
					(*warnings)++;
				if (patterns[i].type & KERN_ERROR)
					(*errors)++;
			}
		}
	}
	return 0;
}

int check_klog(log *log, char *klog, int *warnings, int *errors)
{	
	return scan_klog(log, klog, error_warning_patterns, warnings, errors);
}
