/*
 * Copyright (C) 2010-2018 Canonical
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

#ifndef __FWTS_LOG_SCAN_H__
#define __FWTS_LOG_SCAN_H__

#include <regex.h>

typedef enum {
	FWTS_COMPARE_REGEX = 'r',
	FWTS_COMPARE_STRING = 's',
	FWTS_COMPARE_UNKNOWN = 'u'
} fwts_compare_mode;

typedef struct {
	fwts_compare_mode compare_mode;
	fwts_log_level level;
	const char *pattern;
	const char *advice;
	char *label;
	regex_t compiled;
	bool compiled_ok;
} fwts_log_pattern;

typedef void (*fwts_log_progress_func)(fwts_framework *fw, int percent);
typedef void (*fwts_log_scan_func)(fwts_framework *fw, char *line, int repeated, char *prevline, void *private, int *errors);

void       fwts_log_free(fwts_list *list);
fwts_list *fwts_log_find_changes(fwts_list *log_old, fwts_list *log_new);
char      *fwts_log_remove_timestamp(char *text);
int        fwts_log_scan(fwts_framework *fw, fwts_list *log, fwts_log_scan_func callback, fwts_log_progress_func progress, void *private, int *errors, bool remove_timestamp);
char *fwts_log_unique_label(const char *str, const char *label);
void       fwts_log_scan_patterns(fwts_framework *fw, char *line, int repeated, char *prevline, void *private, int *errors, const char *name, const char *advice);

#endif
