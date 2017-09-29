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

/* Test types */
#define METHOD_MANDATORY	1
#define METHOD_OPTIONAL	2
#define METHOD_MOBILE		4
#define METHOD_SILENT		8

#define ACPI_TYPE_INTBUF	(ACPI_TYPE_INVALID + 1)

typedef void (*fwts_method_return)(fwts_framework *fw, char *name,
	ACPI_BUFFER *ret_buff, ACPI_OBJECT *ret_obj, void *private);

typedef struct {
	ACPI_OBJECT_TYPE type;	/* Type */
	const char 	*name;	/* Field name */
} fwts_package_element;

#define fwts_method_check_type(fw, name, buf, type) 			\
	fwts_method_check_type__(fw, name, buf, type, #type)

int fwts_method_check_type__(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT_TYPE type, char *type_name);
const char *fwts_method_type_name(const ACPI_OBJECT_TYPE type);
void fwts_method_passed_sane(fwts_framework *fw, const char *name, const char *type);
void fwts_method_passed_sane_uint64(fwts_framework *fw, const char *name, const uint64_t value);
void fwts_method_failed_null_object(fwts_framework *fw, const char *name, const char *type);
bool fwts_method_type_matches(ACPI_OBJECT_TYPE t1, ACPI_OBJECT_TYPE t2);
int fwts_method_package_count_min(fwts_framework *fw, const char *name, const char *objname, const ACPI_OBJECT *obj, const uint32_t min);
int fwts_method_package_count_equal(fwts_framework *fw, const char *name, const char *objname, const ACPI_OBJECT *obj, const uint32_t count);
int fwts_method_package_elements_all_type(fwts_framework *fw, const char *name, const char *objname, const ACPI_OBJECT *obj, const ACPI_OBJECT_TYPE type);
int fwts_method_package_elements_type( fwts_framework *fw, const char *name, const char *objname, const ACPI_OBJECT *obj, const fwts_package_element *info, const uint32_t count);

void fwts_method_test_integer_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private);
void fwts_method_test_string_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private);
void fwts_method_test_reference_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private);
void fwts_method_test_NULL_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private);
void fwts_method_test_passed_failed_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private);
void fwts_method_test_polling_return( fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private);

void fwts_evaluate_found_method(fwts_framework *fw, ACPI_HANDLE *parent, char *name, fwts_method_return check_func, void *private, ACPI_OBJECT_LIST *arg_list);
int fwts_evaluate_method(fwts_framework *fw, uint32_t test_type, ACPI_HANDLE *parent, char *name, ACPI_OBJECT *args, uint32_t num_args, fwts_method_return check_func, void *private);

#endif
