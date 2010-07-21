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

#define FWTS_FRAMEWORK_MAGIC	0x2af61aec

typedef enum {
	FWTS_FRAMEWORK_FLAGS_DEFAULT	               = 0x00000000,
	FWTS_FRAMEWORK_FLAGS_STDOUT_SUMMARY            = 0x00000001,
	FWTS_FRAMEWORK_FLAGS_FRAMEWORK_DEBUG           = 0x00000002,
	FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS             = 0x00000004,
	FWTS_FRAMEWORK_FLAGS_NO_S3                     = 0x00000008,
	FWTS_FRAMEWORK_FLAGS_NO_S4                     = 0x00000010,
	FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN               = 0x00000020,
	FWTS_FRAMEWORK_FLAGS_SHOW_TESTS	               = 0x00000040,
	FWTS_FRAMEWORK_FLAGS_BATCH	               = 0x00001000,
	FWTS_FRAMEWORK_FLAGS_INTERACTIVE               = 0x00002000,
	FWTS_FRAMEWORK_FLAGS_BATCH_EXPERIMENTAL        = 0x00004000,
	FWTS_FRAMEWORK_FLAGS_INTERACTIVE_EXPERIMENTAL  = 0x00008000,
} fwts_framework_flags;

/*
 *  Test results
 */
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
	char *lspci;				/* path to lspci */

	char *acpi_table_path;			/* path to raw ACPI tables */
	char *klog;				/* path to dump of kernel log */
	int  s3_multiple;			/* number of s3 multiple tests to run */
	int  s4_multiple;			/* number of s4 multiple tests to run */

	fwts_framework_flags flags;

	int current_test;			/* Nth test being run in a test module */
	char *current_test_name;		/* name of current test */
	struct fwts_framework_ops const *current_ops;	

	/* per test stats */
	fwts_results	sub_tests;		/* results for each test in test module */
	fwts_results	test_run;		/* totals over all the tests (1 or more) in a module */
	fwts_results	total;			/* totals over all tests */

	int sub_test_progress;			/* Percentage completion of current test */
} fwts_framework;

typedef int (*fwts_framework_tests)(fwts_framework *framework);

typedef struct fwts_framework_ops {
	char *(*headline)(void);		/* Headline description of test */
	int (*init)(fwts_framework *);		/* Initialise */
	int (*deinit)(fwts_framework *);	/* De-init */		
	fwts_framework_tests *tests;		/* List of tests to run */
	int total_tests;			/* Number of tests to run */
} fwts_framework_ops;

int  fwts_framework_args(const int argc, char * const *argv);
void fwts_framework_test_add(const char *name, fwts_framework_ops *ops, const int priority, const int flags);
void fwts_framework_passed(fwts_framework *, const char *fmt, ...);
void fwts_framework_failed(fwts_framework *, fwts_log_level level, const char *fmt, ...);
void fwts_framework_warning(fwts_framework *, const char *fmt, ...);
void fwts_framework_advice(fwts_framework *, const char *fmt, ...);
void fwts_framework_sub_test_progress(fwts_framework *fw, const int percent);

#define fwts_progress(fw, percent)	fwts_framework_sub_test_progress(fw, percent)

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

#define fwts_advice(fw, args...)	fwts_framework_advice(fw, ## args)

static inline int fwts_tests_passed(const fwts_framework *fw)
{
	return ((fw->sub_tests.failed + 
		 fw->sub_tests.warning + 
		 fw->sub_tests.aborted) == 0);
}

/*
 *  Where to schedule a test, priority sorted lowest first, highest last
 */
#define FWTS_TEST_FIRST		0		
#define FWTS_TEST_EARLY		10
#define FWTS_TEST_ANYTIME	50
#define FWTS_TEST_LATE		75
#define FWTS_TEST_LAST		100

/*
 *  Batch (run w/o interaction) or interactive (requires user interaction) flags
 */
#define FWTS_BATCH 			FWTS_FRAMEWORK_FLAGS_BATCH
#define FWTS_INTERACTIVE 		FWTS_FRAMEWORK_FLAGS_INTERACTIVE
#define FWTS_BATCH_EXPERIMENTAL		FWTS_FRAMEWORK_FLAGS_BATCH_EXPERIMENTAL
#define FWTS_INTERACTIVE_EXPERIMENTAL	FWTS_FRAMEWORK_FLAGS_INTERACTIVE_EXPERIMENTAL

#define FWTS_REGISTER(name, ops, priority, flags)		\
								\
void name ## init (void) __attribute__ ((constructor));		\
								\
void name ## init (void)					\
{								\
	fwts_framework_test_add(# name, ops, priority, flags);	\
}								\
							
#endif
