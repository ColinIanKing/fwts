/*
 * Copyright (C) 2010-2017 Canonical
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

#ifndef __FWTS_ACPI_METHOD_H__
#define __FWTS_ACPI_METHOD_H__

#include "fwts.h"

/* acpica headers */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "acpi.h"
#pragma GCC diagnostic error "-Wunused-parameter"

int fwts_acpi_init(fwts_framework *fw);
int fwts_acpi_deinit(fwts_framework *fw);
char *fwts_acpi_object_exists(const char *name);
fwts_list *fwts_acpi_object_get_names(void);
void fwts_acpi_object_dump(fwts_framework *fw, const ACPI_OBJECT *obj);
void fwts_acpi_object_evaluate_report_error(fwts_framework *fw, const char *name, const ACPI_STATUS status);
ACPI_STATUS fwts_acpi_object_evaluate(fwts_framework *fw, char *name, ACPI_OBJECT_LIST *arg_list, ACPI_BUFFER *buf);

#endif
