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

#define FRAMEWORK_MAGIC	0x2af61aec


typedef struct framework {
	int (*close)(struct framework *);
	int (*run_test)(struct framework *);
	int (*passed)(struct framework *, const char *test);
	int (*failed)(struct framework *, const char *test);

	int magic;				/* identify struct magic */
	log *debug;				/* log to dump framework debug messages */
	log *results;				/* log for test results */
	struct framework_ops const *ops;	

	/* test stats */
	int tests_passed;		
	int tests_failed;
	int tests_aborted;
	int current_test;

	/* test framework private data */
	void *private;
} framework;


typedef int (*framework_tests)(log *results, struct framework *framework);

typedef struct framework_ops {
	void (*headline)(log *);		/* Headline description of test */
	int (*init)(log *, framework *);	/* Initialise */
	int (*deinit)(log *, framework *);	/* De-init */		
	framework_tests *tests;			/* List of tests to run */
} framework_ops;

framework *framework_open(const char *name, const char *resultlog, const framework_ops *ops, void *private);

/* Standalone */

#define FRAMEWORK(name, resultlog, ops, private)	\
main(int argc, char **argv) {				\
	framework *fw;					\
	fw = framework_open(# name, resultlog, ops, private);\
	fw->run_test(fw);				\
	fw->close(fw);					\
}

#endif
