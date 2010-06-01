#ifndef __SCAN_DMESG_ACPI_WARNINGS_H__
#define __SCAN_DMESG_ACPI_WARNINGS_H__

#define KERN_WARNING            0x00000001
#define KERN_ERROR              0x00000002

typedef struct {
        char *pattern;
        int  type;
} klog_pattern;

typedef void (*klog_scan_func_t)(log *log, char *line, char *prevline, void *private, int *warnings, int *errors);

int  klog_scan(log *log, char *klog, klog_scan_func_t callback, void *private, int *warnings, int *errors);
void klog_scan_patterns(log *log, char *line, char *prevline, void *private, int *warnings, int *errors);
char *klog_strncmp(char **klog, const char *needle, const int needle_len, char *buffer, int len);

char *klog_read(void);
int  klog_clear(void);

int  klog_pm_check(log *log, char *klog, int *warnings, int *errors);
int  klog_firmware_check(log *log, char *klog, int *warnings, int *errors);

#endif
