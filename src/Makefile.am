SUBDIRS = lib
 
AM_CPPFLAGS = -I$(top_srcdir)/src/lib/include -Wall -Werror
AM_LDFLAGS = -lm

bin_PROGRAMS = fwts
fwts_SOURCES = main.c \
	acpi/klog/klog.c \
	acpi/wakealarm/wakealarm.c \
	acpi/syntaxcheck/syntaxcheck.c \
	acpi/s3/s3.c \
	acpi/s4/s4.c \
	acpi/ac_adapter/ac_adapter.c \
	acpi/acpiinfo/acpiinfo.c \
	acpi/battery/battery.c \
	acpi/brightness/brightness.c \
	acpi/common/common.c \
	acpi/cstates/cstates.c \
	acpi/fadt/fadt.c \
	acpi/fan/fan.c \
	acpi/mcfg/mcfg.c \
	apic/apicedge/apicedge.c \
	acpi/osilinux/osilinux.c \
	acpi/lid/lid.c \
	acpi/powerbutton/powerbutton.c \
	bios/ebda_region/ebda_region.c \
	bios/mtrr/mtrr.c \
	dmi/dmi_decode/dmi_decode.c \
	hpet/hpet_check/hpet_check.c \
	pci/maxreadreq/maxreadreq.c \
	cpu/virt/virt.c cpu/virt/virt_svm.c cpu/virt/virt_vmx.c \
	kernel/version/version.c

fwts_LDADD = $(top_srcdir)/src/lib/src/libtestsuite.a

