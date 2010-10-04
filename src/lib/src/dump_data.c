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

#include <unistd.h>
#include <stdlib.h>

#include "fwts.h"

void fwts_dump_raw_data(char *buffer, const int len, const uint8_t *data, const int where, const int bytes)
{
        int i;
	int n = 0;

	n = snprintf(buffer, len, "  %4.4x: ", where);

        for (i=0;i<bytes;i++)
                n += snprintf(buffer + n, len - n, "%2.2x ", data[i]);

        for (;i<16;i++)
                n += snprintf(buffer + n, len -n , "   ");

        n += snprintf(buffer + n, len -n , " ");

        for (i=0;i<bytes;i++)
		buffer[n++] = (data[i] < 32 || data[i] > 126) ? '.' : data[i];
	buffer[n] = 0;
}
