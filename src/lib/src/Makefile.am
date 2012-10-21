AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/include \
	-I$(top_srcdir)/src/acpica/source/include \
	-I$(top_srcdir)/src/acpica/source/compiler \
	-Wall -Werror

pkglib_LTLIBRARIES = libfwts.la
libfwts_la_LDFLAGS = -lpcre -lpthread -ljson -version-info 1:0:0 \
	-L$(top_srcdir)/src/acpica/source/compiler -lfwtsiasl

libfwts_la_SOURCES = \
	fwts_acpi.c \
	fwts_acpid.c \
	fwts_acpi_tables.c \
	fwts_acpi_method.c \
	fwts_args.c \
	fwts_alloc.c \
	fwts_binpaths.c \
	fwts_checkeuid.c \
	fwts_checksum.c \
	fwts_cmos.c \
	fwts_cpu.c \
	fwts_dump.c \
	fwts_dump_data.c \
	fwts_ebda.c \
	fwts_fileio.c \
	fwts_firmware.c \
	fwts_formatting.c \
	fwts_framework.c \
	fwts_get.c \
	fwts_guid.c \
	fwts_gpe.c \
	fwts_hwinfo.c \
	fwts_iasl.c \
	fwts_interactive.c \
	fwts_keymap.c \
	fwts_klog.c \
	fwts_list.c \
	fwts_log.c \
	fwts_log_plaintext.c \
	fwts_log_json.c \
	fwts_log_xml.c \
	fwts_log_html.c \
	fwts_memorymap.c \
	fwts_microcode.c \
	fwts_mmap.c \
	fwts_multiproc.c \
	fwts_oops.c \
	fwts_pipeio.c \
	fwts_scan_efi_systab.c \
	fwts_set.c \
	fwts_smbios.c \
	fwts_stringextras.c \
	fwts_summary.c \
	fwts_tag.c \
	fwts_text_list.c \
	fwts_tty.c \
	fwts_uefi.c \
	fwts_virt.c \
	fwts_wakealarm.c \
	fwts_ac_adapter.c \
	fwts_battery.c \
	fwts_button.c \
	fwts_efi_module.c
