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

#define FRAMEWORK_FLAGS_STDOUT_SUMMARY		0x00000001
#define FRAMEWORK_FLAGS_FRAMEWORK_DEBUG		0x00000002

enum {
	BIOS_TEST_TOOLKIT_PASSED_TEXT,
	BIOS_TEST_TOOLKIT_FAILED_TEXT,
	BIOS_TEST_TOOLKIT_ERROR_TEXT,
	BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG
};

typedef struct {
	int env_id;
	char *env_name;
	char *env_default;
	char *env_value;
} framework_setting;

#define ID_NAME(id)	id, # id

static framework_setting framework_settings[] = {
	ID_NAME(BIOS_TEST_TOOLKIT_PASSED_TEXT),       "PASSED", NULL,
	ID_NAME(BIOS_TEST_TOOLKIT_FAILED_TEXT),	      "FAILED", NULL,
	ID_NAME(BIOS_TEST_TOOLKIT_ERROR_TEXT),        "ERROR",  NULL,
	ID_NAME(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),   "off",    NULL,
};

static void framework_debug(framework* framework, char *fmt, ...);

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

static void framework_debug(framework* framework, char *fmt, ...)
{
	va_list ap;
	char buffer[1024];	
	static debug = -1;

	if (debug == -1)
		debug = !strcmp(framework_get_env(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),"on") |
			(framework->flags & FRAMEWORK_FLAGS_FRAMEWORK_DEBUG);
	if (debug == 0)
		return;

	va_start(ap, fmt);

        vsnprintf(buffer, sizeof(buffer), fmt, ap);
	
	log_printf(framework->debug, LOG_DEBUG, "%s", buffer);

        va_end(ap);
}

static int framework_summary(framework *framework)
{
	log_summary(framework->results, "%d passed, %d failed, %d aborted\n", framework->tests_passed, framework->tests_failed, framework->tests_aborted);

	if (framework->flags & FRAMEWORK_FLAGS_STDOUT_SUMMARY) {
		if (framework->tests_aborted > 0 || framework->tests_failed)
			printf("FAILED\n");
		else 
			printf("PASSED\n");
	}
}


int framework_run_test(framework *framework)
{		
	framework_tests *test;

	framework_debug(framework, "framework_run_test() entered\n");

	if (framework->ops->headline) {
		framework->ops->headline(framework->results);
		log_underline(framework->results,'-');
	}

	framework_debug(framework, "framework_run_test() calling ops->init()\n");

	if (framework->ops->init) {
		if (framework->ops->init(framework->results, framework)) {
			/* Init failed, so abort */
			framework_debug(framework, "framework_run_test() init failed, aborting!\n");
			for (test = framework->ops->tests; *test != NULL; test++) {
				framework->tests_aborted++;
			}
			log_underline(framework->results,'=');
			framework_summary(framework);
			return 0;
		}
	}
	
	for (test = framework->ops->tests, framework->current_test = 0; *test != NULL; test++, framework->current_test++) {
		framework_debug(framework, "exectuting test %d\n", framework->current_test);
		(*test)(framework->results, framework);
		log_newline(framework->results);
	}

	framework_debug(framework, "framework_run_test() calling ops->deinit()\n");
	if (framework->ops->deinit)
		framework->ops->deinit(framework->results, framework);
	framework_debug(framework, "framework_run_test() complete\n");

	framework_summary(framework);
	log_underline(framework->results,'=');

	return 0;
}

void framework_close(framework *framework)
{
	int failed = (framework->tests_aborted > 0 || framework->tests_failed);

	if (framework && (framework->magic == FRAMEWORK_MAGIC)) {
		free(framework);
	}
	
	exit(failed ? EXIT_FAILURE : EXIT_SUCCESS);
}

void framework_passed(framework *framework, const char *test)
{
	framework_debug(framework, "test %d passed: %s\n", framework->current_test, test);
	framework->tests_passed++;
	log_printf(framework->results, LOG_RESULT, "%s: test %d, %s\n", 
		framework_get_env(BIOS_TEST_TOOLKIT_PASSED_TEXT), framework->current_test, test);
}

void framework_failed(framework *framework, const char *test)
{
	framework_debug(framework, "test %d failed: %s\n", framework->current_test, test);
	framework->tests_failed++;
	log_printf(framework->results, LOG_RESULT, "%s: test %d, %s\n", 
		framework_get_env(BIOS_TEST_TOOLKIT_FAILED_TEXT), framework->current_test, test);
}

static void framework_syntax(char **argv)
{
	printf("Usage %s: [OPTION]\n", argv[0]);
	printf("Arguments:\n");
	printf("--framework-debug\tEnable run-time framework debug\n");
	printf("--help\t\t\tGet help\n");
	printf("--stdout-summary\tOutput SUCCESS or FAILED to stdout at end of tests\n");
	printf("--results-output=file\tOutput results to a named file. Filename can also be stdout or stderr\n");
	printf("--debug-output=file\tOutput debug to a named file. Filename can also be stdout or stderr\n");
}

static int framework_args(int argc, char **argv, framework* framework)
{
	struct option long_options[] = {
		{ "stdout-summary", 0, 0, 0 },		
		{ "framework-debug", 0, 0, 0 },
		{ "help", 0, 0, 0 },
		{ "results-output", 1, 0, 0 },
		{ "debug-output", 1, 0, 0 },
		{ 0, 0, 0, 0 }
	};

	for (;;) {
		int c;
		int option_index;

		if ((c = getopt_long(argc, argv, "", long_options, &option_index)) == -1)
			break;
	
		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				framework->flags |= FRAMEWORK_FLAGS_STDOUT_SUMMARY;
				break;	
			case 1:
				framework->flags |= FRAMEWORK_FLAGS_FRAMEWORK_DEBUG;
				break;		
			case 2:
				framework_syntax(argv);
				exit(EXIT_SUCCESS);
			case 3:
				framework->results_logname = strdup(optarg);
				break;
			case 4:
				framework->debug_logname = strdup(optarg);
				framework->flags |= FRAMEWORK_FLAGS_FRAMEWORK_DEBUG;
				break;
			}
		case '?':
			break;
		}
	}	
}


#define LOGFILE(name1, name2)	\
	(name1 != NULL) ? name1 : name2

framework *framework_open(int argc, char **argv,
			  const char *name, 
			  const char *results_log, 
			  const framework_ops *ops, void *private)
{
	framework *fw;

	if ((fw = calloc(1, sizeof(framework))) == NULL) {
		return NULL;
	}

	fw->magic = FRAMEWORK_MAGIC;
	fw->ops = ops;
	fw->private = private;

	framework_args(argc, argv, fw);

	fw->debug = log_open("framework", LOGFILE(fw->debug_logname, "stderr"), "a+");

	framework_debug(fw, "debug log opened\n");

	if (!ops)
		return NULL;


	fw->results = log_open(name, LOGFILE(fw->results_logname, results_log), "a+");

	framework_debug(fw, "framework_open completed\n");

	return fw;
}
