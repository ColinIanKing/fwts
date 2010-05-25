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

#ifndef __FRAMEWORK_H__
#define __FRAMEWORK_H__

#include <stdio.h>

#include "log.h"
#include "helpers.h"
#include "checkeuid.h"
#include "text_file.h"
#include "scan_dmesg_acpi_warnings.h"

#define FRAMEWORK_MAGIC	0x2af61aec


typedef struct framework {
	int magic;				/* identify struct magic */
	log *debug;				/* log to dump framework debug messages */
	log *results;				/* log for test results */
	char *results_logname;			/* filename of results log */
	char *debug_logname;			/* filename of framework debug log */
	char *iasl;				/* path to iasl */

	struct framework_ops const *ops;	
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
} framework;


typedef int (*framework_tests)(log *results, struct framework *framework);

typedef struct framework_ops {
	void (*headline)(log *);		/* Headline description of test */
	int (*init)(log *, framework *);	/* Initialise */
	int (*deinit)(log *, framework *);	/* De-init */		
	framework_tests *tests;			/* List of tests to run */
} framework_ops;

void framework_add(char *name, const framework_ops *ops);

framework *framework_init(int argc, char **argv,
                          const char *name,
                          const char *results_log);
framework *framework_open(int argc, char **argv, const char *name, const char *resultlog, const framework_ops *op);
void framework_close(framework *);
int framework_run_test(framework *framework, const char *name, const framework_ops *ops);
void framework_passed(framework *, const char *);
void framework_failed(framework *, const char *);

#define FRAMEWORK(name, ops)				\
							\
void name ## init (void) __attribute__ ((constructor));	\
							\
void name ## init (void)				\
{							\
	framework_add(# name, ops);			\
}							\
							
#endif
