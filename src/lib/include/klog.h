#ifndef __SCAN_DMESG_ACPI_WARNINGS_H__
#define __SCAN_DMESG_ACPI_WARNINGS_H__

typedef void (*klog_scan_func_t)(log *log, char *line, char *prevline, void *private, int *warnings, int *errors);

char *klog_read(void);
int  klog_check(log *log, char *klog, int *warnings, int *errors);
int  klog_clear(void);

int  klog_scan(log *log, char *klog, klog_scan_func_t callback, void *private, int *warnings, int *errors);

#endif
