/*
 * Copyright (C) 2015-2016 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "fwts_acpi_object_eval.h"

static fwts_acpi_table_info *table;

static int stao_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "STAO", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI STAO table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  Extract a STAO ACPI String from the raw buffer
 */
static bool stao_acpi_string(
	fwts_framework *fw,
	char *str,
	char *end,
	bool *passed,
	size_t *len)
{
	char *ptr = str;

	while (*ptr) {
		if (ptr > end) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"STAOAcpiStringNotNullTerminated",
				"STAO ACPI String has reached the end of the "
				"ACPI table and did not have a 0x00 string "
				"terminator");
			*len = 0;
			return false;
		}
		ptr++;
	}
	*len = ptr - str;
	return true;
}

/*
 *  STAO Status Override Table
 *    http://wiki.xenproject.org/mediawiki/images/0/02/Status-override-table.pdf
 */
static int stao_test1(fwts_framework *fw)
{
	const fwts_acpi_table_stao *stao = (const fwts_acpi_table_stao *)table->data;
	bool passed = true;
	char *ptr, *end;
	fwts_list_link  *item;
	fwts_list *objects;
	int i = -1, strings = 0;
	char *ptr1, *ptr2;
	char *expanded;

	if (stao->header.length > (uint32_t)table->length) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"STAOBadLength",
			"STAO header reports that the table is %" PRIu32
			" bytes long, however this is longer than the ACPI "
			"table size of %zu bytes.",
			stao->header.length,
			table->length);
		goto done;
	}

	/* Now we have got some sane data, dump the STAO */
        fwts_log_info_verbatum(fw, "STAO Status Override Table:");
        fwts_log_info_verbatum(fw, "  UART:                     0x%2.2" PRIx8, stao->uart);

	ptr = (char *)stao->namelist;
	end = (char *)table->data + stao->header.length;

	while (ptr < end) {
		size_t len;

		if (!stao_acpi_string(fw, ptr, end, &passed, &len))
			break;

		fwts_log_info_verbatum(fw, "  ACPI String:              '%s'", ptr);
		strings++;
		/*  String length + null byte */
		ptr += len + 1;
	}
	if (!strings)
		goto done;

        if (fwts_acpi_init(fw) != FWTS_OK) {
		fwts_log_error(fw, "Cannot initialise ACPI, skipping ACPI string check");
		goto done;
	}
	if ((objects = fwts_acpi_object_get_names()) == NULL) {
		fwts_log_info(fw, "Cannot find any ACPI objects");
		fwts_acpi_deinit(fw);
		goto deinit;
	}
	ptr = (char *)stao->namelist;
	end = (char *)table->data + table->length;

	while (ptr < end) {
		bool not_found = true;
		size_t len;

		if (!stao_acpi_string(fw, ptr, end, &passed, &len))
			break;

		/* Find number of '.' in object path */
		for (i = 0, ptr1 = ptr; *ptr1; ptr1++) {
			if (*ptr1 == '.')
				i++;
		}

		/*
		 *  Allocate buffer big enough to take expanded path
		 *  and pad out any fields that are not 4 chars in size
		 *  between each . separator
		 *
		 *  Converts \_SB.A.BB.CCC.DDDD.EE to
		 *	     \_SB_.A___.BB__.CCC_.DDDD.EE__
		 */
		expanded = malloc(1 + (5 * (i + 1)));
		if (!expanded) {
			fwts_log_error(fw, "Cannot allocate temporary ACPI string buffer");
			goto deinit;
		}

		for (i = -1, ptr1 = ptr, ptr2 = expanded; ; ptr1++) {
			if (*ptr1 == '.' || *ptr1 == '\0') {
				while (i < 4) {
					*ptr2++ = '_';
					i++;
				}
				i = 0;
			} else {
				i++;
			}
			*ptr2++ = *ptr1;
			if (!*ptr1)
				break;
		}

		/* Search for object */
		fwts_list_foreach(item, objects) {
			char *name = fwts_list_data(char *, item);
			if (!strcmp(expanded, name)) {
				not_found = false;
				break;
			}
		}

		if (not_found) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"STAOAcpiStringNotFound",
				"STAO ACPI String '%s' not found in ACPI object name space.",
				expanded);
		}
		free(expanded);
		ptr += len + 1;
	}

deinit:
	fwts_acpi_deinit(fw);
done:
	if (passed)
		fwts_passed(fw, "No issues found in STAO table.");

	return FWTS_OK;
}

static fwts_framework_minor_test stao_tests[] = {
	{ stao_test1, "STAO Status Override Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops stao_ops = {
	.description = "STAO Status Override Table test.",
	.init        = stao_init,
	.minor_tests = stao_tests
};

FWTS_REGISTER("stao", &stao_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)
