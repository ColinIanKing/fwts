/*
 * Copyright (C) 2022-2025 Canonical
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

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static fwts_acpi_table_info *table;
acpi_table_init(NHLT, &table)

static void nhltdump_data_hexdump(fwts_framework *fw, uint8_t *data, size_t size)
{
	size_t i;

	for (i = 0; i < size; i += 16) {
		char buffer[128];
		size_t left = size - i;

		fwts_dump_raw_data(buffer, sizeof(buffer), data + i, i, left > 16 ? 16 : left);
		fwts_log_info_verbatim(fw,  "    %s", buffer + 2);
	}
}

static char *nhltdump_linktype_to_string(const uint8_t linktype)
{
	char *str;

	switch (linktype) {
	case 0:
		str = "HD-Audio";
		break;
	case 1:
		str = "DSP ";
		break;
	case 2:
		str = "PDM";
		break;
	case 3:
		str = "SSP";
		break;
	case 4:
		str = "Slimbus";
		break;
	case 5:
		str = "SoundWire";
		break;
	default:
		str = "reserved";
		break;
	}

	return str;
}

static char *nhltdump_devtype_to_string(const uint8_t linktype, const uint8_t devtype)
{
	char *str;

	if (linktype == 2) {
		switch (devtype) {
		case 0:
			str = "PDM on cAVS1.8 (CNL+) platforms only";
			break;
		case 1:
			str = "PDM on cAVS1.5 (KBL) based platforms";
			break;
		default:
			str = "reserved";
			break;
		}
	} else if (linktype == 3) {
		switch (devtype) {
		case 0:
			str = "BT Sideband";
			break;
		case 1:
			str = "FM";
			break;
		case 2:
			str = "Modem";
			break;
		case 4:
			str = "SSP Analog Codec";
			break;
		default:
			str = "reserved";
			break;
		}
	} else
		str = "TBD";

	return str;
}

static char *nhltdump_direction_to_string(const uint8_t direction)
{
	char *str;

	switch (direction) {
	case 0:
		str = "Render";
		break;
	case 1:
		str = "Capture";
		break;
	case 2:
		str = "Render with loopback / feedback";
		break;
	case 3:
		str = "Feedback for render (smartamp)";
		break;
	default:
		str = "";
		break;
	}

	return str;
}

/*
 *  NHLT Table
 *   see https://01.org/sites/default/files/595976_intel_sst_nhlt.pdf
 */
static int nhltdump_test1(fwts_framework *fw)
{
	fwts_acpi_table_nhlt *nhlt = (fwts_acpi_table_nhlt *)table->data;
	uint32_t i, j;
	fwts_acpi_table_nhlt_ep_descriptor *ep_descriptor = (fwts_acpi_table_nhlt_ep_descriptor *)
		((uint8_t *)table->data + sizeof(fwts_acpi_table_nhlt));
	fwts_acpi_table_nhlt_specific_config *spec_config = NULL;
	fwts_acpi_table_nhlt_formats_configuration *formats_conf = NULL;
	fwts_acpi_table_nhlt_format_configuration *format_conf = NULL;
	char guid_str[37];
	uint32_t table_length = table->length;
	uint32_t offset = 0;
	char *str_info;

	fwts_log_info_simp_int(fw, "TableLength: ", table_length);
	offset += sizeof(fwts_acpi_table_nhlt);
	if (offset > table_length) {
		fwts_log_info_verbatim(fw, "Table too short, skip dumping remaining data.");
		return FWTS_OK;
	}
	fwts_log_info_verbatim(fw, "NHLT Table:");
	fwts_log_info_verbatim(fw, "  EndpointDescriptorCount: %" PRIu8, nhlt->ep_descriptor_count);

	for (i = 0; i < nhlt->ep_descriptor_count; i++) {
		offset += ep_descriptor->ep_descriptor_len;
		if (offset > table_length) {
			fwts_log_info_verbatim(fw, "Table too short, skip dumping remaining data.");
			return FWTS_OK;
		}
		fwts_log_info_verbatim(fw, "  EndpointDescriptor %" PRIu8, (i + 1));
		fwts_log_info_simp_int(fw, "    EndpointDescriptorLength: ", ep_descriptor->ep_descriptor_len);
		str_info = nhltdump_linktype_to_string(ep_descriptor->link_type);
		fwts_log_info_verbatim(fw, "    LinkType: %" PRIu8 " (%s)", ep_descriptor->link_type, str_info);
		fwts_log_info_simp_int(fw, "    Instance ID: ", ep_descriptor->instance_id);
		fwts_log_info_simp_int(fw, "    Vendor ID: ", ep_descriptor->vendor_id);
		fwts_log_info_simp_int(fw, "    Device ID: ", ep_descriptor->device_id);
		fwts_log_info_simp_int(fw, "    Revision ID: ", ep_descriptor->revision_id);
		fwts_log_info_simp_int(fw, "    Subsystem ID: ", ep_descriptor->subsystem_id);
		str_info = nhltdump_devtype_to_string(ep_descriptor->link_type, ep_descriptor->device_type);
		fwts_log_info_verbatim(fw, "    Device Type: %" PRIu8 " (%s)", ep_descriptor->device_type, str_info);
		str_info = nhltdump_direction_to_string(ep_descriptor->direction);
		fwts_log_info_verbatim(fw, "    Direction: %" PRIu8 " (%s)", ep_descriptor->direction, str_info);
		fwts_log_info_simp_int(fw, "    Virtual Bus ID: ", ep_descriptor->virtual_bus_id);

		spec_config = (fwts_acpi_table_nhlt_specific_config *)((uint8_t *)ep_descriptor
				+ sizeof(fwts_acpi_table_nhlt_ep_descriptor));
		fwts_log_info_verbatim(fw, "  EndpointConfig");
		fwts_log_info_simp_int(fw, "    CapabilitiesSize: ", spec_config->capabilities_size);
		fwts_log_info_verbatim(fw, "    Capabilities:");
		nhltdump_data_hexdump(fw, spec_config->capabilities, spec_config->capabilities_size);

		fwts_log_info_verbatim(fw, "  FormatsConfig");
		formats_conf = (fwts_acpi_table_nhlt_formats_configuration *)((uint8_t *)spec_config
				+ sizeof(spec_config->capabilities_size) + spec_config->capabilities_size);

		fwts_log_info_verbatim(fw, "  FormatsCount %" PRIu8, formats_conf->formatscount);
		format_conf = formats_conf->formatsconfiguration;
		for (j = 0; j < formats_conf->formatscount; j++) {
			fwts_log_info_verbatim(fw, "    FormatConfig %" PRIu8, (j + 1));
			fwts_log_info_simp_int(fw, "      wFormatTag: ", format_conf->format.wformattag);
			fwts_log_info_simp_int(fw, "      nChannels: ", format_conf->format.nchannels);
			fwts_log_info_simp_int(fw, "      nSamplesPerSec: ", format_conf->format.nsamplespersec);
			fwts_log_info_simp_int(fw, "      nAvgBytesPerSec: ", format_conf->format.navgbytespersec);
			fwts_log_info_simp_int(fw, "      nBlockAlign: ", format_conf->format.nblockalign);
			fwts_log_info_simp_int(fw, "      wBitsPerSample: ", format_conf->format.wbitspersample);
			fwts_log_info_simp_int(fw, "      cbSize: ", format_conf->format.cbsize);
			fwts_log_info_simp_int(fw, "      wValidBitsPerSample: ", format_conf->format.wvalidbitspersample);
			fwts_log_info_simp_int(fw, "      dwChannelMask: ", format_conf->format.dwchannelmask);
			fwts_guid_buf_to_str(format_conf->format.subformat, guid_str, sizeof(guid_str));
			fwts_log_info_verbatim(fw, "      SubFormat:  %s", guid_str);

			fwts_log_info_verbatim(fw, "      SpecificConfig");
			fwts_log_info_simp_int(fw, "        CapabilitiesSize: ", format_conf->formatconfiguration.capabilities_size);
			fwts_log_info_verbatim(fw, "        Capabilities:");
			nhltdump_data_hexdump(fw, format_conf->formatconfiguration.capabilities,
					format_conf->formatconfiguration.capabilities_size);
			format_conf = (fwts_acpi_table_nhlt_format_configuration *)
					((uint8_t *)format_conf->formatconfiguration.capabilities
					+ format_conf->formatconfiguration.capabilities_size);
		}
		ep_descriptor = (fwts_acpi_table_nhlt_ep_descriptor *)((uint8_t *)ep_descriptor + ep_descriptor->ep_descriptor_len);
	}

	/* TODO: dumping the OEDConfig, currently the specification not clear defined the remaining data */

	return FWTS_OK;
}

static fwts_framework_minor_test nhltdump_tests[] = {
	{ nhltdump_test1, "Dump non-HD Audio endpoints configuration performed via ACPI(NHLT)." },
	{ NULL, NULL }
};

static fwts_framework_ops nhltdump_ops = {
	.description = "Dump configurations performed via NHLT.",
	.init        = NHLT_init,
	.minor_tests = nhltdump_tests
};

FWTS_REGISTER("nhltdump", &nhltdump_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UTILS)

#endif
