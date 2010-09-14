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

#ifndef __FWTS_TYPES_H__
#define __FWTS_TYPES_H__

#include <stdint.h>

typedef enum {
	FWTS_OK    = 0,
	FWTS_ERROR = -1,
	FWTS_SKIP  = -2,
} fwts_status;

typedef enum {
	FWTS_FALSE = 0,
	FWTS_TRUE  = 1,
	FWTS_BOOL_ERROR = -1
} fwts_bool;

/*
 *  Some standard types
 */
typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

typedef int8_t		int8;
typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t		int64;

#endif
