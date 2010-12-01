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

#include <sys/klog.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pcre.h>
#include <json/json.h>

#include "fwts.h"

/*
 *  klog pattern matching strings data file, data stored in json format
 */
#define KLOG_DATA_JSON_PATH		"/usr/share/fwts/klog.json"


/*
 *  fwts_klog_free()
 *	free kernel log list
 */
void fwts_klog_free(fwts_list *klog)
{
	fwts_text_list_free(klog);
}

/*
 *  fwts_klog_clear()
 *	clear messages out of kernel log
 */
int fwts_klog_clear(void)
{
	if (klogctl(5, NULL, 0) < 0)
		return FWTS_ERROR;
	return FWTS_OK;
}

/*
 *  fwts_klog_read()
 *	read kernel log and return as list of lines
 */
fwts_list *fwts_klog_read(void)
{
	int len;
	char *buffer;
	fwts_list *list;

	if ((len = klogctl(10, NULL, 0)) < 0)
		return NULL;

	if ((buffer = calloc(1, len)) < 0) {
		free(buffer);
		return NULL;
	}
	
	if (klogctl(3, buffer, len) < 0) {
		free(buffer);
		return NULL;
	}

	list = fwts_list_from_text(buffer);
	free(buffer);
	
	return list;
}

int fwts_klog_scan(fwts_framework *fw, 
		   fwts_list *klog, 
		   fwts_klog_scan_func scan_func,
		   fwts_klog_progress_func progress_func,
		   void *private, 
		   int *match)
{
	*match= 0;
	char *prev;
	fwts_list_link *item;
	int i;

	if (!klog)
		return FWTS_ERROR;

	prev = "";

	for (i=0,item = klog->head; item != NULL; item=item->next,i++) {
		char *ptr = (char *)item->data;

		if ((ptr[0] == '<') && (ptr[2] == '>'))
			ptr += 3;

		scan_func(fw, ptr, prev, private, match);
		if (progress_func  && ((i % 25) == 0))
			progress_func(fw, 100 * i / fwts_list_len(klog));
		prev = ptr;
	}

	return FWTS_OK;
}

void fwts_klog_scan_patterns(fwts_framework *fw, 
	char *line, 
	char *prevline, 
	void *private, 
	int *errors)
{
	fwts_klog_pattern *pattern = (fwts_klog_pattern *)private;
	int vector[1];
	static char *advice = 
		"This is a bug picked up by the kernel, but as yet, the "
		"firmware test suite has no diagnostic advice for this particular problem.";

	while (pattern->pattern != NULL) {
		switch (pattern->compare_mode) {
		case FWTS_COMPARE_REGEX:
			if (pcre_exec(pattern->re, NULL, line, strlen(line), 0, 0, vector, 1) == 0) {
				fwts_tag_failed(fw, pattern->tag);
				fwts_failed_level(fw, pattern->level, "%s Kernel message: %s", fwts_log_level_to_str(pattern->level), line);
				(*errors)++;
				if (pattern->advice != NULL)
					fwts_advice(fw, "%s", pattern->advice);
				else
					fwts_advice(fw, "%s", advice);
				return;
			}
			break;
		case FWTS_COMPARE_STRING:
		default:
			if (strstr(line, pattern->pattern) != NULL) {
				fwts_tag_failed(fw, pattern->tag);
				fwts_failed_level(fw, pattern->level, "%s Kernel message: %s", fwts_log_level_to_str(pattern->level), line);
				(*errors)++;
				if (pattern->advice != NULL)
					fwts_advice(fw, "%s", pattern->advice);
				else
					fwts_advice(fw, "%s", advice);
				return;	
			}
		}
		pattern++;
	}
}

/*
 *  fwts_klog_compare_mode_str_to_val()
 *	convert compare mode strings (from json database) to compare_mode values
 */
static fwts_compare_mode fwts_klog_compare_mode_str_to_val(const char *str)
{
	if (strcmp(str, "regex") == 0)
		return FWTS_COMPARE_REGEX;
	else if (strcmp(str, "string") == 0)
		return FWTS_COMPARE_STRING;
	else
		return FWTS_COMPARE_UNKNOWN;
}
		
#define JSON_ERROR	((json_object*)-1)

/*
 *  fwts_json_str()
 *	given a key, fetch the string value associated with this object
 *	and report an error if it cannot be found.
 */
static const char *fwts_json_str(fwts_framework *fw, const char *table, int index, json_object *obj, const char *key)
{
	const char *str;

	if ((str = json_object_get_string(json_object_object_get(obj, key))) == NULL) {
		fwts_log_error(fw, "Cannot fetch %s val from item %d, table %s.", key, index, table);
		return NULL;
	}
	return str;
}

static int fwts_klog_check(fwts_framework *fw,
	const char *table,
	fwts_klog_progress_func progress,
	fwts_list *klog,
	int *errors)
{
	int ret = FWTS_ERROR;
	int n;
	int i;
	json_object *klog_objs;
	json_object *klog_table;
	fwts_klog_pattern *patterns;

	if ((klog_objs = json_object_from_file(KLOG_DATA_JSON_PATH)) == JSON_ERROR) {
		fwts_log_error(fw, "Cannot load klog data from %s.", KLOG_DATA_JSON_PATH);
		return FWTS_ERROR;
	}
		
	if ((klog_table = json_object_object_get(klog_objs, table)) == JSON_ERROR) {
		fwts_log_error(fw, "Cannot fetch klog table object '%s' from %s.", table, KLOG_DATA_JSON_PATH);
		goto fail_put;
	}
		
	n = json_object_array_length(klog_table);

	/* Last entry is null to indicate end, so alloc n+1 items */
	if ((patterns = calloc(n+1, sizeof(fwts_klog_pattern))) == NULL) {
		fwts_log_error(fw, "Cannot allocate pattern table.");
		goto fail_put;
	}

	/* Now fetch json objects and compile regex */
	for (i=0; i<n; i++) {
		const char *error;
		int erroffset;
		const char *str;
		json_object *obj;

		if ((obj = json_object_array_get_idx(klog_table, i)) == JSON_ERROR) {
			fwts_log_error(fw, "Cannot fetch %d item from table %s.", i, table);
			goto fail;
		}
		if ((str = fwts_json_str(fw, table, i, obj, "compare_mode")) == NULL)
			goto fail;
		patterns[i].compare_mode = fwts_klog_compare_mode_str_to_val(str);

		if ((str = fwts_json_str(fw, table, i, obj, "log_level")) == NULL)
			goto fail;
		patterns[i].level   = fwts_log_str_to_level(str);

		if ((str = fwts_json_str(fw, table, i, obj, "tag")) == NULL)
			goto fail;
		patterns[i].tag     = fwts_tag_id_str_to_tag(str);

		if ((patterns[i].pattern = fwts_json_str(fw, table, i, obj, "pattern")) == NULL)
			goto fail;

		if ((patterns[i].advice = fwts_json_str(fw, table, i, obj, "advice")) == NULL)
			goto fail;

		if ((patterns[i].re = pcre_compile(patterns[i].pattern, 0, &error, &erroffset, NULL)) == NULL)
			fwts_log_error(fw, "Regex %s failed to compile: %s.", patterns[i].pattern, error);
	}
	/* We've now collected up the scan patterns, lets scan the log for errors */
	ret = fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, progress, patterns, errors);

fail:
	for (i=0; i<n; i++)
		if (patterns[i].re)
			pcre_free(patterns[i].re);
	free(patterns);
fail_put:
	json_object_put(klog_objs);
	
	return ret;
}

int fwts_klog_firmware_check(fwts_framework *fw, fwts_klog_progress_func progress, 
	fwts_list *klog, int *errors)
{	
	return fwts_klog_check(fw, "firmware_error_warning_patterns", 
		progress, klog, errors);
}

int fwts_klog_pm_check(fwts_framework *fw, fwts_klog_progress_func progress, 
	fwts_list *klog, int *errors)
{
	return fwts_klog_check(fw, "pm_error_warning_patterns", 
		progress, klog, errors);
}

int fwts_klog_common_check(fwts_framework *fw, fwts_klog_progress_func progress, 
	fwts_list *klog, int *errors)
{
	return fwts_klog_check(fw, "common_error_warning_patterns", 
		progress, klog, errors);
}

static void fwts_klog_regex_find_callback(fwts_framework *fw, char *line, 
	char *prev, void *pattern, int *match)
{
	const char *error;
	int erroffset;
	pcre *re;
	int rc;
	int vector[1];

	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (re != NULL) {
		rc = pcre_exec(re, NULL, line, strlen(line), 0, 0, vector, 1);
		if (rc == 0)
			(*match)++;
		pcre_free(re);
	}
}

/*
 * fwts_klog_regex_find()
 * 	scan a kernel log list of lines for a given regex pattern
 *	uses fwts_klog_regex_find_callback() callback
 */
int fwts_klog_regex_find(fwts_framework *fw, fwts_list *klog, char *pattern)
{
	int found = 0;

	fwts_klog_scan(fw, klog, fwts_klog_regex_find_callback, NULL, pattern, &found);

	return found;
}
