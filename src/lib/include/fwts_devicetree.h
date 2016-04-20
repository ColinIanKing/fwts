/*
 * Copyright (C) 2016 IBM Corporation
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

#ifndef __FWTS_DEVICETREE_H__
#define __FWTS_DEVICETREE_H__

#include "fwts.h"

#ifdef HAVE_LIBFDT
#define FWTS_HAS_DEVICETREE 1
#else
#define FWTS_HAS_DEVICETREE 0
#endif

#if FWTS_HAS_DEVICETREE

int fwts_devicetree_read(fwts_framework *fwts);

#else /* !FWTS_HAS_DEVICETREE */
static inline int fwts_devicetree_read(fwts_framework *fwts
		__attribute__((unused)))
{
	return FWTS_OK;
}
#endif

#endif
