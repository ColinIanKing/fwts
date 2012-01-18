/*
 * Copyright (C) 2010-2012 Canonical
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
#include <unistd.h>

/* acpica headers */
#include "acpi.h"
#include "fwts_acpi_method.h"

typedef struct {
	ACPI_STATUS status;
	const fwts_log_level level;
	const char *error_type;
	const char *error_text;
} acpi_eval_error;

static acpi_eval_error  errors[] = {
	/* ACPI_STATUS			fwts_log_level		error_type		error_text */
	{ AE_ERROR,			LOG_LEVEL_HIGH,		"AEError",		"Environment error" },
	{ AE_NO_ACPI_TABLES,		LOG_LEVEL_HIGH,		"AENoACPITables",	"NO ACPI tables" },
	{ AE_NO_NAMESPACE,		LOG_LEVEL_HIGH,		"AENoNamespace",	"No namespace" },
	{ AE_NO_MEMORY,			LOG_LEVEL_CRITICAL,	"AENoMemory",		"Out of memory" },
	{ AE_NOT_FOUND,			LOG_LEVEL_CRITICAL,	"AENotFound",		"Not found" },
	{ AE_NOT_EXIST,			LOG_LEVEL_CRITICAL,	"AENotExist",		"Not exist" },
	{ AE_ALREADY_EXISTS,		LOG_LEVEL_HIGH,		"AEAlreadyExists",	"Already exists" },
	{ AE_TYPE,			LOG_LEVEL_CRITICAL,	"AETtype",		"Type" },
	{ AE_NULL_OBJECT,		LOG_LEVEL_CRITICAL,	"AENullObject",		"Null object" },
	{ AE_NULL_ENTRY,		LOG_LEVEL_CRITICAL,	"AENullEntry",		"Null entry" },
	{ AE_BUFFER_OVERFLOW,		LOG_LEVEL_CRITICAL,	"AEBufferOverflow",	"Buffer overflow" },
	{ AE_STACK_OVERFLOW,		LOG_LEVEL_CRITICAL,	"AEStackOverflow",	"Stack overflow" },
	{ AE_STACK_UNDERFLOW,		LOG_LEVEL_CRITICAL,	"AEStackUnderflow",	"Stack underflow" },
	{ AE_NOT_IMPLEMENTED,		LOG_LEVEL_HIGH,		"AENotImplemented",	"Not implemented" },
	{ AE_SUPPORT,			LOG_LEVEL_HIGH,		"AESupport",		"Support" },
	{ AE_LIMIT,			LOG_LEVEL_CRITICAL,	"AELimit",		"Limit" },
	{ AE_TIME,			LOG_LEVEL_HIGH,		"AETime",		"Timeout" },
	{ AE_ACQUIRE_DEADLOCK,		LOG_LEVEL_CRITICAL,	"AEAcqDeadlock",	"Acquire deadlock" },
	{ AE_RELEASE_DEADLOCK,		LOG_LEVEL_CRITICAL,	"AERelDeadlock",	"Release deadlock" },
	{ AE_NOT_ACQUIRED,		LOG_LEVEL_CRITICAL,	"AENotAcq",		"Not acquired" },
	{ AE_ALREADY_ACQUIRED,		LOG_LEVEL_CRITICAL,	"AEAlreadyAcq",		"Already acquired" },
	{ AE_NO_HARDWARE_RESPONSE,	LOG_LEVEL_HIGH,		"AENoHWResponse",	"No hardware response" },
	{ AE_NO_GLOBAL_LOCK,		LOG_LEVEL_HIGH,		"AENoGlobalLock",	"No global lock" },
	{ AE_ABORT_METHOD,		LOG_LEVEL_CRITICAL,	"AEAbortMethod",	"Abort method" },
	{ AE_SAME_HANDLER,		LOG_LEVEL_HIGH,		"AESameHandler",	"Same handler" },
	{ AE_NO_HANDLER,		LOG_LEVEL_CRITICAL,	"AENoHandler",		"No handler" },
	{ AE_OWNER_ID_LIMIT,		LOG_LEVEL_HIGH,		"AEOwnerIDLimit",	"Owner ID limit" },

	{ AE_BAD_PARAMETER,		LOG_LEVEL_HIGH,		"AEBadParam",		"Args: Bad paramater" },
	{ AE_BAD_CHARACTER,		LOG_LEVEL_HIGH,		"AEBadChar",		"Args: Bad character" },
	{ AE_BAD_PATHNAME,		LOG_LEVEL_HIGH,		"AEBadPathname",	"Args: Bad pathname" },
	{ AE_BAD_DATA,			LOG_LEVEL_HIGH,		"AEBadData",		"Args: Bad data" },
	{ AE_BAD_HEX_CONSTANT,		LOG_LEVEL_HIGH,		"AEBadHexConst",	"Args: Bad hex constant" },
	{ AE_BAD_OCTAL_CONSTANT,	LOG_LEVEL_HIGH,		"AEBadOctConst",	"Args: Bad octal constant" },
	{ AE_BAD_DECIMAL_CONSTANT,	LOG_LEVEL_HIGH,		"AEBadDecConst",	"Args: Bad decimal constant" },
	{ AE_MISSING_ARGUMENTS,		LOG_LEVEL_CRITICAL,	"AEMissingArgs",	"Args: Missing arguments" },
	{ AE_BAD_ADDRESS,		LOG_LEVEL_CRITICAL,	"AEBadAddr",		"Args: Bad address" },

	{ AE_AML_BAD_OPCODE, 		LOG_LEVEL_CRITICAL,	"AEAMLBadOpCode",	"Bad AML opcode" },
	{ AE_AML_NO_OPERAND, 		LOG_LEVEL_HIGH,		"AEAMLNoOperand",	"Missing operand" },
	{ AE_AML_OPERAND_TYPE, 		LOG_LEVEL_HIGH,		"AEAMLOperandType",	"Incorrect operand type" },
	{ AE_AML_OPERAND_VALUE, 	LOG_LEVEL_HIGH,		"AEAMLOperandValue",	"Incorrect operand value" },
	{ AE_AML_UNINITIALIZED_LOCAL, 	LOG_LEVEL_HIGH,		"AEAMLUninitLocal",	"Uninitialized local variable" },
	{ AE_AML_UNINITIALIZED_ARG, 	LOG_LEVEL_HIGH,		"AEAMLUninitArg",	"Uninitialized argument" },
	{ AE_AML_UNINITIALIZED_ELEMENT, LOG_LEVEL_HIGH,		"AEAMLUninitElement",	"Uninitialized element" },
	{ AE_AML_NUMERIC_OVERFLOW, 	LOG_LEVEL_HIGH,		"AEAMLNumericOverflow",	"Numeric overflow" },
	{ AE_AML_REGION_LIMIT, 		LOG_LEVEL_CRITICAL,	"AEAMLRegionLimit",	"Region limit" },
	{ AE_AML_BUFFER_LIMIT, 		LOG_LEVEL_CRITICAL,	"AEAMLBufferLimit",	"Buffer limit" },
	{ AE_AML_PACKAGE_LIMIT, 	LOG_LEVEL_CRITICAL,	"AEAMLPackgeLimit",	"Package limit" },
	{ AE_AML_DIVIDE_BY_ZERO, 	LOG_LEVEL_CRITICAL,	"AEAMLDivideByZero",	"Division by zero" },
	{ AE_AML_NAME_NOT_FOUND, 	LOG_LEVEL_HIGH,		"AEAMLNameNotFound",	"Name not found" },
	{ AE_AML_INTERNAL, 		LOG_LEVEL_HIGH,		"AEAMLInternal",	"Internal ACPICA execution engine error" },
	{ AE_AML_INVALID_SPACE_ID, 	LOG_LEVEL_HIGH,		"AEAMLInvalidSpaceID",	"Invalid space ID error" },
	{ AE_AML_STRING_LIMIT, 		LOG_LEVEL_HIGH,		"AEAMLStringLimit",	"String limit" },
	{ AE_AML_NO_RETURN_VALUE, 	LOG_LEVEL_HIGH,		"AEAMLNoReturnValue",	"No return value" },
	{ AE_AML_METHOD_LIMIT, 		LOG_LEVEL_HIGH,		"AEAMLMethodLimit",	"Method limit" },
	{ AE_AML_NOT_OWNER, 		LOG_LEVEL_HIGH,		"AEAMLNotOwner",	"Not owner" },
	{ AE_AML_MUTEX_ORDER, 		LOG_LEVEL_HIGH,		"AEAMLMutexOrder",	"Mutex order" },
	{ AE_AML_MUTEX_NOT_ACQUIRED, 	LOG_LEVEL_HIGH,		"AEAMLMutexNotAcq",	"Mutux not acquired" },
	{ AE_AML_INVALID_RESOURCE_TYPE, LOG_LEVEL_HIGH,		"AEAMLInvResourceType", "Invalid resource type" },
	{ AE_AML_INVALID_INDEX, 	LOG_LEVEL_HIGH,		"AEAMLInvIndex",	"Invalid index" },
	{ AE_AML_REGISTER_LIMIT, 	LOG_LEVEL_HIGH,		"AEAMLRegisterLimit",	"Register limit" },
	{ AE_AML_NO_WHILE, 		LOG_LEVEL_CRITICAL,	"AEAMLNoWhile",		"No while" },
	{ AE_AML_ALIGNMENT, 		LOG_LEVEL_CRITICAL,	"AEAMLAlignment",	"Misaligmnent" },
	{ AE_AML_NO_RESOURCE_END_TAG, 	LOG_LEVEL_HIGH,		"AEAMLNoResEndTag",	"No resource end tag"},
	{ AE_AML_BAD_RESOURCE_VALUE, 	LOG_LEVEL_HIGH,		"AEAMLBadResValue",	"Bad resource value" },
	{ AE_AML_CIRCULAR_REFERENCE,	LOG_LEVEL_CRITICAL,	"AEAMLCircularRef",	"Circular reference" },
	{ AE_AML_BAD_RESOURCE_LENGTH,	LOG_LEVEL_HIGH,		"AEAMLBadResLength",	"Bad resource length" },
	{ AE_AML_ILLEGAL_ADDRESS, 	LOG_LEVEL_CRITICAL,	"AEAMLIllegalAddr",	"Illegal address" },
	/* { AE_AML_INFINITE_LOOP, 	LOG_LEVEL_HIGH,		"AEAMLInfiniteLoop",	"Infinite loop" }, */
	{ 0,				0,			NULL,			NULL }
};


static fwts_list *fwts_method_names;
static bool fwts_method_initialized = false;


/*
 *  fwts_method_init()
 *	Initialise ACPIA engine and collect method namespace
 */
int fwts_method_init(fwts_framework *fw)
{
	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	fwts_method_names = fwts_acpica_get_object_names(8);
	fwts_method_initialized = true;

	return FWTS_OK;
}

/*
 *  fwts_method_deinit()
 *	Close ACPIA engine and free method namespace
 */
int fwts_method_deinit(fwts_framework *fw)
{
	int ret = FWTS_ERROR;

	if (fwts_method_initialized) {
		fwts_list_free(fwts_method_names, free);
		fwts_method_names = NULL;
		ret = fwts_acpica_deinit();

		fwts_method_initialized = false;
	}

	return ret;
}


/*
 *  fwts_method_get_names()
 *	return list of method names
 */
fwts_list *fwts_method_get_names(void)
{
	return fwts_method_names;
}

/*  
 *  fwts_method_exists() 
 *	return first matching name
 */
char *fwts_method_exists(char *name)
{
	int name_len = strlen(name);
	fwts_list_link	*item;

	fwts_list_foreach(item, fwts_method_names) {
		char *method_name = fwts_list_data(char*, item);
		int len = strlen(method_name);

		if (strncmp(name, method_name + len - name_len, name_len) == 0)
			return method_name;
	}
	return NULL;
}


/*
 *   fwts_method_dump_object()
 *	dump out an object, minimal form
 */
static void fwts_method_dump_object_recursive(fwts_framework *fw, ACPI_OBJECT *obj, int depth, int index)
{
	int i;
	char index_buf[5];

	if (index > -1)
		snprintf(index_buf, sizeof(index_buf), "%2.2d: ", index);
	else
		index_buf[0] = '\0';

	switch (obj->Type) {
	case ACPI_TYPE_INTEGER:
		fwts_log_info_verbatum(fw, "%*s%sINTEGER: 0x%8.8llx", depth * 2, "",
			index_buf, (unsigned long long)obj->Integer.Value);
		break;
	case ACPI_TYPE_STRING:
		fwts_log_info_verbatum(fw, "%*s%sSTRING:  0x%s", depth * 2, "",
			index_buf, obj->String.Pointer);
		break;
	case ACPI_TYPE_BUFFER:
		fwts_log_info_verbatum(fw, "%*s%sBUFFER:  (%d bytes)", depth * 2, "",
			index_buf, obj->Buffer.Length);
		break;
	case ACPI_TYPE_PACKAGE:
		fwts_log_info_verbatum(fw, "%*s%sPackage has %d elements:",depth * 2, "",
			index_buf, obj->Package.Count);
		for (i=0; i < obj->Package.Count; i++) {
			ACPI_OBJECT *element = &obj->Package.Elements[i];
			fwts_method_dump_object_recursive(fw, element, depth + 1, i);
		}
		break;
	default:
		fwts_log_info_verbatum(fw, "%*s%sUnknown type %d\n", depth * 2, "",
			index_buf, obj->Type);
		break;
	}
}

/*
 *   fwts_method_dump_object()
 *	dump out an object, minimal form
 */
void fwts_method_dump_object(fwts_framework *fw, ACPI_OBJECT *obj)
{
	fwts_method_dump_object_recursive(fw, obj, 1, -1);
}

/*
 *  fwts_method_evaluate_report_error()
 *	report any errors found during object evaluation
 */
void fwts_method_evaluate_report_error(fwts_framework *fw, char *name, ACPI_STATUS status)
{
	int i;

	/* Generic cases */
	for (i=0; errors[i].error_type; i++) {
		if (status == errors[i].status) {
			fwts_failed(fw, errors[i].level, errors[i].error_type,
				"Detected error '%s' when evaluating '%s'.",
				errors[i].error_text, name);
			return;
		}
	}

	/* Special cases */
	switch (status) {
	case AE_OK:
		break;
	case AE_AML_INFINITE_LOOP:
		fwts_warning(fw, "Detected an infinite loop when evaluating method '%s'. ", name);
		fwts_advice(fw, "This may occur because we are emulating the execution "
				"in this test environment and cannot handshake with "
				"the embedded controller or jump to the BIOS via SMIs. "
				"However, the fact that AML code spins forever means that "
				"lockup conditions are not being checked for in the AML bytecode.");
		break;
	/* Unknown?! */
	default:
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "AMLFailedToEvaluate", "Failed to evaluate '%s', got error code %d.", name, status);
		break;
	}
}

/*
 *  fwts_method_evaluate()
 *	evaluate object, return error status (handle this with
 *	fwts_method_evaluate_report_error()).
 * 	This returns buf which auto allocates any return values
 *	which need to be freed post-evalutation using free().
 */
ACPI_STATUS fwts_method_evaluate(fwts_framework *fw,
	char *name,
        ACPI_OBJECT_LIST *arg_list,
	ACPI_BUFFER	 *buf)
{
        buf->Length  = ACPI_ALLOCATE_BUFFER;
        buf->Pointer = NULL;

        return AcpiEvaluateObject(NULL, name, arg_list, buf);
}

