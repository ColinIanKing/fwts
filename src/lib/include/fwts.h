/*
 * Copyright (C) 2010-2013 Canonical
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

#ifndef __FWTS_H__
#define __FWTS_H__

#if defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
#define FWTS_ARCH_INTEL	1
#endif

#define FWTS_UNUSED(var)	(void)var

#define FWTS_JSON_DATA_PATH	"/usr/share/fwts"

#include "fwts_version.h"
#include "fwts_types.h"
#include "fwts_tag.h"
#include "fwts_binpaths.h"
#include "fwts_framework.h"
#include "fwts_log.h"
#include "fwts_list.h"
#include "fwts_text_list.h"
#include "fwts_set.h"
#include "fwts_get.h"
#include "fwts_acpi.h"
#include "fwts_acpi_tables.h"
#include "fwts_acpid.h"
#include "fwts_checkeuid.h"
#include "fwts_cpu.h"
#include "fwts_dump.h"
#include "fwts_dump_data.h"
#include "fwts_memorymap.h"
#include "fwts_fileio.h"
#include "fwts_firmware.h"
#include "fwts_gpe.h"
#include "fwts_iasl.h"
#include "fwts_klog.h"
#include "fwts_pipeio.h"
#include "fwts_stringextras.h"
#include "fwts_tty.h"
#include "fwts_wakealarm.h"
#include "fwts_virt.h"
#include "fwts_formatting.h"
#include "fwts_summary.h"
#include "fwts_mmap.h"
#include "fwts_microcode.h"
#include "fwts_interactive.h"
#include "fwts_keymap.h"
#include "fwts_cmos.h"
#include "fwts_acpica.h"
#include "fwts_oops.h"
#include "fwts_hwinfo.h"
#include "fwts_args.h"
#include "fwts_multiproc.h"
#include "fwts_ebda.h"
#include "fwts_alloc.h"
#include "fwts_guid.h"
#include "fwts_scan_efi_systab.h"
#include "fwts_checksum.h"
#include "fwts_smbios.h"
#include "fwts_ac_adapter.h"
#include "fwts_battery.h"
#include "fwts_button.h"
#include "fwts_json.h"
#include "fwts_ioport.h"
#include "fwts_release.h"

#endif
