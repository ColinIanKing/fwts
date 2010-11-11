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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <getopt.h>
#include <sys/utsname.h>

#include "fwts.h"

#define RESULTS_LOG	"results.log"

#define FWTS_RUN_ALL_FLAGS		\
	(FWTS_BATCH |			\
	 FWTS_INTERACTIVE |		\
	 FWTS_BATCH_EXPERIMENTAL |	\
	 FWTS_INTERACTIVE_EXPERIMENTAL |\
	 FWTS_POWER_STATES |		\
	 FWTS_UTILS)

#define LOGFILE(name1, name2)	\
	(name1 != NULL) ? name1 : name2

enum {
	FWTS_PASSED_TEXT,
	FWTS_FAILED_TEXT,
	FWTS_FAILED_LOW_TEXT,
	FWTS_FAILED_HIGH_TEXT,
	FWTS_FAILED_MEDIUM_TEXT,
	FWTS_FAILED_CRITICAL_TEXT,
	FWTS_WARNING_TEXT,
	FWTS_ERROR_TEXT,
	FWTS_ADVICE_TEXT,
	FWTS_SKIPPED_TEXT,
	FWTS_ABORTED_TEXT,
	FWTS_FRAMEWORK_DEBUG
};

static fwts_list *fwts_framework_test_list;

typedef struct {
	const int env_id;
	const char *env_name;
	const char *env_default;
	char *env_value;
} fwts_framework_setting;

#define ID_NAME(id)	id, # id

static fwts_framework_setting fwts_framework_settings[] = {
	{ ID_NAME(FWTS_PASSED_TEXT),		"PASSED",  NULL },
	{ ID_NAME(FWTS_FAILED_TEXT),		"FAILED",  NULL },
	{ ID_NAME(FWTS_FAILED_LOW_TEXT),	"FAILED_LOW", NULL },
	{ ID_NAME(FWTS_FAILED_HIGH_TEXT),	"FAILED_HIGH", NULL },
	{ ID_NAME(FWTS_FAILED_MEDIUM_TEXT),	"FAILED_MEDIUM", NULL },
	{ ID_NAME(FWTS_FAILED_CRITICAL_TEXT),	"FAILED_CRITICAL", NULL },
	{ ID_NAME(FWTS_WARNING_TEXT),		"WARNING", NULL },
	{ ID_NAME(FWTS_ERROR_TEXT),		"ERROR",   NULL },
	{ ID_NAME(FWTS_ADVICE_TEXT),		"ADVICE",  NULL },
	{ ID_NAME(FWTS_SKIPPED_TEXT),		"SKIPPED", NULL },
	{ ID_NAME(FWTS_ABORTED_TEXT),		"ABORTED", NULL },
	{ ID_NAME(FWTS_FRAMEWORK_DEBUG),	"off",     NULL },
};

/*
 *  fwts_framework_compare_priority()
 *	used to register tests sorted on run priority
 */
static int fwts_framework_compare_priority(void *data1, void *data2)
{
	fwts_framework_test *test1 = (fwts_framework_test *)data1;
	fwts_framework_test *test2 = (fwts_framework_test *)data2;

	return (test1->priority - test2->priority);
}

/*
 * fwts_framework_test_add()  
 *    register a test, called by FWTS_REGISTER() macro
 */
void fwts_framework_test_add(const char *name, 
	fwts_framework_ops *ops, 
	const int priority, 
	const int flags)
{
	fwts_framework_test *new_test;

	if (flags & ~FWTS_RUN_ALL_FLAGS) {
		fprintf(stderr, "Test %s flags must be FWTS_BATCH, FWTS_INTERACTIVE, FWTS_BATCH_EXPERIMENTAL, \n"
			        "FWTS_INTERACTIVE_EXPERIMENTAL or FWTS_POWER_STATES, got %x\n", name, flags);
		exit(EXIT_FAILURE);
	}

	if (fwts_framework_test_list == NULL) {
		fwts_framework_test_list = fwts_list_init();
		if (fwts_framework_test_list == NULL) {
			fprintf(stderr, "FATAL: Could not allocate memory setting up test framework\n");
			exit(EXIT_FAILURE);
		}
	}

	/* This happens early, so if it goes wrong, bail out */
	if ((new_test = calloc(1, sizeof(fwts_framework_test))) == NULL) {
		fprintf(stderr, "FATAL: Could not allocate memory adding tests to test framework\n");
		exit(EXIT_FAILURE);
	}

	/* Total up minor tests in this test */
	for (ops->total_tests = 0; ops->minor_tests[ops->total_tests].test_func != NULL; ops->total_tests++)
		;

	new_test->name = name;
	new_test->ops  = ops;
	new_test->priority = priority;
	new_test->flags = flags;

	/* Add test, sorted on run order priority */
	fwts_list_add_ordered(fwts_framework_test_list, new_test, fwts_framework_compare_priority);
}

/*
 *  fwts_framework_compare_name()
 *	for sorting tests in name order
 */
static int fwts_framework_compare_name(void *data1, void *data2)
{
	fwts_framework_test *test1 = (fwts_framework_test *)data1;
	fwts_framework_test *test2 = (fwts_framework_test *)data2;

	return strcmp(test1->name, test2->name);
}

/* 
 *  fwts_framework_show_tests()
 *	dump out registered tests.
 */
static void fwts_framework_show_tests(fwts_framework *fw, int full)
{
	fwts_list_link *item;
	fwts_list *sorted;
	int i;
	int need_nl = 0;
	int total = 0;

	typedef struct {
		const char *title;	/* Test category */
		const int  flag;	/* Mask of category */
	} fwts_categories;

	fwts_categories categories[] = {
		{ "Batch",			FWTS_BATCH },
		{ "Interactive",		FWTS_INTERACTIVE },
		{ "Batch Experimental",		FWTS_BATCH_EXPERIMENTAL },
		{ "Interactive Experimental",	FWTS_INTERACTIVE_EXPERIMENTAL },
		{ "Power States",		FWTS_POWER_STATES },
		{ "Utilities",			FWTS_UTILS },
		{ NULL,			0 },
	};

	/* Dump out tests registered under all categories */
	for (i=0; categories[i].title != NULL; i++) {
		fwts_framework_test *test;

		/* If no category flags are set, or category matches user requested
		   category go and dump name and purpose of tests */
		if (((fw->flags & FWTS_RUN_ALL_FLAGS) == 0) ||
		    ((fw->flags & FWTS_RUN_ALL_FLAGS) & categories[i].flag)) {
			if ((sorted = fwts_list_init()) == NULL) {
				fprintf(stderr, "FATAL: Could not sort sort tests by name, out of memory.");
				exit(EXIT_FAILURE);
			}
			fwts_list_foreach(item, fwts_framework_test_list) {
				test = (fwts_framework_test*)item->data;
				if ((test->flags & FWTS_RUN_ALL_FLAGS) == categories[i].flag)
					fwts_list_add_ordered(sorted, item->data, fwts_framework_compare_name);
			}

			if (fwts_list_len(sorted) > 0) {
				if (need_nl)
					printf("\n");
				need_nl = 1;
				printf("%s%s:\n", categories[i].title,
					categories[i].flag & FWTS_UTILS ? "" : " tests");
	
				fwts_list_foreach(item, sorted) {
					test = (fwts_framework_test*)item->data;
					if (full == FWTS_TRUE) {
						int j;
						printf(" %-13.13s (%d test%s):\n", 
							test->name, test->ops->total_tests,
							test->ops->total_tests > 1 ? "s" : "");
						for (j=0; j<test->ops->total_tests;j++)
							printf("  %s\n", test->ops->minor_tests[j].name);
						total += test->ops->total_tests;
					}
					else {
						printf(" %-13.13s %s\n", test->name, test->ops->headline());
					}
				}
			}
			fwts_list_free(sorted, NULL);
		}
	}
	if (full == FWTS_TRUE)
		printf("\nTotal of %d tests\n", total);
}


/*
 *  fwts_framework_strtrunc()
 *	truncate overlong string 
 */
static void fwts_framework_strtrunc(char *dest, const char *src, int max)
{
	strncpy(dest, src, max);

	if ((strlen(src) > max) && (max > 3)) {
		dest[max-1] = 0;
		dest[max-2] = '.';
		dest[max-3] = '.';
	}
}

/*
 *  fwts_framework_format_results()
 *	format results into human readable summary.
 */
static void fwts_framework_format_results(char *buffer, int buflen, fwts_results *results)
{
	int n = 0;

	if (buflen)
		*buffer = 0;

	if (results->passed && buflen > 0) {
		n += snprintf(buffer, buflen, "%d passed", results->passed);
		buffer += n;
		buflen -= n;
	}
	if (results->failed && buflen > 0) {
		n += snprintf(buffer, buflen, "%s%d failed", n > 0 ? ", " : "", results->failed);
		buffer += n;
		buflen -= n;
	}
	if (results->warning && buflen > 0) {
		n += snprintf(buffer, buflen, "%s%d warnings", n > 0 ? ", " : "", results->warning);
		buffer += n;
		buflen -= n;
	}
	if (results->aborted && buflen > 0) {
		n += snprintf(buffer, buflen, "%s%d aborted", n > 0 ? ", " : "", results->aborted);
		buffer += n;
		buflen -= n;
	}
	if (results->skipped && buflen > 0) {
		n += snprintf(buffer, buflen, "%s%d skipped", n > 0 ? ", " : "", results->skipped);
	}
}

/*
 *  fwts_framework_minor_test_progress()
 *	output per test progress report or progress that can be pipe'd into
 *	dialog --guage
 *
 */
void fwts_framework_minor_test_progress(fwts_framework *fw, const int percent)
{
	float major_percent;
	float minor_percent;
	float process_percent;
	float progress;

	if (percent >=0 && percent <=100)
		fw->minor_test_progress = percent;

	major_percent = (float)100.0 / (float)fw->major_tests_total;
	minor_percent = ((float)major_percent / (float)fw->current_ops->total_tests);
	process_percent = ((float)minor_percent / 100.0);

	progress = (float)(fw->current_major_test_num-1) * major_percent;
	progress += (float)(fw->current_minor_test_num-1) * minor_percent;
	progress += (float)(percent) * process_percent;

	/* Feedback required? */
	if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
		int percent;
		char buf[55];

		fwts_framework_strtrunc(buf, fw->current_minor_test_name, sizeof(buf));

		percent = (100 * (fw->current_minor_test_num-1) / fw->current_ops->total_tests) + 
			  (fw->minor_test_progress / fw->current_ops->total_tests);
		fprintf(stderr, "  %-55.55s: %3.0f%%\r", buf, progress);
		fflush(stderr);
	}

	/* Output for the dialog tool, dialog --title "fwts" --gauge "" 12 80 0 */
	if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG) {
		fprintf(stdout, "XXX\n");
		fprintf(stdout, "%d\n", (int)progress);
		fprintf(stdout, "Sofar: %d passes, %d failures, %d warnings, %d aborted, %d skipped\n\n",
			fw->total.passed,
			fw->total.failed,
			fw->total.warning,
			fw->total.aborted,
			fw->total.skipped);
		fprintf(stdout, "%s\n\n", fw->current_major_test->ops->headline());
		fprintf(stdout, "Running test #%d: %s\n", 
			fw->current_major_test_num,
			fw->current_minor_test_name);
		fprintf(stdout, "XXX\n");
		fflush(stdout);
	}
}


/*
 *  fwts_framework_underline()
 *	underlining into log
 */
static inline void fwts_framework_underline(fwts_framework *fw, const int ch)
{
	fwts_log_underline(fw->results, ch);
}

/*
 *  fwts_framework_get_env()
 *	get a variable - if already fetched return cached value, otherwise
 *	try to gather from environment. If not in environment, return 
 *	predefined default.
 */
static char *fwts_framework_get_env(const int env_id)
{
	int i;

	for (i=0;i<sizeof(fwts_framework_settings)/sizeof(fwts_framework_setting);i++) {
		if (fwts_framework_settings[i].env_id == env_id) {	
			if (fwts_framework_settings[i].env_value)
				return fwts_framework_settings[i].env_value;
			else {
				const char *value = getenv(fwts_framework_settings[i].env_name);
				if (value == NULL) {
					value = fwts_framework_settings[i].env_default;
				}
				fwts_framework_settings[i].env_value = strdup(value);
				if (fwts_framework_settings[i].env_value)
					return fwts_framework_settings[i].env_value;
				else
					return "";
			}
		}
	}
	return "";
}

/*
 *  fwts_framework_free_env()
 *	free alloc'd environment variables
 */
static void fwts_framework_free_env(void)
{
	int i;

	for (i=0;i<sizeof(fwts_framework_settings)/sizeof(fwts_framework_setting);i++)
		if (fwts_framework_settings[i].env_value)
			free(fwts_framework_settings[i].env_value);
}

static int fwts_framework_test_summary(fwts_framework *fw)
{
	fwts_framework_underline(fw,'=');
	fwts_log_summary(fw, "%d passed, %d failed, %d warnings, %d aborted, %d skipped.", 
		fw->major_tests.passed, fw->major_tests.failed, fw->major_tests.warning, fw->major_tests.aborted, fw->major_tests.skipped);
	fwts_framework_underline(fw,'=');

	if (fw->flags & FWTS_FRAMEWORK_FLAGS_STDOUT_SUMMARY) {
		if (fw->major_tests.aborted > 0)
			printf("%s\n", fwts_framework_get_env(FWTS_ABORTED_TEXT));
		else if (fw->major_tests.skipped > 0)
			printf("%s\n", fwts_framework_get_env(FWTS_SKIPPED_TEXT));
		else if (fw->major_tests.failed > 0) {
			/* We intentionally report the highest logged error level */
			if (fw->failed_level & LOG_LEVEL_CRITICAL)
				printf("%s\n", fwts_framework_get_env(FWTS_FAILED_CRITICAL_TEXT));
			else if (fw->failed_level & LOG_LEVEL_HIGH)
				printf("%s\n", fwts_framework_get_env(FWTS_FAILED_HIGH_TEXT));
			else if (fw->failed_level & LOG_LEVEL_MEDIUM)
				printf("%s\n", fwts_framework_get_env(FWTS_FAILED_MEDIUM_TEXT));
			else if (fw->failed_level & LOG_LEVEL_LOW)
				printf("%s\n", fwts_framework_get_env(FWTS_FAILED_LOW_TEXT));
			else printf("%s\n", fwts_framework_get_env(FWTS_FAILED_TEXT));
		}
		else if (fw->major_tests.warning > 0)
			printf("%s\n", fwts_framework_get_env(FWTS_WARNING_TEXT));
		else
			printf("%s\n", fwts_framework_get_env(FWTS_PASSED_TEXT));
	}

	fwts_log_newline(fw->results);

	return FWTS_OK;
}

static int fwts_framework_total_summary(fwts_framework *fw)
{
	fwts_log_summary(fw, "Summary: %d passed, %d failed, %d warnings, %d aborted, %d skipped.", 
		fw->total.passed, fw->total.failed, fw->total.warning, fw->total.aborted, fw->total.skipped);

	return FWTS_OK;
}

static int fwts_framework_run_test(fwts_framework *fw, const int num_tests, const fwts_framework_test *test)
{		
	fwts_framework_minor_test *minor_test;	

	fw->current_minor_test_name = "";

	fw->major_tests.aborted = 0;
	fw->major_tests.failed  = 0;
	fw->major_tests.passed  = 0;
	fw->major_tests.warning = 0;
	fw->major_tests.skipped = 0;
	fw->failed_level = 0;

	fwts_log_set_owner(fw->results, test->name);

	fw->current_ops = test->ops;
	fw->current_minor_test_num = 1;

	/* Not a utility test?, then we require a test summary at end of the test run */
	if (!(test->flags & FWTS_UTILS))
		fw->print_summary = 1;

	if (test->ops->headline) {
		fwts_log_heading(fw, "%s", test->ops->headline());
		fwts_framework_underline(fw,'-');
		if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
			char buf[70];
			fwts_framework_strtrunc(buf, test->ops->headline(), sizeof(buf));
			fprintf(stderr, "Test: %-70.70s\n", buf);
		}
	}

	fwts_framework_minor_test_progress(fw, 0);

	if (test->ops->init) {
		int ret;
		if ((ret = test->ops->init(fw)) != FWTS_OK) {
			/* Init failed or skipped, so abort */
			if (ret == FWTS_SKIP) {
				for (minor_test = test->ops->minor_tests; *minor_test->test_func != NULL; minor_test++) {
					fw->major_tests.skipped++;
					fw->total.skipped++;
				}
				if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS)
					fprintf(stderr, " Test skipped.\n");

			} else {
				fwts_log_error(fw, "Aborted test, initialisation failed.");
				for (minor_test = test->ops->minor_tests; *minor_test->test_func != NULL; minor_test++) {
					fw->major_tests.aborted++;
					fw->total.aborted++;
				}
				if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS)
					fprintf(stderr, " Test aborted.\n");
			}
			if (!(test->flags & FWTS_UTILS))
				fwts_framework_test_summary(fw);
			return FWTS_OK;
		}
	}

	for (minor_test = test->ops->minor_tests; *minor_test->test_func != NULL; minor_test++, fw->current_minor_test_num++) {
		fw->current_minor_test_name = minor_test->name;

		fw->minor_tests.aborted = 0;
		fw->minor_tests.failed  = 0;
		fw->minor_tests.passed  = 0;
		fw->minor_tests.warning = 0;
		fw->minor_tests.skipped = 0;

		if (minor_test->name != NULL)
			fwts_log_info(fw, "Test %d of %d: %s", 
				fw->current_minor_test_num, 
				test->ops->total_tests, minor_test->name);

		fwts_framework_minor_test_progress(fw, 0);
		(*minor_test->test_func)(fw);
		fwts_framework_minor_test_progress(fw, 100);
	
		fw->major_tests.aborted += fw->minor_tests.aborted;
		fw->major_tests.failed  += fw->minor_tests.failed;
		fw->major_tests.passed  += fw->minor_tests.passed;
		fw->major_tests.warning += fw->minor_tests.warning;
		fw->major_tests.skipped += fw->minor_tests.skipped;

		if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
			char resbuf[80];
			char namebuf[55];
			fwts_framework_format_results(resbuf, sizeof(resbuf), &fw->minor_tests);
			fwts_framework_strtrunc(namebuf, minor_test->name, sizeof(namebuf));
			fprintf(stderr, "  %-55.55s %s\n", namebuf,
				*resbuf ? resbuf : "     ");
		}
		fwts_log_nl(fw);
	}

	fw->total.aborted += fw->major_tests.aborted;
	fw->total.failed  += fw->major_tests.failed;
	fw->total.passed  += fw->major_tests.passed;
	fw->total.warning += fw->major_tests.warning;
	fw->total.skipped += fw->major_tests.skipped;

	if (test->ops->deinit)
		test->ops->deinit(fw);

	if (!(test->flags & FWTS_UTILS))
		fwts_framework_test_summary(fw);

	fwts_log_set_owner(fw->results, "fwts");

	return FWTS_OK;
}

/*
 *  fwts_framework_tests_run()
 *	
 */
static void fwts_framework_tests_run(fwts_framework *fw, fwts_list *tests_to_run)
{
	fwts_list_link *item;

	fw->current_major_test_num = 1;
	fw->major_tests_total  = fwts_list_len(tests_to_run);

	fwts_list_foreach(item, tests_to_run) {
		fwts_framework_test *test = (fwts_framework_test*)item->data;

		fw->current_major_test = test;
		fwts_framework_run_test(fw, fwts_list_len(tests_to_run), test);
		fw->current_major_test_num++;
	}
}

/*
 *  fwts_framework_test_find()
 *	find a named test, return test if found, NULL otherwise
 */
static fwts_framework_test *fwts_framework_test_find(fwts_framework *fw, const char *name)
{
	fwts_list_link *item;

	
	fwts_list_foreach(item, fwts_framework_test_list) {
		fwts_framework_test *test = (fwts_framework_test*)item->data;
		if (strcmp(name, test->name) == 0)
			return test;
	}

	return NULL;
}

/*
 *  fwts_framework_advice()
 *	log advice message
 */
void fwts_framework_advice(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[4096];

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fwts_log_nl(fw);
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_NONE, "%s: %s", 
		fwts_framework_get_env(FWTS_ADVICE_TEXT), buffer);
	fwts_log_nl(fw);
	va_end(ap);
}

/*
 *  fwts_framework_passed()
 *	log a passed test message
 */
void fwts_framework_passed(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[4096];

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fw->minor_tests.passed++;
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_NONE, "%s: Test %d, %s", 
		fwts_framework_get_env(FWTS_PASSED_TEXT), fw->current_minor_test_num, buffer);
	va_end(ap);
}

/*
 *  fwts_framework_failed()
 *	log a failed test message
 */
void fwts_framework_failed(fwts_framework *fw, fwts_log_level level, const char *fmt, ...)
{
	va_list ap;
	char buffer[4096];

	fw->failed_level |= level;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fwts_summary_add(fw->current_major_test->name, level, buffer);
	fw->minor_tests.failed++;
	fwts_log_printf(fw->results, LOG_RESULT, level, "%s [%s]: Test %d, %s", 
		fwts_framework_get_env(FWTS_FAILED_TEXT), fwts_log_level_to_str(level), fw->current_minor_test_num, buffer);
	va_end(ap);
}

/*
 *  fwts_framework_warning()
 *	log a warning message
 */
void fwts_framework_warning(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fw->minor_tests.warning++;
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_MEDIUM, "%s: Test %d, %s", 
		fwts_framework_get_env(FWTS_WARNING_TEXT), fw->current_minor_test_num, buffer);
	va_end(ap);
}

/*
 *  fwts_framework_skipped()
 *	log a skipped test message
 */
void fwts_framework_skipped(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fw->minor_tests.skipped++;
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_MEDIUM, "%s: Test %d, %s", 
		fwts_framework_get_env(FWTS_SKIPPED_TEXT), fw->current_minor_test_num, buffer);
	va_end(ap);
}

/*
 *  fwts_framework_aborted()
 *	log an aborted test message
 */
void fwts_framework_aborted(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fw->minor_tests.skipped++;
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_MEDIUM, "%s: Test %d, %s", 
		fwts_framework_get_env(FWTS_ABORTED_TEXT), fw->current_minor_test_num, buffer);
	va_end(ap);
}

/*
 *  fwts_framework_show_version()
 *	dump version of fwts
 */
static void fwts_framework_show_version(char * const *argv)
{
	printf("%s, Version %s, %s\n", argv[0], FWTS_VERSION, FWTS_DATE);
}


/*
 *  fwts_framework_strdup()
 *	dup a string. if it's already allocated, free previous allocation before duping
 */
static void fwts_framework_strdup(char **ptr, const char *str)
{
	if (ptr == NULL)
		return;

	if (*ptr)
		free(*ptr);
	*ptr = strdup(str);
}

/*
 *  fwts_framework_syntax()
 *	dump some help
 */
static void fwts_framework_syntax(char * const *argv)
{
	int i;
	typedef struct {
		char *opt;	/* option */
		char *info;	/* what it does */
	} fwts_syntax_info;

	static fwts_syntax_info syntax_help[] = {
		{ "Arguments:",			NULL },
		{ "-",				"Output results to stdout." },
		{ "-a, --all",			"Run all tests." },
		{ "-b, --batch",		"Just run non-interactive tests." },
		{ "--batch-experimental",	"Just run Batch Experimental tests." },
		{ "--dmidecode=path",		"Specify path to dmidecode table." },
		{ "-d, --dump",			"Dump out logs." },
		{ "--dumpfile",			"Load ACPI tables using file generated by acpidump\n" },
		{ "",				"or acpidump.log from fwts --dump." },
		{ "-f, --force-clean",		"Force a clean results log file." },
		{ "-h, --help",			"Print this help." },
		{ "--iasl=path",		"Specify path to iasl." },
		{ "-i, --interactive",		"Just run interactive tests." },
		{ "--interactive-experimental",	"Just run Interactive Experimental tests." },
		{ "-k, --klog=file",		"Specify kernel log file rather than reading it" },
		{ "",				"from the kernel." },
		{ "--log-fields",		"Show available log filtering fields." },
		{ "--log-filter=expr",		"Define filters to dump out specific log fields:" },
		{ "\te.g. --log-filter=RES,SUM  - dump out results and summary.", NULL },
		{ "\t     --log-filter=ALL,~INF - dump out all fields except info fields.", NULL },
		{ "--log-format=fields",	"Define output log format." },
		{ "\te.g. --log-format=\"\%date \%time [\%field] (\%owner): \"", NULL },
		{ "\tfields: \%date  - date",	NULL },
		{ "\t\t\%time  - time", 	NULL },
		{ "\t\t\%field - log filter field",	NULL },
		{ "\t\t\%owner - name of test program",	NULL },
		{ "\t\t\%level - failure test level",	NULL },
		{ "\t\t\%line  - log line number",	NULL },
		{ "--lp-tags",			"Output LaunchPad bug tags." },
		{ "--lspci=path",		"Specify path to lspci." },
		{ "--no-s3",			"Don't run S3 suspend/resume tests." },
		{ "",				"  deprecated, use --skip_test=s3 instead." },
		{ "--no-s4",			"Don't run S4 hibernate/resume tests." },
		{ "",				"  deprecated, use --skip_test=s4 instead." },
		{ "-P, --power-states",		"Test S3, S4 power states." },
		{ "-q, --quiet",		"Run quietly." },
		{ "--results-no-separators",	"No horizontal separators in results log." },
		{ "-r, --results-output=file",	"Output results to a named file. Filename can" },
		{ "",				"also be stdout or stderr." },
		{ "--s3-delay-delta=N",		"Time to be added to delay between S3 iterations." },
		{ "--s3-min-delay=N",		"Minimum time between S3 iterations." },
		{ "--s3-max-delay=N",		"Maximum time between S3 iterations." },
		{ "--s3-multiple=N",		"Run S3 tests N times." },
		{ "--s4-multiple=N",		"Run S4 tests N times." },
		{ "-p, --show-progress",	"Output test progress report to stderr." },
		{ "-D, --show-progress-dialog",	"Output test progress for use in dialog tool." },
		{ "-s, --show-tests",		"Show available tests." },
		{ "--show-tests-full",		"Show available tests including all minor tests." },
		{ "-S, --skip_test=t1[,t2]",	"Ship tests named t1, t2.." },
		{ "--stdout-summary",		"Output SUCCESS or FAILED to stdout at end of tests." },
		{ "-t, --table-path=path",	"Path to ACPI tables." },
		{ "-v, --version",		"Show version." },
		{ "-w, --log-width=N",		"Define the output log width in characters." },
		{ NULL, NULL }
	};

	printf("Usage %s: [OPTION] [TEST]\n", argv[0]);
	for (i=0; syntax_help[i].opt != NULL; i++) 
		if (syntax_help[i].info) 
			printf("%-28.28s %s\n", syntax_help[i].opt, syntax_help[i].info);
		else
			printf("%s\n", syntax_help[i].opt);

}

/* 
 * fwts_framework_heading_info()
 *	log basic system info so we can track the tests
 */
static void fwts_framework_heading_info(fwts_framework *fw, fwts_list *tests_to_run)
{
	struct tm tm;
	time_t now;
	struct utsname buf;
	char *tests = NULL;
	int len = 1;
	fwts_list_link *item;

	time(&now);
	localtime_r(&now, &tm);

	uname(&buf);

	fwts_log_info(fw, "Results generated by fwts: Version %s (built %s).", FWTS_VERSION, FWTS_DATE);
	fwts_log_nl(fw);
	fwts_log_info(fw, "This test run on %2.2d/%2.2d/%-2.2d at %2.2d:%2.2d:%2.2d on host %s %s %s %s %s.",
		tm.tm_mday, tm.tm_mon, (tm.tm_year+1900) % 100,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		buf.sysname, buf.nodename, buf.release, buf.version, buf.machine);
	fwts_log_nl(fw);
	
	fwts_list_foreach(item, tests_to_run) {
		fwts_framework_test *test = (fwts_framework_test*)item->data;
		len += strlen(test->name) + 1;
	}

	if ((tests = calloc(len, 1)) != NULL) {
		fwts_list_foreach(item, tests_to_run) {
			fwts_framework_test *test = (fwts_framework_test*)item->data;
			if (item != tests_to_run->head) 
				strcat(tests, " ");
			strcat(tests, test->name);
		}

		fwts_log_info(fw, "Running tests: %s.\n", 
			fwts_list_len(tests_to_run) == 0 ? "None" : tests);
		fwts_log_newline(fw->results);
		free(tests);
	}
}

/*
 *  fwts_framework_skip_test()
 *	try to find a test in list of tests to be skipped, return NULL of cannot be found
 */
static fwts_framework_test *fwts_framework_skip_test(fwts_list *tests_to_skip, fwts_framework_test *test)
{
	fwts_list_link *item;

	fwts_list_foreach(item, tests_to_skip) 
		if (test == (fwts_framework_test*)item->data)
			return test;

	return NULL;
}

/*
 *  fwts_framework_skip_test_parse()
 *	parse optarg of comma separated list of tests to skip
 */
static int fwts_framework_skip_test_parse(fwts_framework *fw, const char *arg, fwts_list *tests_to_skip)
{
	char *str;
	char *token;
	char *saveptr = NULL;
	fwts_framework_test *test;

	for (str = (char*)arg; (token = strtok_r(str, ",", &saveptr)) != NULL; str = NULL) {
		if ((test = fwts_framework_test_find(fw, token)) == NULL) {
			fprintf(stderr, "No such test '%s'\n", token);
			return FWTS_ERROR;
		} else
			fwts_list_append(tests_to_skip, test);
	}

	return FWTS_OK;
}

/*
 *  fwts_framework_args()
 *	parse args and run tests
 */
int fwts_framework_args(const int argc, char * const *argv)
{
	int ret = FWTS_OK;
	int i;

	struct option long_options[] = {
		{ "stdout-summary", 0, 0, 0 },		
		{ "fwts-debug", 0, 0, 0 },
		{ "help", 0, 0, 0 },
		{ "results-output", 1, 0, 0 },
		{ "results-no-separators", 0, 0, 0 },
		{ "debug-output", 1, 0, 0 },
		{ "log-filter", 1, 0, 0 },
		{ "log-fields", 0, 0, 0 },	
		{ "log-format", 1, 0, 0 },
		{ "iasl", 1, 0, 0 },
		{ "show-progress", 0, 0, 0 },
		{ "show-tests", 0, 0, 0 },
		{ "klog", 1, 0, 0, },
		{ "dmidecode", 1, 0, 0, },
		{ "s3-multiple", 1, 0, 0, },
		{ "no-s3", 0, 0, 0 },
		{ "no-s4", 0, 0, 0 },
		{ "log-width", 1, 0, 0 },
		{ "lspci", 1, 0, 0, },
		{ "batch", 0, 0, 0 },
		{ "interactive", 0, 0, 0 },
		{ "force-clean", 0, 0, 0 },
		{ "version", 0, 0, 0 },
		{ "dump", 0, 0, 0 },
		{ "s4-multiple", 1, 0, 0, },
		{ "table-path", 1, 0, 0 },
		{ "batch-experimental", 0, 0, 0 },
		{ "interactive-experimental", 0, 0, 0 },
		{ "s3-min-delay", 1, 0, 0 },
		{ "s3-max-delay", 1, 0, 0 },
		{ "s3-delay-delta", 1, 0, 0 },
		{ "power-states", 0, 0, 0 },
		{ "all", 0, 0, 0 },
		{ "show-progress-dialog", 0, 0, 0 },
		{ "skip-test", 1, 0, 0 },
		{ "quiet", 0, 0, 0},
		{ "dumpfile", 1, 0, 0 },
		{ "lp-tags", 0, 0, 0 },
		{ "show-tests-full", 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};

	fwts_list *tests_to_run;
	fwts_list *tests_to_skip;
	fwts_framework *fw;

	if ((fw = (fwts_framework *)calloc(1, sizeof(fwts_framework))) == NULL)
		return FWTS_ERROR;

	fw->magic = FWTS_FRAMEWORK_MAGIC;
	fw->flags = FWTS_FRAMEWORK_FLAGS_DEFAULT |
		    FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS;

	fw->s3_min_delay = 0;
	fw->s3_max_delay = 30;
	fw->s3_delay_delta = 0.5;

	fwts_summary_init();

	fwts_framework_strdup(&fw->iasl, FWTS_IASL_PATH);
	fwts_framework_strdup(&fw->dmidecode, FWTS_DMIDECODE_PATH);
	fwts_framework_strdup(&fw->lspci, FWTS_LSPCI_PATH);
	fwts_framework_strdup(&fw->results_logname, RESULTS_LOG);

	tests_to_run  = fwts_list_init();
	tests_to_skip = fwts_list_init();
	if ((tests_to_run == NULL) || (tests_to_skip == NULL)) {
		fwts_log_error(fw, "Run out of memory preparing to run tests.");
		goto tidy_close;
	}

	for (;;) {
		int c;
		int option_index;

		if ((c = getopt_long(argc, argv, "abdDfhik:pPqr:sS:t:vw:?", long_options, &option_index)) == -1)
			break;

		switch (c) {
		case 0:
			switch (option_index) {
			case 0: /* --stdout-summary */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_STDOUT_SUMMARY;
				break;	
			case 1: /* --fwts-debug */
				fprintf(stderr, "--fwts-debug has been deprecated\n");
				goto tidy_close;
				break;		
			case 2: /* --help */
				fwts_framework_syntax(argv);
				goto tidy_close;
			case 3: /* --results-output */
				fwts_framework_strdup(&fw->results_logname, optarg);
				break;
			case 4: /* --results-no-separators */
				fwts_log_filter_unset_field(LOG_SEPARATOR);
				break;
			case 5: /* --debug-output */
				fprintf(stderr, "--debug-output has been deprecated\n");
				goto tidy_close;
				break;
			case 6: /* --log-filter */
				fwts_log_filter_unset_field(~0);
				fwts_log_set_field_filter(optarg);
				break;
			case 7: /* --log-fields */
				fwts_log_print_fields();
				goto tidy_close;
				break;
			case 8: /* --log-format */
				fwts_log_set_format(optarg);
				break;	
			case 9: /* --iasl */
				fwts_framework_strdup(&fw->iasl, optarg);
				break;
			case 10: /* --show-progress */
				fw->flags = (fw->flags & 
						~(FWTS_FRAMEWORK_FLAGS_QUIET |
					  	  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG))
						| FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS;
			break;
				break;
			case 11: /* --show-tests */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_SHOW_TESTS;
				break;
			case 12: /* --klog */
				fwts_framework_strdup(&fw->klog, optarg);
				break;
			case 13: /* --dmidecode */
				fwts_framework_strdup(&fw->dmidecode, optarg);
				break;
			case 14: /* --s3-multiple */
				fw->s3_multiple = atoi(optarg);
				break;
			case 15: /* --no-s3 */
				fwts_framework_skip_test_parse(fw, "s3", tests_to_skip);
				fprintf(stderr, "--no-s3 is deprecated, use --skip-tests s3 or -S s3\n");
				break;
			case 16: /* --no-s4 */
				fwts_framework_skip_test_parse(fw, "s4", tests_to_skip);
				fprintf(stderr, "--no-s4 is deprecated, use --skip-tests s4 or -S s4\n");
				break;
			case 17: /* --log-width=N */
				fwts_log_set_line_width(atoi(optarg));
				break;
			case 18: /* --lspci=pathtolspci */
				fwts_framework_strdup(&fw->lspci, optarg);
				break;
			case 19: /* --batch */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH;
				break;
			case 20: /* --interactive */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_INTERACTIVE;
				break;
			case 21: /* --force-clean */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN;
				break;
			case 22: /* --version */
				fwts_framework_show_version(argv);
				goto tidy_close;
				break;
			case 23: /* --dump */
				fwts_dump_info(fw, NULL);
				goto tidy_close;
				break;
			case 24: /* --s4-multiple */
				fw->s4_multiple = atoi(optarg);
				break;
			case 25: /* --table-path */
				fwts_framework_strdup(&fw->acpi_table_path, optarg);
				break;
			case 26: /* --batch-experimental */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH_EXPERIMENTAL;
				break;
			case 27: /* --interactive-experimental */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_INTERACTIVE_EXPERIMENTAL;
				break;
			case 28: /* --s3-min-delay */
				fw->s3_min_delay = atoi(optarg);
				break;
			case 29: /* --s3-max-delay */
				fw->s3_max_delay = atoi(optarg);
				break;
			case 30: /* --s3-delay-delta */
				fw->s3_delay_delta = atof(optarg);
				break;
			case 31: /* --power-states */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_POWER_STATES;
				break;
			case 32: /* --all */
				fw->flags |= FWTS_RUN_ALL_FLAGS;
				break;
			case 33: /* --show-progress-dialog */
				fw->flags = (fw->flags & 
						~(FWTS_FRAMEWORK_FLAGS_QUIET |
						  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS)) 
						| FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG;
				break;
			case 34: /* --skip-test */
				if (fwts_framework_skip_test_parse(fw, optarg, tests_to_skip) != FWTS_OK)
					goto tidy_close;
				break;
			case 35: /* --quiet */
				fw->flags = (fw->flags &
						~(FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS |
						  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG))
						| FWTS_FRAMEWORK_FLAGS_QUIET;
				break;
			case 36: /* --dumpfile */
				fw->acpi_table_acpidump_file = strdup(optarg);
				break;
			case 37: /* --lp-flags */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_LP_TAGS;
				break;
			case 38: /* --show-tests-full */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_SHOW_TESTS_FULL;
				break;
			}
			break;
		case 'a': /* --all */
			fw->flags |= FWTS_RUN_ALL_FLAGS;
			break;
		case 'b': /* --batch */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH;
			break;
		case 'd': /* --dump */
			fwts_dump_info(fw, NULL);
			goto tidy_close;
			break;
		case 'D': /* --show-progress-dialog */
			fw->flags = (fw->flags & 
					~(FWTS_FRAMEWORK_FLAGS_QUIET |
					  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS)) 
					| FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG;
			break;
		case 'f':
			fw->flags |= FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN;
			break;
		case 'h':
		case '?':
			fwts_framework_syntax(argv);
			goto tidy_close;
			break;
		case 'i': /* --interactive */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_INTERACTIVE;
			break;
		case 'k': /* --klog */
			fwts_framework_strdup(&fw->klog, optarg);
			break;
		case 'l': /* --lp-flags */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_LP_TAGS;
			break;
		case 'p': /* --show-progress */
			fw->flags = (fw->flags & 
					~(FWTS_FRAMEWORK_FLAGS_QUIET |
					  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG))
					| FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS;
			break;
		case 'P': /* --power-states */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_POWER_STATES;
			break;
		case 'q': /* --quiet */
			fw->flags = (fw->flags &
					~(FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS |
					  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG))
					| FWTS_FRAMEWORK_FLAGS_QUIET;
			break;
		case 'r': /* --results-output */
			fwts_framework_strdup(&fw->results_logname, optarg);
			break;
		case 's': /* --show-tests */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_SHOW_TESTS;
			break;
		case 'S': /* --skip-test */
			if (fwts_framework_skip_test_parse(fw, optarg, tests_to_skip) != FWTS_OK)
				goto tidy_close;
			break;
		case 't': /* --table-path */
			fwts_framework_strdup(&fw->acpi_table_path, optarg);
			break;
		case 'v': /* --version */
			fwts_framework_show_version(argv);
			goto tidy_close;
			break;
		case 'w': /* --log-width=N */
			fwts_log_set_line_width(atoi(optarg));
			break;
		}
	}	

	for (i=optind; i<argc; i++)
		if (!strcmp(argv[i], "-")) {
			fwts_framework_strdup(&fw->results_logname, "stdout");
			fw->flags = (fw->flags &
					~(FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS |
					  FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS_DIALOG))
					| FWTS_FRAMEWORK_FLAGS_QUIET;
			break;
		}

	/* S3 test options */
	if ((fw->s3_min_delay < 0) || (fw->s3_min_delay > 3600)) {
		fprintf(stderr, "--s3-min-delay cannot be less than zero or more than 1 hour!\n");
		goto tidy_close;
	}
	if (fw->s3_max_delay < fw->s3_min_delay || fw->s3_max_delay > 3600)  {
		fprintf(stderr, "--s3-max-delay cannot be less than --s3-min-delay or more than 1 hour!\n");
		goto tidy_close;
	}
	if (fw->s3_delay_delta <= 0.001) {
		fprintf(stderr, "--s3-delay_delta cannot be less than 0.001\n");
		goto tidy_close;
	}

	if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_TESTS) {
		fwts_framework_show_tests(fw, FWTS_FALSE);
		goto tidy_close;
	}
	if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_TESTS_FULL) {
		fwts_framework_show_tests(fw, FWTS_TRUE);
		goto tidy_close;
	}


	if ((fw->flags & FWTS_RUN_ALL_FLAGS) == 0)
		fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH;

	if ((fw->iasl == NULL) ||
	    (fw->dmidecode == NULL) ||
	    (fw->lspci == NULL) || 
	    (fw->results_logname == NULL)) {
		ret = FWTS_ERROR;
		fprintf(stderr, "%s: Memory allocation failure.", argv[0]);
		goto tidy_close;
	}

	/* Results log */
	if ((fw->results = fwts_log_open("fwts", 
			fw->results_logname, 
			fw->flags & FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN ? "w" : "a")) == NULL) {
		ret = FWTS_ERROR;
		fprintf(stderr, "%s: Cannot open results log '%s'.\n", argv[0], fw->results_logname);
		goto tidy_close;
	}


	if (optind < argc)  {
		/* Run specified tests */
		for (; optind < argc; optind++) {
			if (!strcmp(argv[optind], "-"))
				continue;

			fwts_framework_test *test = fwts_framework_test_find(fw, argv[optind]);

			if (test == NULL) {
				fprintf(stderr, "No such test '%s'\n",argv[optind]);
				fwts_framework_show_tests(fw, FWTS_FALSE);
				ret = FWTS_ERROR;
				goto tidy;
			}

			if (fwts_framework_skip_test(tests_to_skip, test) == NULL) 
				fwts_list_append(tests_to_run, test);
		}
	}

	if (fwts_list_len(tests_to_run) == 0) {
		/* Find tests that are eligible for running */
		fwts_list_link *item;
		fwts_list_foreach(item, fwts_framework_test_list) {
			fwts_framework_test *test = (fwts_framework_test*)item->data;
			if (fw->flags & test->flags & FWTS_RUN_ALL_FLAGS)
				if (fwts_framework_skip_test(tests_to_skip, test) == NULL) 
					fwts_list_append(tests_to_run, test);
		}
	}

	if (!(fw->flags & FWTS_FRAMEWORK_FLAGS_QUIET))
		printf("Running %d tests, results written to %s\n", 
			fwts_list_len(tests_to_run),
			fw->results_logname);

	fwts_framework_heading_info(fw, tests_to_run);
	fwts_framework_tests_run(fw, tests_to_run);

	if (fw->print_summary) {
		fwts_log_set_owner(fw->results, "summary");
		fwts_log_summary(fw, "");
		fwts_framework_total_summary(fw);
		fwts_log_summary(fw, "");
		fwts_summary_report(fw);
	}

tidy:
	fwts_list_free(tests_to_skip, NULL);
	fwts_list_free(tests_to_run, NULL);
	fwts_log_close(fw->results);

tidy_close:
	fwts_acpi_free_tables();
	fwts_summary_deinit();

	free(fw->iasl);
	free(fw->dmidecode);
	free(fw->lspci);
	free(fw->results_logname);
	fwts_framework_free_env();
	fwts_list_free(fwts_framework_test_list, free);

	/* Failed tests flagged an error */
	if ((fw->total.failed > 0) || 
	    (fw->total.warning > 0))	
		ret = FWTS_ERROR;

	free(fw);
	
	return ret;
}
