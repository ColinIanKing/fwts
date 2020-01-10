/*
 * Copyright (C) 2020 Canonical
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
 */
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include "fwts_acpi_object_eval.h"

#define DEVICE_PROPERITY_UUID "DAFFD814-6EBA-4D8C-8A91-BC9BBF4AA301"
#define HIERARCHICAL_DATA_EXTENSION "DBB8E3E6-5886-4BA6-8795-1319F52A966B"

/*
 *  dsddump_init()
 *	initialize ACPI
 */
static int dsddump_init(fwts_framework *fw)
{
	if (fwts_acpi_init(fw) != FWTS_OK) {
		fwts_log_error(fw, "Cannot initialise ACPI.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

/*
 *  dsddump_deinit
 *	de-intialize ACPI
 */
static int dsddump_deinit(fwts_framework *fw)
{
	return fwts_acpi_deinit(fw);
}

static void print_package_element(fwts_framework *fw, ACPI_OBJECT *obj)
{
	char full_name[128];
	ACPI_STATUS status;
	ACPI_BUFFER buffer;

	buffer.Length = sizeof(full_name);
	buffer.Pointer = full_name;

	switch (obj->Type) {
		case ACPI_TYPE_INTEGER:
			fwts_log_info_verbatim(fw, "    Value: 0x%8.8" PRIx64, obj->Integer.Value);
			break;
		case ACPI_TYPE_STRING:
			fwts_log_info_verbatim(fw, "    Value: %s", obj->String.Pointer);
			break;
		case ACPI_TYPE_LOCAL_REFERENCE:
			status = AcpiGetName(obj->Reference.Handle, ACPI_FULL_PATHNAME, &buffer);
			if (ACPI_SUCCESS(status))
				fwts_log_info_verbatim(fw, "    Value: %s", full_name);
			break;
		default:
			fwts_log_info_verbatim(fw, "    Value: %s", "Unknown");
			break;
	}
}

static void parse_device_properity(fwts_framework *fw, ACPI_OBJECT *pkg)
{
	uint32_t i;

	fwts_log_info_verbatim(fw, "  Package:");
	for (i = 0; i < pkg->Package.Count; i++) {
		ACPI_OBJECT *element = &pkg->Package.Elements[i];

		fwts_log_info_verbatim(fw, "    Key: %s", element->Package.Elements[0].String.Pointer);

		if (element->Package.Elements[1].Type == ACPI_TYPE_PACKAGE) {
			ACPI_OBJECT *sub_pkg = &element->Package.Elements[1];
			uint32_t j;

			for (j = 0; j < sub_pkg->Package.Count; j++)
				print_package_element(fw, &sub_pkg->Package.Elements[j]);
		} else
			print_package_element(fw, &element->Package.Elements[1]);
	}
}

static void parse_hierarchical_data_ext(fwts_framework *fw, ACPI_OBJECT *pkg)
{
	uint32_t i;

	fwts_log_info_verbatim(fw, "  Package:");
	for (i = 0; i < pkg->Package.Count; i++) {
		ACPI_OBJECT *element = &pkg->Package.Elements[i];
		fwts_log_info_verbatim(fw, "    Key: %s", element->Package.Elements[0].String.Pointer);
		print_package_element(fw, &element->Package.Elements[1]);
	}
}

static void dsddump_package(
	fwts_framework *fw,
	ACPI_OBJECT *uuid,
	ACPI_OBJECT *pkg)
{
	char guid[37];

	fwts_guid_buf_to_str(uuid->Buffer.Pointer, guid, sizeof(guid));

	if (!strncmp(guid, DEVICE_PROPERITY_UUID, sizeof(guid))) {
		fwts_log_info_verbatim(fw, "  Device Properties UUID: %s", guid);
		parse_device_properity(fw, pkg);
	} else if (!strncmp(guid, HIERARCHICAL_DATA_EXTENSION, sizeof(guid))) {
		fwts_log_info_verbatim(fw, "  Hierarchical Data Extension UUID: %s", guid);
		parse_hierarchical_data_ext(fw, pkg);
	}
}

static int dsddump_test1(fwts_framework *fw)
{
	const size_t name_len = 4;
	fwts_list_link	*item;
	fwts_list *objects;
	bool found = false;

	if ((objects = fwts_acpi_object_get_names()) == NULL) {
		fwts_log_info(fw, "Cannot find any ACPI objects");
		return FWTS_ERROR;
	}

	fwts_list_foreach(item, objects) {
		char *name = fwts_list_data(char *, item);
		const size_t len = strlen(name);

		if (!strncmp("_DSD", name + len - name_len, name_len)) {
			ACPI_OBJECT_LIST arg_list;
			ACPI_OBJECT *obj;
			ACPI_BUFFER buf;
			int ret;

			arg_list.Count   = 0;
			arg_list.Pointer = NULL;

			ret = fwts_acpi_object_evaluate(fw, name, &arg_list, &buf);
			if ((ACPI_FAILURE(ret) != AE_OK) || (buf.Pointer == NULL))
				continue;

			/*  Do we have a valid buffer in the package to dump? */
			obj = buf.Pointer;
			if ((obj->Type == ACPI_TYPE_PACKAGE) &&
			    (obj->Package.Count) &&
			    (obj->Package.Elements[0].Type == ACPI_TYPE_BUFFER) &&
			    (obj->Package.Elements[0].Buffer.Pointer != NULL) &&
			    (obj->Package.Elements[0].Buffer.Length == 16)) {
				uint32_t i;

				fwts_log_info_verbatim(fw, "Name: %s", name);
				for (i = 0; i < obj->Package.Count; i += 2)
					dsddump_package(fw, &obj->Package.Elements[i], &obj->Package.Elements[i+1]);

				fwts_log_nl(fw);
				found = true;
			}
			free(buf.Pointer);
		}
	}

	if (!found)
		fwts_log_info_verbatim(fw, "No _DSD objects found.");

	return FWTS_OK;
}

static fwts_framework_minor_test dsddump_tests[] = {
	{ dsddump_test1, "Dump ACPI _DSD (Device Specific Data)." },
	{ NULL, NULL }
};

static fwts_framework_ops dsddump_ops = {
	.description = "Dump ACPI _DSD (Device Specific Data).",
	.init        = dsddump_init,
	.deinit      = dsddump_deinit,
	.minor_tests = dsddump_tests
};

FWTS_REGISTER("dsddump", &dsddump_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UTILS)

#endif
