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
 */
#ifndef __FWTS_FIRMWARE_H__
#define __FWTS_FIRMWARE_H__

enum {
	FWTS_FIRMWARE_UNKNOWN = 0,
	FWTS_FIRMWARE_BIOS = 1,
	FWTS_FIRMWARE_UEFI = 2,
	FWTS_FIRMWARE_OTHER = 100,
};

int fwts_firmware_detect(void);

#endif
