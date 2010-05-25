SUBDIRS = lib

AM_CPPFLAGS = -I$(top_srcdir)/src/lib/include

bin_PROGRAMS = testsuite
testsuite_SOURCES = main.c acpi/klog/klog.c acpi/wakealarm/wakealarm.c acpi/syntaxcheck/syntaxcheck.c dmi/dmi_decode/dmi_decode.c

testsuite_LDADD = $(top_srcdir)/src/lib/src/libtestsuite.a

