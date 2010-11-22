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
#include "fwts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* acpica headers */
#include "acpi.h"

/* Test types */
#define	METHOD_MANDITORY	1
#define METHOD_OPTIONAL		2

#define method_check_type(fw, name, buf, type) \
	method_check_type__(fw, name, buf, type, #type)

typedef void (*method_test_return)(fwts_framework *fw, char *name, ACPI_BUFFER *ret_buff, ACPI_OBJECT *ret_obj, void *private);

static fwts_list *methods;

static int method_init(fwts_framework *fw)
{
	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	methods = fwts_acpica_get_object_names(8);
	return FWTS_OK;
}

static int method_deinit(fwts_framework *fw)
{
	fwts_list_free(methods, free);
	return fwts_acpica_deinit();
}

static char *method_headline(void)
{
	return "ACPI DSDT Method Semantic Tests.";
}

static int method_exits(char *name)
{
	int name_len = strlen(name);
	fwts_list_link	*item;

	fwts_list_foreach(item, methods) {
		char *method_name = (char*)item->data;
		int len = strlen(method_name);

		if (strncmp(name, method_name + len - name_len, name_len) == 0)
			return FWTS_TRUE;
	}

	return FWTS_FALSE;
}

static void method_dump_package(fwts_framework *fw, ACPI_OBJECT *obj)
{
	int i;

	fwts_log_info(fw, "Package has %d elements:", obj->Package.Count);

	for (i=0;i<obj->Package.Count;i++) {
		switch (obj->Package.Elements[i].Type) {
		case ACPI_TYPE_INTEGER:
			fwts_log_info_verbatum(fw, "  %2.2d: INTEGER: 0x%8.8x", i, obj->Package.Elements[i].Integer.Value);
			break;
		case ACPI_TYPE_STRING:
			fwts_log_info_verbatum(fw, "  %2.2d: STRING:  0x%s", i, obj->Package.Elements[i].String.Pointer);
			break;
		default:
			fwts_log_info_verbatum(fw, "  %2.2d: Uknown type %d\n", i, obj->Package.Elements[i].Type);
			break;
		}
	}
}

static void method_execute_found_method(fwts_framework *fw, char *name, 
	method_test_return check_func,
	void *private,
	ACPI_OBJECT_LIST *arg_list)
{
	ACPI_BUFFER       buf;
	ACPI_STATUS	  ret;
	int sem_acquired;
	int sem_released;

	buf.Length  = ACPI_ALLOCATE_BUFFER;
	buf.Pointer = NULL;

	ret = AcpiEvaluateObject(NULL, name, arg_list, &buf);
	if (ACPI_FAILURE(ret))
		fwts_log_error(fw, "Failed to execute %s.", name);
	else {
		if (check_func != NULL) {
			ACPI_OBJECT *obj = buf.Pointer;
			check_func(fw, name, &buf, obj, private);
		}		
	}
	if (buf.Length && buf.Pointer)
		free(buf.Pointer);

	fwts_acpica_sem_count_get(&sem_acquired, &sem_released);
	if (sem_acquired != sem_released)
		fwts_failed(fw, "%s left %d locks in an acquired state.", name, sem_acquired - sem_released);
	else
 		if ((sem_acquired + sem_released) > 0) 
			fwts_passed(fw, "%s correctly acquired and released locks %d times.", name, sem_acquired);

}

static int method_execute_method(fwts_framework *fw,
	int test_type,  /* Manditory or optional */
	char *name, 
	ACPI_OBJECT *args, 
	int num_args, 
	method_test_return check_func,
	void *private)
{
	fwts_list_link	*item;
	int name_len = strlen(name);
	int found = 0;

	fwts_acpica_sem_count_clear();

	fwts_list_foreach(item, methods) {
		char *method_name = (char*)item->data;
		int len = strlen(method_name);
		if (strncmp(name, method_name + len - name_len, name_len) == 0) {
			ACPI_OBJECT_LIST  arg_list;

			found++;
			arg_list.Count   = num_args;
			arg_list.Pointer = args;
			fwts_acpica_simulate_sem_timeout(FWTS_FALSE);
			method_execute_found_method(fw, method_name, check_func, private, &arg_list);

			/*
			arg_list.Count   = num_args;
			arg_list.Pointer = args;
			fwts_acpica_simulate_sem_timeout(FWTS_TRUE);
			method_execute_found_method(fw, method_name, check_func, private, &arg_list);
			*/
		}
	}
	
	if (!found) {
		if (test_type == METHOD_OPTIONAL) {
			fwts_skipped(fw, "Optional method %s did not exist.", name);
			return FWTS_NOT_EXIST;
		} else {
			fwts_failed(fw, "Method %s did not exist.", name);
			return FWTS_NOT_EXIST;
		}
	}

	return FWTS_OK;
}

static int method_name_check(fwts_framework *fw)
{
	fwts_list_link	*item;
	int failed = 0;

	fwts_log_info(fw, "Found %d Methods\n", methods->len);

	fwts_list_foreach(item, methods) {
		char *ptr;

		for (ptr = item->data; *ptr; ptr++) {
			if (!((*ptr == '\\') ||
			     (*ptr == '.') ||
			     (*ptr == '_') ||
			     (isdigit(*ptr)) ||
			     (isupper(*ptr))) ) {
				fwts_failed(fw, "Method %s contains an illegal character: '%c'.", item->data, *ptr);
				failed++;
				break;
			}
		}
	}
	if (!failed)
		fwts_passed(fw, "Method names contain legal characters.");

	return FWTS_OK;
}

static int method_check_type__(fwts_framework *fw, char *name, ACPI_BUFFER *buf, int type, char *type_name)
{
	ACPI_OBJECT *obj = buf->Pointer;

	if (obj->Type != type) {
		fwts_failed(fw, "Method %s did not return %s.", name, type_name);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static void method_test_integer_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw, "%s correctly returned sane looking value 0x%8.8x", name, (uint32_t)obj->Integer.Value);
}

static void method_test_NULL_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (buf->Length && buf->Pointer)
		fwts_failed(fw, "%s returned values, but was expected to return nothing.", name);
	else
		fwts_passed(fw, "%s returned no values as expected.", name);
}

#if 0
static void method_test_OSI_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_log_info(fw, "_OSI returned 0x%8.8x\n", (uint32_t)obj->Integer.Value);
}

static int method_test_OSI(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_STRING;
	arg[0].String.Pointer = "Windows 2001";
	arg[0].String.Length = strlen(arg[0].String.Pointer);

	return method_execute_method(fw, METHOD_MANDITORY, "\\_OSI", arg, 1, method_test_OSI_return, NULL);
}
#endif

static void method_test_BIF_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		if (obj->Package.Count != 13) {
			fwts_failed(fw, "_BIF package should return 13 elements, got %d instead.",
				obj->Package.Count);
}

		for (i=0;(i<9) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BIF package element %d is not of type DWORD Integer.");
				failed++;
			}
		}
		for (i=9;(i<13) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_STRING) {
				fwts_failed(fw, "_BIF package element %d is not of type STRING.", i);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Power Unit */
		if (obj->Package.Elements[0].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "Expected Power Unit (Element 0) to be 0 (mWh) or 1 (mAh), got 0x%8.8x.", 
					obj->Package.Elements[0].Integer.Value);
			failed++;
		}
		/* Design Capacity */
		if (obj->Package.Elements[1].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Capacity (Element 1) is unknown: 0x%8.8x.", 
					obj->Package.Elements[1].Integer.Value);
		}
		/* Last Full Charge Capacity */
		if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Last Full Charge Capacity (Element 2) is unknown: 0x%8.8x.", 
					obj->Package.Elements[2].Integer.Value);
		}
		/* Battery Technology */
		if (obj->Package.Elements[3].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "Expected Battery Technology Unit (Element 3) to be 0 (Primary) or 1 (Secondary), got 0x%8.8x.", 
					obj->Package.Elements[3].Integer.Value);
			failed++;
		}
		/* Design Voltage */
		if (obj->Package.Elements[4].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Voltage (Element 4) is unknown: 0x%8.8x.", 
					obj->Package.Elements[4].Integer.Value);
			failed++;
		}
		/* Design capacity warning */
		if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Capacity Warning (Element 5) is unknown: 0x%8.8x.", 
					obj->Package.Elements[5].Integer.Value);
			failed++;
		}
		/* Design capacity low */
		if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Capacity Warning (Element 6) is unknown: 0x%8.8x.", 
					obj->Package.Elements[6].Integer.Value);
			failed++;
		}
		if (!failed)
			fwts_passed(fw, "Battery _BIF package looks sane.");
	}
}

static int method_test_BIF(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BIF", NULL, 0, method_test_BIF_return, NULL);
}

static void method_test_BIX_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		if (obj->Package.Count != 16) {
			fwts_failed(fw, "_BIX package should return 16 elements, got %d instead.",
				obj->Package.Count);
}

		for (i=0;(i<16) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BIX package element %d is not of type DWORD Integer.");
				failed++;
			}
		}
		for (i=16;(i<20) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_STRING) {
				fwts_failed(fw, "_BIX package element %d is not of type STRING.", i);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Power Unit */
		if (obj->Package.Elements[1].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "Expected Power Unit (Element 1) to be 0 (mWh) or 1 (mAh), got 0x%8.8x.", 
					obj->Package.Elements[1].Integer.Value);
			failed++;
		}
		/* Design Capacity */
		if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Capacity (Element 2) is unknown: 0x%8.8x.", 
					obj->Package.Elements[2].Integer.Value);
		}
		/* Last Full Charge Capacity */
		if (obj->Package.Elements[3].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Last Full Charge Capacity (Element 3) is unknown: 0x%8.8x.", 
					obj->Package.Elements[3].Integer.Value);
		}
		/* Battery Technology */
		if (obj->Package.Elements[4].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "Expected Battery Technology Unit (Element 4) to be 0 (Primary) or 1 (Secondary), got 0x%8.8x.", 
					obj->Package.Elements[4].Integer.Value);
			failed++;
		}
		/* Design Voltage */
		if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Voltage (Element 5) is unknown: 0x%8.8x.", 
					obj->Package.Elements[5].Integer.Value);
			failed++;
		}
		/* Design capacity warning */
		if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Capacity Warning (Element 6) is unknown: 0x%8.8x.", 
					obj->Package.Elements[6].Integer.Value);
			failed++;
		}
		/* Design capacity low */
		if (obj->Package.Elements[7].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Design Capacity Warning (Element 7) is unknown: 0x%8.8x.", 
					obj->Package.Elements[7].Integer.Value);
			failed++;
		}
		/* Cycle Count */
		if (obj->Package.Elements[10].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "Cycle Count (Element 10) is unknown: 0x%8.8x.", 
					obj->Package.Elements[10].Integer.Value);
			failed++;
		}
		if (!failed)
			fwts_passed(fw, "Battery _BIX package looks sane.");
	}
}
static int method_test_BIX(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BIX", NULL, 0, method_test_BIX_return, NULL);
}


static void method_test_BST_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		if (obj->Package.Count != 4) {
			fwts_failed(fw, "_BST package should return 4 elements, got %d instead.",
				obj->Package.Count);
}

		for (i=0;(i<4) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BIF package element %d is not of type DWORD Integer.");
				failed++;
			}
		}

		/* Sanity check each field */
		/* Battery State */
		if (obj->Package.Elements[0].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "Expected Battery State (Element 0) to be 0..2, got 0x%8.8x.", 
					obj->Package.Elements[0].Integer.Value);
			failed++;
		}
		/* Battery Present Rate - cannot check, pulled from EC */
		/* Battery Remaining Capacity - cannot check, pulled from EC */
		/* Battery Present Voltage - cannot check, pulled from EC */
		if (!failed)
			fwts_passed(fw, "Battery _BST package looks sane.");
	}
}

static int method_test_BST(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BST", NULL, 0, method_test_BST_return, NULL);
}

static void method_test_BMD_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		if (obj->Package.Count != 5) {
			fwts_failed(fw, "_BMD package should return 4 elements, got %d instead.",
				obj->Package.Count);
}

		for (i=0;(i<4) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BMD package element %d is not of type DWORD Integer.");
				failed++;
			}
		}
		/* TODO: check return values */
	}
}

static int method_test_BMD(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BMD", NULL, 0, method_test_BMD_return, NULL);
}

/* section 7.3 OEM-Supplied System-Level Control Methods */
/* _BFS, _PTS, _GTS, _S0 .. _S5, _TTS, _WAK */

static int method_test_BFS(fwts_framework *fw)
{
	int i;

	if (method_exits("_BFS")) {
		for (i=1; i<6; i++) {
			ACPI_OBJECT arg[1];
	
			arg[0].Type = ACPI_TYPE_INTEGER;
			arg[0].Integer.Value = i;
	
			if (method_execute_method(fw, METHOD_OPTIONAL, "_BFS", arg, 1,
			  			  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
				break;
		}
	}
	else {
		fwts_skipped(fw, "Optional control method _BFS does not exist.");
	}

	return FWTS_OK;
}

static int method_test_PTS(fwts_framework *fw)
{
	int i;

	for (i=1; i<6; i++) {
		ACPI_OBJECT arg[1];

		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;

		fwts_log_info(fw, "Test _PTS(%d).", i);

		if (method_execute_method(fw, METHOD_MANDITORY, "_PTS", arg, 1,
					  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_GTS(fwts_framework *fw)
{
	int i;

	if (method_exits("_BFS")) {
		for (i=1; i<6; i++) {
			ACPI_OBJECT arg[1];
	
			arg[0].Type = ACPI_TYPE_INTEGER;
			arg[0].Integer.Value = i;

			fwts_log_info(fw, "Test _GTS(%d) (Going To Sleep, State S%d.", i, i);

			if (method_execute_method(fw, METHOD_OPTIONAL, "_GTS", arg, 1, 
						  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
				break;
		}
	}
	else {
		fwts_skipped(fw, "Optional control method _GTS does not exist.");
	}
	return FWTS_OK;
}

static int method_test_TTS(fwts_framework *fw)
{
	int i;

	if (method_exits("_BFS")) {
		for (i=1; i<6; i++) {
			ACPI_OBJECT arg[1];
	
			arg[0].Type = ACPI_TYPE_INTEGER;
			arg[0].Integer.Value = i;

			fwts_log_info(fw, "Test _TTS(%d) Transition To State S%d.", i, i);
	
			if (method_execute_method(fw, METHOD_MANDITORY, "_TTS", arg, 1,
						  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
				break;
		}
	}
	else {
		fwts_skipped(fw, "Optional control method _TTS does not exist.");
	}
	return FWTS_OK;
}

static void method_test_WAK_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	method_dump_package(fw, obj);
	uint32_t Sstate = *(uint32_t*)private;
	int failed = 0;

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		if (obj->Package.Count != 2) {
			fwts_failed(fw, "_WAK should return package of 2 integers, got %d elements instead.", 
				obj->Package.Count);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER))  {
				fwts_failed(fw, "_WAK should return package of 2 integers.", 
					obj->Package.Count);
			}
			else {
				if (obj->Package.Elements[0].Integer.Value > 0x00000002) {	
					fwts_failed(fw, "Expecting condition bit-field (element 0) of packages to be in range, got 0x%8.8x.", 
						obj->Package.Elements[0].Integer.Value);
					failed++;
				}
				if (obj->Package.Elements[1].Integer.Value != Sstate) {	
					fwts_failed(fw, "Expecting power supply S-state (element 1) of packages to be 0x%8.8x, got 0x%8.8x.", 
						Sstate, obj->Package.Elements[0].Integer.Value);
					failed++;
				}
				if (!failed)
					fwts_passed(fw, "_WAK correctly returned sane looking package.");
			}
		}
	}
}

static int method_test_WAK(fwts_framework *fw)
{
	uint32_t i;

	for (i=1; i<6; i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;
		fwts_log_info(fw, "Test _WAK(%d) System Wake, State S%d.", i, i);
		if (method_execute_method(fw, METHOD_MANDITORY, "_WAK", arg, 1,
					  method_test_WAK_return, &i) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static void method_test_PSR_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if (obj->Integer.Value > 2)
			fwts_failed(fw, "_PSR returned 0x%8.8x\n, expected 0 (offline) or 1 (online)", (uint32_t)obj->Integer.Value);
		else
			fwts_passed(fw, "_PSR correctly returned sane looking value 0x%8.8x", (uint32_t)obj->Integer.Value);
	}
}

/* Section 10.3 AC Adapters and Power Sources Objects */

static int method_test_PSR(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_MANDITORY, "_PSR", NULL, 0, method_test_PSR_return, NULL);
}

/* Section 9.4.1 Lid control */

static void method_test_LID_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw, "_LID correctly returned sane looking value 0x%8.8x", (uint32_t)obj->Integer.Value);
}

static int method_test_LID(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_LID", NULL, 0, method_test_LID_return, NULL);
}

/* Section 11.4 Thermal */

static void method_test_THERM_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		char *method = (char*)private;
		if (obj->Integer.Value >= 2732)
			fwts_passed(fw, "%s correctly returned sane looking value 0x%8.8x (%5.1f degrees K)", 
				method,
				(uint32_t)obj->Integer.Value,
				(float)((uint32_t)obj->Integer.Value) / 10.0);
		else 
			fwts_passed(fw, "%s correctly returned value below 0 degrees C: 0x%8.8x (%5.1f degrees K)", 
				method,
				(uint32_t)obj->Integer.Value,
				(float)((uint32_t)obj->Integer.Value) / 10.0);

	}
}

#define method_test_THERM(name, type)			\
static int method_test ## name(fwts_framework *fw)	\
{							\
	return method_execute_method(fw, type, # name, NULL, 0, method_test_THERM_return, # name);				\
}			

method_test_THERM(_CRT, METHOD_OPTIONAL)
method_test_THERM(_HOT, METHOD_OPTIONAL)
method_test_THERM(_TMP, METHOD_OPTIONAL)
method_test_THERM(_NTT, METHOD_OPTIONAL)
method_test_THERM(_PSV, METHOD_OPTIONAL)

static void method_test_RTV_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw, "_RTV`correctly returned sane looking value 0x%8.8x", (uint32_t)obj->Integer.Value);
}

static int method_test_RTV(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_RTV", NULL, 0, method_test_RTV_return, "_RTV");
}

#define method_test_integer(name, type)			\
static int method_test ## name(fwts_framework *fw)	\
{							\
	return method_execute_method(fw, type, # name, NULL, 0, method_test_integer_return, # name);				\
}			

method_test_integer(_ALC, METHOD_OPTIONAL)
method_test_integer(_ALI, METHOD_OPTIONAL)
method_test_integer(_ALT, METHOD_OPTIONAL)
method_test_integer(_BBN, METHOD_OPTIONAL)
method_test_integer(_BDN, METHOD_OPTIONAL)

static int method_test_BMA(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;
	return method_execute_method(fw, METHOD_OPTIONAL, "_BMA", arg, 1, method_test_integer_return, NULL);
}

static int method_test_BMS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;
	return method_execute_method(fw, METHOD_OPTIONAL, "_BMS", arg, 1, method_test_integer_return, NULL);
}

static int method_test_BTP(fwts_framework *fw)
{
	static int values[] = { 0, 1, 100, 200, 0x7fffffff };
	int i;

	for (i=0;i<5;i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		if (method_execute_method(fw, METHOD_OPTIONAL, "_BTP", arg, 1,
					  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_BTM(fwts_framework *fw)
{
	static int values[] = { 0, 1, 100, 200, 0x7fffffff };
	int i;

	for (i=0;i<5;i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		if (method_execute_method(fw, METHOD_OPTIONAL, "_BTM", arg, 1,
					  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_BMC(fwts_framework *fw)
{
	static int values[] = { 0, 1, 2, 4 };
	int i;

	for (i=0;i<4;i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		if (method_execute_method(fw, METHOD_OPTIONAL, "_BMC", arg, 1,
					  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static void method_test_PCL_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	/* FIXME */
}

static int method_test_PCL(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_PCL", NULL, 0, method_test_PCL_return, "_PCL");
}

/* Tests */

static fwts_framework_minor_test method_tests[] = {
	{ method_name_check, "Check Method Names." },
	{ method_test_ALC, "Check _ALC Ambient Light Colour Chromaticity." },
	{ method_test_ALI, "Check _ALI Ambient Light Illuminance." },
	{ method_test_ALT, "Check _ALT Ambient Light Temperature." },
	{ method_test_BBN, "Check _BBN Base Bus Number." },
	{ method_test_BDN, "Check _BDN BIOS Dock Name." },
	{ method_test_BIF, "Check _BIF (Battery Information) Method." },
	{ method_test_BIX, "Check _BIX (Battery Information Extended) Method." },
	{ method_test_BMA, "Check _BMA (Battery Measurement Averaging) Method." },
	{ method_test_BMS, "Check _BMS (Battery Measurement Sampling Time) Method." },
	{ method_test_BMD, "Check _BMD (Battery Maintenance Data) Method." },
	{ method_test_BMC, "Check _BMC (Battery Maintenance Control) Method." },
	{ method_test_BST, "Check _BST (Battery Status) Method." },
	{ method_test_BTP, "Check _BTP (Battery Trip Point) Method." },
	{ method_test_BTM, "Check _BTM (Battery Time) Method." },
	{ method_test_PCL, "Check _PCL (Power Consumer List) Method." },
	/* { method_test_OSI, "Check _OSI Method." }, */
	{ method_test_BFS, "Check _BFS (Back from Sleep) Method." },
	{ method_test_PTS, "Check _PTS (Prepare to Sleep) Method." },
	{ method_test_GTS, "Check _GTS (Going to Sleep) Method." },
	{ method_test_TTS, "Check _TTS (Trasition to State) Method." },
	{ method_test_WAK, "Check _WAK (System Wake Method)." },
	{ method_test_PSR, "Check _PSR (Power Source) Method." },
	{ method_test_LID, "Check _LID (Lid Ststus) Method." },
	{ method_test_CRT, "Check _CRT (Critical Trip Point) Method." },
	{ method_test_HOT, "Check _HOT (Hot Temperature) Method." },
	{ method_test_TMP, "Check _TMP (Thermal Zone Current Temp) Method." },
	{ method_test_NTT, "Check _NTT (Notification Temp Threshold) Method." },
	{ method_test_PSV, "Check _PSV (Passive Temp) Method." },
	{ method_test_RTV, "Check _RTV (Relative Temp Values) Method." },
	/* { method_test_SCP, "Check _SCP (Set Cooling Policy) Method." }, */
	{ NULL, NULL }
};

static fwts_framework_ops method_ops = {
	.headline    = method_headline,
	.init        = method_init,
	.deinit      = method_deinit,
	.minor_tests = method_tests
};

FWTS_REGISTER(method, &method_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
