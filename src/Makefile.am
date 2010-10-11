SUBDIRS = lib
 
AM_CPPFLAGS = -I$(top_srcdir)/src/lib/include -Wall -Werror
AM_LDFLAGS = -lm -lpcre

bin_PROGRAMS = fwts
fwts_SOURCES = main.c \
	acpi/klog/klog.c \
	acpi/wakealarm/wakealarm.c \
	acpi/syntaxcheck/syntaxcheck.c \
	acpi/s3/s3.c \
	acpi/s4/s4.c \
	acpi/ac_adapter/ac_adapter.c \
	acpi/acpiinfo/acpiinfo.c \
	acpi/acpidump/acpidump.c \
	acpi/apicinstance/apicinstance.c \
	acpi/battery/battery.c \
	acpi/brightness/brightness.c \
	acpi/checksum/checksum.c \
	acpi/common/common.c \
	acpi/cstates/cstates.c \
	acpi/dmar/dmar.c \
	acpi/fadt/fadt.c \
	acpi/fan/fan.c \
	acpi/mcfg/mcfg.c \
	apic/apicedge/apicedge.c \
	acpi/osilinux/osilinux.c \
	acpi/lid/lid.c \
	acpi/powerbutton/powerbutton.c \
	acpi/wmi/wmi.c \
	bios/ebda_region/ebda_region.c \
	bios/mtrr/mtrr.c \
	bios/bios_info/bios_info.c \
	bios/bios32/bios32.c \
	bios/hdaaudio/hdaaudio.c \
	bios/romdump/romdump.c \
	bios/os2gap/os2gap.c \
	bios/e820dump/e820dump.c \
	bios/smbios/smbios.c \
	cmos/cmosdump/cmosdump.c \
	dmi/dmi_decode/dmi_decode.c \
	hpet/hpet_check/hpet_check.c \
	pci/crs/crs.c \
	pci/maxreadreq/maxreadreq.c \
	cpu/virt/virt.c cpu/virt/virt_svm.c cpu/virt/virt_vmx.c \
	cpu/maxfreq/maxfreq.c \
	cpu/cpufreq/cpufreq.c \
	cpu/thermal_trip/thermal_trip.c \
	cpu/nx/nx.c \
	cpu/microcode/microcode.c \
	hotkey/hotkey/hotkey.c \
	kernel/version/version.c

fwts_LDADD = $(top_srcdir)/src/lib/src/libtestsuite.a

man_MANS = ../doc/fwts.1

