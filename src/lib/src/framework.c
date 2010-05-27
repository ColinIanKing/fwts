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

#include "framework.h"

#define RESULTS_LOG	"results.log"

#define LOGFILE(name1, name2)	\
	(name1 != NULL) ? name1 : name2

#define FRAMEWORK_FLAGS_STDOUT_SUMMARY		0x00000001
#define FRAMEWORK_FLAGS_FRAMEWORK_DEBUG		0x00000002
#define FRAMEWORK_FLAGS_SHOW_PROGRESS		0x00000004

#define FRAMEWORK_DEFAULT_FLAGS			0

enum {
	BIOS_TEST_TOOLKIT_PASSED_TEXT,
	BIOS_TEST_TOOLKIT_FAILED_TEXT,
	BIOS_TEST_TOOLKIT_ERROR_TEXT,
	BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG
};

typedef struct framework_list {
	const char *name;
	const framework_ops *ops;
	struct framework_list *next;
} framework_list;

static framework_list *framework_list_head;
static framework_list *framework_list_tail;


typedef struct {
	int env_id;
	char *env_name;
	char *env_default;
	char *env_value;
} framework_setting;

#define ID_NAME(id)	id, # id

static framework_setting framework_settings[] = {
	{ ID_NAME(BIOS_TEST_TOOLKIT_PASSED_TEXT),       "PASSED", NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_FAILED_TEXT),	      "FAILED", NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_ERROR_TEXT),        "ERROR",  NULL },
	{ ID_NAME(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),   "off",    NULL },
};

static void framework_debug(framework* framework, char *fmt, ...);

void framework_add(char *name, const framework_ops *ops)
{
	framework_list *item;

	if ((item = malloc(sizeof(framework_list))) == NULL) {
		fprintf(stderr, "FATAL: Could not allocate memory initialising framework\n");		
		exit(EXIT_FAILURE);
	}
	item->name = name;
	item->ops = ops;
	item->next = NULL;
	if (framework_list_head == NULL) {
		framework_list_head = item;
		framework_list_tail = item;
	} 
	else {
		framework_list_tail->next = item;
		framework_list_tail = item;
	}
}

static void framework_show_tests(void)
{
	framework_list *item;

	printf("Available tests: ");

	for (item = framework_list_head; item != NULL; item = item->next)
		printf("%s%s", item == framework_list_head ? "" : " ", item->name);

	printf("\n");
}
	

static void framework_underline(framework *fw, const int ch)
{
	log_underline(fw->results, ch);
}

static char *framework_get_env(const int env_id)
{
	int i;

	for (i=0;i<sizeof(framework_settings)/sizeof(framework_setting);i++) {
		if (framework_settings[i].env_id == env_id) {
			if (framework_settings[i].env_value)
				return framework_settings[i].env_value;
			else {
				char *value = getenv(framework_settings[i].env_name);
				if (value == NULL) {
					value = framework_settings[i].env_default;
				}
				framework_settings[i].env_value = malloc(strlen(value)+1);
				if (framework_settings[i].env_value) {
					strcpy(framework_settings[i].env_value, value);
					return framework_settings[i].env_value;
				} else {
					return "";
				}
			}
		}
	}
	return "";
}

static void framework_debug(framework* fw, char *fmt, ...)
{
	va_list ap;
	char buffer[1024];	
	static int debug = -1;

	if (debug == -1)
		debug = (!strcmp(framework_get_env(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),"on")) |
			(fw->flags & FRAMEWORK_FLAGS_FRAMEWORK_DEBUG);
	if (debug == 0)
		return;

	va_start(ap, fmt);

        vsnprintf(buffer, sizeof(buffer), fmt, ap);
	
	log_printf(fw->debug, LOG_DEBUG, "%s", buffer);

        va_end(ap);
}

static int framework_test_summary(framework *fw)
{
	framework_underline(fw,'=');
	log_summary(fw->results, "%d passed, %d failed, %d aborted", fw->tests_passed, fw->tests_failed, fw->tests_aborted);
	framework_underline(fw,'=');

	if (fw->flags & FRAMEWORK_FLAGS_STDOUT_SUMMARY) {
		if ((fw->tests_aborted > 0) || (fw->tests_failed > 0))
			printf("FAILED\n");
		else 
			printf("PASSED\n");
	}

	log_newline(fw->results);

	fw->total_tests_aborted += fw->tests_aborted;
	fw->total_tests_failed  += fw->tests_failed;
	fw->total_tests_passed  += fw->tests_passed;

	return 0;
}

static int framework_total_summary(framework *fw)
{
	log_set_owner(fw->results, "framework");
	log_summary(fw->results, "All tests: %d passed, %d failed, %d aborted", fw->total_tests_passed, fw->total_tests_failed, fw->total_tests_aborted);

	return 0;
}

static int framework_run_test(framework *fw, const char *name, const framework_ops *ops)
{		
	int num = 0;
	framework_tests *test;

	framework_debug(fw, "framework_run_test() entered\n");

	log_set_owner(fw->results, name);

	if (ops->headline) {
		ops->headline(fw->results);		
		framework_underline(fw,'-');
	}

	framework_debug(fw, "framework_run_test() calling ops->init()\n");

	fw->tests_aborted = 0;
	fw->tests_failed = 0;
	fw->tests_passed = 0;

	if (ops->init) {
		if (ops->init(fw->results, fw)) {
			/* Init failed, so abort */
			framework_debug(fw, "framework_run_test() init failed, aborting!\n");
			for (test = ops->tests; *test != NULL; test++) {
				fw->tests_aborted++;
			}
			framework_test_summary(fw);
			return 0;
		}
	}

	for (test = ops->tests; *test != NULL; test++)
		num++;

	for (test = ops->tests, fw->current_test = 1; *test != NULL; test++, fw->current_test++) {
		if (fw->flags & FRAMEWORK_FLAGS_SHOW_PROGRESS) {
			fprintf(stderr, "%-20.20s: Test %d of %d started\n", name, fw->current_test, num);		
			fflush(stderr);
		}

		framework_debug(fw, "exectuting test %d\n", fw->current_test);

		(*test)(fw->results, fw);

		if (fw->flags & FRAMEWORK_FLAGS_SHOW_PROGRESS) {
			fprintf(stderr, "%-20.20s: Test %d of %d completed (%d passed, %d failed, %d aborted)\n", 
				name, fw->current_test, num,
				fw->tests_passed, fw->tests_failed, fw->tests_aborted);
		}
	}

	framework_debug(fw, "framework_run_test() calling ops->deinit()\n");
	if (ops->deinit)
		ops->deinit(fw->results, fw);
	framework_debug(fw, "framework_run_test() complete\n");

	framework_test_summary(fw);

	return 0;
}

static void framework_run_registered_tests(framework *fw)
{
	framework_list *item;

	framework_debug(fw, "framework_run_registered_tests()\n");
	for (item = framework_list_head; item != NULL; item = item->next) {
		framework_debug(fw, "framework_run_registered_tests() - test %s\n",item->name);
		framework_run_test(fw, item->name, item->ops);
	}
	framework_debug(fw, "framework_run_registered_tests() done\n");
}

static int framework_run_registered_test(framework *fw, const char *name)
{
	framework_list *item;
	framework_debug(fw, "framework_run_registered_tests() - run test %s\n",name);
	for (item = framework_list_head; item != NULL; item = item->next) {
		if (strcmp(name, item->name) == 0) {
			framework_debug(fw, "framework_run_registered_tests() - test %s\n",item->name);
			framework_run_test(fw, item->name, item->ops);
			return 0;
		}
	}
	fw->results = log_open(name, LOGFILE(fw->results_logname, RESULTS_LOG), "a+");
	log_printf(fw->results, LOG_ERROR, "Test %s does not exist!", name);
	log_close(fw->results);

	return 1;
}

static void framework_close(framework *fw)
{
	int failed = (fw->total_tests_aborted > 0 || fw->total_tests_failed);

	if (fw && (fw->magic == FRAMEWORK_MAGIC)) {
		free(fw);
	}
	
	exit(failed ? EXIT_FAILURE : EXIT_SUCCESS);
}

void framework_passed(framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	framework_debug(fw, "test %d passed: %s\n", fw->current_test, buffer);
	fw->tests_passed++;
	log_printf(fw->results, LOG_RESULT, "%s: test %d, %s", 
		framework_get_env(BIOS_TEST_TOOLKIT_PASSED_TEXT), fw->current_test, buffer);

	va_end(ap);
}

void framework_failed(framework *fw, const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	framework_debug(fw, "test %d failed: %s\n", fw->current_test, buffer);
	fw->tests_failed++;
	log_printf(fw->results, LOG_RESULT, "%s: test %d, %s", 
		framework_get_env(BIOS_TEST_TOOLKIT_FAILED_TEXT), fw->current_test, buffer);

	va_end(ap);
}

static void framework_syntax(char **argv)
{
	printf("Usage %s: [OPTION] [TEST]\n", argv[0]);
	printf("Arguments:\n");
	printf("--dmidecode\t\tSpecify path to dmidecode\n");
	printf("--iasl\t\t\tSpecify path to iasl\n");
	printf("--framework-debug\tEnable run-time framework debug\n");
	printf("--help\t\t\tGet help\n");
	printf("--stdout-summary\tOutput SUCCESS or FAILED to stdout at end of tests\n");
	printf("--results-no-separators\tNo horizontal separators in results log\n");
	printf("--results-output=file\tOutput results to a named file. Filename can also be stdout or stderr\n");
	printf("--debug-output=file\tOutput debug to a named file. Filename can also be stdout or stderr\n");	
	printf("--dsdt=file\t\tSpecify DSDT file rather than reading it from the ACPI table\n");
	printf("--klog=file\t\tSpecify kernel log file rather than reading it from the kernel\n");
	printf("--log-fields\t\tShow available log filtering fields\n");
	printf("--log-filter=expr\tDefine filters to dump out specific log fields\n");
	printf("\t\te.g. --log-filter=RES,SUM  - dump out results and summary\n");
	printf("\t\t     --log-filter=ALL,~INF - dump out all fields except info fields\n");
	printf("--log-format=fields\tDefine output log format\n");
	printf("\t\te.g. --log-format=%%date %%time [%%field] (%%owner): %%text\n");
	printf("\t\t     fields are: %%date - date, %%time - time, %%field log filter field\n");
	printf("\t\t                 %%owner - name of test program, %%text - log text\n");
	printf("--show-progress\t\tOutput test progress report to stderr\n");
	printf("--show-tests\t\tShow available tests\n");
}


int framework_args(int argc, char **argv)
{
	struct option long_options[] = {
		{ "stdout-summary", 0, 0, 0 },		
		{ "framework-debug", 0, 0, 0 },
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
		{ 0, 0, 0, 0 }
	};

	framework *fw;

	fw = (framework *)calloc(1, sizeof(framework));
	if (fw == NULL) {
		return 1;
	}

	fw->magic = FRAMEWORK_MAGIC;
	fw->flags = FRAMEWORK_DEFAULT_FLAGS;

	for (;;) {
		int c;
		int option_index;

		if ((c = getopt_long(argc, argv, "", long_options, &option_index)) == -1)
			break;
	
		switch (c) {
		case 0:
			switch (option_index) {
			case 0: /* --stdout-summary */
				fw->flags |= FRAMEWORK_FLAGS_STDOUT_SUMMARY;
				break;	
			case 1: /* --framework-debug */
				fw->flags |= FRAMEWORK_FLAGS_FRAMEWORK_DEBUG;
				break;		
			case 2: /* --help */
				framework_syntax(argv);
				exit(EXIT_SUCCESS);
			case 3: /* --results-output */
				fw->results_logname = strdup(optarg);
				break;
			case 4: /* --results-no-separators */
				log_filter_unset_field(LOG_SEPARATOR);
				break;
			case 5: /* --debug-output */
				fw->debug_logname = strdup(optarg);
				fw->flags |= FRAMEWORK_FLAGS_FRAMEWORK_DEBUG;
				break;
			case 6: /* --log-filter */
				log_filter_unset_field(~0);
				log_set_field_filter(optarg);
				break;
			case 7: /* --log-fields */
				log_print_fields();
				exit(EXIT_SUCCESS);
				break;
			case 8: /* --log-format */
				log_set_format(optarg);
				break;	
			case 9: /* --iasl */
				fw->iasl = strdup(optarg);
				break;
			case 10: /* --show-progress */
				fw->flags |= FRAMEWORK_FLAGS_SHOW_PROGRESS;
				break;
			case 11: /* --show-tests */
				framework_show_tests();
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
			}
		case '?':
			break;
		}
	}	

	fw->debug = log_open("framework", LOGFILE(fw->debug_logname, "stderr"), "a+");
	fw->results = log_open("framework", LOGFILE(fw->results_logname, RESULTS_LOG), "a+");

	if (optind < argc) 
		for (; optind < argc; optind++) {
			if (framework_run_registered_test(fw, argv[optind])) {
				fprintf(stderr, "No such test '%s'\n",argv[optind]);
				framework_show_tests();
				exit(EXIT_FAILURE);
			}
		}
	else 
		framework_run_registered_tests(fw);

	framework_total_summary(fw);

	log_close(fw->results);
	log_close(fw->debug);

	framework_close(fw);

	return 0;
}

