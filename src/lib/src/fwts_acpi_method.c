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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* acpica headers */
#include "acpi.h"

static fwts_list *fwts_method_names;
static bool fwts_method_initialized = false;

int fwts_method_init(fwts_framework *fw)
{
	if (fwts_acpica_init(fw) != FWTS_OK)
		return FWTS_ERROR;

	fwts_method_names = fwts_acpica_get_object_names(8);
	fwts_method_initialized = true;

	return FWTS_OK;
}

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

fwts_list *fwts_method_get_names(void)
{
	return fwts_method_names;
}

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

void fwts_method_dump_package(fwts_framework *fw, ACPI_OBJECT *obj)
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

void fwts_method_evaluate_report_error(fwts_framework *fw, char *name, ACPI_STATUS status)
{
	switch (status) {
	case AE_OK:
		break;
	case AE_AML_BAD_OPCODE:
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "AMLBadOpCode", "Detected a bad AML opcode when evaluating method '%s'.", name);
		break;
	case AE_AML_NO_OPERAND:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLNoOperand", "Detected a AML opcode with a missing operand when evaluating method '%s'.", name);
		break;
	case AE_AML_OPERAND_TYPE:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLOperandType", "Detected a AML opcode with an incorrect operand type when evaluating method '%s'.", name);
		break;
	case AE_AML_OPERAND_VALUE:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLOperandValue", "Detected a AML opcode with an incorrect operand value when evaluating method '%s'.", name);
		break;
	case AE_AML_UNINITIALIZED_LOCAL:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLUninitValue", "Detected an uninitialized local variable when evaluating method '%s'.", name);
		break;
	case AE_AML_UNINITIALIZED_ARG:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLUninitArg", "Detected an uninitialized argument when evaluating method '%s'.", name);
		break;
	case AE_AML_UNINITIALIZED_ELEMENT:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLUninitElement", "Detected an uninitialized element when evaluating method '%s'.", name);
		break;
	case AE_AML_NUMERIC_OVERFLOW:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLNumericOverflow", "Detected a numeric overflow when evaluating method '%s'.", name);
		break;
	case AE_AML_REGION_LIMIT:
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "AMLRegionLimit", "Detected a region limit when evaluating method '%s'.", name);
		break;
	case AE_AML_BUFFER_LIMIT:
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "AMLBufferLimit", "Detected a buffer limit when evaluating method '%s'.", name);
		break;
	case AE_AML_PACKAGE_LIMIT:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLPackageLimit", "Detected a package limit when evaluating method '%s'.", name);
		break;
	case AE_AML_DIVIDE_BY_ZERO:
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "AMLDivByZero", "Detected a division by zero when evaluating method '%s'.", name);
		break;
	case AE_AML_NAME_NOT_FOUND:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLNameNotFound", "Detected a name not found error when evaluating method '%s'.", name);
		break;
	case AE_AML_INTERNAL:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLInternal", "Detected an internal ACPICA execution engine error when evaluating method '%s'.", name);
		break;
	case AE_AML_INVALID_SPACE_ID:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLInvalidSpaceId", "Detected an invalid space ID error when evaluating method '%s'.", name);
		break;
	case AE_AML_STRING_LIMIT:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLStringLimit", "Detected a string limit error when evaluating method '%s'.", name);
		break;
	case AE_AML_NO_RETURN_VALUE:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLNoReturnValue", "Detected a no return value error when evaluating method '%s'.", name);
		break;
	case AE_AML_METHOD_LIMIT:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLMethodLimit", "Detected a method limit error when evaluating method '%s'.", name);
		break;
	case AE_AML_NOT_OWNER:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLNotOwner", "Detected a not owner error when evaluating method '%s'.", name);
		break;
	case AE_AML_MUTEX_ORDER:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLMutexOrder", "Detected a mutex order errro when evaluating method '%s'.", name);
		break;
	case AE_AML_MUTEX_NOT_ACQUIRED:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLMutexNotAcquired", "Detected a mutux not acquired error when evaluating method '%s'.", name);
		break;
	case AE_AML_INVALID_RESOURCE_TYPE:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLInvalidResourceType", "Detected an invalid resource type when evaluating method '%s'.", name);
		break;
	case AE_AML_INVALID_INDEX:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLInvalidIndex", "Detected an invalid index error when evaluating method '%s'.", name);
		break;
	case AE_AML_REGISTER_LIMIT:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLRegisterLimit", "Detected a register limit error when evaluating method '%s'.", name);
		break;
	case AE_AML_NO_WHILE:
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "AMLNoWhile", "Detected a no while error when evaluating method '%s'.", name);
		break;
	case AE_AML_ALIGNMENT:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLAlignment", "Detected an aligmnent error when evaluating method '%s'.", name);
		break;
	case AE_AML_NO_RESOURCE_END_TAG:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLNoRsrceEndTag", "Detected a no resource end tag error when evaluating method '%s'.", name);
		break;
	case AE_AML_BAD_RESOURCE_VALUE:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLBadRsrceValue", "Detected a bad resource value error when evaluating method '%s'.", name);
		break;
	case AE_AML_CIRCULAR_REFERENCE:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLCircularRef", "Detected a circular reference when evaluating method '%s'.", name);
		break;
	case AE_AML_BAD_RESOURCE_LENGTH:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AMLBadRscrcLength", "Detected a bad resource length when evaluating method '%s'.", name);
		break;
	case AE_AML_ILLEGAL_ADDRESS:
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "AMLIllegalAddr", "Detected an illegal address when evaluating method '%s'.", name);
		break;
	case AE_AML_INFINITE_LOOP:
		fwts_warning(fw, "Detected an infinite loop when evaluating method '%s'. ", name);
		fwts_advice(fw, "This may occur because we are emulating the execution "
				"in this test environment and cannot handshake with "
				"the embedded controller or jump to the BIOS via SMIs. "
				"However, the fact that AML code spins forever means that "
				"lockup conditions are not being checked for in the AML bytecode.");
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "AMLFailedToEvaluate", "Failed to evaluate %s, got error code %d.", name, status);
		break;
	}
}

ACPI_STATUS fwts_method_evaluate(fwts_framework *fw,
	char *name,
        ACPI_OBJECT_LIST *arg_list,
	ACPI_BUFFER	 *buf)
{
        buf->Length  = ACPI_ALLOCATE_BUFFER;
        buf->Pointer = NULL;

        return AcpiEvaluateObject(NULL, name, arg_list, buf);
}

