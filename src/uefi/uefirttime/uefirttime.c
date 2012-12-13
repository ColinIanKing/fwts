/*
 * Copyright (C) 2012 Canonical
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

#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "fwts.h"
#include "fwts_uefi.h"
#include "efi_runtime.h"
#include "fwts_efi_module.h"

#define IS_LEAP(year) \
		((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

static int fd;
static uint32_t dayofmonth[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool dayvalid(EFI_TIME *Time)
{
	if (Time->Day < 1)
		return false;

	if (Time->Day > dayofmonth[Time->Month - 1])
		return false;

	/* check month 2 */
	if (Time->Month == 2 && (!IS_LEAP(Time->Year) && Time->Day > 28))
		return false;

	return true;
}

static bool checktimefields(fwts_framework *fw, EFI_TIME *Time)
{
	if (Time->Year < 1900 || Time->Year > 9999) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadYear",
			"Time returned an invalid year %" PRIu16
			", should be between 1900 and 9999.",
			Time->Year);
		return false;
	}

	if (Time->Month < 1 || Time->Month > 12) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadMonth",
			"Time returned an invalid month %" PRIu8
			", should be between 1 and 12.",
			Time->Month);
		return false;
	}

	if (!dayvalid(Time)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadDay",
			"Time returned an invalid day %" PRIu8
			", should be between 1 and 28-31 depends on month/year.",
			Time->Day);
		return false;
	}

	if (Time->Hour > 23) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadHour",
			"Time returned an invalid hour %" PRIu8
			", should be between 0 and 23.",
			Time->Hour);
		return false;
	}

	if (Time->Minute > 59) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadMinute",
			"Time returned an invalid minute %" PRIu8
			", should be between 0 and 59.",
			Time->Minute);
		return false;
	}

	if (Time->Second > 59) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadSecond",
			"Time returned an invalid second %" PRIu8
			", should be between 0 and 59.",
			Time->Second);
		return false;
	}

	if (Time->Nanosecond > 999999999) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadNanosecond",
			"Time returned an invalid nanosecond %" PRIu32
			", should be between 0 and 999999999.",
			Time->Nanosecond);
		return false;
	}

	if (!(Time->TimeZone == FWTS_UEFI_UNSPECIFIED_TIMEZONE ||
		(Time->TimeZone >= -1440 && Time->TimeZone <= 1440))) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadTimezone",
			"Time returned an invalid timezone %" PRId16
			", should be between -1440 and 1440 or 2047.",
			Time->TimeZone);
		return false;
	}

	if (Time->Daylight & (~(FWTS_UEFI_TIME_ADJUST_DAYLIGHT |
				FWTS_UEFI_TIME_IN_DAYLIGHT))) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"UEFIRuntimeTimeFieldBadDaylight",
			"Time returned an invalid daylight %" PRIu8
			", all other bits except UEFI_TIME_IN_DAYLIGHT and "
			"UEFI_TIME_ADJUST_DAYLIGHT must be zero.",
			Time->Daylight);
		return false;
	}
	return true;
}


static void addonehour(EFI_TIME *time)
{
	if (time->Hour != 23) {
		time->Hour += 1;
		return;
	}
	time->Hour = 0;

	if ((time->Day != dayofmonth[time->Month - 1]) &&
		(!(time->Month == 2 && (!IS_LEAP(time->Year)
		&& time->Day == 28)))) {
		time->Day += 1;
		return;
	}
	time->Day = 1;

	if (time->Month != 12) {
		time->Month += 1;
		return;
	}
	time->Month = 1;
	time->Year += 1;
	return;
}

static int uefirttime_init(fwts_framework *fw)
{
	if (fwts_firmware_detect() != FWTS_FIRMWARE_UEFI) {
		fwts_log_info(fw, "Cannot detect any UEFI firmware. Aborted.");
		return FWTS_ABORTED;
	}

	if (fwts_lib_efi_runtime_load_module(fw) != FWTS_OK) {
		fwts_log_info(fw, "Cannot load efi_runtime module. Aborted.");
		return FWTS_ABORTED;
	}

	fd = open("/dev/efi_runtime", O_RDONLY);
	if (fd == -1) {
		fwts_log_info(fw, "Cannot open efi_runtime driver. Aborted.");
		return FWTS_ABORTED;
	}

	return FWTS_OK;
}

static int uefirttime_deinit(fwts_framework *fw)
{
	FWTS_UNUSED(fw);

	close(fd);
	fwts_lib_efi_runtime_unload_module(fw);

	return FWTS_OK;
}


static int uefirttime_test1(fwts_framework *fw)
{
	long ioret;
	struct efi_gettime gettime;
	EFI_TIME efi_time;

	EFI_TIME_CAPABILITIES efi_time_cap;
	uint64_t status;

	gettime.Capabilities = &efi_time_cap;
	gettime.Time = &efi_time;
	gettime.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_TIME, &gettime);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetTime",
			"Failed to get time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (!checktimefields(fw, gettime.Time))
		return FWTS_ERROR;

	fwts_passed(fw, "UEFI runtime service GetTime interface test passed.");

	return FWTS_OK;
}

static int uefirttime_test2(fwts_framework *fw)
{

	long ioret;
	struct efi_settime settime;
	uint64_t status;
	struct efi_gettime gettime;

	EFI_TIME oldtime;
	EFI_TIME newtime;
	EFI_TIME time;
	EFI_TIME_CAPABILITIES efi_time_cap;

	gettime.Capabilities = &efi_time_cap;
	gettime.Time = &oldtime;
	gettime.status = &status;
	ioret = ioctl(fd, EFI_RUNTIME_GET_TIME, &gettime);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetTime",
			"Failed to get time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	/* refer to UEFI SCT 2.3 test items */
	/* change year */
	time = oldtime;
	if (time.Year != 2012)
		time.Year = 2012;
	else
		time.Year = 2016;

	/* change month */
	if (time.Month != 1)
		time.Month = 1;
	else
		time.Month = 12;

	/* Change daylight */
	if (time.Daylight & FWTS_UEFI_TIME_ADJUST_DAYLIGHT)
		time.Daylight &= ~FWTS_UEFI_TIME_ADJUST_DAYLIGHT;
	else
		time.Daylight |= FWTS_UEFI_TIME_ADJUST_DAYLIGHT;

	/* Change time zone */
	if (time.TimeZone != 0)
		time.TimeZone = 0;
	else
		time.TimeZone = 1;

	settime.Time = &time;
	settime.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_SET_TIME, &settime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetTime",
			"Failed to set time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	sleep(1);

	gettime.Time = &newtime;

	ioret = ioctl(fd, EFI_RUNTIME_GET_TIME, &gettime);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetTime",
			"Failed to get time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (!((oldtime.Year == 2012) && (newtime.Year == 2016)) &&
	    !((oldtime.Year != 2012) && (newtime.Year == 2012))) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetTimeYear",
			"Failed to set year with UEFI runtime service.");
		return FWTS_ERROR;
	}

	if (!((oldtime.Month == 1) && (newtime.Month == 12)) &&
	    !((oldtime.Month != 1) && (newtime.Month == 1))) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetTimeMonth",
			"Failed to set month with UEFI runtime service.");
		return FWTS_ERROR;
	}

	if (!((oldtime.Daylight & FWTS_UEFI_TIME_ADJUST_DAYLIGHT) &&
	    (!(newtime.Daylight & FWTS_UEFI_TIME_ADJUST_DAYLIGHT))) &&
	    !((!(oldtime.Daylight & FWTS_UEFI_TIME_ADJUST_DAYLIGHT)) &&
	    (newtime.Daylight & FWTS_UEFI_TIME_ADJUST_DAYLIGHT))) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetTimeDaylight",
			"Failed to set daylight with UEFI runtime service.");
		return FWTS_ERROR;
	}

	if (!((oldtime.TimeZone == 0) && (newtime.TimeZone == 1)) &&
	    !((oldtime.TimeZone != 0) && (newtime.TimeZone == 0))) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetTimeTimezone",
			"Failed to set timezone with UEFI runtime service.");
		return FWTS_ERROR;
	}

	/* restore the previous time. */
	settime.Time = &oldtime;
	ioret = ioctl(fd, EFI_RUNTIME_SET_TIME, &settime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetTime",
			"Failed to set time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	fwts_passed(fw, "UEFI runtime service SetTime interface test passed.");

	return FWTS_OK;
}

static int uefirttime_test3(fwts_framework *fw)
{
	long ioret;
	struct efi_getwakeuptime getwakeuptime;
	uint64_t status;
	uint8_t enabled, pending;
	EFI_TIME efi_time;

	getwakeuptime.Enabled = &enabled;
	getwakeuptime.Pending = &pending;
	getwakeuptime.Time = &efi_time;
	getwakeuptime.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_WAKETIME, &getwakeuptime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetWakeupTime",
			"Failed to get wakeup time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (!checktimefields(fw, getwakeuptime.Time))
		return FWTS_ERROR;

	fwts_passed(fw, "UEFI runtime service GetWakeupTime interface test passed.");

	return FWTS_OK;
}

static int uefirttime_test4(fwts_framework *fw)
{

	long ioret;
	struct efi_setwakeuptime setwakeuptime;
	uint64_t status;
	EFI_TIME oldtime;
	EFI_TIME newtime;

	struct efi_gettime gettime;
	EFI_TIME_CAPABILITIES efi_time_cap;

	struct efi_getwakeuptime getwakeuptime;
	uint8_t enabled, pending;

	gettime.Capabilities = &efi_time_cap;
	gettime.Time = &oldtime;
	gettime.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_TIME, &gettime);

	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetTime",
			"Failed to get time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	/* change the hour, add 1 hour*/
	addonehour(&oldtime);

	setwakeuptime.Time = &oldtime;
	setwakeuptime.status = &status;
	setwakeuptime.Enabled = true;

	ioret = ioctl(fd, EFI_RUNTIME_SET_WAKETIME, &setwakeuptime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetWakeupTime",
			"Failed to set wakeup time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	sleep(1);

	getwakeuptime.Enabled = &enabled;
	getwakeuptime.Pending = &pending;
	getwakeuptime.Time = &newtime;
	getwakeuptime.status = &status;

	ioret = ioctl(fd, EFI_RUNTIME_GET_WAKETIME, &getwakeuptime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetWakeupTime",
			"Failed to get wakeup time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (*getwakeuptime.Enabled != true) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetWakeupTimeEnable",
			"Failed to set wakeup alarm clock, wakeup timer should be enabled.");
		return FWTS_ERROR;
	}

	if (*getwakeuptime.Pending != false) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetWakeupTimePending",
			"Get error alarm signle status.");
		return FWTS_ERROR;
	}

	if ((oldtime.Year != newtime.Year) || (oldtime.Month != newtime.Month) ||
		(oldtime.Day != newtime.Day) || (oldtime.Hour != newtime.Hour)) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetWakeupTimeVerify",
			"Failed to verify wakeup time after change.");
		return FWTS_ERROR;
	}

	setwakeuptime.Enabled = false;

	ioret = ioctl(fd, EFI_RUNTIME_SET_WAKETIME, &setwakeuptime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetWakeupTime",
			"Failed to set wakeup time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	sleep(1);

	ioret = ioctl(fd, EFI_RUNTIME_GET_WAKETIME, &getwakeuptime);
	if (ioret == -1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeGetWakeupTime",
			"Failed to get wakeup time with UEFI runtime service.");
		fwts_uefi_print_status_info(fw, status);
		return FWTS_ERROR;
	}

	if (*getwakeuptime.Enabled != false) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "UEFIRuntimeSetWakeupTimeEnable",
			"Failed to set wakeup alarm clock, wakeup timer should be disabled.");
		return FWTS_ERROR;
	}

	fwts_passed(fw, "UEFI runtime service SetWakeupTime interface test passed.");

	return FWTS_OK;
}

static fwts_framework_minor_test uefirttime_tests[] = {
	{ uefirttime_test1, "Test UEFI RT service get time interface." },
	{ uefirttime_test2, "Test UEFI RT service set time interface." },
	{ uefirttime_test3, "Test UEFI RT service get wakeup time interface." },
	{ uefirttime_test4, "Test UEFI RT service set wakeup time interface." },
	{ NULL, NULL }
};

static fwts_framework_ops uefirttime_ops = {
	.description = "UEFI Runtime service time interface tests.",
	.init        = uefirttime_init,
	.deinit      = uefirttime_deinit,
	.minor_tests = uefirttime_tests
};

FWTS_REGISTER(uefirttime, &uefirttime_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UNSAFE | FWTS_FLAG_ROOT_PRIV);
