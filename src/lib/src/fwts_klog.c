/*
 * Copyright (C) 2010-2012 Canonical
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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fwts.h"

/*
 *  klog pattern matching strings data file, data stored in json format
 */
#define KLOG_DATA_JSON_FILE		"klog.json"


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

	if ((buffer = calloc(1, len)) == NULL)
		return NULL;

	if (klogctl(3, buffer, len) < 0) {
		free(buffer);
		return NULL;
	}

	list = fwts_list_from_text(buffer);
	free(buffer);

	return list;
}

char *fwts_klog_remove_timestamp(char *text)
{
	char *ptr = text;

	if ((ptr[0] == '<') && (ptr[2] == '>'))
		ptr += 3;

	if (*ptr == '[')
		while (*ptr && *ptr !=']')
			ptr++;
		if (*ptr == ']')
			ptr+=2;	/* Skip ] and trailing space */

	return ptr;
}

int fwts_klog_scan(fwts_framework *fw,
		   fwts_list *klog,
		   fwts_klog_scan_func scan_func,
		   fwts_klog_progress_func progress_func,
		   void *private,
		   int *match)
{
	typedef struct {
		char *line;
		int repeated;
	} klog_reduced_item;

	*match= 0;
	char *prev;
	fwts_list_link *item;
	fwts_list *klog_reduced;
	int i;

	if (!klog)
		return FWTS_ERROR;

	if ((klog_reduced = fwts_list_new()) == NULL)
		return FWTS_ERROR;

	/*
	 *  Form a reduced log by stripping out repeated kernel warnings
	 */
	i = 0;
	fwts_list_foreach(item, klog) {
		char *newline = fwts_klog_remove_timestamp(fwts_list_data(char *, item));
		if (progress_func  && ((i % 25) == 0))
			progress_func(fw, 50 * i / fwts_list_len(klog));
		if (*newline) {
			int matched = 0;
			fwts_list_link *l;
			fwts_list_foreach(l, klog_reduced) {
				char *line;
				klog_reduced_item *reduced = fwts_list_data(klog_reduced_item *, l);

				line = fwts_klog_remove_timestamp(reduced->line);
				if (strcmp(newline, line) == 0) {
					reduced->repeated++;
					matched = 1;
					break;
				}
			}
			if (!matched) {
				klog_reduced_item *new;

				if ((new = calloc(1, sizeof(klog_reduced_item))) == NULL) {
					fwts_list_free(klog_reduced, free);
					return FWTS_ERROR;
				}
				new->line = fwts_list_data(char *, item);
				new->repeated = 0;

				fwts_list_append(klog_reduced, new);
			}
		}
		i++;
	}

	prev = "";

	i = 0;
	fwts_list_foreach(item, klog_reduced) {
		klog_reduced_item *reduced = fwts_list_data(klog_reduced_item *, item);
		char *line = reduced->line;

		if ((line[0] == '<') && (line[2] == '>'))
			line += 3;

		scan_func(fw, line, reduced->repeated, prev, private, match);
		if (progress_func  && ((i % 25) == 0))
			progress_func(fw, (50+(50 * i)) / fwts_list_len(klog_reduced));
		prev = line;
		i++;
	}
	if (progress_func)
		progress_func(fw, 100);

	fwts_list_free(klog_reduced, free);

	return FWTS_OK;
}

static char *fwts_klog_unique_label(const char *str)
{
	static char buffer[1024];
	const char *src = str;
	char *dst;
	int count = 0;
	int forceupper = 1;

	strcpy(buffer, "Klog");
	dst = buffer + 4;

	while ((dst < (buffer+sizeof(buffer)-1)) && (count < 4) && (*src)) {
		if ((*src == '|') || (*src == '/') || (*src == ' ')) {
			src++;
			count++;
			forceupper = 1;
			continue;
		}
		if (!isalnum(*src)) {
			src++;
			continue;
		}
		*dst++ = forceupper ? toupper(*src) : *src;
		src++;

		forceupper = 0;
	}
	*dst = '\0';
	return buffer;
}

void fwts_klog_scan_patterns(fwts_framework *fw,
	char *line,
	int  repeated,
	char *prevline,
	void *private,
	int *errors)
{
	fwts_klog_pattern *pattern = (fwts_klog_pattern *)private;
	int vector[1];
	static char *advice =
		"This is a bug picked up by the kernel, but as yet, the "
		"firmware test suite has no diagnostic advice for this particular problem.";

	FWTS_UNUSED(prevline);

	while (pattern->pattern != NULL) {
		int matched = 0;
		switch (pattern->compare_mode) {
		case FWTS_COMPARE_REGEX:
			if (pattern->re)
				matched = (pcre_exec(pattern->re, pattern->extra, line, strlen(line), 0, 0, vector, 1) == 0);
			break;
		case FWTS_COMPARE_STRING:
		default:
			matched = (strstr(line, pattern->pattern) != NULL) ;
			break;
		}

		if (matched) {
			if (pattern->level == LOG_LEVEL_INFO)
				fwts_log_info(fw, "Kernel message: %s", line);
			else {
				fwts_tag_failed(fw, pattern->tag);
				fwts_failed(fw, pattern->level, fwts_klog_unique_label(pattern->pattern), "%s Kernel message: %s", fwts_log_level_to_str(pattern->level), line);
				(*errors)++;
			}
			if (repeated)
				fwts_log_info(fw, "Message repeated %d times.", repeated);

			if ((pattern->advice) != NULL && (*pattern->advice))
				fwts_advice(fw, "%s", pattern->advice);
			else
				fwts_advice(fw, "%s", advice);
			return;
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

/*
 *  fwts_json_str()
 *	given a key, fetch the string value associated with this object
 *	and report an error if it cannot be found.
 */
static const char *fwts_json_str(
	fwts_framework *fw,
	const char *table,
	int index,
	json_object *obj,
	const char *key,
	bool log_error)
{
	const char *str;

	str = json_object_get_string(json_object_object_get(obj, key));
	if (FWTS_JSON_ERROR(str)) {
		if (log_error)
			fwts_log_error(fw, "Cannot fetch %s val from item %d, table %s.",
				key, index, table);
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
	int fd;
	json_object *klog_objs;
	json_object *klog_table;
	fwts_klog_pattern *patterns;
	char json_data_path[PATH_MAX];

	snprintf(json_data_path, sizeof(json_data_path), "%s/%s", fw->json_data_path, KLOG_DATA_JSON_FILE);

	/*
	 * json_object_from_file() can fail when files aren't readable
	 * so check if we can open for read before calling json_object_from_file()
	 */
	if ((fd = open(json_data_path, O_RDONLY)) < 0) {
		fwts_log_error(fw, "Cannot read file %s.", json_data_path);
		return FWTS_ERROR;
	}
	close(fd);

	klog_objs = json_object_from_file(json_data_path);
	if (FWTS_JSON_ERROR(klog_objs)) {
		fwts_log_error(fw, "Cannot load klog data from %s.", json_data_path);
		return FWTS_ERROR;
	}

	klog_table = json_object_object_get(klog_objs, table);
	if (FWTS_JSON_ERROR(klog_table)) {
		fwts_log_error(fw, "Cannot fetch klog table object '%s' from %s.", table, json_data_path);
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

		obj = json_object_array_get_idx(klog_table, i);
		if (FWTS_JSON_ERROR(obj)) {
			fwts_log_error(fw, "Cannot fetch %d item from table %s.", i, table);
			goto fail;
		}
		if ((str = fwts_json_str(fw, table, i, obj, "compare_mode", true)) == NULL)
			goto fail;
		patterns[i].compare_mode = fwts_klog_compare_mode_str_to_val(str);

		if ((str = fwts_json_str(fw, table, i, obj, "log_level", true)) == NULL)
			goto fail;
		patterns[i].level   = fwts_log_str_to_level(str);

		if ((str = fwts_json_str(fw, table, i, obj, "tag", true)) == NULL)
			goto fail;
		patterns[i].tag     = fwts_tag_id_str_to_tag(str);

		if ((patterns[i].pattern = fwts_json_str(fw, table, i, obj, "pattern", true)) == NULL)
			goto fail;

		if ((patterns[i].advice = fwts_json_str(fw, table, i, obj, "advice", true)) == NULL)
			goto fail;

		/* Labels appear in fwts 0.26.0, so are optional with older versions */
		str = fwts_json_str(fw, table, i, obj, "label", false);
		if (str) {
			patterns[i].label = strdup(str);
		} else {
			/* Not specified, so automagically generate */
			patterns[i].label = strdup(fwts_klog_unique_label(patterns[i].pattern));
		}
		if (patterns[i].label == NULL)
			goto fail;

		if ((patterns[i].re = pcre_compile(patterns[i].pattern, 0, &error, &erroffset, NULL)) == NULL) {
			fwts_log_error(fw, "Regex %s failed to compile: %s.", patterns[i].pattern, error);
			patterns[i].re = NULL;
		} else {
			patterns[i].extra = pcre_study(patterns[i].re, 0, &error);
			if (error != NULL) {
				fwts_log_error(fw, "Regex %s failed to optimize: %s.", patterns[i].pattern, error);
				patterns[i].re = NULL;
			}
		}
	}
	/* We've now collected up the scan patterns, lets scan the log for errors */
	ret = fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, progress, patterns, errors);

fail:
	for (i=0; i<n; i++) {
		if (patterns[i].re)
			pcre_free(patterns[i].re);
		if (patterns[i].extra)
			pcre_free(patterns[i].extra);
		if (patterns[i].label)
			free(patterns[i].label);
	}
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

static void fwts_klog_regex_find_callback(fwts_framework *fw, char *line, int repeated,
	char *prev, void *pattern, int *match)
{
	const char *error;
	int erroffset;
	pcre *re;
	pcre_extra *extra;
	int rc;
	int vector[1];

	FWTS_UNUSED(fw);
	FWTS_UNUSED(repeated);
	FWTS_UNUSED(prev);

	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (re != NULL) {
		extra = pcre_study(re, 0, &error);
		if (error)
			return;

		rc = pcre_exec(re, extra, line, strlen(line), 0, 0, vector, 1);
		if (extra)
			free(extra);
		pcre_free(re);
		if (rc == 0)
			(*match)++;
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
