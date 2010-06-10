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

#ifndef __FWTS_LOG_H__
#define __FWTS_LOG_H__

#include <stdio.h>

#define LOG_MAGIC	0xfe23ab13

typedef enum {
	LOG_RESULT    = 0x00000001,
	LOG_ERROR     = 0x00000002,
	LOG_WARNING   = 0x00000004,
	LOG_DEBUG     = 0x00000008,
	LOG_INFO      = 0x00000010,
	LOG_SUMMARY   = 0x00000020,
	LOG_SEPARATOR = 0x00000040,
	LOG_NEWLINE   = 0x00000080,
	LOG_ADVICE    = 0x00000100,
} fwts_log_field;

typedef struct log_t {
	int magic;
	FILE *fp;	
	char *owner;
} fwts_log;

fwts_log *fwts_log_open(const char* owner, const char *name, const char *mode);
int       fwts_log_close(fwts_log *log);
int       fwts_log_printf(fwts_log *log, fwts_log_field field, const char *fmt, ...);
void      fwts_log_newline(fwts_log *log);
void      fwts_log_underline(fwts_log *log, int ch);
void      fwts_log_set_field_filter(char *str);
int       fwts_log_set_owner(fwts_log *log, const char *owner);
void      fwts_log_set_format(char *str);
void      fwts_log_print_fields(void);
void      fwts_log_filter_set_field(const fwts_log_field filter);
void      fwts_log_filter_unset_field(const fwts_log_field filter);

#define fwts_log_result(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_RESULT, fmt, ## args) 
	
#define fwts_log_error(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_ERROR, fmt, ## args)

#define fwts_log_warning(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_WARNING, fmt, ## args)

#define fwts_log_info(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_INFO, fmt, ## args)

#define fwts_log_summary(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_SUMMARY, fmt, ## args)

#define fwts_log_advice(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_ADVICE, fmt, ## args)

#define fwts_log_nl(fw) \
	fwts_log_printf(fw->results, LOG_NEWLINE, "\n");

#endif
