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
	LOG_RESULT	    = 0x00000001,
	LOG_ERROR           = 0x00000002,
	LOG_WARNING         = 0x00000004,
	LOG_DEBUG           = 0x00000008,
	LOG_INFO            = 0x00000010,
	LOG_SUMMARY         = 0x00000020,
	LOG_SEPARATOR       = 0x00000040,
	LOG_NEWLINE         = 0x00000080,
	LOG_ADVICE          = 0x00000100,
	LOG_HEADING	    = 0x00000200,
	LOG_TAG		    = 0x00000400,

	LOG_FIELD_MASK	    = 0x0000ffff,

	LOG_VERBATUM	    = 0x10000000,
} fwts_log_field;

typedef enum {
	LOG_LEVEL_NONE	    = 0x00000000,
	LOG_LEVEL_CRITICAL  = 0x00000001,
	LOG_LEVEL_HIGH      = 0x00000002,
	LOG_LEVEL_MEDIUM    = 0x00000004,
	LOG_LEVEL_LOW       = 0x00000008,
} fwts_log_level;

typedef struct log_t {
	int magic;
	FILE *fp;	
	char *owner;
} fwts_log;

fwts_log *fwts_log_open(const char* owner, const char *name, const char *mode);
int       fwts_log_close(fwts_log *log);
int       fwts_log_printf(fwts_log *log, const fwts_log_field field, const fwts_log_level level, const char *fmt, ...);
int       fwts_log_vprintf(fwts_log *log, const fwts_log_field field, const fwts_log_level level, const char *fmt, va_list args);
void      fwts_log_newline(fwts_log *log);
void      fwts_log_underline(fwts_log *log, const int ch);
void      fwts_log_set_field_filter(const char *str);
int       fwts_log_set_owner(fwts_log *log, const char *owner);
void      fwts_log_set_format(const char *str);
void      fwts_log_print_fields(void);
void      fwts_log_filter_set_field(const fwts_log_field filter);
void      fwts_log_filter_unset_field(const fwts_log_field filter);
char     *fwts_log_level_to_str(const fwts_log_level level);
int 	  fwts_log_line_number(void);
void	  fwts_log_set_line_width(const int width);



#define fwts_log_result(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_NONE, fmt, ## args) 

#define fwts_log_warning(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_WARNING, LOG_LEVEL_NONE, fmt, ## args) 

#define fwts_log_warning_verbatum(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_WARNING | LOG_VERBATUM, LOG_LEVEL_NONE, fmt, ## args) 
	
#define fwts_log_error(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_ERROR, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_error_verbatum(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_ERROR | LOG_VERBATUM, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_info(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_INFO, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_info_verbatum(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_INFO | LOG_VERBATUM, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_summary(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_SUMMARY, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_summary_verbatum(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_SUMMARY | LOG_VERBATUM, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_advice(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_ADVICE, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_heading(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_HEADING, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_tag(fw, fmt, args...)	\
	fwts_log_printf(fw->results, LOG_TAG | LOG_VERBATUM, LOG_LEVEL_NONE, fmt, ## args)

#define fwts_log_nl(fw) \
	fwts_log_printf(fw->results, LOG_NEWLINE, LOG_LEVEL_NONE, "")


#endif
