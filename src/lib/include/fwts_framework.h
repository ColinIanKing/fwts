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

#ifndef __FWTS_FRAMEWORK_H__
#define __FWTS_FRAMEWORK_H__

#include <stdio.h>
#include "fwts_log.h"

#define FRAMEWORK_MAGIC	0x2af61aec

#define FRAMEWORK_FLAGS_STDOUT_SUMMARY          0x00000001
#define FRAMEWORK_FLAGS_FRAMEWORK_DEBUG         0x00000002
#define FRAMEWORK_FLAGS_SHOW_PROGRESS           0x00000004
#define FRAMEWORK_FLAGS_NO_S3                   0x00000008
#define FRAMEWORK_FLAGS_NO_S4                   0x00000010

#define FRAMEWORK_FLAGS_DEFAULT			0

typedef struct {
	int passed;
	int failed;
	int aborted;
	int warning;
} fwts_results;

typedef struct {
	int magic;				/* identify struct magic */
	fwts_log *debug;			/* log to dump framework debug messages */
	fwts_log *results;			/* log for test results */
	char *results_logname;			/* filename of results log */
	char *debug_logname;			/* filename of framework debug log */

	char *iasl;				/* path to iasl */
	char *dmidecode;			/* path to dmidecode */
	char *acpidump;				/* path to acpidump */
	char *lspci;				/* path to lspci */

	char *dsdt;				/* path to dsdt file */
	char *klog;				/* path to dump of kernel log */
	int  s3_multiple;			/* number of s3 multiple tests to run */

	struct fwts_framework_ops const *ops;	
	int flags;

	int current_test;
	char *current_test_name;

	/* per test stats */
	fwts_results	sub_tests;
	fwts_results	test_run;
	fwts_results	total;
} fwts_framework;

typedef int (*fwts_framework_tests)(fwts_framework *framework);

typedef struct fwts_framework_ops {
	char *(*headline)(void);	/* Headline description of test */
	int (*init)(fwts_framework *);	/* Initialise */
	int (*deinit)(fwts_framework *);/* De-init */		
	fwts_framework_tests *tests;	/* List of tests to run */
} fwts_framework_ops;

int  fwts_framework_args(int argc, char **argv);
void fwts_framework_test_add(char *name, const fwts_framework_ops *ops, const int priority);
void fwts_framework_passed(fwts_framework *, const char *fmt, ...);
void fwts_framework_failed(fwts_framework *, fwts_log_level level, const char *fmt, ...);
void fwts_framework_warning(fwts_framework *, const char *fmt, ...);
void fwts_framework_advice(fwts_framework *, const char *fmt, ...);

#define fwts_passed(fw, args...)	fwts_framework_passed(fw, ## args)
#define fwts_failed(fw, args...)	fwts_framework_failed(fw, LOG_LEVEL_MEDIUM, ## args)
#define fwts_failed_level(fw, level, args...) \
					fwts_framework_failed(fw, level, ## args)
#define fwts_failed_critical(fw, args...)	\
					fwts_framework_failed(fw, LOG_LEVEL_CRITICAL, ## args)
#define fwts_failed_high(fw, args...)	\
					fwts_framework_failed(fw, LOG_LEVEL_HIGH, ## args)
#define fwts_failed_medium(fw, args...)	\
					fwts_framework_failed(fw, LOG_LEVEL_MEDIUM, ## args)
#define fwts_failed_low(fw, args...)	\
					fwts_framework_failed(fw, LOG_LEVEL_LOW, ## args)

#define fwts_warning(fw, args...)	fwts_framework_warning(fw, ## args)

#define fwts_advice(fw, args...)	fwts_framework_advice(fw, ## args);

static inline int fwts_tests_passed(fwts_framework *fw)
{
	return ((fw->sub_tests.failed + 
		 fw->sub_tests.warning + 
		 fw->sub_tests.aborted) == 0);
}

#define TEST_FIRST	0
#define TEST_EARLY	10
#define TEST_ANYTIME	50
#define TEST_LATE	75
#define TEST_LAST	100

#define FRAMEWORK(name, ops, priority)			\
							\
void name ## init (void) __attribute__ ((constructor));	\
							\
void name ## init (void)				\
{							\
	fwts_framework_test_add(# name, ops, priority);	\
}							\
							
#endif
