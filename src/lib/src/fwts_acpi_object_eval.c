/*
 * Copyright (C) 2010-2014 Canonical
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
#include "fwts_acpi_object_eval.h"

typedef struct {
	const ACPI_STATUS status;
	const fwts_log_level level;
	const char *error_type;
	const char *error_text;
	const char *advice;
} acpi_eval_error;

static const acpi_eval_error errors[] = {
	/* ACPI_STATUS			fwts_log_level		error_type		error_text */
	{ AE_ERROR,			LOG_LEVEL_HIGH,		"AEError",		"Unspecified error",
		"An internal unspecified ACPICA execution error has occurred."
	},
	{ AE_NO_ACPI_TABLES,		LOG_LEVEL_HIGH,		"AENoACPITables",	"No ACPI tables",
		"No ACPI tables could be found.  Normally this indicates the tables could not be loaded "
		"from firmware or a provided firmware dump file."
	},
	{ AE_NO_NAMESPACE,		LOG_LEVEL_HIGH,		"AENoNamespace",	"No namespace",
		"An ACPI namespace was not loaded. This happens if an ACPI root node could not be found."
	},
	{ AE_NO_MEMORY,			LOG_LEVEL_CRITICAL,	"AENoMemory",		"Out of memory",
		"No more memory could be allocated from the heap."
	},
	{ AE_NOT_FOUND,			LOG_LEVEL_CRITICAL,	"AENotFound",		"Not found",
		"An ACPI object or entity was requested but could not be found. Running fwts with the "
		"--acpica=slack option may work around this issue."
	},
	{ AE_NOT_EXIST,			LOG_LEVEL_CRITICAL,	"AENotExist",		"Not exist",
		"An ACPI object or entity was requested but does not exist.",
	},
	{ AE_ALREADY_EXISTS,		LOG_LEVEL_HIGH,		"AEAlreadyExists",	"Already exists",
		"An ACPI object or entity already exists.  Sometimes this occurs when code is not "
		"serialized and it has been created a second time by another thread. ACPICA can detect "
		"these errors and enable auto serialization to try to work around this error. "
	},
	{ AE_TYPE,			LOG_LEVEL_CRITICAL,	"AETtype",		"Type",
		"The type of an object is incorrect and does not match the expected type. ACPI will "
		"generally abort execution of the AML op-code that causes this error."
	},
	{ AE_NULL_OBJECT,		LOG_LEVEL_CRITICAL,	"AENullObject",		"Null object",
		"An ACPI object is NULL when it was expected to be non-NULL."
	},
	{ AE_NULL_ENTRY,		LOG_LEVEL_CRITICAL,	"AENullEntry",		"Null entry",
		"The requested ACPI object did not exist.  This can occur when a root entry is "
		"NULL."
	},
	{ AE_BUFFER_OVERFLOW,		LOG_LEVEL_CRITICAL,	"AEBufferOverflow",	"Buffer overflow",
		"An ACPI buffer that was provided was too small to contain the data to be copied "
		"into it. This generally indicates an AML bug."
	},
	{ AE_STACK_OVERFLOW,		LOG_LEVEL_CRITICAL,	"AEStackOverflow",	"Stack overflow",
		"An ACPI method call failed because the call nesting was too deep and the "
		"internal APCI execution stack overflowed. This generally occurs with AML that "
		"may be recursing too deeply."
	},
	{ AE_STACK_UNDERFLOW,		LOG_LEVEL_CRITICAL,	"AEStackUnderflow",	"Stack underflow",
		"An ACPI method call failed because the internal ACPI execution stack underflowed. "
		"This occurred while walking the object stack and popping off an object from an empty stack."
	},
	{ AE_NOT_IMPLEMENTED,		LOG_LEVEL_HIGH,		"AENotImplemented",	"Not implemented",
		"The ACPICA execution engine has not implemented a feature."
	},
	{ AE_SUPPORT,			LOG_LEVEL_HIGH,		"AESupport",		"Support",
		"Normally this occurs when AML attempts to perform an action that is not supported, "
		"such as converting objects in an unsupported way or attempting to access memory using "
		"an unsupported address space type."
	},
	{ AE_LIMIT,			LOG_LEVEL_CRITICAL,	"AELimit",		"Limit",
		"A predefined limit was reached, for example, attempting to access an I/O port above the "
		"allowed 64K limit or a semaphore handle is out of range."
	},
	{ AE_TIME,			LOG_LEVEL_HIGH,		"AETime",		"Timeout",
		"A timeout has been reached, for example a semaphore has timed out."
	},
	{ AE_ACQUIRE_DEADLOCK,		LOG_LEVEL_CRITICAL,	"AEAcqDeadlock",	"Acquire deadlock",
		"A mutex acquire was aborted because a potential deadlock was detected."
	},
	{ AE_RELEASE_DEADLOCK,		LOG_LEVEL_CRITICAL,	"AERelDeadlock",	"Release deadlock",
		"A mutex release was aborted because a potential deadlock was detected."
	},
	{ AE_NOT_ACQUIRED,		LOG_LEVEL_CRITICAL,	"AENotAcq",		"Not acquired",
		"An attempt to release a mutex or an ACPI global lock failed because it "
		"had not previously acquired. This normally indicates an AML programming bug."
	},
	{ AE_ALREADY_ACQUIRED,		LOG_LEVEL_CRITICAL,	"AEAlreadyAcq",		"Already acquired",
		"An attempt to re-acquire mutex or an ACPI global lock failed because it "
		"had not previously acquired. This normally indicates an AML programming bug."
	},
	{ AE_NO_HARDWARE_RESPONSE,	LOG_LEVEL_HIGH,		"AENoHWResponse",	"No hardware response",
		"ACPI gave up waiting for hardware to respond. For example, a SMI_CMD failed or "
		"the platform does not have an ACPI global lock defined or an ACPI event could "
		"not be enabled."
	},
	{ AE_NO_GLOBAL_LOCK,		LOG_LEVEL_HIGH,		"AENoGlobalLock",	"No global lock",
		"The platform does not define an ACPI global lock in the FACS."
	},
	{ AE_ABORT_METHOD,		LOG_LEVEL_CRITICAL,	"AEAbortMethod",	"Abort method",
		"Execution of an ACPI method was aborted because the internal ACPICA AcpiGbl_AbortMethod "
		"flag was set to true.  This should not occur."
	},
	{ AE_SAME_HANDLER,		LOG_LEVEL_HIGH,		"AESameHandler",	"Same handler",
		"An attempt was made to install the same handler that is already installed."
	},
	{ AE_NO_HANDLER,		LOG_LEVEL_CRITICAL,	"AENoHandler",		"No handler",
		"A handler for a General Purpose Event operation was not installed."
	},
	{ AE_OWNER_ID_LIMIT,		LOG_LEVEL_HIGH,		"AEOwnerIDLimit",	"Owner ID limit",
		"A maximum of 255 OwnerIds have been used. There are no more Owner IDs available for "
		"ACPI tables or control methods."
	},

	{ AE_BAD_PARAMETER,		LOG_LEVEL_HIGH,		"AEBadParam",		"Args: Bad paramater",
		"A parameter passed is out of range or invalid."
	},
	{ AE_BAD_CHARACTER,		LOG_LEVEL_HIGH,		"AEBadChar",		"Args: Bad character",
		"An invalid character was found in an ACPI name."
	},
	{ AE_BAD_PATHNAME,		LOG_LEVEL_HIGH,		"AEBadPathname",	"Args: Bad pathname",
		"An internal AML namestring could not be built because of an invalid pathname."
	},
	{ AE_BAD_DATA,			LOG_LEVEL_HIGH,		"AEBadData",		"Args: Bad data",
		"A buffer or package contained some incorrect data."
	},
	{ AE_BAD_HEX_CONSTANT,		LOG_LEVEL_HIGH,		"AEBadHexConst",	"Args: Bad hex constant",
		"An invalid character was found in a hexadecimal constant."
	},
	{ AE_BAD_OCTAL_CONSTANT,	LOG_LEVEL_HIGH,		"AEBadOctConst",	"Args: Bad octal constant",
		"An invalid character was found in an octal constant."
	},
	{ AE_BAD_DECIMAL_CONSTANT,	LOG_LEVEL_HIGH,		"AEBadDecConst",	"Args: Bad decimal constant",
		"An invalid character was found in a decimal constant."
	},
	{ AE_MISSING_ARGUMENTS,		LOG_LEVEL_CRITICAL,	"AEMissingArgs",	"Args: Missing arguments",
		"Too few arguments were passed into an ACPI control method."
	},
	{ AE_BAD_ADDRESS,		LOG_LEVEL_CRITICAL,	"AEBadAddr",		"Args: Bad address",
		"An illegal null I/O address was referenced by the AML."
	},

	{ AE_AML_BAD_OPCODE, 		LOG_LEVEL_CRITICAL,	"AEAMLBadOpCode",	"Bad AML opcode",
		"An invalid AML opcode encountered during AML execution."
	},
	{ AE_AML_NO_OPERAND, 		LOG_LEVEL_HIGH,		"AEAMLNoOperand",	"Missing operand",
		"An AML opcode was missing a required operand."
	},
	{ AE_AML_OPERAND_TYPE, 		LOG_LEVEL_HIGH,		"AEAMLOperandType",	"Incorrect operand type",
		"An AML opcode had an operand of an incorrect type."
	},
	{ AE_AML_OPERAND_VALUE, 	LOG_LEVEL_HIGH,		"AEAMLOperandValue",	"Incorrect operand value",
		"An AML opcode has an operand of an inappropriate or invalid value.",
	},
	{ AE_AML_UNINITIALIZED_LOCAL, 	LOG_LEVEL_HIGH,		"AEAMLUninitLocal",	"Uninitialized local variable",
		"A method attempted to use a local variable that was not initialized."
	},
	{ AE_AML_UNINITIALIZED_ARG, 	LOG_LEVEL_HIGH,		"AEAMLUninitArg",	"Uninitialized argument",
		"A method attempted to use an uninitialized argument."
	},
	{ AE_AML_UNINITIALIZED_ELEMENT, LOG_LEVEL_HIGH,		"AEAMLUninitElement",	"Uninitialized element",
		"A method attempted to use an empty element in an ACPI package."
	},
	{ AE_AML_NUMERIC_OVERFLOW, 	LOG_LEVEL_HIGH,		"AEAMLNumericOverflow",	"Numeric overflow",
		"A numeric overflow occurred, for example when coverting a BCD value."
	},
	{ AE_AML_REGION_LIMIT, 		LOG_LEVEL_CRITICAL,	"AEAMLRegionLimit",	"Region limit",
		"An attempt to access beyond the end of an ACPI Operation Region occurred."
	},
	{ AE_AML_BUFFER_LIMIT, 		LOG_LEVEL_CRITICAL,	"AEAMLBufferLimit",	"Buffer limit",
		"An attempt to access beyond the end of an ACPI buffer occurred."
	},
	{ AE_AML_PACKAGE_LIMIT, 	LOG_LEVEL_CRITICAL,	"AEAMLPackgeLimit",	"Package limit",
		"An attempt to access beyond the endof an ACPI package occurred."
	},
	{ AE_AML_DIVIDE_BY_ZERO, 	LOG_LEVEL_CRITICAL,	"AEAMLDivideByZero",	"Division by zero",
		"A division by zero error occurred while excuting the AML Divide op-code"
	},
	{ AE_AML_NAME_NOT_FOUND, 	LOG_LEVEL_HIGH,		"AEAMLNameNotFound",	"Name not found",
		"An ACPI named reference could not be resolved."
	},
	{ AE_AML_INTERNAL, 		LOG_LEVEL_HIGH,		"AEAMLInternal",	"Internal ACPICA execution engine error",
		"An internal error occurred in the ACPICA execution interpreter.  A bug report should "
		"be filed against fwts so that this can be resolved."
	},
	{ AE_AML_INVALID_SPACE_ID, 	LOG_LEVEL_HIGH,		"AEAMLInvalidSpaceID",	"Invalid space ID error",
		"An invalid ACPI Operation Region Space ID has been encountered."
	},
	{ AE_AML_STRING_LIMIT, 		LOG_LEVEL_HIGH,		"AEAMLStringLimit",	"String limit",
		"A string more than 200 characters long has been used."
	},
	{ AE_AML_NO_RETURN_VALUE, 	LOG_LEVEL_HIGH,		"AEAMLNoReturnValue",	"No return value",
		"A method did not return the required value that was expected."
	},
	{ AE_AML_METHOD_LIMIT, 		LOG_LEVEL_HIGH,		"AEAMLMethodLimit",	"Method limit",
		"A control method reached a limit of 255 concurrent thread executions and hit the ACPICA "
		"re-entrancy limit."
	},
	{ AE_AML_NOT_OWNER, 		LOG_LEVEL_HIGH,		"AEAMLNotOwner",	"Not owner",
		"A thread attempted to release a mutex that it did not own.",
	},
	{ AE_AML_MUTEX_ORDER, 		LOG_LEVEL_HIGH,		"AEAMLMutexOrder",	"Mutex order",
		"Deadlock prevention has detected that the current mutex sync level is too large."
	},
	{ AE_AML_MUTEX_NOT_ACQUIRED, 	LOG_LEVEL_HIGH,		"AEAMLMutexNotAcq",	"Mutux not acquired",
		"An attempt was made to release a mutex that had not been acquired."
	},
	{ AE_AML_INVALID_RESOURCE_TYPE, LOG_LEVEL_HIGH,		"AEAMLInvResourceType", "Invalid resource type",
		"An ACPI resource list contained an invalid resource type."
	},
	{ AE_AML_INVALID_INDEX, 	LOG_LEVEL_HIGH,		"AEAMLInvIndex",	"Invalid index",
		"An attempt to get a node with an index that was too large occurred, for example an ACPI "
		"ArgN or LocalN were N was out of range."
	},
	{ AE_AML_REGISTER_LIMIT, 	LOG_LEVEL_HIGH,		"AEAMLRegisterLimit",	"Register limit",
		"An attempt to use a bank value that is beyong the capacity of a register occurred."
	},
	{ AE_AML_NO_WHILE, 		LOG_LEVEL_CRITICAL,	"AEAMLNoWhile",		"No while",
		"A Break or Continue op-code was reached without a matching While op-code."
	},
	{ AE_AML_ALIGNMENT, 		LOG_LEVEL_CRITICAL,	"AEAMLAlignment",	"Misaligmnent",
		"A non-aligned memory transfer was attempted on a machine that does not "
		"support it."
	},
	{ AE_AML_NO_RESOURCE_END_TAG, 	LOG_LEVEL_HIGH,		"AEAMLNoResEndTag",	"No resource end tag",
		"An ACPI resource list had a missing End Tag."
	},
	{ AE_AML_BAD_RESOURCE_VALUE, 	LOG_LEVEL_HIGH,		"AEAMLBadResValue",	"Bad resource value",
		"An ACPI resource element had an invalid value."
	},
	{ AE_AML_CIRCULAR_REFERENCE,	LOG_LEVEL_CRITICAL,	"AEAMLCircularRef",	"Circular reference",
		"Two references refer to each other, which is not allowed."
	},
	{ AE_AML_BAD_RESOURCE_LENGTH,	LOG_LEVEL_HIGH,		"AEAMLBadResLength",	"Bad resource length",
		"The length of an ACPI Resource Descriptor was incorrect."
	},
	{ AE_AML_ILLEGAL_ADDRESS, 	LOG_LEVEL_CRITICAL,	"AEAMLIllegalAddr",	"Illegal address",
		"An memory, PCI configuration or I/O address was encountered with an illegal address."
	},
	/* { AE_AML_INFINITE_LOOP, 	LOG_LEVEL_HIGH,		"AEAMLInfiniteLoop",	"Infinite loop", NULL }, */
	{ 0,				0,			NULL,			NULL , 		NULL}
};

static fwts_list *fwts_object_names;
static bool fwts_acpi_initialized = false;

/*
 *  fwts_acpi_init()
 *	Initialise ACPIA engine and collect method namespace
 */
int fwts_acpi_init(fwts_framework *fw)
{
	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	/* Gather all object names */
	fwts_object_names = fwts_acpica_get_object_names(0);
	fwts_acpi_initialized = true;

	return FWTS_OK;
}

/*
 *  fwts_acpi_deinit()
 *	Close ACPIA engine and free method namespace
 */
int fwts_acpi_deinit(fwts_framework *fw)
{
	int ret = FWTS_ERROR;

	FWTS_UNUSED(fw);

	if (fwts_acpi_initialized) {
		fwts_list_free(fwts_object_names, free);
		fwts_object_names = NULL;
		ret = fwts_acpica_deinit();

		fwts_acpi_initialized = false;
	}

	return ret;
}

/*
 *  fwts_acpi_object_get_names()
 *	return list of object names
 */
fwts_list *fwts_acpi_object_get_names(void)
{
	return fwts_object_names;
}

/*
 *  fwts_acpi_object_exists()
 *	return first matching name
 */
char *fwts_acpi_object_exists(const char *name)
{
	size_t name_len = strlen(name);
	fwts_list_link	*item;

	fwts_list_foreach(item, fwts_object_names) {
		char *method_name = fwts_list_data(char*, item);
		size_t len = strlen(method_name);

		if (strncmp(name, method_name + len - name_len, name_len) == 0)
			return method_name;
	}
	return NULL;
}


/*
 *   fwts_acpi_object_dump_recursive()
 *	dump out an object, minimal form
 */
static void fwts_acpi_object_dump_recursive(
	fwts_framework *fw,
	const ACPI_OBJECT *obj,
	const int depth,
	const int index)
{
	uint32_t i;
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
		fwts_log_info_verbatum(fw, "%*s%sSTRING:  %s", depth * 2, "",
			index_buf, obj->String.Pointer);
		break;
	case ACPI_TYPE_BUFFER:
		fwts_log_info_verbatum(fw, "%*s%sBUFFER:  (%d bytes)", depth * 2, "",
			index_buf, obj->Buffer.Length);
		break;
	case ACPI_TYPE_PACKAGE:
		fwts_log_info_verbatum(fw, "%*s%sPackage has %d elements:",depth * 2, "",
			index_buf, obj->Package.Count);
		for (i = 0; i < obj->Package.Count; i++) {
			ACPI_OBJECT *element = &obj->Package.Elements[i];
			fwts_acpi_object_dump_recursive(fw, element, depth + 1, i);
		}
		break;
	default:
		fwts_log_info_verbatum(fw, "%*s%sUnknown type %d\n", depth * 2, "",
			index_buf, obj->Type);
		break;
	}
}

/*
 *   fwts_method_object_dump()
 *	dump out an object, minimal form
 */
void fwts_acpi_object_dump(fwts_framework *fw, const ACPI_OBJECT *obj)
{
	fwts_acpi_object_dump_recursive(fw, obj, 1, -1);
}

/*
 *  fwts_acpi_object_evaluate_report_error()
 *	report any errors found during object evaluation
 */
void fwts_acpi_object_evaluate_report_error(
	fwts_framework *fw,
	const char *name,
	const ACPI_STATUS status)
{
	int i;

	/* Generic cases */
	for (i=0; errors[i].error_type; i++) {
		if (status == errors[i].status) {
			fwts_failed(fw, errors[i].level, errors[i].error_type,
				"Detected error '%s' when evaluating '%s'.",
				errors[i].error_text, name);
			if (errors[i].advice != NULL)
				fwts_advice(fw, "%s", errors[i].advice);
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
 *  fwts_acpi_object_evaluate()
 *	evaluate object, return error status (handle this with
 *	fwts_method_evaluate_report_error()).
 * 	This returns buf which auto allocates any return values
 *	which need to be freed post-evalutation using free().
 */
ACPI_STATUS fwts_acpi_object_evaluate(fwts_framework *fw,
	char *name,
        ACPI_OBJECT_LIST *arg_list,
	ACPI_BUFFER	 *buf)
{
	FWTS_UNUSED(fw);

        buf->Length  = ACPI_ALLOCATE_BUFFER;
        buf->Pointer = NULL;

        return AcpiEvaluateObject(NULL, name, arg_list, buf);
}

