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

typedef struct fwts_framework {
	int magic;				/* identify struct magic */
	fwts_log *debug;			/* log to dump framework debug messages */
	fwts_log *results;			/* log for test results */
	char *results_logname;			/* filename of results log */
	char *debug_logname;			/* filename of framework debug log */
	char *iasl;				/* path to iasl */
	char *dmidecode;			/* path to dmidecode */
	char *dsdt;				/* path to dsdt file */
	char *klog;				/* path to dump of kernel log */
	int  s3_multiple;			/* number of s3 multiple tests to run */

	struct fwts_framework_ops const *ops;	
	int flags;

	/* per test stats */
	int tests_passed;		
	int tests_failed;
	int tests_aborted;
	int current_test;

	/* overall test stats */
	int total_tests_passed;		
	int total_tests_failed;
	int total_tests_aborted;
} fwts_framework;


typedef int (*fwts_framework_tests)(struct fwts_framework *framework);

typedef struct fwts_framework_ops {
	char *(*headline)(void);			/* Headline description of test */
	int (*init)(fwts_framework *);	/* Initialise */
	int (*deinit)(fwts_framework *);	/* De-init */		
	fwts_framework_tests *tests;			/* List of tests to run */
} fwts_framework_ops;

int  fwts_framework_args(int argc, char **argv);
void fwts_framework_add(char *name, const fwts_framework_ops *ops, const int priority);
void fwts_framework_passed(fwts_framework *, const char *fmt, ...);
void fwts_framework_failed(fwts_framework *, const char *fmt, ...);

#define TEST_EARLY	0
#define TEST_ANYTIME	50
#define TEST_LATE	75
#define TEST_LAST	100

#define FRAMEWORK(name, ops, priority)			\
							\
void name ## init (void) __attribute__ ((constructor));	\
							\
void name ## init (void)				\
{							\
	fwts_framework_add(# name, ops, priority);	\
}							\
							
#endif
