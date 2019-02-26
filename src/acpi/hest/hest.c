/*
 * Copyright (C) 2015-2019 Canonical
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

static int hest_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "HEST", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI HEST table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  ACPI Section 18.3.2.1 IA-32 Architecture Machine Check Exception
 */
static void hest_check_ia32_arch_machine_check_exception(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	ssize_t i, total_size;

	fwts_acpi_table_hest_ia32_machine_check_exception *exception =
		(fwts_acpi_table_hest_ia32_machine_check_exception *)*data;

	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_ia32_machine_check_exception)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTIA32MachineCheckTooShort",
			"HEST IA-32 Architecture Machine Check Exception "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_ia32_machine_check_exception), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	total_size = sizeof(fwts_acpi_table_hest_ia32_machine_check_exception) +
		(exception->number_of_hardware_banks *
		sizeof(fwts_acpi_table_hest_machine_check_bank));

	if (*length < total_size) {
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST IA-32 Architecture Machine Check Exception:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, exception->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, exception->source_id);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, exception->reserved1);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, exception->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, exception->enabled);
	fwts_log_info_verbatim(fw, "  Number of Records:        0x%8.8" PRIx32, exception->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max Sections Per Record:  0x%8.8" PRIx32, exception->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Global Capability Data:   0x%16.16" PRIx64, exception->global_capability_init_data);
	fwts_log_info_verbatim(fw, "  Global Control Data:      0x%16.16" PRIx64, exception->global_control_init_data);
	fwts_log_info_verbatim(fw, "  Number of Hardware Banks: 0x%8.8" PRIx32, exception->number_of_hardware_banks);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[0]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[1]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[2]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[3]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[4]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[5]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, exception->reserved2[6]);
	fwts_log_nl(fw);

	if (exception->flags & ~0x5) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"HESTIA32FlagsReserved",
			"HEST IA-32 Architecture MCE Flags Reserved bits "
			"[1] & [3:7] must be zero, instead got 0x%" PRIx8,
			exception->flags);
		*passed = false;
	} else if (exception->flags == 0x5) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTIA32BadFlags",
			"HEST IA-32 Architecture MCE Flags bits [0] & [2] "
			"are mutually exclusive, instead got 0x%" PRIx8,
			exception->flags);
		*passed = false;
	}

	for (i = 0; i < exception->number_of_hardware_banks; i++) {
		fwts_acpi_table_hest_machine_check_bank *bank = &exception->bank[i];

		fwts_log_info_verbatim(fw, "  HEST IA-32 Architecture Machine Check Exception Bank %zd", i);
		fwts_log_info_verbatim(fw, "    Bank Number:            0x%2.2" PRIx8, bank->bank_number);
		fwts_log_info_verbatim(fw, "    Clear Status On Init.:  0x%2.2" PRIx8, bank->clear_status_on_initialization);
		fwts_log_info_verbatim(fw, "    Status Data Format:     0x%2.2" PRIx8, bank->status_data_format);
		fwts_log_info_verbatim(fw, "    Reserved:               0x%2.2" PRIx8, bank->reserved);
		fwts_log_info_verbatim(fw, "    Control Reg. MSR Addr:  0x%8.8" PRIx32, bank->control_register_msr_address);
		fwts_log_info_verbatim(fw, "    Control Init Data:      0x%16.16" PRIx64, bank->control_init_data);
		fwts_log_info_verbatim(fw, "    Status Reg. MSR Addr:   0x%8.8" PRIx32, bank->status_register_msr_address);
		fwts_log_info_verbatim(fw, "    Addr Reg. MSR Addr:     0x%8.8" PRIx32, bank->address_register_msr_address);
		fwts_log_info_verbatim(fw, "    Misc Reg. MSR Addr:     0x%8.8" PRIx32, bank->misc_register_msr_address);
		fwts_log_nl(fw);

		if (bank->clear_status_on_initialization > 1) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"HESTIA32InvalidClearStatusOnInit",
				"HEST Machine Check Exception Bank Clear Status On Initialization "
				"has value 0x%" PRIx8 " and only valid values are "
				"0x00 (Clear) or 0x01 (Don't Clear)",
				bank->clear_status_on_initialization);
		}
		if (bank->status_data_format > 2) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"HESTIA32InvalidBankStatusDataFormat",
				"HEST Machine Check Exception Bank Status Data Format "
				"has value 0x%" PRIx8 " and only valid values are "
				"0x00 (IA-32 MCA), 0x01 (Intel 64 MCA) or "
				"0x02 (AMD64 MCA).",
				bank->status_data_format);
		}
	}

	*length -= total_size;
	*data += total_size;
}

/*
 *  ACPI Section 18.3.2.2 IA-32 Architecture Corrected Machine Check
 */
static void hest_check_ia32_arch_corrected_machine_check(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	ssize_t i, total_size;

	fwts_acpi_table_hest_ia32_corrected_machine_check *check =
		(fwts_acpi_table_hest_ia32_corrected_machine_check *)*data;

	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_ia32_corrected_machine_check)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTIA32CorrectedMachineCheckTooShort",
			"HEST IA-32 Architecture Corrected Machine Check "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_ia32_corrected_machine_check), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	total_size = sizeof(fwts_acpi_table_hest_ia32_corrected_machine_check) +
		(check->number_of_hardware_banks *
		sizeof(fwts_acpi_table_hest_machine_check_bank));

	if (*length < total_size) {
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST IA-32 Architecture Machine Check:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, check->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, check->source_id);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, check->reserved1);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, check->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, check->enabled);
	fwts_log_info_verbatim(fw, "  Number of Records:        0x%8.8" PRIx32, check->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max Sections Per Record:  0x%8.8" PRIx32, check->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Hardware Error Notification:");
	fwts_log_info_verbatim(fw, "    Type:                   0x%2.2" PRIx8, check->notification.type);
	fwts_log_info_verbatim(fw, "    Length:                 0x%2.2" PRIx8, check->notification.length);
	fwts_log_info_verbatim(fw, "    Config. Write. Enable:  0x%4.4" PRIx16,
		check->notification.configuration_write_enable);
	fwts_log_info_verbatim(fw, "    Poll Interval:          0x%4.4" PRIx16,
		check->notification.poll_interval);
	fwts_log_info_verbatim(fw, "    Interrupt Vector:       0x%4.4" PRIx16,
		check->notification.vector);
	fwts_log_info_verbatim(fw, "    Sw. to Polling Value:   0x%4.4" PRIx16,
		check->notification.switch_to_polling_threshold_value);
	fwts_log_info_verbatim(fw, "    Sw. to Polling Window:  0x%4.4" PRIx16,
		check->notification.switch_to_polling_threshold_window);
	fwts_log_info_verbatim(fw, "    Error: Thresh. Value:   0x%4.4" PRIx16,
		check->notification.error_threshold_value);
	fwts_log_info_verbatim(fw, "    Error: Thresh. Window:  0x%4.4" PRIx16,
		check->notification.error_threshold_window);
	fwts_log_info_verbatim(fw, "  Number of Hardware Banks: 0x%8.8" PRIx32, check->number_of_hardware_banks);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, check->reserved2[0]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, check->reserved2[1]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, check->reserved2[2]);
	fwts_log_nl(fw);

	if (check->flags & ~0x5) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"HESTIA32CorrectedMachineCheckFlagsReserved",
			"HEST IA-32 Architecture MC Flags Reserved bits "
			"[1] & [3:7] must be zero, instead got 0x%" PRIx8,
			check->flags);
		*passed = false;
	} else if (check->flags == 0x5) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTIA32CorrectedMachineCheckBadFlags",
			"HEST IA-32 Architecture MC Flags bits [0] & [2] "
			"are mutually exclusive, instead got 0x%" PRIx8,
			check->flags);
		*passed = false;
	}

	if (check->notification.type > 11) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidHardwareErrorNotificationType",
			"HEST Hardware Error Notification Type is "
			"an invalid reserved value of %2.2" PRIu8 ","
			"expecting value 0 to 11",
			check->notification.type);
	}

	for (i = 0; i < check->number_of_hardware_banks; i++) {
		fwts_acpi_table_hest_machine_check_bank *bank = &check->bank[i];

		fwts_log_info_verbatim(fw, "  HEST IA-32 Architecture Machine Check Bank %zd", i);
		fwts_log_info_verbatim(fw, "    Bank Number:            0x%2.2" PRIx8, bank->bank_number);
		fwts_log_info_verbatim(fw, "    Clear Status On Init.:  0x%2.2" PRIx8, bank->clear_status_on_initialization);
		fwts_log_info_verbatim(fw, "    Status Data Format:     0x%2.2" PRIx8, bank->status_data_format);
		fwts_log_info_verbatim(fw, "    Reserved:               0x%2.2" PRIx8, bank->reserved);
		fwts_log_info_verbatim(fw, "    Control Reg. MSR Addr:  0x%8.8" PRIx32, bank->control_register_msr_address);
		fwts_log_info_verbatim(fw, "    Control Init Data:      0x%16.16" PRIx64, bank->control_init_data);
		fwts_log_info_verbatim(fw, "    Status Reg. MSR Addr:   0x%8.8" PRIx32, bank->status_register_msr_address);
		fwts_log_info_verbatim(fw, "    Addr Reg. MSR Addr:     0x%8.8" PRIx32, bank->address_register_msr_address);
		fwts_log_info_verbatim(fw, "    Misc Reg. MSR Addr:     0x%8.8" PRIx32, bank->misc_register_msr_address);
		fwts_log_nl(fw);

		if (bank->clear_status_on_initialization > 1) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"HESTIA32InvalidClearStatusOnInit",
				"HEST IA-32 Architecture Machine Check Bank Clear Status On Initialization "
				"has value 0x%" PRIx8 " and only valid values are "
				"0x00 (Clear) or 0x01 (Don't Clear)",
				bank->clear_status_on_initialization);
		}
		if (bank->status_data_format > 2) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"HESTIA32InvalidBankStatusDataFormat",
				"HEST IA-32 Architecture Machine Check Bank Status Data Format "
				"has value 0x%" PRIx8 " and only valid values are "
				"0x00 (IA-32 MCA), 0x01 (Intel 64 MCA) or "
				"0x02 (AMD64 MCA).",
				bank->status_data_format);
		}
	}

	*length -= total_size;
	*data += total_size;
}

/*
 *  ACPI Section 18.3.2.2.1, IA-32 Architecture Non-Maskable Interrupt
 *    - note, this should be a higher section number, the ACPI 6.0
 *	specification seems to have numbered this incorrectly.
 */
static void hest_check_acpi_table_hest_nmi_error(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	fwts_acpi_table_hest_nmi_error *err =
		(fwts_acpi_table_hest_nmi_error *)*data;

	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_nmi_error)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTIA-32ArchitectureNmiTooShort",
			"HEST IA-32 Architecture Non-Mastable Interrupt "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_nmi_error), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST IA-32 Architecture Non-Maskable Interrupt:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, err->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, err->source_id);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, err->reserved1);
	fwts_log_info_verbatim(fw, "  Number of Records:        0x%8.8" PRIx32, err->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max Sections Per Record:  0x%8.8" PRIx32, err->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Max Raw Data Length:      0x%8.8" PRIx32, err->max_raw_data_length);
	fwts_log_nl(fw);

	if (err->reserved1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"HESTInvalidRecordsToPreallocate",
			"HEST IA-32 Architecture NMI Reserved field "
			"at offset 4 must be zero, instead got 0x%" PRIx16,
			err->reserved1);
	}
	if (err->number_of_records_to_preallocate < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidRecordsToPreallocate",
			"HEST IA-32 Architecture NMI Number of Records "
			"to Preallocate is 0x%" PRIx16 " and must be "
			"more than zero.",
			err->number_of_records_to_preallocate);
	}
	if (err->max_sections_per_record < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidMaxSectionsPerRecord",
			"HEST A-32 Architecture NMI Max Sections Per "
			"Record is 0x%" PRIx16 " and must be "
			"more than zero.",
			err->max_sections_per_record);
	}

	*length -= sizeof(fwts_acpi_table_hest_nmi_error);
	*data += sizeof(fwts_acpi_table_hest_nmi_error);
}

/*
 *  ACPI Section 18.3.2.3 PCI Express Root Port AER Structure
 */
static void hest_check_pci_express_root_port_aer(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	fwts_acpi_table_hest_pci_express_root_port_aer *aer =
		(fwts_acpi_table_hest_pci_express_root_port_aer *)*data;

	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_pci_express_root_port_aer)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTPciExpressRootPortAerTooShort",
			"HEST PCI Express Root Port AER "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_pci_express_root_port_aer), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST PCI Express Root Port AER:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, aer->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, aer->source_id);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, aer->reserved1);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, aer->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, aer->enabled);
	fwts_log_info_verbatim(fw, "  Number of Records:        0x%8.8" PRIx32, aer->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max Sections Per Record:  0x%8.8" PRIx32, aer->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Bus:                      0x%8.8" PRIx32, aer->bus);
	fwts_log_info_verbatim(fw, "  Device:                   0x%4.4" PRIx16, aer->device);
	fwts_log_info_verbatim(fw, "  Function:                 0x%4.4" PRIx16, aer->function);
	fwts_log_info_verbatim(fw, "  Device Control:           0x%4.4" PRIx16, aer->device_control);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, aer->reserved2);
	fwts_log_info_verbatim(fw, "  Uncorrectable Mask:       0x%8.8" PRIx32, aer->uncorrectable_error_mask);
	fwts_log_info_verbatim(fw, "  Uncorrectable Severity:   0x%8.8" PRIx32, aer->uncorrectable_error_severity);
	fwts_log_info_verbatim(fw, "  Correctable Error Mask:   0x%8.8" PRIx32, aer->correctable_error_mask);
	fwts_log_info_verbatim(fw, "  Advanced Capabilities:    0x%8.8" PRIx32, aer->advanced_error_capabilities_and_control);
	fwts_log_info_verbatim(fw, "  Root Error Command:       0x%8.8" PRIx32, aer->root_error_command);
	fwts_log_nl(fw);

	fwts_acpi_reserved_zero_check(fw, "HEST", "PCI Express Root Port Reserved1", aer->reserved1, sizeof(aer->reserved1), passed);
	fwts_acpi_reserved_bits_check(fw, "HEST", "PCI Express Root Port Flags", aer->flags, sizeof(aer->flags), 2, 7, passed);
	fwts_acpi_reserved_zero_check(fw, "HEST", "PCI Express Root Port Reserved2", aer->reserved2, sizeof(aer->reserved2), passed);

	if (aer->number_of_records_to_preallocate < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidRecordsToPreallocate",
			"HEST PCI Express Root Port Number of Records "
			"to Preallocate is 0x%" PRIx16 " and must be "
			"more than zero.",
			aer->number_of_records_to_preallocate);
	}
	if (aer->max_sections_per_record < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidMaxSectionsPerRecord",
			"HEST PCI Express Root Port Max Sections Per "
			"Record is 0x%" PRIx16 " and must be "
			"more than zero.",
			aer->max_sections_per_record);
	}
	*length -= sizeof(fwts_acpi_table_hest_pci_express_root_port_aer);
	*data += sizeof(fwts_acpi_table_hest_pci_express_root_port_aer);
}

/*
 *  ACPI Section 18.3.2.4 PCI Express Device AER Structure
 */
static void hest_check_pci_express_device_aer(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	fwts_acpi_table_hest_pci_express_device_aer *aer =
		(fwts_acpi_table_hest_pci_express_device_aer *)*data;

	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_pci_express_device_aer)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTPciExpressDeviceAerTooShort",
			"HEST PCI Express Device AER "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_pci_express_device_aer), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST PCI Express Device AER:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, aer->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, aer->source_id);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, aer->reserved1);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, aer->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, aer->enabled);
	fwts_log_info_verbatim(fw, "  Number of Records:        0x%8.8" PRIx32, aer->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max Sections Per Record:  0x%8.8" PRIx32, aer->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Bus:                      0x%8.8" PRIx32, aer->bus);
	fwts_log_info_verbatim(fw, "  Device:                   0x%4.4" PRIx16, aer->device);
	fwts_log_info_verbatim(fw, "  Function:                 0x%4.4" PRIx16, aer->function);
	fwts_log_info_verbatim(fw, "  Device Control:           0x%4.4" PRIx16, aer->device_control);
	fwts_log_info_verbatim(fw, "  Uncorrectable Mask:       0x%8.8" PRIx32, aer->uncorrectable_error_mask);
	fwts_log_info_verbatim(fw, "  Uncorrectable Severity:   0x%8.8" PRIx32, aer->uncorrectable_error_severity);
	fwts_log_info_verbatim(fw, "  Correctable Error Mask:   0x%8.8" PRIx32, aer->correctable_error_mask);
	fwts_log_info_verbatim(fw, "  Advanced Capabilities:    0x%8.8" PRIx32, aer->advanced_error_capabilities_and_control);
	fwts_log_nl(fw);

	fwts_acpi_reserved_zero_check(fw, "HEST", "PCI Express Device Reserved1", aer->reserved1, sizeof(aer->reserved1), passed);
	fwts_acpi_reserved_bits_check(fw, "HEST", "PCI Express Device Flags", aer->flags, sizeof(aer->flags), 2, 7, passed);
	fwts_acpi_reserved_zero_check(fw, "HEST", "PCI Express Device Reserved2", aer->reserved2, sizeof(aer->reserved2), passed);

	if (aer->number_of_records_to_preallocate < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidRecordsToPreallocate",
			"HEST PCI Express Device Number of Records "
			"to Preallocate is 0x%" PRIx16 " and must be "
			"more than zero.",
			aer->number_of_records_to_preallocate);
	}
	if (aer->max_sections_per_record < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidMaxSectionsPerRecord",
			"HEST PCI Express Device Max Sections Per "
			"Record is 0x%" PRIx16 " and must be "
			"more than zero.",
			aer->max_sections_per_record);
	}
	*length -= sizeof(fwts_acpi_table_hest_pci_express_device_aer);
	*data += sizeof(fwts_acpi_table_hest_pci_express_device_aer);
}

/*
 *  ACPI Section 18.3.2.5 PCI Express/PCI-X Bridge AER Structure
 */
static void hest_heck_pci_express_bridge_aer(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	fwts_acpi_table_hest_pci_express_bridge_aer *aer =
		(fwts_acpi_table_hest_pci_express_bridge_aer *)*data;

	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_pci_express_bridge_aer)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTPciExpressBridgeAerTooShort",
			"HEST PCI Express Bridge AER "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_pci_express_bridge_aer), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST PCI Express Bridge AER:");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, aer->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, aer->source_id);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, aer->reserved1);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, aer->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, aer->enabled);
	fwts_log_info_verbatim(fw, "  Number of Records:        0x%8.8" PRIx32, aer->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max Sections Per Record:  0x%8.8" PRIx32, aer->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Bus:                      0x%8.8" PRIx32, aer->bus);
	fwts_log_info_verbatim(fw, "  Device:                   0x%4.4" PRIx16, aer->device);
	fwts_log_info_verbatim(fw, "  Function:                 0x%4.4" PRIx16, aer->function);
	fwts_log_info_verbatim(fw, "  Device Control:           0x%4.4" PRIx16, aer->device_control);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, aer->reserved2);
	fwts_log_info_verbatim(fw, "  Uncorrectable Mask:       0x%8.8" PRIx32, aer->uncorrectable_error_mask);
	fwts_log_info_verbatim(fw, "  Uncorrectable Severity:   0x%8.8" PRIx32, aer->uncorrectable_error_severity);
	fwts_log_info_verbatim(fw, "  Correctable Mask:         0x%8.8" PRIx32, aer->correctable_error_mask);
	fwts_log_info_verbatim(fw, "  Advanced Capabilities:    0x%8.8" PRIx32, aer->advanced_error_capabilities_and_control);
	fwts_log_info_verbatim(fw, "  2nd Uncorrectable Mask:   0x%8.8" PRIx32, aer->secondary_uncorrectable_error_mask);
	fwts_log_info_verbatim(fw, "  2nd Uncurrectable Svrity: 0x%8.8" PRIx32, aer->secondary_uncorrectable_error_severity);
	fwts_log_info_verbatim(fw, "  2nd Advanced Capabilities:0x%8.8" PRIx32, aer->secondary_advanced_error_capabilities_and_control);
	fwts_log_nl(fw);

	fwts_acpi_reserved_zero_check(fw, "HEST", "PCI Express Bridge Reserved1", aer->reserved1, sizeof(aer->reserved1), passed);
	fwts_acpi_reserved_bits_check(fw, "HEST", "PCI Express Bridge Flags", aer->flags, sizeof(aer->flags), 2, 7, passed);
	fwts_acpi_reserved_zero_check(fw, "HEST", "PCI Express Bridge Reserved2", aer->reserved2, sizeof(aer->reserved2), passed);

	if (aer->number_of_records_to_preallocate < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidRecordsToPreallocate",
			"HEST PCI Express Bridge Number of Records "
			"to Preallocate is 0x%" PRIx16 " and must be "
			"more than zero.",
			aer->number_of_records_to_preallocate);
	}
	if (aer->max_sections_per_record < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidMaxSectionsPerRecord",
			"HEST PCI Express Brdige Max Sections Per "
			"Record is 0x%" PRIx16 " and must be "
			"more than zero.",
			aer->max_sections_per_record);
	}
	*length -= sizeof(fwts_acpi_table_hest_pci_express_bridge_aer);
	*data += sizeof(fwts_acpi_table_hest_pci_express_bridge_aer);
}


/*
 *  ACPI Section 18.3.2.6 Generic Error Source
 */
static void hest_check_generic_error_source(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	fwts_acpi_table_hest_generic_hardware_error_source *source =
		(fwts_acpi_table_hest_generic_hardware_error_source *)*data;

	/* Enough data for an empty machine check exceptions structure? */
	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_generic_hardware_error_source)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTGenericHardwareErrorSourceTooShort",
			"HEST Generic Hardware Error Source Too Short "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_generic_hardware_error_source), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST Generic Hardware Error Source");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, source->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, source->source_id);
	fwts_log_info_verbatim(fw, "  Related Source ID:        0x%4.4" PRIx16, source->related_source_id);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, source->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, source->enabled);
	fwts_log_info_verbatim(fw, "  Num. Records. Prealloc.:  0x%8.8" PRIx32, source->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max. Sections Per Rec.:   0x%8.8" PRIx32, source->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Max. Raw Data Length:     0x%8.8" PRIx32, source->max_raw_data_length);

	fwts_log_info_verbatim(fw, "  Error Status Address:");
	fwts_log_info_verbatim(fw, "    Address Space ID:       0x%2.2" PRIx8,
		source->error_status_address.address_space_id);
	fwts_log_info_verbatim(fw, "    Register Bit Width      0x%2.2" PRIx8,
		source->error_status_address.register_bit_width);
	fwts_log_info_verbatim(fw, "    Register Bit Offset     0x%2.2" PRIx8,
		source->error_status_address.register_bit_offset);
	fwts_log_info_verbatim(fw, "    Access Size             0x%2.2" PRIx8,
		source->error_status_address.access_width);
	fwts_log_info_verbatim(fw, "    Address                 0x%16.16" PRIx64,
			source->error_status_address.address);
	fwts_log_info_verbatim(fw, "  Hardware Error Notification:");
	fwts_log_info_verbatim(fw, "    Type:                   0x%2.2" PRIx8, source->notification.type);
	fwts_log_info_verbatim(fw, "    Length:                 0x%2.2" PRIx8, source->notification.length);
	fwts_log_info_verbatim(fw, "    Config. Write. Enable:  0x%4.4" PRIx16,
		source->notification.configuration_write_enable);
	fwts_log_info_verbatim(fw, "    Poll Interval:          0x%4.4" PRIx16,
		source->notification.poll_interval);
	fwts_log_info_verbatim(fw, "    Interrupt Vector:       0x%4.4" PRIx16,
		source->notification.vector);
	fwts_log_info_verbatim(fw, "    Sw. to Polling Value:   0x%4.4" PRIx16,
		source->notification.switch_to_polling_threshold_value);
	fwts_log_info_verbatim(fw, "    Sw. to Polling Window:  0x%4.4" PRIx16,
		source->notification.switch_to_polling_threshold_window);
	fwts_log_info_verbatim(fw, "    Error: Thresh. Value:   0x%4.4" PRIx16,
		source->notification.error_threshold_value);
	fwts_log_info_verbatim(fw, "    Error: Thresh. Window:  0x%4.4" PRIx16,
		source->notification.error_threshold_window);
	fwts_log_info_verbatim(fw, "  Error Status Blk. Length: 0x%8.8" PRIx32, source->error_status_block_length);
	fwts_log_nl(fw);

	if (source->number_of_records_to_preallocate < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidRecordsToPreallocate",
			"HEST Hardware Error Source Number of Records "
			"to Preallocate is 0x%" PRIx16 " and must be "
			"more than zero.",
			source->number_of_records_to_preallocate);
	}
	if (source->max_sections_per_record < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidMaxSectionsPerRecord",
			"HEST Hardware Error Source Max Sections Per "
			"Record is 0x%" PRIx16 " and must be "
			"more than zero.",
			source->max_sections_per_record);
	}
	if (source->notification.type > 11) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidHardwareErrorNotificationType",
			"HEST Hardware Error Notification Type is "
			"an invalid reserved value of %2.2" PRIu8 ","
			"expecting value 0 to 11",
			source->notification.type);
	}

	fwts_acpi_reserved_bits_check(fw, "HEST", "HEST Configuration Write Enabled",
		source->notification.configuration_write_enable,
		sizeof(source->notification.configuration_write_enable), 6, 31, passed);

	*length -= sizeof(fwts_acpi_table_hest_generic_hardware_error_source);
	*data += sizeof(fwts_acpi_table_hest_generic_hardware_error_source);
}

/*
 *  ACPI Section 18.3.2.8 Generic Error Source version 2
 */
static void hest_check_generic_error_source_v2(
	fwts_framework *fw,
	ssize_t *length,
	uint8_t **data,
	bool *passed)
{
	fwts_acpi_table_hest_generic_hardware_error_source_v2 *source =
		(fwts_acpi_table_hest_generic_hardware_error_source_v2 *)*data;

	/* Enough data for an empty machine check exceptions structure? */
	if (*length < (ssize_t)sizeof(fwts_acpi_table_hest_generic_hardware_error_source_v2)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTGenericHardwareErrorSourceTooShort",
			"HEST Generic Hardware Error Source Too Short "
			"too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_hest_generic_hardware_error_source_v2), *length);
		*passed = false;
		*length = 0;	/* Forces an early abort */
		return;
	}

	fwts_log_info_verbatim(fw, "HEST Generic Hardware Error Source version 2");
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, source->type);
	fwts_log_info_verbatim(fw, "  Source ID:                0x%4.4" PRIx16, source->source_id);
	fwts_log_info_verbatim(fw, "  Related Source ID:        0x%4.4" PRIx16, source->related_source_id);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%2.2" PRIx8, source->flags);
	fwts_log_info_verbatim(fw, "  Enabled:                  0x%2.2" PRIx8, source->enabled);
	fwts_log_info_verbatim(fw, "  Num. Records. Prealloc.:  0x%8.8" PRIx32, source->number_of_records_to_preallocate);
	fwts_log_info_verbatim(fw, "  Max. Sections Per Rec.:   0x%8.8" PRIx32, source->max_sections_per_record);
	fwts_log_info_verbatim(fw, "  Max. Raw Data Length:     0x%8.8" PRIx32, source->max_raw_data_length);

        fwts_log_info_verbatim(fw, "  Error Status Address:");
        fwts_log_info_verbatim(fw, "    Address Space ID:       0x%2.2" PRIx8,
		source->error_status_address.address_space_id);
        fwts_log_info_verbatim(fw, "    Register Bit Width      0x%2.2" PRIx8,
		source->error_status_address.register_bit_width);
        fwts_log_info_verbatim(fw, "    Register Bit Offset     0x%2.2" PRIx8,
		source->error_status_address.register_bit_offset);
        fwts_log_info_verbatim(fw, "    Access Size             0x%2.2" PRIx8,
		source->error_status_address.access_width);
        fwts_log_info_verbatim(fw, "    Address                 0x%16.16" PRIx64,
			source->error_status_address.address);
        fwts_log_info_verbatim(fw, "  Hardware Error Notification:");
        fwts_log_info_verbatim(fw, "    Type:                   0x%2.2" PRIx8, source->notification.type);
        fwts_log_info_verbatim(fw, "    Length:                 0x%2.2" PRIx8, source->notification.length);
        fwts_log_info_verbatim(fw, "    Config. Write. Enable:  0x%4.4" PRIx16,
		source->notification.configuration_write_enable);
        fwts_log_info_verbatim(fw, "    Poll Interval:          0x%4.4" PRIx16,
		source->notification.poll_interval);
        fwts_log_info_verbatim(fw, "    Interrupt Vector:       0x%4.4" PRIx16,
		source->notification.vector);
        fwts_log_info_verbatim(fw, "    Sw. to Polling Value:   0x%4.4" PRIx16,
		source->notification.switch_to_polling_threshold_value);
        fwts_log_info_verbatim(fw, "    Sw. to Polling Window:  0x%4.4" PRIx16,
		source->notification.switch_to_polling_threshold_window);
        fwts_log_info_verbatim(fw, "    Error: Thresh. Value:   0x%4.4" PRIx16,
		source->notification.error_threshold_value);
        fwts_log_info_verbatim(fw, "    Error: Thresh. Window:  0x%4.4" PRIx16,
		source->notification.error_threshold_window);
	fwts_log_info_verbatim(fw, "  Error Status Blk. Length: 0x%8.8" PRIx32, source->error_status_block_length);
        fwts_log_info_verbatim(fw, "  Read Ack Register:");
        fwts_log_info_verbatim(fw, "    Address Space ID:       0x%2.2" PRIx8,
		source->read_ack_register.address_space_id);
        fwts_log_info_verbatim(fw, "    Register Bit Width      0x%2.2" PRIx8,
		source->read_ack_register.register_bit_width);
        fwts_log_info_verbatim(fw, "    Register Bit Offset     0x%2.2" PRIx8,
		source->read_ack_register.register_bit_offset);
        fwts_log_info_verbatim(fw, "    Access Size             0x%2.2" PRIx8,
		source->read_ack_register.access_width);
        fwts_log_info_verbatim(fw, "    Address                 0x%16.16" PRIx64,
			source->read_ack_register.address);
	fwts_log_info_verbatim(fw, "  Read Ack Preserve:        0x%16.16" PRIx64, source->read_ack_preserve);
	fwts_log_info_verbatim(fw, "  Read Ack Write:           0x%16.16" PRIx64, source->read_ack_write);
	fwts_log_nl(fw);

	if (source->number_of_records_to_preallocate < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidRecordsToPreallocate",
			"HEST Hardware Error Source Number of Records "
			"to Preallocate is 0x%" PRIx16 " and must be "
			"more than zero.",
			source->number_of_records_to_preallocate);
	}
	if (source->max_sections_per_record < 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidMaxSectionsPerRecord",
			"HEST Hardware Error Source Max Sections Per "
			"Record is 0x%" PRIx16 " and must be "
			"more than zero.",
			source->max_sections_per_record);
	}
	if (source->notification.type > 11) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTInvalidHardwareErrorNotificationType",
			"HEST Hardware Error Notification Type is "
			"an invalid reserved value of %2.2" PRIu8 ","
			"expecting value 0 to 11",
			source->notification.type);
	}

	fwts_acpi_reserved_bits_check(fw, "HEST", "HEST Configuration Write Enabled",
		source->notification.configuration_write_enable,
		sizeof(source->notification.configuration_write_enable), 6, 31, passed);

	*length -= sizeof(fwts_acpi_table_hest_generic_hardware_error_source_v2);
	*data += sizeof(fwts_acpi_table_hest_generic_hardware_error_source_v2);
}

/*
 *  ACPI Section 18.3.2.8
 */

/*
 *  HEST Hardware Error Source Table test
 *     ACPI section 18.3.2 "ACPI Error Source"
 */
static int hest_test1(fwts_framework *fw)
{
	bool passed = true;
	fwts_acpi_table_hest *hest = (fwts_acpi_table_hest *)table->data;
        uint8_t *data = (uint8_t *)table->data;
        ssize_t length = (ssize_t)hest->header.length;
	uint32_t hest_type_00_count = 0,
		 hest_type_01_count = 0,
		 hest_type_02_count = 0,
		 hest_type_11_count = 0;

	if (!fwts_acpi_table_length_check(fw, "HEST", table->length, sizeof(fwts_acpi_table_hest))) {
		passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "HEST Hardware Error Source Table test");
	fwts_log_info_verbatim(fw, "  Error Source Count:       0x%2.2" PRIx8, hest->error_source_count);
	fwts_log_nl(fw);

        data += sizeof(fwts_acpi_table_hest);
        length -= sizeof(fwts_acpi_table_hest);

	while (length > 0) {
		uint16_t *type = (uint16_t *)data;

		switch (*type) {
		case 0:
			hest_check_ia32_arch_machine_check_exception(fw, &length, &data, &passed);
			hest_type_00_count++;
			break;
		case 1:
			hest_check_ia32_arch_corrected_machine_check(fw, &length, &data, &passed);
			hest_type_01_count++;
			break;
		case 2:
			hest_check_acpi_table_hest_nmi_error(fw, &length, &data, &passed);
			hest_type_02_count++;
			break;
		case 6:
			hest_check_pci_express_root_port_aer(fw, &length, &data, &passed);
			break;
		case 7:
			hest_check_pci_express_device_aer(fw, &length, &data, &passed);
			break;
		case 8:
			hest_heck_pci_express_bridge_aer(fw, &length, &data, &passed);
			break;
		case 9:
			hest_check_generic_error_source(fw, &length, &data, &passed);
			break;
		case 10:
			hest_check_generic_error_source_v2(fw, &length, &data, &passed);
			break;
		case 11:
			/* the structure of type 11 is the same as type 1 */
			hest_check_ia32_arch_corrected_machine_check(fw, &length, &data, &passed);
			hest_type_11_count++;
			break;
		default:
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"HESTInvalidType",
				"HEST Type 0x%4.4" PRIx16 " is invalid, aborting check",
				*type);
			passed = false;
			length = 0;
			break;
		}
	}
	if (hest_type_00_count > 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTTooManyIA32ArchMachineCheckExceptions",
			"HEST Contained %" PRIu32 " IA32 Architecture "
			"Machine Check Exception Entries, maximum allowed "
			"per HEST is just 1.",
			hest_type_00_count);
	}
	if (hest_type_01_count > 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTTooManyIA32CorrectedMachineChecks",
			"HEST Contained %" PRIu32 " IA32 Architecture "
			"Corrected Machine Check Exception Entries, maximum allowed "
			"per HEST is just 1.",
			hest_type_01_count);
	}
	if (hest_type_02_count > 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"HESTTooManyNmiErrors",
			"HEST Contained %" PRIu32 " NMI Error Entries, "
			"maximum allowed per HEST is just 1.",
			hest_type_02_count);
	}

	if (passed)
		fwts_passed(fw, "No issues found in HEST table.");

done:
	return FWTS_OK;
}

static fwts_framework_minor_test hest_tests[] = {
	{ hest_test1, "HEST Hardware Error Source Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops hest_ops = {
	.description = "HEST Hardware Error Source Table test.",
	.init        = hest_init,
	.minor_tests = hest_tests
};

FWTS_REGISTER("hest", &hest_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
