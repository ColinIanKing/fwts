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

	int magic;
	log *debug;
	log *results;
	struct framework_ops const *ops;
	int tests_passed;
	int tests_failed;
	int tests_aborted;
	int current_test;
	void *private;
} framework;


typedef int (*framework_tests)(log *results, struct framework *framework);

typedef struct framework_ops {
	void (*headline)(log *);	/* Headline description of test */
	int (*init)(log *, framework *);	/* Initialise */
	int (*deinit)(log *, framework *);	/* De-init */		
	framework_tests *tests;		/* List of tests to run */
} framework_ops;

framework *framework_open(const char *name, const char *resultlog, const framework_ops *ops, void *private);

/* Standalone */

#define FRAMEWORK(name, resultlog, ops, private)	\
main(int argc, char **argv) {			\
	framework *fw;				\
	fw = framework_open(# name, resultlog, ops, private);\
	fw->run_test(fw);			\
	fw->close(fw);				\
}

#endif
