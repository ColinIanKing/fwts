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

#define LOG_RESULT	"RES"
#define LOG_ERROR	"ERR"
#define LOG_WARNING	"WRN"
#define LOG_DEBUG	"DBG"
#define LOG_INFO	"INF"
#define LOG_SUMMARY	"SUM"

typedef struct log_t {
	int magic;
	FILE *fp;	
	char *owner;
} log;

log *log_open(const char* owner, const char *name, const char *mode);
int  log_close(log *log);
int  log_printf(log *log, char *prefix, const char *fmt, ...);
void log_newline(log *log);
void log_underline(log *log, int ch);

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
