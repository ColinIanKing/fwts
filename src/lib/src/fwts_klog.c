/*
 * Copyright (C) 2010-2018 Canonical
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
#include <stdbool.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
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
	fwts_log_free(klog);
}

/*
 *  fwts_klog_find_changes()
 *	find new lines added to kernel log, clone them from new list
 *	must be freed with fwts_list_free(klog_diff, NULL);
 */
fwts_list *fwts_klog_find_changes(fwts_list *klog_old, fwts_list *klog_new)
{
	return fwts_log_find_changes(klog_old, klog_new);
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
	return fwts_log_remove_timestamp(text);
}

int fwts_klog_scan(fwts_framework *fw,
	fwts_list *klog,
	fwts_klog_scan_func scan_func,
	fwts_klog_progress_func progress_func,
	void *private,
	int *match)
{
	return fwts_log_scan(fw, klog, scan_func, progress_func, private, match, true);
}

char *fwts_klog_unique_label(const char *str)
{
	static char buffer[1024];
	const char *src = str;
	char *dst;
	int count = 0;
	bool forceupper = true;

	strcpy(buffer, "Klog");
	dst = buffer + 4;

	while ((dst < (buffer + sizeof(buffer) - 1)) &&
	       (count < 4) && (*src)) {
		if ((*src == '|') ||
		    (*src == '/') ||
		    (*src == ' ')) {
			src++;
			count++;
			forceupper = true;
			continue;
		}
		if (!isalnum(*src)) {
			src++;
			continue;
		}
		*dst++ = forceupper ? toupper(*src) : *src;
		src++;

		forceupper = false;
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
	fwts_log_pattern *pattern = (fwts_log_pattern *)private;
	static char *advice =
		"This is a bug picked up by the kernel, but as yet, the "
		"firmware test suite has no diagnostic advice for this particular problem.";

	FWTS_UNUSED(prevline);

	while (pattern->pattern != NULL) {
		bool matched = false;
		switch (pattern->compare_mode) {
		case FWTS_COMPARE_REGEX:
			if (pattern->compiled_ok) {
				int ret = regexec(&pattern->compiled, line, 0, NULL, 0);
				if (!ret) {
					/* A successful regular expression match! */
					matched = true;
				} else if (ret != REG_NOMATCH) {
					char msg[1024];

					regerror(ret, &pattern->compiled, msg, sizeof(msg));
					fwts_log_info(fw, "regular expression engine error: %s.", msg);
				}
			}
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
				fwts_failed(fw, pattern->level, pattern->label,
					"%s Kernel message: %s", fwts_log_level_to_str(pattern->level), line);
				fwts_error_inc(fw, pattern->label, errors);
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
fwts_compare_mode fwts_klog_compare_mode_str_to_val(const char *str)
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
const char *fwts_json_str(
	fwts_framework *fw,
	const char *table,
	int index,
	json_object *obj,
	const char *key,
	bool log_error)
{
	const char *str;
#if JSON_HAS_GET_EX
	json_object *str_obj;

	if (!json_object_object_get_ex(obj, key, &str_obj))
		goto nullobj;
	str = json_object_get_string(str_obj);
	if (FWTS_JSON_ERROR(str))
		goto nullobj;
#else
	str = json_object_get_string(json_object_object_get(obj, key));
	if (FWTS_JSON_ERROR(str))
		goto nullobj;
#endif
	return str;
nullobj:
	if (log_error)
		fwts_log_error(fw, "Cannot fetch %s val from item %d, table %s.",
			key, index, table);
	return NULL;
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
	fwts_log_pattern *patterns;
	char json_data_path[PATH_MAX];

	if (fw->json_data_file) {
		snprintf(json_data_path, sizeof(json_data_path), "%s/%s", fw->json_data_path,(fw->json_data_file));
	}
	else { /* use the hard coded KLOG JSON as default */
		snprintf(json_data_path, sizeof(json_data_path), "%s/%s", fw->json_data_path, KLOG_DATA_JSON_FILE);
	}

	/*
	 * json_object_from_file() can fail when files aren't readable
	 * so check if we can open for read before calling json_object_from_file()
	 */
	if ((fd = open(json_data_path, O_RDONLY)) < 0) {
		fwts_log_error(fw, "Cannot read file %s, check the path and check that the file exists, you may need to specify -j or -J.", json_data_path);
		return FWTS_ERROR;
	}
	(void)close(fd);

	klog_objs = json_object_from_file(json_data_path);
	if (FWTS_JSON_ERROR(klog_objs)) {
		fwts_log_error(fw, "Cannot load klog data from %s.", json_data_path);
		return FWTS_ERROR;
	}

#if JSON_HAS_GET_EX
	if (!json_object_object_get_ex(klog_objs, table, &klog_table)) {
		fwts_log_error(fw, "Cannot fetch klog table object '%s' from %s.", table, json_data_path);
		goto fail_put;
	}
#else
	klog_table = json_object_object_get(klog_objs, table);
	if (FWTS_JSON_ERROR(klog_table)) {
		fwts_log_error(fw, "Cannot fetch klog table object '%s' from %s.", table, json_data_path);
		goto fail_put;
	}
#endif

	n = json_object_array_length(klog_table);

	/* Last entry is null to indicate end, so alloc n+1 items */
	if ((patterns = calloc(n+1, sizeof(fwts_log_pattern))) == NULL) {
		fwts_log_error(fw, "Cannot allocate pattern table.");
		goto fail_put;
	}

	/* Now fetch json objects and compile regex */
	for (i = 0; i < n; i++) {
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

		if ((patterns[i].pattern = fwts_json_str(fw, table, i, obj, "pattern", true)) == NULL)
			goto fail;

		if ((patterns[i].advice = fwts_json_str(fw, table, i, obj, "advice", true)) == NULL)
			goto fail;

		/* Labels appear in fwts 0.26.0, so are optional with older versions */
		str = fwts_json_str(fw, table, i, obj, "label", false);
		if (str) {
			patterns[i].label = strdup(str);
		} else {
			/* if not specified, auto-magically generate */
			patterns[i].label = strdup(fwts_klog_unique_label(patterns[i].pattern));
		}
		if (patterns[i].label == NULL)
			goto fail;

		if (patterns[i].compare_mode == FWTS_COMPARE_REGEX) {
			int rc;

			rc = regcomp(&patterns[i].compiled, patterns[i].pattern, REG_EXTENDED);
			if (rc) {
				fwts_log_error(fw, "Regex %s failed to compile: %d.", patterns[i].pattern, rc);
				patterns[i].compiled_ok = false;
			} else {
				patterns[i].compiled_ok = true;
			}
		}
	}
	/* We've now collected up the scan patterns, lets scan the log for errors */
	ret = fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, progress, patterns, errors);

fail:
	for (i = 0; i < n; i++) {
		if (patterns[i].compiled_ok)
			regfree(&patterns[i].compiled);
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

static void fwts_klog_regex_find_callback(fwts_framework *fw, char *line, int repeated,
	char *prev, void *pattern, int *match)
{
	int rc;
	regex_t compiled;

	FWTS_UNUSED(fw);
	FWTS_UNUSED(repeated);
	FWTS_UNUSED(prev);

	rc = regcomp(&compiled, (char *)pattern, REG_EXTENDED);
	if (rc) {
		fwts_log_error(fw, "Regex %s failed to compile: %d.", (char *)pattern, rc);
	} else {
		rc = regexec(&compiled, line, 0, NULL, 0);
		regfree(&compiled);
		if (!rc)
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

/*
 * fwts_klog_write()
 *	write a message to the kernel log
 */
int fwts_klog_write(fwts_framework *fw, const char *msg)
{
	FILE *fp;

	if ((fp = fopen("/dev/kmsg", "w")) == NULL) {
		fwts_log_info(fw, "Cannot write to kernel log /dev/kmsg.");
		return FWTS_ERROR;
	}

	fprintf(fp, "<7>fwts: %s", msg);
	fflush(fp);
	(void)fclose(fp);

	return FWTS_OK;
}
