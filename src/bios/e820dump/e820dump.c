/*
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

#include "fwts.h"

static char *e820dump_headline(void)
{
	return "Dump INT 15 E820 memmap.";
}

static int e820dump_util(fwts_framework *fw)
{
	fwts_list *e820dump_e820_info;

	if ((e820dump_e820_info = fwts_e820_table_load(fw)) == NULL) {
		fwts_log_warning(fw, "Cannot load E820 table from /sys/firmware/memmap or kernel log.");
		return FWTS_ERROR;
	}

	fwts_e820_table_dump(fw, e820dump_e820_info);
	fwts_e820_table_free(e820dump_e820_info);

	return FWTS_OK;
}

static fwts_framework_tests e820dump_utils[] = {
	e820dump_util,
	NULL
};

static fwts_framework_ops e820dump_ops = {
	e820dump_headline,
	NULL,
	NULL,
	e820dump_utils
};

FWTS_REGISTER(e820dump, &e820dump_ops, FWTS_TEST_ANYTIME, FWTS_UTILS);
