AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/include \
	-I$(top_srcdir)/src/acpica/source/include \
	-I$(top_srcdir)/src/acpica/source/compiler \
	-Wall -Werror

lib_LTLIBRARIES = libfwts.la
libfwts_la_LDFLAGS = -lpthread -ljson -static

libfwts_la_SOURCES = \
	list.c \
	acpi.c \
	acpica.c \
	acpid.c \
	acpi_tables.c \
	interactive.c \
	binpaths.c \
	cpu.c \
	checkeuid.c \
	cmos.c \
	dump.c \
	dump_data.c \
	uefi.c \
	fileio.c \
	firmware.c \
	framework.c \
	formatting.c \
	get.c 	\
	gpe.c \
	iasl.c \
	keymap.c \
	klog.c \
	log.c \
	memorymap.c \
	pipeio.c \
	set.c \
	stringextras.c \
	text_list.c \
	tty.c \
	wakealarm.c \
	summary.c \
	microcode.c \
	virt.c \
	tag.c
