== FWTS Source Structure ==

FWTS comprises of a core fwts library, two special builds of the ACPICA source
and the fwts tests themselves.

== Core FWTS library libfwts.so ==

The core fwts library, libfwts.so, contains the fwts test framework to drive
fwts tests and a support library that provides the tests with commonly used
functionality, such as list handling, log scanning, port I/O, ACPI table
handling, etc.  

Source:
	fwts/src/lib/src 
	fwts/src/lib/include

Conventions:
	All fwts core library functions should start with fwts_ prefix
	C source that provides an API should have a corresponding header in 
	fwts/src/lib/include
	We include all exported headers in fwts/src/lib/include/fwts.h
	Declare variables and functions as static if they are not to be exported

	NULL returns indicates an error
	FWTS_OK indicates success
	FWTS_ERROR indicates generic failure
	-ve returns indicate error

	All structs should be typedef'd
	NO typedef'ing of void * please!
	Where possible use uint64_t, uint32_t, uint16_t, uint8_t
	ALWAYS assume system calls fail and check returns
	NO clever macros that used obfuscated tricks

	Kernel Style (K&R).
	Avoid spaces and tabs at end of line.
	Must compile with -Wall -Werror
	Avoid warnings, exception is the ACPICA core since we don't modify this
	Test with valgrind to check for memory leaks.
	No smart tricks.
	Comments are not optional!

== ACPICA libraries ==

The ACPICA source is built in two modes for fwts, one for the run-time
execution of AML (acpiexec mode) and one for the assembly/dis-assembly of AML
(compile mode). This makes life a bit complex since both modes are not
compatible, so fwts builds these as differenty libraries and exports a thin
shim interface that libfwts and fwts tests can use.  It's quite ugly.

The intention is to regularly pick up the latest ACPICA sources every release
cycle and drop these into fwts.  Note that we only incorporate the minimal
subset of ACPICA source that is required for the execution and 
assembly/disassembly functionality required by fwts.

=== acpiexec mode ===

libfwtsacpica.so:
	fwts/src/acpica/Makefile.am

Interface:
	fwts/src/acpica/fwts_acpica.c
	fwts/src/acpica/fwts_acpica.h

=== compiler mode ===

libfwtsiasl.so:
	fwts/src/acpica/source/compiler/Makefile.am

Interface:
	fwts/src/acpica/source/compiler/fwts_iasl_interface.c
	fwts/src/acpica/source/compiler/fwts_iasl_interface.h
	
=== Notes ===
	
Whenever we update the ACPICA core we need to update README_ACPICA.txt to
reflect any additional ACPIC sources we include into fwts.  

DO NOT modify any of the ACPICA sources - we use the original sources and
should not modify them in fwts.  Note that some on-the-fly source tweaking is
performed to stop name clashes.

== fwts tests ==

The fwts tests are grouped in several sub-directories depending on the type of
testing:

src/acpi	- ACPI specific tests (methods, AML, tables, etc)
src/apic	- APIC tests
src/bios	- Tradition BIOS tests, e.g. SMBIOS, MPTables etc
src/cmos	- CMOS memory
src/cpu		- CPU specific firmware settings
src/dmi		- DMI tables
src/example	- example test template code
src/hotkey	- HotKey tests
src/hpet	- HPET 
src/kernel	- kernel specific
src/pci		- PCI
src/sbbr	- ARM SBBR Tests
src/uefi	- UEFI

When writing new tests use the blank sketch of a test in
src/example/blank/blank.c.

Test Conventions:
	All code must be static, no exporting of functions to other parts of
	fwts.
	If we have common code in multiple tests then refactor it into the
	fwts core library.
	DO NOT call exit() from a test.
	DO NOT call printf() from a test - use the fwts logging functions.

Tests are registered with fwts using the FWTS_REGISTER macro:

	FWTS_REGISTER(const char *example, 
		     fwts_framework_ops *example_ops, priority, flags);

where:
	example	- is the name of the test, no spaces in this name as it is
	stringified.

	example_ops - fwts_framework_ops to configure test, with:
		.description - const char * name of the test.
		.init	     - optional function to call before running the
				test(s).
		.deinit	     - optional function to call after running the
				test(s).
		.minor_tests - array of fwts_framework_minor_test describing
				minor tests to run.

		.minor_tests is an array of functions + text description
		 		tuples of minor tests to run and must be NULL
				terminated.  One can have one or more minor
				tests registered to run.

	priority ranking - when to run the test when we have many tests to run
	and can be:
		FWTS_TEST_FIRST - schedule to run as first test(s)
		FWTS_TEST_EARLY - schedule to run fairly soon at start
		FWTS_TEST_ANYTIME - medium priority
		FWTS_TEST_LATE  - schedule to run close to the end
		FWTS_TEST_LAST  - run at the end
	
		Tests of the same priority are will run in an order chosen at
		link time, but will run before tests of higher priory ranking
		and after tests of lower priory ranking.

	flags - indicates the type of test - where appropriate these can be 
		logically OR'd  together, can be:

		FWTS_BATCH                      batch test
		FWTS_INTERACTIVE                interactive test
		FWTS_BATCH_EXPERIMENTAL         batch experimental test
		FWTS_INTERACTIVE_EXPERIMENTAL   batch interactive  test
		FWTS_POWER_STATES               S3/S4 test
		FWTS_UTILS                      utility
		FWTS_ROOT_PRIV                  needs root privilege to run
		FWTS_TEST_BIOS                  BIOS specific
		FWTS_TEST_UEFI                  UEFI specific
		FWTS_TEST_ACPI                  ACPI specific
		FWTS_TEST_ACPI_COMPLIANCE	Test for ACPI spec compliance

		so, we can have FWTS_BATCH | FWTS_ROOT_PRIV | FWTS_ACPI
		for a batch test that requires root privilege and is an ACPI
		test.

	test init/deinit callbacks.
		These are intended to do pre and post test setup and close
		down, for example loading in tables, checking for resources,
		sanity checking capabilities.

		They return FWTS_OK (all OK) or FWTS_ERROR (something went
		wrong, abort!).

	minor tests.
		These do the specific tests, can have one or more per test
		harness. Must return FWTS_OK (all OK) or FWTS_ERROR (something
		went wrong). FWTS_ERROR does not mean a test failed, but the
		test could not be run because of some external factor, e.g.
		memory allocation failure, I/O issue, etc.
