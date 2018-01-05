/*
 * Copyright (C) 2010-2018 Canonical
 * Some of this work - Copyright (C) 2016-2018 IBM
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

#include "config.h"

#if defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
#define FWTS_ARCH_INTEL	1
#define FWTS_HAS_ACPI	1
#define FWTS_HAS_UEFI	1
#endif

#if defined(__aarch64__)
#define FWTS_ARCH_AARCH64	1
#define FWTS_HAS_ACPI	1
#define FWTS_HAS_UEFI	1
#endif

#if defined(__s390x__)
#define FWTS_ARCH_S390X	1
#endif

#if defined(__PPC64__)
#undef FWTS_HAS_ACPI
#undef FWTS_HAS_UEFI
#endif

/* verision 3-tuple into integer */
#define _VER_(major, minor, patchlevel)                 \
	((major * 10000) + (minor * 100) + patchlevel)

/* check version of GNU GCC */
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#if defined(__GNUC_PATCHLEVEL__)
#define NEED_GNUC(major, minor, patchlevel)                     \
	_VER_(major, minor, patchlevel) <= _VER_(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#define NEED_GNUC(major, minor, patchlevel)                     \
	_VER_(major, minor, patchlevel) <= _VER_(__GNUC__, __GNUC_MINOR__, 0)
#endif
#else
#define NEED_GNUC(major, minor, patchlevel)     (0)
#endif

/* -O0 attribute support */
#if defined(__GNUC__) && !defined(__clang__) && NEED_GNUC(4,6,0)
#define OPTIMIZE0	__attribute__((optimize("-O0")))
#else
#define OPTIMIZE0
#endif

#define FWTS_UNUSED(var)	(void)var

#define FWTS_JSON_DATA_PATH	DATAROOTDIR "/fwts"

#include "fwts_version.h"
#include "fwts_backtrace.h"
#include "fwts_types.h"
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
#include "fwts_arch.h"
#include "fwts_checkeuid.h"
#include "fwts_cpu.h"
#include "fwts_dump.h"
#include "fwts_dump_data.h"
#include "fwts_memorymap.h"
#include "fwts_fileio.h"
#include "fwts_firmware.h"
#include "fwts_gpe.h"
#include "fwts_iasl.h"
#include "fwts_ipmi.h"
#include "fwts_klog.h"
#include "fwts_olog.h"
#include "fwts_pipeio.h"
#include "fwts_stringextras.h"
#include "fwts_tty.h"
#include "fwts_wakealarm.h"
#include "fwts_formatting.h"
#include "fwts_summary.h"
#include "fwts_mmap.h"
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
#include "fwts_pci.h"
#include "fwts_safe_mem.h"
#include "fwts_devicetree.h"

#endif
