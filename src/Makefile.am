SUBDIRS = lib
 
AM_CPPFLAGS = -I$(top_srcdir)/src/lib/include -Wall -Werror

bin_PROGRAMS = testsuite
testsuite_SOURCES = main.c \
	acpi/klog/klog.c \
	acpi/wakealarm/wakealarm.c \
	acpi/syntaxcheck/syntaxcheck.c \
	acpi/s3/s3.c \
	acpi/s4/s4.c \
	acpi/acpiinfo/acpiinfo.c \
	acpi/battery/battery.c \
	acpi/common/common.c \
	apic/apicedge/apicedge.c \
	bios/ebda_region/ebda_region.c \
	dmi/dmi_decode/dmi_decode.c \
	pci/maxreadreq/maxreadreq.c

#hpet/hpet_check/hpet_check.c 

testsuite_LDADD = $(top_srcdir)/src/lib/src/libtestsuite.a

