/*
 * Copyright (C) 2011 Canonical
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

static const char *mpdump_inttype[] = {
	"INT",
	"NMI",
	"SMI",
	"ExtInt"
};

static const char *mpdump_po[] = {
	"Conforms to bus specification",
	"Active High",
	"Reserved",
	"Active Low"
};

static const char *mpdump_po_short[] = {
	"Con",
	"Hi ",
	"Rsv",
	"Lo "
};

static const char *mpdump_el[] = {
	"Conforms to bus specification",
	"Edge-Triggered",
	"Reserved",
	"Level-Triggered"
};

static const char *mpdump_el_short[] = {
	"Con",
	"Edg",
	"Rsv",
	"Lvl"
};

static const char *mpdump_sys_addr_type[] = {
	"I/O",
	"Memory",
	"Prefetch"
};

static const char *mpdump_yes_no[] = {
	"No",
	"Yes"
};

static void mpdump_dump_header(fwts_framework *fw, fwts_mp_config_table_header *header, uint32_t phys_addr)
{
	fwts_log_info_verbatum(fw,"MultiProcessor Header: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw,"  Signature:          %4.4s\n", header->signature);
	fwts_log_info_verbatum(fw,"  Table Length:       0x%x bytes\n", header->base_table_length);
	fwts_log_info_verbatum(fw,"  Spec Revision:      %d (1.%d)\n", header->spec_rev, header->spec_rev);
	fwts_log_info_verbatum(fw,"  OEM ID:             %8.8s\n", header->oem_id);
	fwts_log_info_verbatum(fw,"  Product ID:         %12.12s\n", header->product_id);
	fwts_log_info_verbatum(fw,"  Entry Count:        0x%x\n", header->entry_count);
	fwts_log_info_verbatum(fw,"  LAPIC Address:      0x%8.8x\n", header->lapic_address);
	fwts_log_info_verbatum(fw,"  Extended Length:    0x%x bytes\n", header->extended_table_length);	
	fwts_log_nl(fw);
}

static void mpdump_dump_cpu_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_processor_entry *cpu_entry = (fwts_mp_processor_entry *)data;

	fwts_log_info_verbatum(fw, "CPU Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Local APIC ID:      0x%2.2x", cpu_entry->local_apic_id);
	fwts_log_info_verbatum(fw, "  Local APIC Version: 0x%2.2x", cpu_entry->local_apic_version);
	fwts_log_info_verbatum(fw, "  CPU Flags:          0x%2.2x", cpu_entry->cpu_flags);
	fwts_log_info_verbatum(fw, "    Usable:           %1.1d (%s)", 
		cpu_entry->cpu_flags & 1, 
		mpdump_yes_no[cpu_entry->cpu_flags & 1]);
	fwts_log_info_verbatum(fw, "    Bootstrap CPU:    %1.1d (%s)",
		(cpu_entry->cpu_flags >> 1) & 1,
		mpdump_yes_no[(cpu_entry->cpu_flags >> 1) & 1]);
	fwts_log_info_verbatum(fw, "  CPU Signature:");
	fwts_log_info_verbatum(fw, "    Stepping:         0x%2.2x", cpu_entry->cpu_signature & 0xf);
	fwts_log_info_verbatum(fw, "    Model:            0x%2.2x", (cpu_entry->cpu_signature >> 4) & 0xf);
	fwts_log_info_verbatum(fw, "    Family:           0x%2.2x", (cpu_entry->cpu_signature >> 8) & 0xf);
	fwts_log_info_verbatum(fw, "  Feature Flags:");
	fwts_log_info_verbatum(fw, "    FPU present:      %1.1d (%s)",
		(cpu_entry->feature_flags) & 1,
		mpdump_yes_no[(cpu_entry->feature_flags) & 1]);
	fwts_log_info_verbatum(fw, "    MCE:              %1.1d (%s)", 
		(cpu_entry->feature_flags >> 7) & 1,
		mpdump_yes_no[(cpu_entry->feature_flags >> 7) & 1]);
	fwts_log_info_verbatum(fw, "    CPMPXCHG8B:       %1.1d (%s)",
		(cpu_entry->feature_flags >> 8) & 1,
		mpdump_yes_no[(cpu_entry->feature_flags >> 8) & 1]);
	fwts_log_info_verbatum(fw, "    APIC enabled:     %1.1d (%s)",
		(cpu_entry->feature_flags >> 9) & 1,
		mpdump_yes_no[(cpu_entry->feature_flags >> 9) & 1]);
	fwts_log_nl(fw);
}

static void mpdump_dump_bus_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_bus_entry *bus_entry = (fwts_mp_bus_entry *)data;

	fwts_log_info_verbatum(fw, "Bus Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Bus ID:             0x%2.2x", bus_entry->bus_id);
	fwts_log_info_verbatum(fw, "  Bus Type:           %6.6s", bus_entry->bus_type);
	fwts_log_nl(fw);
}

static void mpdump_dump_io_apic_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_io_apic_entry *io_apic_entry = (fwts_mp_io_apic_entry *)data;

	fwts_log_info_verbatum(fw, "IO APIC Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  IO APIC ID:         0x%2.2x", io_apic_entry->id);
	fwts_log_info_verbatum(fw, "  IO APIC Version:    0x%2.2x", io_apic_entry->version);
	fwts_log_info_verbatum(fw, "  Flags:              0x%2.2x", io_apic_entry->flags);
	fwts_log_info_verbatum(fw, "  Address:            0x%8.8x", io_apic_entry->address);
	fwts_log_nl(fw);
}

static void mpdump_dump_io_interrupt_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_io_interrupt_entry *io_interrupt_entry = (fwts_mp_io_interrupt_entry *)data;

	fwts_log_info_verbatum(fw, "IO Interrupt Assignment Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Interrupt Type:     0x%2.2x (%s)", io_interrupt_entry->type,
		io_interrupt_entry->type < 4 ? mpdump_inttype[io_interrupt_entry->type] : "Unknown");
	fwts_log_info_verbatum(fw, "  Flags:              0x%4.4x", io_interrupt_entry->flags);
	fwts_log_info_verbatum(fw, "     PO (Polarity)    %1.1d (%s)", io_interrupt_entry->flags & 2,
		mpdump_po[io_interrupt_entry->flags & 2]);
	fwts_log_info_verbatum(fw, "     EL (Trigger)     %1.1d (%s)", (io_interrupt_entry->flags >> 2) & 2,
		mpdump_el[(io_interrupt_entry->flags >> 2) & 2]);
	fwts_log_info_verbatum(fw, "  Src Bus ID:         0x%2.2x", io_interrupt_entry->source_bus_id);
	fwts_log_info_verbatum(fw, "  Src Bus IRQ         0x%2.2x", io_interrupt_entry->source_bus_irq);
	fwts_log_info_verbatum(fw, "  Dst I/O APIC:       0x%2.2x", io_interrupt_entry->destination_io_apic_id);
	fwts_log_info_verbatum(fw, "  Dst I/O APIC INTIN: 0x%2.2x", io_interrupt_entry->destination_io_apic_intin);
	fwts_log_nl(fw);
}

static void mpdump_dump_local_interrupt_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_local_interrupt_entry *local_interrupt_entry = (fwts_mp_local_interrupt_entry *)data;

	fwts_log_info_verbatum(fw, "Local Interrupt Assignement Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Interrupt Type:     0x%2.2x (%s)", local_interrupt_entry->type,
		local_interrupt_entry->type < 4 ? mpdump_inttype[local_interrupt_entry->type] : "Unknown");
	fwts_log_info_verbatum(fw, "  Flags:              0x%4.4x", local_interrupt_entry->flags);
	fwts_log_info_verbatum(fw, "     PO (Polarity)    %1.1d (%s)", local_interrupt_entry->flags & 2,
		mpdump_po[local_interrupt_entry->flags & 2]);
	fwts_log_info_verbatum(fw, "     EL (Trigger)     %1.1d (%s)", (local_interrupt_entry->flags >> 2) & 2,
		mpdump_el[(local_interrupt_entry->flags >> 2) & 2]);
	fwts_log_info_verbatum(fw, "  Src Bus ID:         0x%2.2x", local_interrupt_entry->source_bus_id);
	fwts_log_info_verbatum(fw, "  Src Bus IRQ         0x%2.2x", local_interrupt_entry->source_bus_irq);
	fwts_log_info_verbatum(fw, "  Dst I/O APIC:       0x%2.2x", local_interrupt_entry->destination_local_apic_id);
	fwts_log_info_verbatum(fw, "  Dst I/O APIC INTIN: 0x%2.2x", local_interrupt_entry->destination_local_apic_intin);
	fwts_log_nl(fw);
}

static void mpdump_dump_sys_addr_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_system_address_space_entry *sys_addr_entry = (fwts_mp_system_address_space_entry *)data;

	fwts_log_info_verbatum(fw, "System Address Space Mapping Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Bus ID:             0x%2.2x", sys_addr_entry->bus_id);
	fwts_log_info_verbatum(fw, "  Address Type:       0x%2.2x (%s)", sys_addr_entry->address_type,
			sys_addr_entry->address_type < 4 ? mpdump_sys_addr_type[sys_addr_entry->address_type] : "Unknown");
	fwts_log_info_verbatum(fw, "  Address Start:      0x%16.16llx", 
		(unsigned long long)sys_addr_entry->address_base);
	fwts_log_info_verbatum(fw, "  Address End:        0x%16.16llx",
		(unsigned long long)sys_addr_entry->address_base + sys_addr_entry->address_length);
	fwts_log_info_verbatum(fw, "  Address Length      0x%16.16llx",
		(unsigned long long)sys_addr_entry->address_length);
	fwts_log_nl(fw);
}

static void mpdump_dump_bus_hierarchy_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_bus_hierarchy_entry *bus_hierarchy_entry = (fwts_mp_bus_hierarchy_entry*)data;

	fwts_log_info_verbatum(fw, "Bus Hierarchy Descriptor Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Bus ID:             0x%2.2x", bus_hierarchy_entry->bus_id);
	fwts_log_info_verbatum(fw, "  Bus Information:    0x%1.1x", bus_hierarchy_entry->bus_info & 0xf);
	fwts_log_info_verbatum(fw, "  Parent Bus:         0x%8.8x", bus_hierarchy_entry->parent_bus);
	fwts_log_nl(fw);
}

static void multproc_dump_compat_bus_address_space_entry(fwts_framework *fw, void *data, uint32_t phys_addr)
{
	fwts_mp_compat_bus_address_space_entry *compat_bus_entry = (fwts_mp_compat_bus_address_space_entry*)data;

	fwts_log_info_verbatum(fw, "Compatible Bus Hierarchy Descriptor Entry: (@0x%8.8x)", phys_addr);
	fwts_log_info_verbatum(fw, "  Bus ID:             0x%2.2x", compat_bus_entry->bus_id);
	fwts_log_info_verbatum(fw, "  Address Mod:        0x%2.2x", compat_bus_entry->address_mod);
	fwts_log_info_verbatum(fw, "  Predefine Range:    0x%8.8x", compat_bus_entry->range_list);
	fwts_log_nl(fw);
}

static fwts_mp_data mp_data;

static int mpdump_init(fwts_framework *fw)
{	
	if (fwts_check_root_euid(fw))
		return FWTS_ABORTED;

	if (fwts_mp_data_get(&mp_data) != FWTS_OK) {
		fwts_log_error(fw, "Failed to get _MP_ data from firmware.");
		return FWTS_ABORTED;
	}
	return FWTS_OK;
}

static int mpdump_deinit(fwts_framework *fw)
{
	return fwts_mp_data_free(&mp_data);
}

static char *mpdump_headline(void)
{
	return "Dump MultiProcessor Data.";
}

static int mpdump_compare_bus(void *data1, void *data2)
{
	fwts_mp_bus_entry *bus_entry1 = (fwts_mp_bus_entry *)data1;
	fwts_mp_bus_entry *bus_entry2 = (fwts_mp_bus_entry *)data2;

	return bus_entry1->bus_id - bus_entry2->bus_id;
}

static void mpdump_dump_bus(fwts_framework *fw)
{
	fwts_list_link	*entry;
	fwts_list	sorted;

	fwts_list_init(&sorted);
	
	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_BUS_ENTRY)
			fwts_list_add_ordered(&sorted, data, mpdump_compare_bus);
	}

	fwts_log_info_verbatum(fw, "Bus:");
	fwts_log_info_verbatum(fw, "   ID  Type");
	fwts_list_foreach(entry, &sorted) {
		fwts_mp_bus_entry *bus_entry = fwts_list_data(fwts_mp_bus_entry *, entry);
		fwts_log_info_verbatum(fw, "  0x%2.2x %6.6s", 
			bus_entry->bus_id, bus_entry->bus_type);
	}
	fwts_log_nl(fw);
	fwts_list_free_items(&sorted, NULL);
}

static int mpdump_compare_io_irq(void *data1, void *data2)
{
	fwts_mp_io_interrupt_entry *entry1 = (fwts_mp_io_interrupt_entry*)data1;
	fwts_mp_io_interrupt_entry *entry2 = (fwts_mp_io_interrupt_entry*)data2;

	return entry1->source_bus_irq - entry2->source_bus_irq;
}

static int mpdump_compare_local_irq(void *data1, void *data2)
{
	fwts_mp_local_interrupt_entry *entry1 = (fwts_mp_local_interrupt_entry*)data1;
	fwts_mp_local_interrupt_entry *entry2 = (fwts_mp_local_interrupt_entry*)data2;

	return entry1->source_bus_irq - entry2->source_bus_irq;
}

static void mpdump_dump_irq_table(fwts_framework *fw)
{
	fwts_list_link	*entry;
	fwts_list	sorted;

	fwts_list_init(&sorted);
	
	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_IO_INTERRUPT_ENTRY)
			fwts_list_add_ordered(&sorted, data, mpdump_compare_io_irq);
	}

	fwts_log_info_verbatum(fw, "IO Interrupts:");
	fwts_log_info_verbatum(fw, "  IRQ#  Type   APIC  APIC   Polarity   Trigger");
	fwts_log_info_verbatum(fw, "               ID    INTIN");
	fwts_list_foreach(entry, &sorted) {
		fwts_mp_io_interrupt_entry *io_interrupt_entry = 
			fwts_list_data(fwts_mp_io_interrupt_entry *, entry);
		fwts_log_info_verbatum(fw, "  %3d   %-6.6s %3d    %3d     %-7.7s    %-7.7s", 
			io_interrupt_entry->source_bus_irq, 
			io_interrupt_entry->type < 4 ? mpdump_inttype[io_interrupt_entry->type] : "Unknown",
			io_interrupt_entry->destination_io_apic_id,
			io_interrupt_entry->destination_io_apic_intin,
			mpdump_po_short[io_interrupt_entry->flags & 2],
			mpdump_el_short[(io_interrupt_entry->flags >> 2) & 2]);
	}
	fwts_log_nl(fw);
	fwts_list_free_items(&sorted, NULL);
	fwts_list_init(&sorted);

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == FWTS_MP_LOCAL_INTERRUPT_ENTRY)
			fwts_list_add_ordered(&sorted, data, mpdump_compare_local_irq);
	}

	fwts_log_info_verbatum(fw, "Local Interrupts:");
	fwts_log_info_verbatum(fw, "  IRQ#  Type   APIC  APIC   Polarity   Trigger");
	fwts_log_info_verbatum(fw, "               ID    INTIN");
	fwts_list_foreach(entry, &sorted) {
		fwts_mp_local_interrupt_entry *local_interrupt_entry = 
			fwts_list_data(fwts_mp_local_interrupt_entry *, entry);
		fwts_log_info_verbatum(fw, "  %3d   %-6.6s %3d    %3d     %-7.7s    %-7.7s", 
			local_interrupt_entry->source_bus_irq, 
			local_interrupt_entry->type < 4 ? mpdump_inttype[local_interrupt_entry->type] : "Unknown",
			local_interrupt_entry->destination_local_apic_id,
			local_interrupt_entry->destination_local_apic_intin,
			mpdump_po_short[local_interrupt_entry->flags & 2],
			mpdump_el_short[(local_interrupt_entry->flags >> 2) & 2]);
	}
	 
	fwts_list_free_items(&sorted, NULL);
	fwts_log_nl(fw);
}

static int mpdump_compare_system_address_space(void *data1, void *data2)
{
	int64_t diff;
	fwts_mp_system_address_space_entry *sys_addr_entry1 = 
		(fwts_mp_system_address_space_entry *)data1;
	fwts_mp_system_address_space_entry *sys_addr_entry2 = 
		(fwts_mp_system_address_space_entry *)data2;

	diff = sys_addr_entry1->address_base - sys_addr_entry2->address_base;

	if (diff == 0)
		return 0;
	else if (diff < 0)
		return -1;
	else
		return 0;
}

static void mpdump_dump_system_address_table(fwts_framework *fw)
{
	fwts_list_link	*entry;
	fwts_list	sorted;

	fwts_list_init(&sorted);

	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		if (*data == 128)
			fwts_list_add_ordered(&sorted, data, mpdump_compare_system_address_space);
	}

	fwts_log_info_verbatum(fw, "System Address Table:");
	fwts_log_info_verbatum(fw, "  Start Address      End Address      Bus ID  Type");
	fwts_list_foreach(entry, &sorted) {
		fwts_mp_system_address_space_entry *sys_addr_entry =
			fwts_list_data(fwts_mp_system_address_space_entry *, entry);
		fwts_log_info_verbatum(fw, "  %16.16llx - %16.16llx  0x%2.2x   %s",
			(unsigned long long)sys_addr_entry->address_base,
			(unsigned long long)sys_addr_entry->address_base +
			sys_addr_entry->address_length,
			sys_addr_entry->bus_id,
			sys_addr_entry->address_type < 4 ? 
				mpdump_sys_addr_type[sys_addr_entry->address_type] : "Unknown");
	}

	fwts_list_free_items(&sorted, NULL);
	fwts_log_nl(fw);
}

static int mpdump_test1(fwts_framework *fw)
{
	fwts_list_link	*entry;

	fwts_infoonly(fw);

	mpdump_dump_header(fw, mp_data.header, mp_data.phys_addr);
	
	fwts_list_foreach(entry, &mp_data.entries) {
		uint8_t *data = fwts_list_data(uint8_t *, entry);
		uint32_t phys_addr = mp_data.phys_addr + ((void *)data - (void *)mp_data.header);
		switch (*data) {
		case FWTS_MP_CPU_ENTRY:
			mpdump_dump_cpu_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_BUS_ENTRY:
			mpdump_dump_bus_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_IO_APIC_ENTRY:
			mpdump_dump_io_apic_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_IO_INTERRUPT_ENTRY:
			mpdump_dump_io_interrupt_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_LOCAL_INTERRUPT_ENTRY:
			mpdump_dump_local_interrupt_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_SYS_ADDR_ENTRY:
			mpdump_dump_sys_addr_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_BUS_HIERARCHY_ENTRY:
			mpdump_dump_bus_hierarchy_entry(fw, data, phys_addr);
			break;
		case FWTS_MP_COMPAT_BUS_ADDRESS_SPACE_ENTRY:
			multproc_dump_compat_bus_address_space_entry(fw, data, phys_addr);
			break;
		default:
			break;
		}
	}

	fwts_log_info(fw, "Collated Data:");
	fwts_log_nl(fw);
	mpdump_dump_bus(fw);
	mpdump_dump_irq_table(fw);
	mpdump_dump_system_address_table(fw);

	return FWTS_OK;
}

static fwts_framework_minor_test mpdump_tests[] = {
	{ mpdump_test1, "Dump Multi Processor Data." },
	{ NULL, NULL }
};

static fwts_framework_ops mpdump_ops = {
	.headline    = mpdump_headline,
	.init        = mpdump_init,
	.deinit      = mpdump_deinit,
	.minor_tests = mpdump_tests,
};

FWTS_REGISTER(mpdump, &mpdump_ops, FWTS_TEST_ANYTIME, FWTS_UTILS);
