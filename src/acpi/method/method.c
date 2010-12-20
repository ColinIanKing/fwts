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
#include <unistd.h>

/* acpica headers */
#include "acpi.h"

/* Test types */
#define	METHOD_MANDITORY	1
#define METHOD_OPTIONAL		2

#define method_check_type(fw, name, buf, type) \
	method_check_type__(fw, name, buf, type, #type)

#define method_test_integer(name, type)			\
static int method_test ## name(fwts_framework *fw)	\
{ 							\
	return method_execute_method(fw, type, # name, NULL, 0, method_test_integer_return, # name); \
}


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
		char *method_name = fwts_list_data(char*, item);
		int len = strlen(method_name);

		if (strncmp(name, method_name + len - name_len, name_len) == 0)
			return FWTS_TRUE;
	}

	return FWTS_FALSE;
}

static void method_dump_package(fwts_framework *fw, ACPI_OBJECT *obj)
{
	int i;

	fwts_log_info_verbatum(fw, "  Package has %d elements:", obj->Package.Count);

	for (i=0;i<obj->Package.Count;i++) {
		switch (obj->Package.Elements[i].Type) {
		case ACPI_TYPE_INTEGER:
			fwts_log_info_verbatum(fw, "    %2.2d: INTEGER: 0x%8.8llx", i,
				(unsigned long long)obj->Package.Elements[i].Integer.Value);
			break;
		case ACPI_TYPE_STRING:
			fwts_log_info_verbatum(fw, "    %2.2d: STRING:  0x%s", i, obj->Package.Elements[i].String.Pointer);
			break;
		default:
			fwts_log_info_verbatum(fw, "    %2.2d: Uknown type %d\n", i, obj->Package.Elements[i].Type);
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
	if (ACPI_FAILURE(ret) != FWTS_OK) {
		switch (ret) {
		case AE_AML_INFINITE_LOOP:
			fwts_failed_medium(fw, "Detected an infinite loop when executing method '%s'. ", name);
			fwts_advice(fw, "This may occur because we are emulating the execution "
					"in this test environment and cannot handshake with "
					"the embedded controller or jump to the BIOS via SMIs. "
					"However, the fact that AML code spins forever means that "
					"lockup conditions are not being checked for in the AML bytecode.");
			break;
		default:
			fwts_log_error(fw, "Failed to execute %s.", name);
			break;
		}
	}
	else {
		if (check_func != NULL) {
			ACPI_OBJECT *obj = buf.Pointer;
			check_func(fw, name, &buf, obj, private);
		}		
	}
	if (buf.Length && buf.Pointer)
		free(buf.Pointer);

	/* Seen ACPICA core not release locks quickly, so put a delay in. Urgh. */
	usleep(100000);

	fwts_acpica_sem_count_get(&sem_acquired, &sem_released);
	if (sem_acquired != sem_released) {
		fwts_failed(fw, "%s left %d locks in an acquired state.", name, sem_acquired - sem_released);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_MUTEX);
		fwts_advice(fw, "Locks left in an acquired state generally indicates that the AML code is not "
				"releasing a lock. This can sometimes occur when a method hits an error condition "
				"and exits prematurely without releasing an acquired lock. It may be occurring in the "
				"method being tested or other methods used while executing the method.");
	} else
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
		char *method_name = fwts_list_data(char*, item);
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

		for (ptr = fwts_list_data(char *, item); *ptr; ptr++) {
			if (!((*ptr == '\\') ||
			     (*ptr == '.') ||
			     (*ptr == '_') ||
			     (isdigit(*ptr)) ||
			     (isupper(*ptr))) ) {
				fwts_failed_high(fw, "Method %s contains an illegal character: '%c'. This should be corrected.", 
					fwts_list_data(char *, item), *ptr);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD);
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
	ACPI_OBJECT *obj;

	if ((buf == NULL) || (buf->Pointer == NULL)){
		fwts_failed(fw, "Method %s returned a NULL object, and did not return %s.", name, type_name);
		return FWTS_ERROR;
	}

	obj = buf->Pointer;

	if (obj->Type != type) {
		fwts_failed(fw, "Method %s did not return %s.", name, type_name);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static void method_test_integer_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw, "%s correctly returned an integer.", name);
}

static void method_test_NULL_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (buf->Length && buf->Pointer) {
		fwts_failed(fw, "%s returned values, but was expected to return nothing.", name);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		fwts_advice(fw, "This probably won't cause any errors, but it should be fixed as the AML code is "
				"not conforming to the expected behaviour as described in the ACPI specification.");
	} else
		fwts_passed(fw, "%s returned no values as expected.", name);
}

static void method_test_passed_failed_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	char *method = (char *)private;
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		unsigned int val = (uint32_t)obj->Integer.Value;
		if ((val == 0) || (val == 1))
			fwts_passed(fw, "%s correctly returned sane looking value 0x%8.8x.", method, val);
		else {
			fwts_failed(fw, "%s returned 0x%8.8x, should return 1 (success) or 0 (failed).", method, val);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw, "Method %s should be returning the correct 1/0 success/failed return values. "
					"Unexpected behaviour may occur becauses of this error, the AML code does not "
					"conform to the ACPI specification and should be fixed.", method);
		}
	}
}

/* Section 10.1.1.1 Smart Battery */

static void method_test_SBS_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	static char *sbs_info[] = {
		"Maximum 1 Smart Battery, system manager/selector not present",
		"Maximum 1 Smart Battery, system manager/selector present",
		"Maximum 2 Smart Batteries, system manager/selector present",
		"Maximum 3 Smart Batteries, system manager/selector present",
		"Maximum 4 Smart Batteries, system manager/selector present"
	};

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		switch (obj->Integer.Value) {
		case 0 ... 4:
			fwts_passed(fw, "_SBS correctly returned value %d %s",
				(uint32_t)obj->Integer.Value,
				sbs_info[obj->Integer.Value]);
			break;
		default:	
			fwts_failed(fw, "_SBS returned %d, should be between 0 and 4.",
				(uint32_t)obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw, "Smart Battery _SBS is incorrectly informing the OS about the smart battery "
					"configuration. This is a bug and needs to be fixed.");
			break;
		}
	}
}

static int method_test_SBS(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_SBS", NULL, 0, method_test_SBS_return, NULL);
}


/* Section 10.2.2 Battery Control Methods */

static void method_test_BIF_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		if (obj->Package.Count != 13) {
			fwts_failed(fw, "_BIF package should return 13 elements, got %d instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}

		for (i=0;(i<9) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BIF package element %d is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}
		for (i=9;(i<13) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_STRING) {
				fwts_failed(fw, "_BIF package element %d is not of type STRING.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Power Unit */
		if (obj->Package.Elements[0].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "_BIF: Expected Power Unit (Element 0) to be 0 (mWh) or 1 (mAh), got 0x%8.8x.",
					(uint32_t)obj->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design Capacity */
		if (obj->Package.Elements[1].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIF: Design Capacity (Element 1) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[1].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Last Full Charge Capacity */
		if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIF: Last Full Charge Capacity (Element 2) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[2].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Battery Technology */
		if (obj->Package.Elements[3].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "_BIF: Expected Battery Technology Unit (Element 3) to be 0 (Primary) or 1 (Secondary), got 0x%8.8x.",
					(uint32_t)obj->Package.Elements[3].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design Voltage */
		if (obj->Package.Elements[4].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIF: Design Voltage (Element 4) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[4].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity warning */
		if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIF: Design Capacity Warning (Element 5) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[5].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity low */
		if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIF: Design Capacity Warning (Element 6) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[6].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		if (failed)
			fwts_advice(fw, "Battery _BIF package contains errors. It is worth running the "
					"firmware test suite interactive 'battery' test to see if this "
					"is problematic.  This is a bug an needs to be fixed.");
		else
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
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}

		for (i=0;(i<16) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BIX package element %d is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}
		for (i=16;(i<20) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_STRING) {
				fwts_failed(fw, "_BIX package element %d is not of type STRING.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Power Unit */
		if (obj->Package.Elements[1].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "_BIX: Expected Power Unit (Element 1) to be 0 (mWh) or 1 (mAh), got 0x%8.8x.",
					(uint32_t)obj->Package.Elements[1].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design Capacity */
		if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIX: Design Capacity (Element 2) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[2].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Last Full Charge Capacity */
		if (obj->Package.Elements[3].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIX: Last Full Charge Capacity (Element 3) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[3].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Battery Technology */
		if (obj->Package.Elements[4].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "_BIX: Expected Battery Technology Unit (Element 4) to be 0 (Primary) or 1 (Secondary), got 0x%8.8x.",
					(uint32_t)obj->Package.Elements[4].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design Voltage */
		if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIX: Design Voltage (Element 5) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[5].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity warning */
		if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIX: Design Capacity Warning (Element 6) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[6].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity low */
		if (obj->Package.Elements[7].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIX: Design Capacity Warning (Element 7) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[7].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Cycle Count */
		if (obj->Package.Elements[10].Integer.Value > 0x7fffffff) {	
			fwts_failed_low(fw, "_BIX: Cycle Count (Element 10) is unknown: 0x%8.8x.",
					(uint32_t)obj->Package.Elements[10].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		if (failed)
			fwts_advice(fw, "Battery _BIX package contains errors. It is worth running the "
					"firmware test suite interactive 'battery' test to see if this "
					"is problematic.  This is a bug an needs to be fixed.");
		else
			fwts_passed(fw, "Battery _BIX package looks sane.");
	}
}
static int method_test_BIX(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BIX", NULL, 0, method_test_BIX_return, NULL);
}

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

static void method_test_BST_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		if (obj->Package.Count != 4) {
			fwts_failed(fw, "_BST package should return 4 elements, got %d instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}

		for (i=0;(i<4) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BST package element %d is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Battery State */
		if (obj->Package.Elements[0].Integer.Value > 0x00000002) {	
			fwts_failed(fw, "_BST: Expected Battery State (Element 0) to be 0..2, got 0x%8.8x.",
					(uint32_t)obj->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Battery Present Rate - cannot check, pulled from EC */
		/* Battery Remaining Capacity - cannot check, pulled from EC */
		/* Battery Present Voltage - cannot check, pulled from EC */
		if (failed)
			fwts_advice(fw, "Battery _BST package contains errors. It is worth running the "
					"firmware test suite interactive 'battery' test to see if this "
					"is problematic.  This is a bug an needs to be fixed.");
		else
			fwts_passed(fw, "Battery _BST package looks sane.");
	}
}

static int method_test_BST(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BST", NULL, 0, method_test_BST_return, NULL);
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
		fwts_log_nl(fw);
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
		fwts_log_nl(fw);
	}
	return FWTS_OK;
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
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}

		for (i=0;(i<4) && (i<obj->Package.Count);i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, "_BMD package element %d is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
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
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}
	
/* Section 15, Waking and Sleeping */

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
			fwts_log_nl(fw);
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
					  method_test_NULL_return, NULL) == FWTS_NOT_EXIST) {
			fwts_advice(fw, "Could not find _PTS. This method provides a mechanism to "
					"do housekeeping functions, such as write sleep state to the "
					"embedded controller before entering a sleep state. If the "
					"machine cannot suspend (S3), hibernate (S4) or shutdown (S5) "
					"then it could be because _PTS is missing.");
			break;
		}
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static int method_test_GTS(fwts_framework *fw)
{
	int i;

	if (method_exits("_GTS")) {
		for (i=1; i<6; i++) {
			ACPI_OBJECT arg[1];
	
			arg[0].Type = ACPI_TYPE_INTEGER;
			arg[0].Integer.Value = i;

			fwts_log_info(fw, "Test _GTS(%d) (Going To Sleep, State S%d.", i, i);

			if (method_execute_method(fw, METHOD_OPTIONAL, "_GTS", arg, 1,
						  method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
				break;
			fwts_log_nl(fw);
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
			fwts_log_nl(fw);
		}
	}
	else {
		fwts_skipped(fw, "Optional control method _TTS does not exist.");
	}
	return FWTS_OK;
}

static void method_test_Sx_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	char *method = (char *)private;

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		method_dump_package(fw, obj);

		if (obj->Package.Count != 3) {
			fwts_failed(fw, "%s should return package of 3 integers, got %d elements instead.", method,
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
	}
}

#define method_test_Sx(name)				\
static int method_test ## name(fwts_framework *fw)	\
{							\
	return method_execute_method(fw, METHOD_OPTIONAL, # name, NULL, 0, method_test_Sx_return, # name); \
}			

method_test_Sx(_S0)
method_test_Sx(_S1)
method_test_Sx(_S2)
method_test_Sx(_S3)
method_test_Sx(_S4)
method_test_Sx(_S5)

static void method_test_WAK_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	uint32_t Sstate = *(uint32_t*)private;
	int failed = 0;

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		method_dump_package(fw, obj);

		if (obj->Package.Count != 2) {
			fwts_failed(fw, "_WAK should return package of 2 integers, got %d elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER))  {
				fwts_failed(fw, "_WAK should return package of 2 integers, got %d instead.",
					obj->Package.Count);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
			else {
				if (obj->Package.Elements[0].Integer.Value > 0x00000002) {	
					fwts_failed(fw, "_WAK: expecting condition bit-field (element 0) of packages to be in range, got 0x%8.8x.",
						(uint32_t)obj->Package.Elements[0].Integer.Value);
					fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
					failed++;
				}
				if (!(
				    ((obj->Package.Elements[1].Integer.Value == Sstate) && (obj->Package.Elements[0].Integer.Value == 0)) ||
                                    ((obj->Package.Elements[1].Integer.Value == 0) && (obj->Package.Elements[0].Integer.Value != 0)) )) {
					fwts_failed(fw, "_WAK: expecting power supply S-state (element 1) of packages to be 0x%8.8x, got 0x%8.8x.",
						Sstate, (uint32_t)obj->Package.Elements[0].Integer.Value);
					fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
					fwts_advice(fw, "_WAK should return 0 if the wake failed and was unsuccessful (i.e. element[0] "
							"is non-zero) OR should return the S-state. "
							"This can confuse the operating system as this _WAK return indicates that the "
							"S-state was not entered because of too much current being drawn from the "
							"power supply, however, the BIOS may have actually entered this state and the "
							"_WAK method is misinforming the operating system. Currently Linux does not "
							"check for the return type from _WAK, so it should theoretically not affect the "
							"operation of suspend/resume.");
				
					failed++;
				}
			}
		}
		if (!failed)
			fwts_passed(fw, "_WAK correctly returned sane looking package.");
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
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

/* Section 10.3 AC Adapters and Power Sources Objects */

static void method_test_PSR_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if (obj->Integer.Value > 2) {
			fwts_failed(fw, "_PSR returned 0x%8.8x\n, expected 0 (offline) or 1 (online)", (uint32_t)obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else
			fwts_passed(fw, "_PSR correctly returned sane looking value 0x%8.8x", (uint32_t)obj->Integer.Value);
	}
}

static int method_test_PSR(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_MANDITORY, "_PSR", NULL, 0, method_test_PSR_return, NULL);
}

static void method_test_PIF_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		method_dump_package(fw, obj);
	
		if (obj->Package.Count != 6) {
			fwts_failed(fw, "_PIF should return package of 6 elements, got %d elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_BUFFER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[3].Type != ACPI_TYPE_STRING) ||
			    (obj->Package.Elements[4].Type != ACPI_TYPE_STRING) ||
			    (obj->Package.Elements[5].Type != ACPI_TYPE_STRING)) {
				fwts_failed(fw, "_PIF should return package of 1 buffer, 2 integers and 3 strings.");
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			} else {
				fwts_passed(fw, "_PIF correctly returned sane looking package.");
			}
		}
	}
}

static int method_test_PIF(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_PIF", NULL, 0, method_test_PIF_return, NULL);
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

/* Section 11.3 Fan Devices */

static void method_test_FIF_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		method_dump_package(fw, obj);

		if (obj->Package.Count != 4) {
			fwts_failed(fw, "_FIF should return package of 4 elements, got %d elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[3].Type != ACPI_TYPE_INTEGER)) {
				fwts_failed(fw, "_FIF should return package of 4 integers.");
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				fwts_advice(fw, "_FIF is not returning the correct fan information. "
						"It may be worth running the firmware test suite interactive 'fan' test "
						"to see if this affects the control and operation of the fan.");
			} else {
				fwts_passed(fw, "_FIF correctly returned sane looking package.");
			}
		}
	}
}

static int method_test_FIF(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_FIF", NULL, 0, method_test_FIF_return, NULL);
}

static int method_test_FSL(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 50;

	return method_execute_method(fw, METHOD_OPTIONAL, "_FSL", NULL, 0, method_test_NULL_return, NULL);
}

static void method_test_FST_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		method_dump_package(fw, obj);

		if (obj->Package.Count != 3) {
			fwts_failed(fw, "_FST should return package of 3 elements, got %d elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER)) {
				fwts_failed(fw, "_FST should return package of 3 integers.");
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				fwts_advice(fw, "_FST is not returning the correct fan status information. "
						"It may be worth running the firmware test suite interactive 'fan' test "
						"to see if this affects the control and operation of the fan.");
			} else {
				fwts_passed(fw, "_FST correctly returned sane looking package.");
			}
		}
	}
}

static int method_test_FST(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_FST", NULL, 0, method_test_FST_return, NULL);
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
			fwts_failed(fw, "%s returned a debious value below 0 degrees C: 0x%8.8x (%5.1f degrees K)",
				method,
				(uint32_t)obj->Integer.Value,
				(float)((uint32_t)obj->Integer.Value) / 10.0);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw, "An incorrect value could be because the method requires interaction with "
					"I/O ports or the embedded controller and this test frame work is spoofing "
					"these operations. However, it is worth sanity checking these values in "
					"/proc/acpi/thermal_zone.");
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
method_test_THERM(_TST, METHOD_OPTIONAL)

static void method_test_TCx_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		char *method = (char *)private;
		fwts_passed(fw, "%s correctly returned sane looking value 0x%8.8x", method, (uint32_t)obj->Integer.Value);
	}
}

static int method_test_TC1(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_TC1", NULL, 0, method_test_TCx_return, "_TC1");
}

static int method_test_TC2(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_TC2", NULL, 0, method_test_TCx_return, "_TC1");
}

static int method_test_ACx(fwts_framework *fw)
{
	int i;

	for (i=0;i<10;i++) {
		char buffer[5];
		snprintf(buffer, sizeof(buffer), "AC%d", i);
		method_execute_method(fw, METHOD_OPTIONAL, buffer, NULL, 0, method_test_THERM_return, buffer);
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static int method_test_DTI(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 2732 + 800; /* 80 degrees C */
	return method_execute_method(fw, METHOD_OPTIONAL, "_DTI", arg, 1, method_test_NULL_return, NULL);
}

static int method_test_SCP(fwts_framework *fw)
{
	int i;

	for (i=0;i<2;i++) {
		ACPI_OBJECT arg[3];

		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;		/* Mode */
		arg[1].Type = ACPI_TYPE_INTEGER;
		arg[1].Integer.Value = 5;		/* Acoustic limit */
		arg[2].Type = ACPI_TYPE_INTEGER;
		arg[2].Integer.Value = 5;		/* Power limit */
		if (method_execute_method(fw, METHOD_OPTIONAL, "_DTI", arg, 1, method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
		fwts_log_nl(fw);

		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;		/* Mode */
		arg[1].Type = ACPI_TYPE_INTEGER;
		arg[1].Integer.Value = 1;		/* Acoustic limit */
		arg[2].Type = ACPI_TYPE_INTEGER;
		arg[2].Integer.Value = 1;		/* Power limit */
		if (method_execute_method(fw, METHOD_OPTIONAL, "_DTI", arg, 1, method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}


static void method_test_RTV_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw, "_RTV correctly returned sane looking value 0x%8.8x", (uint32_t)obj->Integer.Value);
}

static int method_test_RTV(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_RTV", NULL, 0, method_test_RTV_return, "_RTV");
}

static int method_test_TPT(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 2732 + 900; /* 90 degrees C */
	return method_execute_method(fw, METHOD_OPTIONAL, "_TPT", arg, 1, method_test_NULL_return, NULL);
}

static void method_test_polling_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		char *method = (char *)private;
		if (obj->Integer.Value < 36000) {
			fwts_passed(fw, "%s correctly returned sane looking value %f seconds",
				method, (float)obj->Integer.Value / 10.0);
		} else {
			fwts_failed(fw, "%s returned a value %f seconds > (1 hour) which is probably incorrect.",
				method, (float)obj->Integer.Value / 10.0);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw, "The method is returning a polling interval which is very long and hence "
					"most probably incorrect.");
		}
	}
}

static int method_test_TSP(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_TSP", NULL, 0, method_test_polling_return, "_TSP");
}

static int method_test_TZP(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_TZP", NULL, 0, method_test_polling_return, "_TZP");
}

/* Section 9.2, Ambient Light Sensor Device */

method_test_integer(_ALC, METHOD_OPTIONAL)
method_test_integer(_ALI, METHOD_OPTIONAL)
method_test_integer(_ALT, METHOD_OPTIONAL)

/* TODO _ALR */

static int method_test_ALP(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_ALP", NULL, 0, method_test_polling_return, "_ALP");
}


/* Section 6.5 Other Objects and Control Methods */

method_test_integer(_BBN, METHOD_OPTIONAL)
method_test_integer(_BDN, METHOD_OPTIONAL)

static int method_test_DCK(fwts_framework *fw)
{
	int i;

	for (i=1;i>0;i--) {	/* Dock, Undock */
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;
		if (method_execute_method(fw, METHOD_OPTIONAL, "_DCK", NULL, 0, method_test_passed_failed_return, "_DCK") != FWTS_OK)
			break;
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

/* Section 9.18 Wake Alarm Device */


static int method_test_STP(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 0;	/* wake up instantly */
	
	return method_execute_method(fw, METHOD_OPTIONAL, "_STP", NULL, 0, method_test_passed_failed_return, "_STP");
}

static int method_test_STV(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 100;	/* timer value */
	
	return method_execute_method(fw, METHOD_OPTIONAL, "_STV", NULL, 0, method_test_passed_failed_return, "_STV");
}

static int method_test_TIP(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */
	
	return method_execute_method(fw, METHOD_OPTIONAL, "_TIP", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_TIV(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */
	
	return method_execute_method(fw, METHOD_OPTIONAL, "_TIV", NULL, 0, method_test_integer_return, NULL);
}

/* Appendix B, ACPI Extensions for Display Adapters */

static int method_test_DOS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0 << 2 | 1;		
		/* BIOS should toggle active display, BIOS controls brightness of LCD on AC/DC power changes */
	
	return method_execute_method(fw, METHOD_OPTIONAL, "_DOS", arg, 1, method_test_NULL_return, NULL);
}

static void method_test_DOD_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	int failed = 0;
	static char *dod_type[] = {
		"Other",
		"VGA, CRT or VESA Compatible Analog Monitor",
		"TV/HDTV or other Analog-Video Monitor",
		"External Digital Monitor",

		"Internal/Integrated Digital Flat Panel",
		"Reserved",
		"Reserved",
		"Reserved",

		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",

		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved"
	};

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;

		method_dump_package(fw, obj);

		for (i=0;i<obj->Package.Count;i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) 	
				failed++;
			else {
				uint32_t val = obj->Package.Elements[i].Integer.Value;
				fwts_log_info_verbatum(fw, "Device %d:", i);
				if ((val & 0x80000000)) {
					fwts_log_info_verbatum(fw, "  Video Chip Vendor Scheme %x", val);
				} else {
					fwts_log_info_verbatum(fw, "  Instance:                %d", val & 0xf);
					fwts_log_info_verbatum(fw, "  Display port attachment: %d", (val >> 4) & 0xf);
					fwts_log_info_verbatum(fw, "  Type of display:         %d (%s)",
						(val >> 8) & 0xf, dod_type[(val >> 8) & 0xf]);
					fwts_log_info_verbatum(fw, "  BIOS can detect device:  %d", (val >> 16) & 1);
					fwts_log_info_verbatum(fw, "  Non-VGA device:          %d", (val >> 17) & 1);
					fwts_log_info_verbatum(fw, "  Head or pipe ID:         %d", (val >> 18) & 0x7);
				}
			}
		}
	}
	if (failed) {
		fwts_failed(fw, "Method _DOD did not return a package of %d integers.", obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
	} else
		fwts_passed(fw, "Method _DOD returned a sane package of %d integers.", obj->Package.Count);
}

static int method_test_DOD(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_DOD", NULL, 0, method_test_DOD_return, NULL);
}
	
static void method_test_ROM_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	method_check_type(fw, name, buf, ACPI_TYPE_BUFFER);
}

static int method_test_ROM(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0;
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 4096;

	return method_execute_method(fw, METHOD_OPTIONAL, "_ROM", arg, 2, method_test_ROM_return, NULL);
}

static int method_test_GPD(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_GPD", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_SPD(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];
	int i;

	for (i=0;i<4;i++) {
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;	/* bits 00..11, post device */

		if (method_execute_method(fw, METHOD_OPTIONAL, "_SPD", arg, 1, method_test_passed_failed_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_VPO(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_VPO", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_ADR(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_ADR", NULL, 0, method_test_integer_return, NULL);
}

static void method_test_BCL_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		int i;
		int failed = 0;

		method_dump_package(fw, obj);

		for (i=0;i<obj->Package.Count;i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) 	
				failed++;
		}
		if (failed) {
			fwts_failed(fw, "Method _BCL did not return a package of %d integers.", obj->Package.Count);
		} else {
			if (obj->Package.Count < 3) {
				fwts_failed(fw, "Method _BCL should return a package of more than 2 integers, got just %d.", obj->Package.Count);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			} else {
				if (obj->Package.Elements[0].Integer.Value <
				    obj->Package.Elements[1].Integer.Value) {
					fwts_failed(fw, "Brightness level when on full power (%d) is less than "
						 	"brightness level when on battery power (%d).",
						(uint32_t)obj->Package.Elements[0].Integer.Value,
						(uint32_t)obj->Package.Elements[1].Integer.Value);
					fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
					failed++;
				}

				for (i=2;i<obj->Package.Count-1;i++) {
					if (obj->Package.Elements[i].Integer.Value >
					    obj->Package.Elements[i+1].Integer.Value) {
						fwts_failed(fw, "Brightness level %d (index %d) is greater than "
						 		"brightness level %d (index %d), should be in ascending order.",
						(uint32_t)obj->Package.Elements[i].Integer.Value, i,
						(uint32_t)obj->Package.Elements[i+1].Integer.Value, i+1);
						fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
						failed++;
					}
				}
				if (failed)
					fwts_advice(fw, "Method _BCL seems to be misconfigured and is returning incorrect brightness levels."
							"It is worth sanity checking this with the firmware test suite interactive test "
							"'brightness' to see how broken this is. As it is, _BCL is broken and needs to be "
							"fixed.");
				else
					fwts_passed(fw, "Method _BCL returned a sane package of %d integers.", obj->Package.Count);
			}
		}
	}
}

static int method_test_BCL(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BCL", NULL, 0, method_test_BCL_return, NULL);
}

static int method_test_BCM(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0;

	return method_execute_method(fw, METHOD_OPTIONAL, "_BCM", arg, 1, method_test_NULL_return, NULL);
}

static int method_test_BQC(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_BQC", NULL, 0, method_test_integer_return, NULL);
}

static void method_test_DDC_return(fwts_framework *fw, char *name, ACPI_BUFFER *buf, ACPI_OBJECT *obj, void *private)
{
	uint32_t requested = *(uint32_t*)private;
	switch (obj->Type) {
	case ACPI_TYPE_BUFFER:
		if (requested != obj->Buffer.Length) {
			fwts_failed(fw, "Method _DDC returned a buffer of %d items, expected %d.",
				obj->Buffer.Length, requested);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else	
			fwts_passed(fw, "Method _DDC returned a buffer of %d items as expected.",
				obj->Buffer.Length);
		break;
	case ACPI_TYPE_INTEGER:
			fwts_passed(fw, "Method _DDC could not return a buffer of %d items"
					"and instead returned an error status.",
				obj->Buffer.Length);
		break;
	default:
		fwts_failed(fw, "Method _DDC did not return a buffer or an integer.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		break;
	}
}

static int method_test_DDC(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	uint32_t i;

	for (i=128; i<=256; i <<= 1) {
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = 128;

		if (method_execute_method(fw, METHOD_OPTIONAL, "_DDC", arg, 1, method_test_DDC_return, &i) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_DCS(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_DCS", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_DGS(fwts_framework *fw)
{
	return method_execute_method(fw, METHOD_OPTIONAL, "_DGS", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_DSS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0;

	return method_execute_method(fw, METHOD_OPTIONAL, "_DSS", arg, 1, method_test_NULL_return, NULL);
}


/* Tests */

static fwts_framework_minor_test method_tests[] = {
	{ method_name_check, "Check Method Names." },

	/* Section 9.2 Ambient Light Sensor Device */

	{ method_test_ALC, "Check _ALC (Ambient Light Colour Chromaticity)." },
	{ method_test_ALI, "Check _ALI (Ambient Light Illuminance)." },
	{ method_test_ALT, "Check _ALT (Ambient Light Temperature)." },
	{ method_test_ALP, "Check _ALP (Ambient Light Polling). "},

	/* Section 9.4 Lid Device */

	{ method_test_LID, "Check _LID (Lid Status)." },

	/* Section 9.8 ATA Controllers */

	/* Section 9.9 Floppy Controllers */

	/* Section 9.18 Wake Alarm Device */

	{ method_test_STP, "Check _STP (Set Expired Timer Wake Policy)." },
	{ method_test_STV, "Check _STV (Set Timer Value)." },
	{ method_test_TIP, "Check _TIP (Expired Timer Wake Policy)." },
	{ method_test_TIV, "Check _TIV (Timer Values)." },

	/* Section 10.1.1.1 Smart Battery */

	{ method_test_SBS, "Check _SBS (Smart Battery Subsystem)." },

	/* Section 10.2.2 Battery Controls */

	{ method_test_BIF, "Check _BIF (Battery Information)." },
	{ method_test_BIX, "Check _BIX (Battery Information Extended)." },
	{ method_test_BMA, "Check _BMA (Battery Measurement Averaging)." },
	{ method_test_BMS, "Check _BMS (Battery Measurement Sampling Time)." },
	{ method_test_BST, "Check _BST (Battery Status)." },
	{ method_test_BTP, "Check _BTP (Battery Trip Point)." },
	{ method_test_PCL, "Check _PCL (Power Consumer List)." },
	{ method_test_BTM, "Check _BTM (Battery Time)." },
	{ method_test_BMD, "Check _BMD (Battery Maintenance Data)." },
	{ method_test_BMC, "Check _BMC (Battery Maintenance Control)." },

	/* Section 10.3, AC Adapters and Power Source Objects */

	{ method_test_PSR, "Check _PSR (Power Source)." },
	{ method_test_PIF, "Check _PIF (Power Source Information) Object." },

	/* Section 11.3 Fan Devices */
	
	{ method_test_FIF, "Check _FIF (Fan Information)." },
	/* { method_test_FPS, "Check _FPS (Fan Performance States)." }, */
	{ method_test_FSL, "Check _FSL (Fan Set Level)." },
	{ method_test_FST, "Check _FST (Fan Status)." },

	/* Section 11.4 Thermal */
	{ method_test_ACx, "Check _ACx Active Cooling." },
	{ method_test_CRT, "Check _CRT (Critical Trip Point)." },
	{ method_test_DTI, "Check _DTI Device Temperature Indication." },
	{ method_test_HOT, "Check _HOT (Hot Temperature)." },
	{ method_test_NTT, "Check _NTT (Notification Temp Threshold)." },
	{ method_test_PSV, "Check _PSV (Passive Temp)." },
	{ method_test_RTV, "Check _RTV (Relative Temp Values)." },
	{ method_test_SCP, "Check _SCP (Set Cooling Policy)." },
	{ method_test_TMP, "Check _TMP (Thermal Zone Current Temp)." },
	{ method_test_TC1, "Check _TC1 (Thermal Constant 1) Object." },
	{ method_test_TC2, "Check _TC2 (Thermal Constant 2) Object." },
	{ method_test_TPT, "Check _TPT (Trip Point Temperature)." },
	{ method_test_TSP, "Check _TSP (Thermal Sampling Period) Object." },
	{ method_test_TST, "Check _TST (Temperature Sensor Threshold) Object." },
	{ method_test_TZP, "Check _TZP (Thermal Zone Polling) Object." },

	/* Section 6.5 Other Objects and Controls */
	
	{ method_test_DCK, "Check _DCK (Dock)." },
	{ method_test_BDN, "Check _BDN (BIOS Dock Name)." },
	{ method_test_BBN, "Check _BBN (Base Bus Number)." },

	/* Section 15, Waking and Sleeping */
	{ method_test_BFS, "Check _BFS (Back from Sleep)." },
	{ method_test_PTS, "Check _PTS (Prepare to Sleep)." },
	{ method_test_GTS, "Check _GTS (Going to Sleep)." },
	{ method_test_TTS, "Check _TTS (Transition to State)." },
	{ method_test_S0,  "Check _S0  (System S0 State) Object." },
	{ method_test_S1,  "Check _S1  (System S1 State) Object." },
	{ method_test_S2,  "Check _S2  (System S2 State) Object." },
	{ method_test_S3,  "Check _S3  (System S3 State) Object." },
	{ method_test_S4,  "Check _S4  (System S4 State) Object." },
	{ method_test_S5,  "Check _S5  (System S5 State) Object." },
	{ method_test_WAK, "Check _WAK (System Wake)." },

	/* Appendix B, ACPI Extensions for Display Adapters */
	
	{ method_test_DOS, "Check _DOS (Enable/Disable Output Switching)." },
	{ method_test_DOD, "Check _DOD (Enumerate All Devices Attached to Display Adapter)." },
	{ method_test_ROM, "Check _ROM (Get ROM Data) Object." },
	{ method_test_GPD, "Check _GPD (Get POST Device)." },
	{ method_test_SPD, "Check _SPD (Set POST Device)." },
	{ method_test_VPO, "Check _VPO (Video POST Options)." },
	{ method_test_ADR, "Check _ADR (Return Unique ID for Device)." },
	{ method_test_BCL, "Check _BCL (Query List of Brightness Control Levels Supported)." },
	{ method_test_BCM, "Check _BCM (Set Brightness Level)." },
	{ method_test_BQC, "Check _BQC (Brightness Query Current Level)." },
	{ method_test_DDC, "Check _DDC (Return the EDID for this Device)." },
	{ method_test_DCS, "Check _DCS (Return the Status of Output Device)." },
	{ method_test_DGS, "Check _DGS (Query Graphics State)." },
	{ method_test_DSS, "Check _DSS (Device Set State)." },


	{ NULL, NULL }
};

static fwts_framework_ops method_ops = {
	.headline    = method_headline,
	.init        = method_init,
	.deinit      = method_deinit,
	.minor_tests = method_tests
};

FWTS_REGISTER(method, &method_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
