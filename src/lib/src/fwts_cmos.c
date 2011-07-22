/*
 * Copyright (C) 2010-2011 Canonical
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

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL
#include <sys/io.h>
/*
 *  fwts_cmos_read()
 *	read a byte from cmos memory at a given offset
 */
int fwts_cmos_read(const uint8_t offset, uint8_t *value)
{
	if (ioperm(0x70, 2, 1) < 0)
		return FWTS_ERROR;
	if (ioperm(0x80, 1, 1) < 0)
		return FWTS_ERROR;

	outb(offset, 0x70);	/* specify offset to read */
	outb(0, 0x80);		/* Small Delay */
	*value = inb(0x71);	/* get the value */

	(void)ioperm(0x80, 1, 0);
	(void)ioperm(0x70, 2, 0);

	return FWTS_OK;
}
#else
int fwts_cmos_read(const uint8_t offset, uint8_t *value)
{
	return FWTS_ERROR;
}
#endif
