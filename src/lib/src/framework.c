#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


#include "framework.h"

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
		debug = !strcmp(framework_get_env(BIOS_TEST_TOOLKIT_FRAMEWORK_DEBUG),"on");
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
}


static int framework_run_test(framework *framework)
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

static int framework_close(framework *framework)
{
	if (framework && (framework->magic == FRAMEWORK_MAGIC)) {
		free(framework);
	}
	return 0;
}

static int framework_test_passed(framework *framework, const char *test)
{
	framework_debug(framework, "test %d passed: %s\n", framework->current_test, test);
	framework->tests_passed++;
	log_printf(framework->results, LOG_RESULT, "%s: test %d, %s\n", 
		framework_get_env(BIOS_TEST_TOOLKIT_PASSED_TEXT), framework->current_test, test);
	return 0;
}

static int framework_test_failed(framework *framework, const char *test)
{
	framework_debug(framework, "test %d failed: %s\n", framework->current_test, test);
	framework->tests_failed++;
	log_printf(framework->results, LOG_RESULT, "%s: test %d, %s\n", 
		framework_get_env(BIOS_TEST_TOOLKIT_FAILED_TEXT), framework->current_test, test);
	return 0;
}


framework *framework_open(const char *name, const char *results_log, 
			  const framework_ops *ops, void *private)
{
	framework *newframework;

	if ((newframework = malloc(sizeof(framework))) == NULL) {
		return NULL;
	}

	newframework->debug = log_open("framework", "stderr", NULL);
	framework_debug(newframework, "debug log opened\n");

	if (!ops)
		return NULL;

	newframework->results = log_open(name, results_log, "a+");
	newframework->magic = FRAMEWORK_MAGIC;
	newframework->ops = ops;
	newframework->private = private;
	newframework->tests_passed = 0;
	newframework->tests_failed = 0;
	newframework->tests_aborted = 0;

	newframework->close = framework_close;
	newframework->run_test = framework_run_test;
	newframework->passed = framework_test_passed;
	newframework->failed = framework_test_failed;

	framework_debug(newframework, "framework_open completed\n");

	return newframework;
}
