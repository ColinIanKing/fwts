/*
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 *
 * This code was originally part of the Linux-ready Firmware Developer Kit
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "framework.h"

static text_list *dmi_text;

int dmi_decode_init(log *results, framework *fw)
{
	const char *dmidecode = "/usr/sbin/dmidecode";
	struct stat buffer;
	FILE *dmidata;

	if (check_root_euid(results))
		return 1;

	if (stat(dmidecode, &buffer)) {
		log_error(results, "Make sure dmidecode is installed!n");
		return 1;
	}

	if ((dmidata = popen(dmidecode, "r")) == NULL) {
		log_error(results, "Failed to execute dmidecode\n");
		return 1;
	}

	dmi_text = text_read(dmidata);
	fclose(dmidata);

	if (dmi_text == NULL) {
		log_error(results, "Failed to read output from dmidecode (out of memory)\n");
		return 1;
	}	

	return 0;
}

int dmi_decode_deinit(log *results, framework *fw)
{
	if (dmi_text)
		text_free(dmi_text);

	return 0;
}

void dmi_decode_headline(log *results)
{
	log_info(results, "Test DMI/SMBIOS tables for errors.\n");
}

int dmi_decode_test1(log *results, framework *fw)
{
	char *test = "Sanity check SMBIOS or DMI entry points";
	if (text_strstr(dmi_text, "No SMBIOS nor DMI entry point found"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test2(log *results, framework *fw)
{
	char *test = "DMI structures count";
	if (text_strstr(dmi_text, "Wrong DMI structures count"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test3(log *results, framework *fw)
{
	char *test = "DMI structures length";
	if (text_strstr(dmi_text, "Wrong DMI structures length"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test4(log *results, framework *fw)
{
	char *test = "Out of spec check";
	if (text_strstr(dmi_text, "<OUT OF SPEC>"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test5(log *results, framework *fw)
{
	char *test = "Bad index check";
	if (text_strstr(dmi_text, "<BAD INDEX>"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test6(log *results, framework *fw)
{
	char *test = "Bad checksum";
	if (text_strstr(dmi_text, "Bad checksum! Please report."))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test7(log *results, framework *fw)
{
	char *test = "Serial number not updated, should NOT be 0123456789";
	char *text = text_strstr(dmi_text, "Serial Number:");

	if (strstr(text, "0123456789"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test8(log *results, framework *fw)
{
	char *test = "Template Asset Tag not updated, should NOT be 0123456789";
	char *text = text_strstr(dmi_text, "Asset Tag:");

	if (strstr(text, "0123456789"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test9(log *results, framework *fw)
{
	char *test = "Template UUID number not updated";
	char *text = text_strstr(dmi_text, "UUID:");

	if (strstr(text, "0A0A0A0A-0A0A-0A0A-0A0A-0A0A0A0A0A0A"))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

int dmi_decode_test10(log *results, framework *fw)
{
	char *test = "Template not updated, incorrect default of \"To Be Filled By O.E.M\"";
	if (text_strstr(dmi_text, "To Be Filled By O.E.M."))
		fw->failed(fw, test);	
	else
		fw->passed(fw, test);
	return 0;
}

framework_tests dmi_decode_tests[] = {
	dmi_decode_test1,
	dmi_decode_test2,
	dmi_decode_test3,	
	dmi_decode_test4,
	dmi_decode_test5,
	dmi_decode_test6,
	dmi_decode_test7,
	dmi_decode_test8,
	dmi_decode_test9,
	dmi_decode_test10,
	NULL
};

framework_ops dmi_decode_ops = {
	dmi_decode_headline,
	dmi_decode_init,
	dmi_decode_deinit,
	dmi_decode_tests
};

FRAMEWORK(dmi_decode, "dmi_decode.log", &dmi_decode_ops, NULL);
