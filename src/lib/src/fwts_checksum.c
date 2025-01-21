/*
 * Copyright (C) 2010-2025 Canonical
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

/*
 *  fwts_checksum()
 *	checksum a uint8_t * buffer
 */
uint8_t fwts_checksum(const uint8_t *data, const size_t length)
{
	size_t i;
	uint8_t checksum = 0;

	for (i = 0; i < length; i++)
		checksum += *data++;

	return checksum;
}
