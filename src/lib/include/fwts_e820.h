/* 
 * Copyright (C) 2006, Intel Corporation
 * Copyright (C) 2010 Canonical
 * 
 * This file was originally part of the Linux-ready Firmware Developer Kit
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

#ifndef __FWTS_E820_H__
#define __FWTS_E820_H__

#include "fwts_types.h"
#include "fwts_list.h"
#include "fwts_framework.h"

#define E820_UNKNOWN    0
#define E820_RESERVED   1
#define E820_ACPI       2
#define E820_USABLE     16

int        fwts_e820_type(fwts_list *e820_list, uint64 memory);
int        fwts_e820_is_reserved(fwts_list *e820_list, uint64 memory);
fwts_list *fwts_e820_table_load(fwts_framework *fw);
void       fwts_e820_table_free(fwts_list *e820_list);

#endif
