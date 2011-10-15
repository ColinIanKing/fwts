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
#include "fwts.h"

/* acpica headers */
#include "acpi.h"

int fwts_method_init(fwts_framework *fw);
int fwts_method_deinit(fwts_framework *fw);
char *fwts_method_exists(char *name);
fwts_list *fwts_method_get_names(void);
void fwts_method_dump_object(fwts_framework *fw, ACPI_OBJECT *obj);
void fwts_method_evaluate_report_error(fwts_framework *fw, char *name, ACPI_STATUS status);
ACPI_STATUS fwts_method_evaluate(fwts_framework *fw, char *name, ACPI_OBJECT_LIST *arg_list, ACPI_BUFFER *buf);
