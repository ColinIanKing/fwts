/*
 * Copyright (C) 2010-2023 Canonical
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


#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>

#include "fwts.h"

/*
 *  fwts_check_root_euid()
 *	Check if user has privileges to access ports, memory, etc
 */
int fwts_check_root_euid(fwts_framework *fw, const bool warn)
{
	if (geteuid() != 0) {
		if (warn) {
			if (warn)
				fwts_log_error(fw, "Must be run as root or sudo to be able to read system information.");
		}
		return FWTS_ERROR;
	}
	/* If $SNAP environment variable set, assume it's a snap install */
	if (getenv("SNAP") != NULL) {
		fwts_log_error(fw, "FWTS snaps need to be installed using --devmode to be able to read system information.");
		return FWTS_ERROR;
	}
	return FWTS_OK;
}
