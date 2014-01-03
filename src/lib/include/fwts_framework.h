/*
 * Copyright (C) 2010-2014 Canonical
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
#include <string.h>
#include <stdbool.h>

#include "fwts_log.h"
#include "fwts_list.h"
#include "fwts_acpica_mode.h"

#define FWTS_FRAMEWORK_MAGIC	0x2af61aec

typedef enum {
	FWTS_FLAG_DEFAULT			= 0x00000000,
	FWTS_FLAG_STDOUT_SUMMARY		= 0x00000001,
	FWTS_FLAG_SHOW_PROGRESS			= 0x00000002,
	FWTS_FLAG_FORCE_CLEAN			= 0x00000004,
	FWTS_FLAG_SHOW_TESTS			= 0x00000008,
	FWTS_FLAG_SHOW_PROGRESS_DIALOG		= 0x00000010,
	FWTS_FLAG_ACPICA_DEBUG			= 0x00000020,
	FWTS_FLAG_BATCH				= 0x00001000,
	FWTS_FLAG_INTERACTIVE			= 0x00002000,
	FWTS_FLAG_BATCH_EXPERIMENTAL		= 0x00004000,
	FWTS_FLAG_INTERACTIVE_EXPERIMENTAL	= 0x00008000,
	FWTS_FLAG_POWER_STATES			= 0x00010000,
	FWTS_FLAG_ROOT_PRIV			= 0x00020000,
	FWTS_FLAG_UNSAFE			= 0x00040000,
	FWTS_FLAG_TEST_BIOS			= 0x01000000,
	FWTS_FLAG_TEST_UEFI			= 0x02000000,
	FWTS_FLAG_TEST_ACPI			= 0x04000000,
	FWTS_FLAG_UTILS				= 0x08000000,
	FWTS_FLAG_QUIET				= 0x10000000,
	FWTS_FLAG_SHOW_TESTS_FULL		= 0x20000000,
} fwts_framework_flags;

#define FWTS_FLAG_TEST_MASK		\
	(FWTS_FLAG_TEST_BIOS |	\
	 FWTS_FLAG_TEST_UEFI |	\
	 FWTS_FLAG_TEST_ACPI)

/*
 *  Test results
 */
typedef struct {
	uint32_t passed;
	uint32_t failed;
	uint32_t aborted;
	uint32_t warning;
	uint32_t skipped;
	uint32_t infoonly;
} fwts_results;

/*
 *  Where to schedule a test, priority sorted lowest first, highest last
 */
typedef enum {
	FWTS_TEST_FIRST	= 0,
	FWTS_TEST_EARLY	= 1,
	FWTS_TEST_ANYTIME = 2,
	FWTS_TEST_LATE = 3,
	FWTS_TEST_LAST = 4
} fwts_priority;

static inline void fwts_results_zero(fwts_results *results)
{
	memset(results, 0, sizeof(fwts_results));
}


static inline void fwts_framework_summate_results(fwts_results *total, fwts_results *increment)
{
	total->aborted  += increment->aborted;
	total->failed   += increment->failed;
	total->passed   += increment->passed;
	total->warning  += increment->warning;
	total->skipped  += increment->skipped;
	total->infoonly += increment->infoonly;
}

/*
 *  Test framework context
 */
typedef struct {
	uint32_t magic;				/* identify struct magic */
	fwts_log *results;			/* log for test results */
	char *results_logname;			/* filename of results log */

	char *lspci;				/* path to lspci */

	char *acpi_table_path;			/* path to raw ACPI tables */
	char *acpi_table_acpidump_file;		/* path to ACPI dump file */
	char *klog;				/* path to dump of kernel log */
	char *json_data_path;			/* path to application json data files, e.g. json klog data */

	fwts_framework_flags flags;

	int current_minor_test_num;		/* Nth minor test being run in a test module */
	const char *current_minor_test_name;	/* Name of current minor test being run */
	int current_major_test_num;		/* Nth major test being currently run */
	int major_tests_total;			/* Total number of major tests */

	struct fwts_framework_test *current_major_test; /* current test */

	fwts_results	minor_tests;		/* results for each minor test */
	fwts_results	total;			/* totals over all tests */

	uint32_t	total_run;		/* total number of major tests run */

	int minor_test_progress;		/* Percentage completion of current test */
	bool print_summary;			/* Print summary of results at end of test runs */
	fwts_log_level failed_level;		/* Bit mask of failed levels in test run */

	int firmware_type;			/* Type of firmware */
	bool show_progress;			/* Show progress while running current test */

	fwts_log_type	log_type;		/* Output log type, default is plain text ASCII */

	fwts_list errors_filter_keep;		/* Results to keep, empty = keep all */
	fwts_list errors_filter_discard;	/* Results to discard, empty = discard none */
	bool error_filtered_out;		/* True if a klog message has been filtered out */
	fwts_acpica_mode acpica_mode;		/* ACPICA mode flags */
	void *rsdp;				/* ACPI RSDP address */
} fwts_framework;

typedef struct {
	char *opt;	/* option */
	char *info;	/* what it does */
} fwts_syntax_info;

typedef int (*fwts_framework_minor_test_func)(fwts_framework *framework);

typedef struct {
	fwts_framework_minor_test_func test_func;/* Minor test to run */
	const char  *name;			/* Name of minor test */
} fwts_framework_minor_test;

#include "fwts_args.h"

typedef struct fwts_framework_ops {
	char *description;			/* description of test */
	int (*init)(fwts_framework *);		/* Initialise */
	int (*deinit)(fwts_framework *);	/* De-init */		
	int (*getopts)(fwts_framework *, int argc, char **argv);	/* Arg handling */
	fwts_option *options;
	fwts_args_optarg_handler options_handler;
	fwts_args_optarg_check   options_check;
	fwts_framework_minor_test *minor_tests;	/* NULL terminated array of minor tests to run */
	int total_tests;			/* Number of tests to run */
} fwts_framework_ops;

typedef struct fwts_framework_test {
	const char *name;
	fwts_framework_ops *ops;
	fwts_priority priority;
	fwts_framework_flags flags;
	fwts_results results;			/* Per test results */
	bool	    was_run;
} fwts_framework_test;

int  fwts_framework_args(const int argc, char **argv);
void fwts_framework_test_add(const char *name, fwts_framework_ops *ops, const fwts_priority priority, const fwts_framework_flags flags);
int  fwts_framework_compare_test_name(void *, void *);
void fwts_framework_show_version(FILE *fp, const char *name);

void fwts_framework_passed(fwts_framework *, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
void fwts_framework_failed(fwts_framework *, fwts_log_level level, const char *fmt, ...)
	__attribute__((format(printf, 3, 4)));
void fwts_framework_warning(fwts_framework *, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
void fwts_framework_advice(fwts_framework *, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
void fwts_framework_skipped(fwts_framework *, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
void fwts_framework_aborted(fwts_framework *, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
void fwts_framework_infoonly(fwts_framework *fw);
void fwts_framework_minor_test_progress(fwts_framework *fw, const int percent, const char *message);
void fwts_error_inc(fwts_framework *fw, const char *label, int *count);

void fwts_framework_log(fwts_framework *fw,
	fwts_log_field field,
	const char *label,
        fwts_log_level level,
        uint32_t *count,
        const char *fmt, ...)  __attribute__((format(printf, 6, 7)));

#define fwts_progress(fw, percent)	fwts_framework_minor_test_progress(fw, percent, "")
#define fwts_progress_message(fw, percent, message)	\
	fwts_framework_minor_test_progress(fw, percent, message);

/* Helpers to report tests results */
#define fwts_passed(fw, fmt, args...) \
	fwts_framework_log(fw, LOG_PASSED, NULL, LOG_LEVEL_NONE, &fw->minor_tests.passed, fmt, ## args)
#define fwts_failed(fw, level, label, fmt, args...) \
	fwts_framework_log(fw, LOG_FAILED, label, level, &fw->minor_tests.failed, fmt, ## args)
#define fwts_warning(fw, fmt, args...) \
	fwts_framework_log(fw, LOG_WARNING, NULL, LOG_LEVEL_MEDIUM, &fw->minor_tests.warning, fmt, ## args)
#define fwts_advice(fw, fmt, args...) \
	fwts_framework_log(fw, LOG_ADVICE, NULL, LOG_LEVEL_NONE, NULL, fmt, ## args)
#define fwts_skipped(fw, fmt, args...) \
	fwts_framework_log(fw, LOG_SKIPPED, NULL, LOG_LEVEL_MEDIUM, &fw->minor_tests.skipped, fmt, ## args)
#define fwts_aborted(fw, fmt, args...) \
	fwts_framework_log(fw, LOG_ABORTED, NULL, LOG_LEVEL_MEDIUM, &fw->minor_tests.aborted, fmt, ## args)
#define fwts_infoonly(fw) \
	fwts_framework_log(fw, LOG_INFOONLY, NULL, LOG_LEVEL_NONE, &fw->minor_tests.infoonly, NULL)

static inline int fwts_tests_passed(const fwts_framework *fw)
{
	return ((fw->minor_tests.failed +
		 fw->minor_tests.warning +
		 fw->minor_tests.aborted) == 0);
}

/*
 *  Batch (run w/o interaction) or interactive (requires user interaction) flags
 */
#define FWTS_TEST_INTERACTIVE(flags)	\
	(flags & (FWTS_FLAG_INTERACTIVE | \
		  FWTS_FLAG_INTERACTIVE_EXPERIMENTAL))

#define FWTS_ARRAY_LEN(s) (sizeof(s)/sizeof(s[0]))

/*
 * FWTS_ASSERT(test, message) 
 *	compile time assertion that throws a division by zero
 *	error to stop compilation if condition "test" is not true.
 * 	See http://www.pixelbeat.org/programming/gcc/static_assert.html 
 *
 */
#define FWTS_CONCAT(a, b) a ## b
#define FWTS_CONCAT_EXPAND(a,b) FWTS_CONCAT(a, b)
#define FWTS_ASSERT(e, m) 	\
enum { FWTS_CONCAT_EXPAND(FWTS_ASSERT_ ## m ## _in_line_, __LINE__) = 1 / !!(e) }

#define FWTS_REGISTER(name, ops, priority, flags)		\
/* Ensure name is not too long */				\
FWTS_ASSERT(FWTS_ARRAY_LEN(name) < 16,				\
	fwts_register_name_too_long);				\
								\
static void __test_init (void) __attribute__ ((constructor));	\
								\
static void __test_init (void)					\
{								\
	fwts_framework_test_add(name, ops, priority, flags);	\
}
							
#endif
