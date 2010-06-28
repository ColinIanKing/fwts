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
#include <getopt.h>

#include "fwts.h"

#define RESULTS_LOG	"results.log"

#define LOGFILE(name1, name2)	\
	(name1 != NULL) ? name1 : name2

enum {
	BIOS_TEST_TOOLKIT_PASSED_TEXT,
	BIOS_TEST_TOOLKIT_FAILED_TEXT,
	BIOS_TEST_TOOLKIT_WARNING_TEXT,
	BIOS_TEST_TOOLKIT_ERROR_TEXT,
	BIOS_TEST_TOOLKIT_ADVICE_TEXT,
	BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG
};

typedef struct fwts_framework_test {
	const char *name;
	const 	    fwts_framework_ops *ops;
	int   	    priority;
	int	    flags;
} fwts_framework_test;

static fwts_list *fwts_framework_test_list;

typedef struct {
	int env_id;
	char *env_name;
	char *env_default;
	char *env_value;
} fwts_framework_setting;

#define ID_NAME(id)	id, # id

static fwts_framework_setting fwts_framework_settings[] = {
	{ ID_NAME(BIOS_TEST_TOOLKIT_PASSED_TEXT),      "PASSED",  NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_FAILED_TEXT),      "FAILED",  NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_WARNING_TEXT),     "WARNING", NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_ERROR_TEXT),       "ERROR",   NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_ADVICE_TEXT),      "ADVICE",  NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),  "off",     NULL },
};

static void fwts_framework_debug(fwts_framework* framework, char *fmt, ...);

void fwts_framework_test_add(char *name, fwts_framework_ops *ops, const int priority, int flags)
{
	fwts_framework_test *new_test;
	fwts_list_element   *new_list_item;
	fwts_list_element   **list_item;

	if (flags & ~(FWTS_BATCH | FWTS_INTERACTIVE)) {
		fprintf(stderr, "Test %s flags must be FWTS_BATCH or FWTS_INTERACTIVE, got %x\n",name,flags);
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
	new_test = calloc(1, sizeof(fwts_framework_test));
	new_list_item = calloc(1, sizeof(fwts_list_element));

	if (new_test == NULL || new_list_item == NULL) {
		fprintf(stderr, "FATAL: Could not allocate memory adding tests to test framework\n");
		exit(EXIT_FAILURE);
	}

	new_list_item->data = (void*)new_test;

	new_test->name = name;
	new_test->ops  = ops;
	new_test->priority = priority;
	new_test->flags = flags;

	for (ops->total_tests = 0; ops->tests[ops->total_tests] != NULL; ops->total_tests++)
		;

	/* Insert into list based on order of priority */
	for (list_item = &fwts_framework_test_list->head; *list_item != NULL; list_item = &(*list_item)->next) {
		fwts_framework_test *test = (fwts_framework_test *)(*list_item)->data;
		if (test->priority >= new_test->priority) {
			new_list_item->next = (*list_item);
			break;
		}
	}
	if (new_list_item->next == NULL)
		fwts_framework_test_list->tail = new_list_item;

	*list_item = new_list_item;
}

static void fwts_framework_show_tests(void)
{
	fwts_list_element *item;

	printf("Available tests:\n");

	for (item = fwts_framework_test_list->head; item != NULL; item = item->next) {
		fwts_framework_test *test = (fwts_framework_test*)item->data;
		printf(" %-13.13s %s\n", test->name, test->ops->headline());
	}
}

void fwts_framework_sub_test_progress(fwts_framework *fw, const int percent)
{
	if (percent >=0 && percent <=100)
		fw->sub_test_progress = percent;

	if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
		int percent;

		percent = (100 * (fw->current_test-1) / fw->current_ops->total_tests) + 
			  (fw->sub_test_progress / fw->current_ops->total_tests);
		fprintf(stderr, "%-20.20s: %3d%%\r", fw->current_test_name, percent);
		fflush(stderr);
	
	}
}

static void fwts_framework_underline(fwts_framework *fw, const int ch)
{
	fwts_log_underline(fw->results, ch);
}

static char *fwts_framework_get_env(const int env_id)
{
	int i;

	for (i=0;i<sizeof(fwts_framework_settings)/sizeof(fwts_framework_setting);i++) {
		if (fwts_framework_settings[i].env_id == env_id) {	
			if (fwts_framework_settings[i].env_value)
				return fwts_framework_settings[i].env_value;
			else {
				char *value = getenv(fwts_framework_settings[i].env_name);
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

static void fwts_framework_free_env(void)
{
	int i;

	for (i=0;i<sizeof(fwts_framework_settings)/sizeof(fwts_framework_setting);i++)
		if (fwts_framework_settings[i].env_value)
			free(fwts_framework_settings[i].env_value);
}

static void fwts_framework_debug(fwts_framework* fw, char *fmt, ...)
{
	va_list ap;
	char buffer[1024];	
	static int debug = -1;

	if (debug == -1)
		debug = (!strcmp(fwts_framework_get_env(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),"on")) |
			(fw->flags & FWTS_FRAMEWORK_FLAGS_FRAMEWORK_DEBUG);
	if (debug == 0)
		return;

	va_start(ap, fmt);

        vsnprintf(buffer, sizeof(buffer), fmt, ap);
	
	fwts_log_printf(fw->debug, LOG_DEBUG, LOG_LEVEL_NONE, "%s", buffer);

        va_end(ap);
}

static int fwts_framework_test_summary(fwts_framework *fw)
{
	fwts_framework_underline(fw,'=');
	fwts_log_summary(fw, "%d passed, %d failed, %d warnings, %d aborted.", 
		fw->test_run.passed, fw->test_run.failed, fw->test_run.warning, fw->test_run.aborted);
	fwts_framework_underline(fw,'=');

	if (fw->flags & FWTS_FRAMEWORK_FLAGS_STDOUT_SUMMARY) {
		if ((fw->test_run.aborted > 0) || (fw->test_run.failed > 0))
			printf("%s\n", fwts_framework_get_env(BIOS_TEST_TOOLKIT_FAILED_TEXT));
		else if (fw->test_run.warning > 0)
			printf("%s\n", fwts_framework_get_env(BIOS_TEST_TOOLKIT_WARNING_TEXT));
		else
			printf("%s\n", fwts_framework_get_env(BIOS_TEST_TOOLKIT_PASSED_TEXT));
	}

	fwts_log_newline(fw->results);

	return FWTS_OK;
}

static int fwts_framework_total_summary(fwts_framework *fw)
{
	fwts_log_summary(fw, "Summary: %d passed, %d failed, %d warnings, %d aborted.", 
		fw->total.passed, fw->total.failed, fw->total.warning, fw->total.aborted);

	return FWTS_OK;
}

static int fwts_framework_run_test(fwts_framework *fw, const char *name, const fwts_framework_ops *ops)
{		
	fwts_framework_tests *test;	

	fwts_framework_debug(fw, "fwts_framework_run_test() entered");

	fw->test_run.aborted = 0;
	fw->test_run.failed  = 0;
	fw->test_run.passed  = 0;
	fw->test_run.warning = 0;

	fwts_log_set_owner(fw->results, name);

	fw->current_test_name = strdup(name);
	fw->current_ops = ops;
	fw->current_test = 1;

	fwts_framework_sub_test_progress(fw, 0);

	if (ops->headline) {
		fwts_log_heading(fw, "%s", ops->headline());
		fwts_framework_underline(fw,'-');
	}

	fwts_framework_debug(fw, "fwts_framework_run_test() calling ops->init()");

	if (ops->init) {
		int ret;
		if ((ret = ops->init(fw)) != FWTS_OK) {
			if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
				fprintf(stderr, "%-20.20s: Test %s.\n", name,
					ret == FWTS_SKIP ? "skipped" : "aborted");		
				fflush(stderr);
			}
			/* Init failed, so abort */
			fwts_log_error(fw, "Aborted test, initialisation failed.");
			fwts_framework_debug(fw, "fwts_framework_run_test() init failed, aborting!");
			for (test = ops->tests; *test != NULL; test++) {
				fw->test_run.aborted++;
				fw->total.aborted++;
			}
			fwts_framework_test_summary(fw);
			free(fw->current_test_name);
			return FWTS_OK;
		}
	}


	for (test = ops->tests; *test != NULL; test++, fw->current_test++) {
#if 0
		if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
			fprintf(stderr, "%-20.20s: Test %d of %d started.\n", name, fw->current_test, ops->total_tests);		
			fflush(stderr);
		}
#endif

		fwts_framework_debug(fw, "exectuting test %d", fw->current_test);

		fw->sub_tests.aborted = 0;
		fw->sub_tests.failed  = 0;
		fw->sub_tests.passed  = 0;
		fw->sub_tests.warning = 0;

		fwts_framework_sub_test_progress(fw, 0);
		(*test)(fw);
		fwts_framework_sub_test_progress(fw, 100);
	
		fw->test_run.aborted += fw->sub_tests.aborted;
		fw->test_run.failed  += fw->sub_tests.failed;
		fw->test_run.passed  += fw->sub_tests.passed;
		fw->test_run.warning += fw->sub_tests.warning;

		if (fw->flags & FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS) {
			fprintf(stderr, "%-20.20s: Test %d of %d completed (%d passed, %d failed, %d warnings, %d aborted).\n", 
				name, fw->current_test, ops->total_tests,
				fw->sub_tests.passed, fw->sub_tests.failed, 
				fw->sub_tests.warning, fw->sub_tests.aborted);
		}
	}
	fw->total.aborted += fw->test_run.aborted;
	fw->total.failed  += fw->test_run.failed;
	fw->total.passed  += fw->test_run.passed;
	fw->total.warning += fw->test_run.warning;

	fwts_framework_debug(fw, "fwts_framework_run_test() calling ops->deinit()");
	if (ops->deinit)
		ops->deinit(fw);
	fwts_framework_debug(fw, "fwts_framework_run_test() complete");

	fwts_framework_test_summary(fw);

	fwts_log_set_owner(fw->results, "fwts");

	free(fw->current_test_name);

	return FWTS_OK;
}

static void fwts_framework_run_registered_tests(fwts_framework *fw)
{
	fwts_list_element *item;

	fwts_framework_debug(fw, "fwts_framework_run_registered_tests()");
	for (item = fwts_framework_test_list->head; item != NULL; item = item->next) {
		fwts_framework_test *test = (fwts_framework_test*)item->data;
		if (fw->flags & test->flags & (FWTS_BATCH | FWTS_INTERACTIVE)) {
			fwts_framework_debug(fw, "fwts_framework_run_registered_tests() - test %s",test->name);
			fwts_framework_run_test(fw, test->name, test->ops);
		}
		else 
			fwts_framework_debug(fw, "fwts_framework_run_registered_tests() - skipping %s, does not match batch/interactive test flags",test->name);
	}
	fwts_framework_debug(fw, "fwts_framework_run_registered_tests() done");
}

static int fwts_framework_run_registered_test(fwts_framework *fw, const char *name)
{
	fwts_list_element *item;

	fwts_framework_debug(fw, "fwts_framework_run_registered_tests() - run test %s",name);
	for (item = fwts_framework_test_list->head; item != NULL; item = item->next) {
		fwts_framework_test *test = (fwts_framework_test*)item->data;
		if (strcmp(name, test->name) == 0) {
			fwts_framework_debug(fw, "fwts_framework_run_registered_tests() - test %s",test->name);
			fwts_framework_run_test(fw, test->name, test->ops);
			return FWTS_OK;
		}
	}
	fwts_log_set_owner(fw->results, name);
	fwts_log_printf(fw->results, LOG_ERROR, LOG_LEVEL_CRITICAL, "Test %s does not exist!", name);
	fwts_log_close(fw->results);

	return FWTS_ERROR;
}

static void fwts_framework_close(fwts_framework *fw)
{
	int failed = ((fw->total.aborted > 0) || 
		      (fw->total.failed > 0) || 
		      (fw->total.warning > 0));

	free(fw->iasl);
	free(fw->acpidump);
	free(fw->dmidecode);
	free(fw->lspci);
	free(fw->debug_logname);
	free(fw->results_logname);

	fwts_framework_free_env();

	fwts_list_free(fwts_framework_test_list, free);

	if (fw && (fw->magic == FWTS_FRAMEWORK_MAGIC))
		free(fw);
	
	exit(failed ? EXIT_FAILURE : EXIT_SUCCESS);
}

void fwts_framework_advice(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[4096];

	va_start(ap, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fwts_framework_debug(fw, "test %d ADVICE: %s.", fw->current_test, buffer);
	fwts_log_nl(fw);
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_NONE, "%s: %s", 
		fwts_framework_get_env(BIOS_TEST_TOOLKIT_ADVICE_TEXT), buffer);
	fwts_log_nl(fw);

	va_end(ap);
}

void fwts_framework_passed(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[4096];

	va_start(ap, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fwts_framework_debug(fw, "test %d PASSED: %s.", fw->current_test, buffer);
	fw->sub_tests.passed++;
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_NONE, "%s: test %d, %s", 
		fwts_framework_get_env(BIOS_TEST_TOOLKIT_PASSED_TEXT), fw->current_test, buffer);

	va_end(ap);
}

void fwts_framework_failed(fwts_framework *fw, fwts_log_level level, const char *fmt, ...)
{
	va_list ap;
	char buffer[4096];

	va_start(ap, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, ap);

	fwts_summary_add(fw->current_test_name, level, buffer);

	fwts_framework_debug(fw, "test %d FAILED [%s]: %s.", fw->current_test, fwts_log_level_to_str(level), buffer);
	fw->sub_tests.failed++;
	fwts_log_printf(fw->results, LOG_RESULT, level, "%s [%s]: test %d, %s", 
		fwts_framework_get_env(BIOS_TEST_TOOLKIT_FAILED_TEXT), fwts_log_level_to_str(level), fw->current_test, buffer);


	va_end(ap);
}

void fwts_framework_warning(fwts_framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	fwts_framework_debug(fw, "test %d WARNING: %s.", fw->current_test, buffer);
	fw->sub_tests.warning++;
	fwts_log_printf(fw->results, LOG_RESULT, LOG_LEVEL_MEDIUM, "%s: test %d, %s", 
		fwts_framework_get_env(BIOS_TEST_TOOLKIT_WARNING_TEXT), fw->current_test, buffer);

	va_end(ap);
}

static void fwts_framework_syntax(char **argv)
{
	printf("Usage %s: [OPTION] [TEST]\n", argv[0]);
	printf("Arguments:\n");
	printf("--acpidump=path\t\tSpecify path to acpidump.\n");
	printf("-b, --batch\t\tJust run non-interactive tests.\n");
	printf("--debug-output=file\tOutput debug to a named file.\n");
	printf("\t\t\tFilename can also be stdout or stderr.\n");	
	printf("--dmidecode=path\tSpecify path to dmidecode.\n");
	printf("--dsdt=file\t\tSpecify DSDT file rather than reading it from the ACPI\n");
	printf("\t\t\ttable on the machine.\n");
	printf("-f, --force-clean\tForce a clean results log file\n");
	printf("--fwts-debug\t\tEnable run-time test suite framework debug.\n");
	printf("-h, --help\t\tGet this help.\n");
	printf("--iasl=path\t\tSpecify path to iasl.\n");
	printf("-i, --interactive\tJust run interactive tests.\n");
	printf("--klog=file\t\tSpecify kernel log file rather than reading it\n");
	printf("\t\t\tfrom the kernel.\n");
	printf("--log-fields\t\tShow available log filtering fields.\n");
	printf("--log-filter=expr\tDefine filters to dump out specific log fields:\n");
	printf("\te.g. --log-filter=RES,SUM  - dump out results and summary\n");
	printf("\t     --log-filter=ALL,~INF - dump out all fields except info fields\n");
	printf("-w, --log-width=N\t\tDefine the output log width in characters.\n");
	printf("--log-format=fields\tDefine output log format.\n");
	printf("\te.g. --log-format=\"%%date %%time [%%field] (%%owner): \"\n");
	printf("\tfields: %%date  - date\n");
	printf("\t\t%%time  - time\n");
	printf("\t\t%%field - log filter field\n");
	printf("\t\t%%owner - name of test program\n");
	printf("\t\t%%level - failure test level\n");
	printf("\t\t%%line  - log line number\n");
	printf("--lspci=path\t\tSpecify path to lspci.\n");
	printf("--no-s3\t\t\tDon't run S3 suspend/resume tests.\n");
	printf("--no-s4\t\t\tDon't run S4 hibernate/resume tests.\n");
	printf("--results-no-separators\tNo horizontal separators in results log.\n");
	printf("-r, --results-output=file\n\t\t\tOutput results to a named file. Filename can also be\n");
	printf("\t\t\tstdout or stderr.\n");
	printf("--s3-multiple=N\t\tRun S3 tests N times.\n");
	printf("-p, --show-progress\tOutput test progress report to stderr.\n");
	printf("-s, --show-tests\tShow available tests.\n");
	printf("--stdout-summary\tOutput SUCCESS or FAILED to stdout at end of tests.\n");
}


int fwts_framework_args(int argc, char **argv)
{
	int ret = FWTS_OK;

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
		{ "dsdt", 1, 0, 0, },
		{ "klog", 1, 0, 0, },
		{ "dmidecode", 1, 0, 0, },
		{ "s3-multiple", 1, 0, 0, },
		{ "no-s3", 0, 0, 0 },
		{ "no-s4", 0, 0, 0 },
		{ "log-width", 1, 0, 0 },
		{ "lspci", 1, 0, 0, },
		{ "acpidump", 1, 0, 0 },
		{ "batch", 0, 0, 0 },
		{ "interactive", 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};

	fwts_framework *fw;

	if ((fw = (fwts_framework *)calloc(1, sizeof(fwts_framework))) == NULL)
		return FWTS_ERROR;

	fw->magic = FWTS_FRAMEWORK_MAGIC;
	fw->flags = FWTS_FRAMEWORK_FLAGS_DEFAULT;

	fwts_summary_init();

	for (;;) {
		int c;
		int option_index;

		if ((c = getopt_long(argc, argv, "?r:fhbipsw:", long_options, &option_index)) == -1)
			break;
	
		switch (c) {
		case 0:
			switch (option_index) {
			case 0: /* --stdout-summary */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_STDOUT_SUMMARY;
				break;	
			case 1: /* --fwts_framework_-debug */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_FRAMEWORK_DEBUG;
				break;		
			case 2: /* --help */
				fwts_framework_syntax(argv);
				exit(EXIT_SUCCESS);
			case 3: /* --results-output */
				fw->results_logname = strdup(optarg);
				break;
			case 4: /* --results-no-separators */
				fwts_log_filter_unset_field(LOG_SEPARATOR);
				break;
			case 5: /* --debug-output */
				fw->debug_logname = strdup(optarg);
				fw->flags |= FWTS_FRAMEWORK_FLAGS_FRAMEWORK_DEBUG;
				break;
			case 6: /* --log-filter */
				fwts_log_filter_unset_field(~0);
				fwts_log_set_field_filter(optarg);
				break;
			case 7: /* --log-fields */
				fwts_log_print_fields();
				exit(EXIT_SUCCESS);
				break;
			case 8: /* --log-format */
				fwts_log_set_format(optarg);
				break;	
			case 9: /* --iasl */
				fw->iasl = strdup(optarg);
				break;
			case 10: /* --show-progress */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS;
				break;
			case 11: /* --show-tests */
				fwts_framework_show_tests();
				exit(EXIT_SUCCESS);
				break;
			case 12: /* --dsdt */
				fw->dsdt = strdup(optarg);
				break;
			case 13: /* --klog */
				fw->klog = strdup(optarg);
				break;
			case 14: /* --dmidecode */
				fw->dmidecode = strdup(optarg);
				break;
			case 15: /* --s3-multiple */
				fw->s3_multiple = atoi(optarg);
				break;
			case 16: /* --no-s3 */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_NO_S3;
				break;
			case 17: /* --no-s4 */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_NO_S4;
				break;
			case 18: /* --log-width=N */
				fwts_log_set_line_width(atoi(optarg));
				break;
			case 19: /* --lspci=pathtolspci */
				fw->lspci = strdup(optarg);
				break;
			case 20: /* --acpidump=pathtoacpidump */
				fw->acpidump= strdup(optarg);
				break;
			case 21: /* --batch */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH;
				break;
			case 22: /* --interactive */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_INTERACTIVE;
				break;
			case 23: /* --force-clean */
				fw->flags |= FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN;
				break;
			}
			break;
		case 'f':
			fw->flags |= FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN;
			break;
		case 'h':
		case '?':
			fwts_framework_syntax(argv);
			exit(EXIT_SUCCESS);
			break;
		case 'b': /* --batch */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH;
			break;
		case 'i': /* --interactive */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_INTERACTIVE;
			break;
		case 'p': /* --show-progress */
			fw->flags |= FWTS_FRAMEWORK_FLAGS_SHOW_PROGRESS;
			break;
		case 's': /* --show-tests */
			fwts_framework_show_tests();
			exit(EXIT_SUCCESS);
			break;
		case 'w': /* --log-width=N */
			fwts_log_set_line_width(atoi(optarg));
			break;
		case 'r': /* --results-output */
			fw->results_logname = strdup(optarg);
			break;
		}
	}	

	if ((fw->flags & 
	    (FWTS_FRAMEWORK_FLAGS_BATCH | 
	     FWTS_FRAMEWORK_FLAGS_INTERACTIVE)) == 0)
		fw->flags |= FWTS_FRAMEWORK_FLAGS_BATCH;

	if (!fw->iasl)
		fw->iasl = strdup(FWTS_IASL_PATH);
	if (!fw->acpidump)
		fw->acpidump = strdup(FWTS_ACPIDUMP_PATH);
	if (!fw->dmidecode)
		fw->dmidecode = strdup(FWTS_DMIDECODE_PATH);
	if (!fw->lspci)
		fw->lspci = strdup(FWTS_LSPCI_PATH);
	if (!fw->debug_logname)
		fw->debug_logname = strdup("stderr");
	if (!fw->results_logname)
		fw->results_logname = strdup(RESULTS_LOG);

	if ((fw->iasl == NULL) ||
	    (fw->acpidump == NULL) ||
	    (fw->dmidecode == NULL) ||
	    (fw->lspci == NULL) || 
	    (fw->debug_logname == NULL) ||
	    (fw->results_logname == NULL)) {
		ret = FWTS_ERROR;
		fprintf(stderr, "%s: Memory allocation failure.", argv[0]);
		goto tidy_close;
	}

	if ((fw->debug = fwts_log_open("fwts", 
			fw->debug_logname, "a+")) == NULL) {
		ret = FWTS_ERROR;
		fprintf(stderr, "%s: Cannot open debug log.\n", argv[0]);
		goto tidy_close;
	}
	
	if ((fw->results = fwts_log_open("fwts", 
			fw->results_logname, 
			fw->flags & FWTS_FRAMEWORK_FLAGS_FORCE_CLEAN ? "w" : "a")) == NULL) {
		ret = FWTS_ERROR;
		fprintf(stderr, "%s: Cannot open results log '%s'.\n", argv[0], fw->results_logname);
		goto tidy_close;
	}

	if (optind < argc) 
		for (; optind < argc; optind++) {
			if (fwts_framework_run_registered_test(fw, argv[optind])) {
				fprintf(stderr, "No such test '%s'\n",argv[optind]);
				fwts_framework_show_tests();
				ret = FWTS_ERROR;
				goto tidy;
			}
		}
	else 
		fwts_framework_run_registered_tests(fw);

	fwts_log_set_owner(fw->results, "summary");
	fwts_log_summary(fw, "");
	fwts_framework_total_summary(fw);
	fwts_log_summary(fw, "");
	fwts_summary_report(fw);
	fwts_summary_deinit();

tidy:
	fwts_log_close(fw->results);
	fwts_log_close(fw->debug);

tidy_close:
	fwts_framework_close(fw);

	return ret;
}

