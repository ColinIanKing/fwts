/*
 * Copyright (C) 2015 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static fwts_acpi_table_info *table;

static int madt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "APIC", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI MADT (APIC) table does not exist, skipping test");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 *  MADT Multiple APIC Description Table
 */
static int madt_test1(fwts_framework *fw)
{
	bool passed = true;

	fwts_acpi_table_madt *madt = (fwts_acpi_table_madt*)table->data;
	fwts_list msi_frame_ids;
	const uint8_t *data = table->data;
	ssize_t length = table->length;
	int i = 0;

	fwts_list_init(&msi_frame_ids);

	if (madt->flags & 0xfffffffe) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"MADTFlagsNonZero",
			"MADT flags field, bits 1..31 are reserved and "
			"should be zero, but are set as: %" PRIx32 ".\n",
			madt->flags);
	}

	data += sizeof(fwts_acpi_table_madt);
	length -= sizeof(fwts_acpi_table_madt);

	while (length > (ssize_t)sizeof(fwts_acpi_madt_sub_table_header)) {
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;
		ssize_t skip = 0;
		i++;

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);

		switch (hdr->type) {
		case FWTS_ACPI_MADT_LOCAL_APIC: {
				fwts_acpi_madt_processor_local_apic *lapic = (fwts_acpi_madt_processor_local_apic *)data;
				if (lapic->flags & 0xfffffffe) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTAPICFlagsNonZero",
						"MADT Local APIC flags field, bits 1..31 are reserved and "
						"should be zero, but are set as: %" PRIx32 ".",
						lapic->flags);
				}
				skip = sizeof(fwts_acpi_madt_processor_local_apic);
			}
			break;
		case FWTS_ACPI_MADT_IO_APIC: {
				fwts_acpi_madt_io_apic *ioapic = (fwts_acpi_madt_io_apic*)data;
				if (ioapic->io_apic_phys_address == 0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTIOAPICAddrZero",
						"MADT IO APIC address is zero, appears not to be defined.");
				}
				skip = sizeof(fwts_acpi_madt_io_apic);
			}
			break;
		case FWTS_ACPI_MADT_INTERRUPT_OVERRIDE: {
				fwts_acpi_madt_interrupt_override *int_override = (fwts_acpi_madt_interrupt_override*)data;
				if (int_override->bus != 0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTIRQSrcISA",
						"MADT Interrupt Source Override Bus should be 0 for ISA bus.");
				}
				if (int_override->flags & 0xfffffff0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTIRQSrcFlags",
						"MADT Interrupt Source Override flags, bits 4..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						int_override->flags);
				}
				skip = sizeof(fwts_acpi_madt_interrupt_override);
			}
			break;
		case FWTS_ACPI_MADT_NMI_SOURCE: {
				fwts_acpi_madt_nmi *nmi = (fwts_acpi_madt_nmi*)data;
				if (nmi->flags & 0xfffffff0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTNMISrcFlags",
						"MADT Non-Maskable Interrupt Source, flags, bits 4..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						nmi->flags);
				}
				skip = sizeof(fwts_acpi_madt_nmi);
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_APIC_NMI: {
				fwts_acpi_madt_local_apic_nmi *nmi = (fwts_acpi_madt_local_apic_nmi*)data;
				if (nmi->flags & 0xfffffff0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTLAPICNMIFlags",
						"MADT Local APIC NMI flags, bits 4..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						nmi->flags);
				}
				skip = sizeof(fwts_acpi_madt_local_apic_nmi);
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_APIC_OVERRIDE:
			skip = sizeof(fwts_acpi_madt_local_apic_addr_override);
			break;
		case FWTS_ACPI_MADT_IO_SAPIC: {
				fwts_acpi_madt_io_sapic *sapic = (fwts_acpi_madt_io_sapic*)data;
				if (sapic->address == 0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADIOSPAICAddrZero",
						"MADT I/O SAPIC address is zero, appears not to be defined.");
				}
				skip = sizeof(fwts_acpi_madt_io_sapic);
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_SAPIC: {
				fwts_acpi_madt_local_sapic *local_sapic = (fwts_acpi_madt_local_sapic*)data;
				skip = sizeof(fwts_acpi_madt_local_sapic) + strlen(local_sapic->uid_string) + 1;
			}
			break;
		case FWTS_ACPI_MADT_INTERRUPT_SOURCE: {
				fwts_acpi_madt_platform_int_source *src = (fwts_acpi_madt_platform_int_source*)data;
				if (src->flags & 0xfffffff0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTPlatIRQSrcFlags",
						"MADT Platform Interrupt Source, flags, bits 4..31 are "
						"reserved and should be zero, but are set as: %" PRIx32 ".",
						src->flags);
				}
				if (src->type > 3) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTPlatIRQType",
						"MADT Platform Interrupt Source, type field is %" PRIu8
						", should be 1..3.", src->type);
				}
				if (src->io_sapic_vector == 0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTPlatIRQIOSAPICVector",
						"MADT Platform Interrupt Source, IO SAPIC Vector is "
						"zero, appears not to be defined.");
				}
				if (src->pis_flags & 0xfffffffe) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTPlatIRQSrcFlagsNonZero",
						"MADT Platform Interrupt Source, Platform Interrupt Source flag "
						"bits 1..31 are reserved and should be zero, but are "
						"set as: %" PRIx32 ".", src->pis_flags);
				}
				skip = (sizeof(fwts_acpi_madt_platform_int_source));
			}
			break;
		case FWTS_ACPI_MADT_LOCAL_X2APIC:
			skip = (sizeof(fwts_acpi_madt_local_x2apic));
			break;
		case FWTS_ACPI_MADT_LOCAL_X2APIC_NMI: {
				fwts_acpi_madt_local_x2apic_nmi *nmi = (fwts_acpi_madt_local_x2apic_nmi*)data;
				if (nmi->flags & 0xfffffff0) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTLAPICX2APICNMIFlags",
						"MADT Local x2APIC NMI, flags, bits 4..31 are reserved and "
						"should be zero, but are set as: %" PRIx32 ".",
						nmi->flags);
				}
				skip = (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			}
			break;
		case FWTS_ACPI_MADT_GIC_C_CPU_INTERFACE: {
				fwts_acpi_madt_gic *gic = (fwts_acpi_madt_gic*)data;

				if (gic->reserved) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"MADTGICCReservedNonZero",
						"MADT GIC C Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gic->reserved);
				}
				if (gic->flags & 0xfffffffc) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTGICFLags",
						"MADT GIC, flags, bits 2..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						gic->flags & 0xfffffffc);
				}
				skip = sizeof(fwts_acpi_madt_gic);
			}
			break;
		case FWTS_ACPI_MADT_GIC_D_GOC_DISTRIBUTOR: {
				fwts_acpi_madt_gicd *gicd = (fwts_acpi_madt_gicd*)data;

				if (gicd->reserved) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"MADTGICDReservedNonZero",
						"MADT GIC Distributor Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gicd->reserved);
				}
				if (gicd->reserved2) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"MADTGICDReserved2NonZero",
						"MADT GIC Distributor Structure second reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gicd->reserved2);
				}
			}
			skip = sizeof(fwts_acpi_madt_gicd);
			break;
		case FWTS_ACPI_MADT_GIC_V2M_MSI_FRAME: {
				fwts_acpi_madt_gic_msi *gic_msi = (fwts_acpi_madt_gic_msi*)data;
				fwts_list_link *item;

				if (gic_msi->reserved) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"MADTGICMSIReservedNonZero",
						"MADT GIC MSI Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gic_msi->reserved);
				}
				if (gic_msi->flags & 0xfffffffe) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"MADTGICMSIFLags",
						"MADT GIC MSI Frame, flags, bits 1..31 are reserved "
						"and should be zero, but are set as: %" PRIx32 ".",
						gic_msi->flags);
				}

				/* Check against previously IDs to see if unique */
				fwts_list_foreach(item, &msi_frame_ids) {
					uint32_t *frame_id = fwts_list_data(uint32_t *, item);

					/* Frame ID must be unique according to the spec */
					if (*frame_id == gic_msi->frame_id) {
						passed = false;
						fwts_failed(fw, LOG_LEVEL_MEDIUM,
							"MADTGICMSINonUniqueFrameId",
							"MADT GIC MSI Frame ID 0x%" PRIx32 " is not unique "
							"and has already be defined in a previous GIC MSI "
							"Frame.", gic_msi->frame_id);
					} else {
						fwts_list_append(&msi_frame_ids, &(gic_msi->frame_id));
					}
				}
			}
			skip = sizeof(fwts_acpi_madt_gic_msi);
			break;
		case FWTS_ACPI_MADT_GIC_R_REDISTRIBUTOR: {
				fwts_acpi_madt_gicr *gicr = (fwts_acpi_madt_gicr*)data;

				if (gicr->reserved) {
					passed = false;
					fwts_failed(fw, LOG_LEVEL_LOW,
						"MADTGICRReservedNonZero",
						"MADT GIC Redistributor Structure reserved field should be zero, "
						"instead got 0x%" PRIx32 ".", gicr->reserved);
				}
			}
			skip = sizeof(fwts_acpi_madt_gicr);
			break;
		default:
			skip = 0;
			break;
		}
		data   += skip;
		length -= skip;
	}
	fwts_list_free_items(&msi_frame_ids, NULL);

	if (passed)
		fwts_passed(fw, "No issues found in MADT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test madt_tests[] = {
	{ madt_test1, "MADT Multiple APIC Description Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops madt_ops = {
	.description = "MADT Multiple APIC Description Table test.",
	.init        = madt_init,
	.minor_tests = madt_tests
};

FWTS_REGISTER("madt", &madt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV | FWTS_FLAG_TEST_ACPI)
