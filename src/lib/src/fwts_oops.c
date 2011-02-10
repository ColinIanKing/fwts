/*
 * Copyright (C) 2011 Canonical
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
#include <sys/types.h>

#include "fwts.h"

#define FWTS_OOPS_GOT_OOPS		(0x0001)
#define FWTS_OOPS_GOT_STACK		(0x0002)
#define FWTS_OOPS_GOT_CALL_TRACE	(0x0004)
#define FWTS_OOPS_GOT_END_TRACE		(0x0008)

#define FWTS_OOPS_DUMPABLE		\
	(FWTS_OOPS_GOT_OOPS | FWTS_OOPS_GOT_STACK | FWTS_OOPS_GOT_CALL_TRACE | FWTS_OOPS_GOT_END_TRACE)


void fwts_oops_dump(fwts_framework *fw, fwts_list_link *bug_item, int *oopses)
{
	fwts_list_link *item = bug_item;
	int lines = 0;
	int dumpable = 0;

	fwts_list_foreach_continue(item) {
		char *line = fwts_klog_remove_timestamp(fwts_list_data(char *, item));

		if (strstr(line, "Oops:"))
			dumpable |= FWTS_OOPS_GOT_OOPS;
		if (strstr(line, "Stack:"))
			dumpable |= FWTS_OOPS_GOT_STACK;
		if (strstr(line, "Call Trace:"))
			dumpable |= FWTS_OOPS_GOT_CALL_TRACE;
		if (strstr(line, "--[ end trace")) { 
			dumpable |= FWTS_OOPS_GOT_END_TRACE;
			break;
		}
		lines++;

		/* We are looking for an Oops message within 5 lines of a "BUG:" */
		if ((lines > 5) && (!(dumpable & FWTS_OOPS_GOT_OOPS)))
			return;

	}

	/* Sanity check: too many lines? it is a bit suspect */
	if (lines > 100)
		return;

	if (item == NULL)
		return;


	/* Found all the features that indicate an oops, so dump it */
	if (dumpable & FWTS_OOPS_DUMPABLE) {
		(*oopses)++;
		fwts_log_info(fw, "Found OOPS (%d):", *oopses);

		while (bug_item != NULL && bug_item != item) {
			fwts_log_info_verbatum(fw, "  %s", 
				fwts_klog_remove_timestamp(fwts_list_data(char *, bug_item)));
			bug_item = bug_item->next;
		}
		fwts_log_nl(fw);
	}
}

int fwts_oops_check(fwts_framework *fw, fwts_list *klog, int *oopses)
{
	fwts_list_link *item;
	*oopses = 0;

	if ((fw == NULL) || (oopses == NULL) || (klog == NULL))
		return FWTS_ERROR;

	fwts_list_foreach(item, klog) {
		if (strncmp("BUG:", fwts_klog_remove_timestamp(fwts_list_data(char *, item)), 4) == 0)
			fwts_oops_dump(fw, item, oopses);
	}

	return FWTS_OK;
}
