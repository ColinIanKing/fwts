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

#endif
