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

#include "fwts.h"

#define OS2_GAP_ADDRESS 	(15*1024*1024)
#define OS2_GAP_SIZE		(1024*1024)

static fwts_list *os2gap_e820_info;

static char *os2gap_headline(void)
{
	return "OS/2 memory hole test";
}

static int os2gap_init(fwts_framework *fw)
{
	if ((os2gap_e820_info = fwts_e820_table_load(fw)) == NULL) {
		fwts_log_warning(fw, "No E820 table found");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}


static int os2gap_deinit(fwts_framework *fw)
{
	if (os2gap_e820_info)
		fwts_e820_table_free(os2gap_e820_info);

	return FWTS_OK;
}

static int os2gap_test1(fwts_framework *fw)
{
	fwts_log_info(fw, "This test checks if the OS/2 15Mb memory hole is absent");

	if (fwts_e820_is_reserved(os2gap_e820_info, OS2_GAP_ADDRESS)) {
		fwts_failed_high(fw, "The memory map has OS/2 memory hole at %p..%p.",
			(void*)OS2_GAP_ADDRESS,
			(void*)(OS2_GAP_ADDRESS + OS2_GAP_SIZE));
		fwts_log_nl(fw);
		fwts_e820_table_dump(fw, os2gap_e820_info);
	} else
		fwts_passed(fw, "No OS/2 memory hole found.");

	return FWTS_OK;
}


/*
 *  Null terminated array of tests to run, in this
 *  scenario, we just have one test.
 */
static fwts_framework_tests os2gap_tests[] = {
	os2gap_test1,
	NULL
};

static fwts_framework_ops os2gap_ops = {
	os2gap_headline,
	os2gap_init,
	os2gap_deinit,
	os2gap_tests
};

FWTS_REGISTER(os2gap, &os2gap_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
