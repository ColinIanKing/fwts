#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "framework.h"

static char *wkalarm = "/sys/class/rtc/rtc0/wakealarm";

typedef struct {
	int some_private_data;
} wakealarm_private;

wakealarm_private private;

int get_alarm_irq_state(void)
{
	FILE *fp;
	char field[32];
	char value[32];

	if ((fp = fopen("/proc/driver/rtc", "r")) == NULL) {
		return -1;
	}
	while (fscanf(fp, "%s : %s\n", field, value) != EOF) {	
		if (!strcmp(field, "alarm_IRQ")) {
			fclose(fp);
			return strcmp(value, "no");
		}
	}
	fclose(fp);
	
	return -1;
}

int wakealarm_init(log *results, framework *fw)
{
	return check_root_euid(results);
}

int wakealarm_deinit(log *results, framework *fw)
{
	return 0;
}

void wakealarm_headline(log *results)
{
	log_info(results, "Test ACPI Wakealarm\n");
}

int wakealarm_test1(log *results, framework *fw)
{
	struct stat buf;
	char *test = "Test1: existance of /sys/class/rtc/rtc0/wakealarm";

	log_info(results, "%s\n", test);

	if (stat(wkalarm, &buf) == 0)
		fw->passed(fw, test);
	else
		fw->failed(fw, test);
}

int wakealarm_test2(log *results, framework *fw)
{
	char *test = "Test2: trigger rtc wakealarm";
	char *wkalarm = "/sys/class/rtc/rtc0/wakealarm";

	log_info(results, "%s\n", test);
	
	if (set("0", wkalarm)) {
		log_error(results, "Cannot write to %s\n", wkalarm);
		fw->failed(fw, test);
		return;
	}
	if (set("+2", wkalarm)) {
		log_error(results, "Cannot write to %s\n", wkalarm);
		fw->failed(fw, test);
		return;
	}
	if (!get_alarm_irq_state()) {
		log_error(results, "Wakealarm %s did not get set\n", wkalarm);
		fw->failed(fw, test);
		return;
	}

	fw->passed(fw, test);
}

int wakealarm_test3(log *results, framework *fw)
{
	char *test = "Test3: check if wakealarm is fired";

	log_info(results, "%s\n", test);

	if (set("0", wkalarm)) {
		log_error(results, "Cannot write to %s\n", wkalarm);
		fw->failed(fw, test);
		return;
	}
	if (set("+2", wkalarm)) {
		log_error(results, "Cannot write to %s\n", wkalarm);
		fw->failed(fw, test);
		return;
	}
	if (!get_alarm_irq_state()) {
		log_error(results, "Wakealarm %s did not get set\n", wkalarm);
		fw->failed(fw, test);
		return;
	}
	log_info(results, "Wait 3 seconds for alarm to fire\n");
	sleep(3);
	if (get_alarm_irq_state()) {
		log_error(results, "Wakealarm %s did not fire\n", wkalarm);
		fw->failed(fw, test);
		return;
	}

	fw->passed(fw, test);
}

int wakealarm_test4(log *results, framework *fw)
{
	int i;
	char *test = "Test4: multiple wakealarm firing tests";

	log_info(results, "%s\n", test);

	for (i=1; i<30; i+= 10) {
		char seconds[16];

		if (set("0", wkalarm)) {
			log_error(results, "Cannot write to %s\n", wkalarm);
			fw->failed(fw, test);
			return;
		}

		snprintf(seconds, sizeof(seconds), "+%d",i);
		if (set(seconds, wkalarm)) {
			log_error(results, "Cannot write to %s\n", wkalarm);
			fw->failed(fw, test);
			return;
		}
		if (!get_alarm_irq_state()) {
			log_error(results, "Wakealarm %s did not get set\n", wkalarm);
			fw->failed(fw, test);
			return;
		}
		log_info(results, "Wait %d seconds for %d second alarm to fire\n", i + 1, i);
		sleep(i + 1);
		if (get_alarm_irq_state()) {
			log_error(results, "Wakealarm %s did not fire\n", wkalarm);
			fw->failed(fw, test);
			return;
		}
	}
	fw->passed(fw, test);
}

framework_tests wakealarm_tests[] = {
	wakealarm_test1,
	wakealarm_test2,
	wakealarm_test3,	
	wakealarm_test4,
	NULL
};

framework_ops wakealarm_ops = {
	wakealarm_headline,
	wakealarm_init,
	wakealarm_deinit,
	wakealarm_tests
};

FRAMEWORK(wakealarm, "wakealarm.log", &wakealarm_ops, &private);
