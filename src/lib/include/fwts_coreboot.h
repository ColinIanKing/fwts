/*
 * Copyright (C) 2010-2021 Canonical
 * Copyright (C) 2017-2021 Google Inc.
 * Copyright (C) 2018-2021 9elements Cyber Security
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

#ifndef __FWTS_COREBOOT_H__
#define __FWTS_COREBOOT_H__

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

extern char *fwts_coreboot_cbmem_console_dump(void);

#endif

fwts_list* fwts_coreboot_cbmem_log(void);

#endif
