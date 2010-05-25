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

#ifndef __LOG_H__
#define __LOG_H__

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
} log_field;

typedef struct log_t {
	int magic;
	FILE *fp;	
	char *owner;
} log;

log *log_open(const char* owner, const char *name, const char *mode);
int  log_close(log *log);
int  log_printf(log *log, log_field field, const char *fmt, ...);
void log_newline(log *log);
void log_underline(log *log, int ch);
void log_set_field_filter(char *str);
int  log_set_owner(log *log, const char *owner);
void log_print_fields(void);
void log_filter_set_field(const log_field filter);
void log_filter_unset_field(const log_field filter);

#define log_result(results, fmt, args...)	\
	log_printf(results, LOG_RESULT, fmt, ## args) 
	
#define log_error(results, fmt, args...)	\
	log_printf(results, LOG_ERROR, fmt, ## args)

#define log_warning(results, fmt, args...)	\
	log_printf(results, LOG_WARNING, fmt, ## args)

#define log_info(results, fmt, args...)	\
	log_printf(results, LOG_INFO, fmt, ## args)

#define log_summary(results, fmt, args...)	\
	log_printf(results, LOG_SUMMARY, fmt, ## args)

#endif
