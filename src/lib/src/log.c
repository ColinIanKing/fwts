#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "log.h"

int log_printf(log *log, char *prefix, const char *fmt, ...)
{
	char buffer[1024];
	int n = 0;
	struct tm tm;
	time_t now;

	if ((!log) || (log && log->magic != LOG_MAGIC))
		return 0;

	va_list ap;

	va_start(ap, fmt);

	time(&now);
	localtime_r(&now, &tm);

	n = snprintf(buffer, sizeof(buffer), 
		"%2.2d/%2.2d/%-2.2d %2.2d:%2.2d:%2.2d %s ", 		
		tm.tm_mday, tm.tm_mon, (tm.tm_year+1900) % 100,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		prefix);

	if (log->owner)
		n += snprintf(buffer+n, sizeof(buffer)-n, "(%s): ", log->owner);

	vsnprintf(buffer+n, sizeof(buffer)-n, fmt, ap);

	n = fwrite(buffer, 1, strlen(buffer), log->fp);
	fflush(log->fp);

	va_end(ap);

	return n;
}

void log_underline(log *log, int ch)
{
	int i;

	if ((!log) || (log && log->magic != LOG_MAGIC))
		return;

	for (i=0;i<80;i++) {
		fputc(ch, log->fp);
	}
	fputc('\n', log->fp);
	fflush(log->fp);
}

void log_newline(log *log)
{
	if ((!log) || (log && log->magic != LOG_MAGIC))
		return;

	fprintf(log->fp, "\n");
	fflush(log->fp);
}

log *log_open(const char *owner, const char *name, const char *mode)
{
	log *newlog;

	if ((newlog = malloc(sizeof(log))) == NULL) {
		return NULL;
	}

	newlog->magic = LOG_MAGIC;

	if (owner) {
		if ((newlog->owner = malloc(strlen(owner)+1)) == NULL) {		
			free(newlog);
			return NULL;
		}
		strcpy(newlog->owner, owner);
	}

	if (strcmp("stderr", name) == 0) {
		newlog->fp = stderr;
	} else if (strcmp("stdout", name) == 0) {
		newlog->fp = stdout;
	} else if ((newlog->fp = fopen(name, mode)) == NULL) {
		free(newlog);
		return NULL;
	}

	return newlog;
}

int log_close(log *log)
{
	if (log && (log->magic == LOG_MAGIC)) {
		if (log->fp && (log->fp != stdout && log->fp != stderr))
			fclose(log->fp);
		if (log->owner)
			free(log->owner);
		free(log);
	}
	return 0;
}
