/*
 * Copyright (C) 2010 Canonical
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

#include <sys/klog.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pcre.h>

#include "fwts.h"

void fwts_klog_free(fwts_list *klog)
{
	fwts_text_list_free(klog);
}

int fwts_klog_clear(void)
{
	if (klogctl(5, NULL, 0) < 0)
		return FWTS_ERROR;
	return FWTS_OK;
}

fwts_list *fwts_klog_read(void)
{
	int len;
	char *buffer;
	fwts_list *list;

	if ((len = klogctl(10, NULL, 0)) < 0)
		return NULL;

	if ((buffer = calloc(1, len)) < 0) {
		free(buffer);
		return NULL;
	}
	
	if (klogctl(3, buffer, len) < 0) {
		free(buffer);
		return NULL;
	}

	list = fwts_list_from_text(buffer);
	free(buffer);
	
	return list;
}

int fwts_klog_scan(fwts_framework *fw, 
		   fwts_list *klog, 
		   fwts_klog_scan_func scan_func,
		   fwts_klog_progress_func progress_func,
		   void *private, 
		   int *match)
{
	*match= 0;
	char *prev;
	fwts_list_link *item;
	int i;

	if (!klog)
		return FWTS_ERROR;

	prev = "";

	for (i=0,item = klog->head; item != NULL; item=item->next,i++) {
		char *ptr = (char *)item->data;

		if ((ptr[0] == '<') && (ptr[2] == '>'))
			ptr += 3;

		scan_func(fw, ptr, prev, private, match);
		if (progress_func  && ((i % 25) == 0))
			progress_func(fw, 100 * i / fwts_list_len(klog));
		prev = ptr;
	}
	return FWTS_OK;
}

#define FW_BUG	"\\[Firmware Bug\\]: "

/* List of common errors and warnings */

static fwts_klog_pattern common_error_warning_patterns[] = {
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"Temperature above threshold, cpu clock throttled",	
		"Test caused CPU temperature above critical threshold. Insufficient cooling?"
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_MEDIUM,
		"BIOS never enumerated boot CPU",
		"The boot processor is not enumerated!"
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"acpi_shpchprm.*_HPP fail",
		"Hotplug _HPP method failed"
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"shpchp.*acpi_pciehprm.*OSHP fail",
		"ACPI Hotplug OSHP method failed"
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"shpchp.*acpi_shpchprm.*evaluate _BBN fail",
		"Hotplug _BBN method is missing"
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"Error while parsing _PSD domain information",
		"_PSD domain information is corrupt!"
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"Wrong _BBN value, reboot and use option 'pci=noacpi'",
		"The BIOS has wrong _BBN value, which will make PCI root bridge have wrong bus number"
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI.*apic on CPU.* stops in C2",
		"The local apic timer incorrectly stops during C2 idle state."
		"The ACPI specification forbids this and Linux needs the local "
		"APIC timer to work. The most likely cause of this is that the "
		"firmware uses a hardware C3 or C4 state that is mapped to "
		"the ACPI C2 state."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"Disabling IRQ",
		"The kernel detected an irq storm. This is most probably an IRQ routing bug."
	},
	{
		FWTS_COMPARE_STRING,
		0,
		NULL,
		NULL
	}
};


/* List of errors and warnings */
static fwts_klog_pattern firmware_error_warning_patterns[] = {
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI.*Handling Garbled _PRT entry",
		"BIOS has a garbled _PRT entry; source_name and source_index swapped."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_MEDIUM,
		"Invalid _PCT data",
		"The ACPI _PCT data is invalid."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"ACPI Error.*ACPI path has too many parent prefixes",
		"A path to an ACPI obejct has too many ^ parent prefixes and references "
		"passed the top of the root node. Please check AML for all ^ prefixed ACPI path names."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"ACPI Error.*A valid RSDP was not found",
		"An ACPI-compatible system must provide an RSDP (Root System Description Pointer " 
		"in the system’s low address space. This structure’s only purpose is to provide "
		"the physical address of the RSDT and XSDT."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"ACPI Error.*Cannot release Mutex",
		"Attempted to release of a Mutex that was not previous acquired. This needs "
		"fixing as it could lead to race conditions when operating on a resource that "
		"needs to be proteced by a Mutex."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Could not disable .* event",	
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Could not enable .* event",	
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Current brightness invalid",
		"ACPI video driver has encountered a brightness level that is outside the expected range."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Namespace lookup failure, AE_ALREADY_EXISTS",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"ACPI Error.*Field.*exceeds Buffer",
		"The field exceeds the allocated buffer size. This can lead to unexpected results when "
	 	"fetching data outside this region."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"ACPI Error.*Illegal I/O port address/length above 64K",
		"A port address or length has exceeded the maximum allowed 64K address limit. This "
		"will lead to unpredicable errors."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Incorrect return type",
		"An ACPI Method has returned an unexpected and incorrect return type."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Needed .*\\[Buffer/String/Package\\], found \\[Integer\\]",
		"An ACPI Method has returned an Integer type when a Buffer, String or Package was expected."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Needed .*\\[Reference\\], found \\[Device\\]",
		"An ACPI Method has returned an Device type when a Reference type was expected."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*No handler for Region",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Region .* has no handler",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Missing expected return value",
		"The ACPI Method did not return a value and was expected too. This is a bug and needs fixing."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"Package List length.*larger than.*truncated",
		"A Method has returned a Package List that was larger than expected."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Result stack is empty!",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"ACPI Error.*Found unknown opcode",
		"An illegal AML opcode has been found and is ignored. This indicates either badly compiled "
		"code or opcode corruption in the DSDT or SSDT tables or a bug in the ACPI execution engine. "
		"Recommend disassembing using iasl to find any offending code."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"ACPI Warning.*Detected an unsupported executable opcode",
		"An illegal AML opcode has been found and is ignored. This indicates either badly compiled "
		"code or opcode corruption in the DSDT or SSDT tables or a bug in the ACPI execution engine. "
		"Recommend disassembing using iasl to find any offending code."
	},

	/* Method parse/execution failures */
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method parse/execution failed.*AE_AML_NO_RETURN_VALUE",
		"The ACPI Method was expected to return a value and did not."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_ALREADY_EXISTS",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_INVALID_TABLE_LENGTH",
		"The ACPI Method returned a table of the incorrect length. This can lead to unexepected results."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_AML_BUFFER_LIMIT",
		"Method failed: ResourceSourceIndex is present but ResourceSource is not."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_NOT_EXIST",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_NOT_FOUND",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_LIMIT",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_AML_OPERAND_TYPE",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_TIME",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_AML_PACKAGE_LIMIT",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_OWNER_ID_LIMIT",
		"Method failed to allocate owner ID."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Method (execution|parse/execution) failed.*AE_AML_MUTEX_NOT_ACQUIRED",
		"A Mutex acquire failed, which could possibly indicate that it was previously acquired "
		"and not released, or a race has occurred. Some AML code fails to miss Mutex acquire "
		"failures, so it is a good idea to verify all Mutex Acquires using the syntaxcheck test."
	},

	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.*SMBus or IPMI write requires Buffer of",
		"An incorrect SMBus or IPMI write buffer size was used."
	},
	{		
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI (Warning|Error).*Evaluating .* failed",
		"Executing the ACPI Method leaded in an execution failure. This needs investigating."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Optional field.*has zero address or length",
		"An ACPI table contains Generic Address Structure that has an address "
		"that is incorrectly set to zero, or a zero length. This needs to be "
		"fixed. "
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		"ACPI (Warning|Error).*32/64.*(length|address) mismatch in.*tbfadt",
		"The FADT table contains Generic Address Structure that has a mismatch "
		"between the 32 bit and 64 bit versions of an address. This should be "
		"fixed so there are no mismatches. "
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		"ACPI (Warning|Error).*32/64.*(length|address) mismatch in",
		"A table contains Generic Address Structure that has a mismatch "
		"between the 32 bit and 64 bit versions of an address. This should be "
		"fixed so there are no mismatches. "
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Return Package type mismatch",
		"ACPI AML interpreter executed a Method that returned a package with incorrectly typed data. "
		"The offending method needs to be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Return Package has no elements",
		"ACPI AML interpreter executed a Method that returned a package with no elements inside it. "
		"This is most probably a bug in the Method and needs to be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Return Package is too small",
		"ACPI AML interpreter executed a Method that returned a package with too few elements inside it. "
		"This is most probably a bug in the Method and needs to be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		"ACPI Warning.*Incorrect checksum in table",
		"The ACPI table listed above has an incorrect checksum, this could be a BIOS bug or due to table corruption."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,	
		"ACPI Warning.*_BQC returned an invalid level",
		"Method _BQC (Brightness Query Current) returned an invalid display brightness level."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Could not enable fixed event",
		NULL,
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Return type mismatch",
		"The ACPI Method returned an incorrect type, this should be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Parameter count mismatch",
		"The ACPI Method was executing with a different number of parameters than the "
		"Method expected. This should be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Insufficient arguments",
		"The ACPI Method has not enough arguments as expected. "
		"This should be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Package has no elements",
		"The ACPI Method returned a package with no elements in it, and some were exepected."
		"This should be fixed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Converted Buffer to expected String",
		"Method returned a Buffer type instead of a String type and ACPI driver automatically "
		"converted it to a String.  It is worth fixing this in the DSDT or SSDT even if the "
		"kernel fixes it at run time."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Return Package type mistmatch at index",
		"The ACPI Method returned a package that contained data of the incorrect data type. "
		"This data type needs fixing."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Invalid length for.*fadt",
		"This item in the FADT is the incorrect length. Should be corrected."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Warning.*Invalid throttling state",
		NULL,
	},

	/* ACPI Exceptions */
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Exception:.*AE_TIME.*Returned by Handler for.*[EmbeddedControl]",
		"This is most probably caused by when a read or write operation to the EC memory "
		"has failed because of a timeout waiting for the Embedded Controller to complete "
		"the transaction.  Normally, the kernel waits for 500ms for the Embedded Controller "
		"status port to indicate that a transaction is complete, but in this case it has not "
		"and a AE_TIME error has been returned. "
	},
	
	/* Catch all warning */
	{ 
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,	
		"ACPI Warning", 
		"ACPI AML intepreter has found some non-conforming AML code. "
		"This should be investigated and fixed." 
	},

	{	FWTS_COMPARE_REGEX,	
		LOG_LEVEL_HIGH,
		"ACPI.*BIOS bug: multiple APIC/MADT found, using",
		"The kernel has detected more than one ACPI Multiple APIC Description Table (MADT) ("
		"these tables have the \"APIC\" signature). "
		"There should only be one MADT and the kernel will by default select the first one. "
		"However, one can override this and select the Nth MADT using acpi_apic_instance=N.",
	},

	{ 
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI.*Invalid physical address in GAR", 
		"ACPI Generic Address is invalid" 
	},

	/* powernow-k8 bugs */
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "powernow-k8.*(No PSB or ACPI _PSS objects|No compatible ACPI _PSS|Your BIOS does not provide ACPI _PSS objects)",
		"The _PSS object (Performance Supported States) is an optional "
		"object that indicates the number of supported processor performance states. "
		"The powernow-k8 driver source states: "
         	"If you see this message, complain to BIOS manufacturer. If "
         	"he tells you \"we do not support Linux\" or some similar "
         	"nonsense, remember that Windows 2000 uses the same legacy "
         	"mechanism that the old Linux PSB driver uses. Tell them it "
         	"is broken with Windows 2000. "
		"The reference to the AMD documentation is chapter 9 in the "
         	"BIOS and Kernel Developer's Guide, which is available on "
         	"www.amd.com."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "powernow-k8.*Try again with latest",
		NULL,
	},

	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,	
		FW_BUG "ACPI.*Invalid bit width in GAR", 
		"ACPI Generic Address width must be 8, 16, 32 or 64" 
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI.*Invalid address space type in GAR", 
		"ACPI Generic Address space type must be system memory or system IO space."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI.*no secondary bus range in _CRS", 
		"_CRS Method should return a secondary bus address for the "
		"status/command port. The kernel is having to guess this "
		"based on the _BBN or assume it's 0x00-0xff." 
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "ACPI.*Invalid BIOS _PSS frequency", 
		"_PSS (Performance Supported States) package has an incorrectly "
		"define core frequency (first DWORD entry in the _PSS package)." 
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		FW_BUG "ACPI.*brightness control misses _BQC function",
		"_BQC (Brightness Query Current level) seems to be missing. "
		"This method returns the current brightness level of a "
		"built-in display output device."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		FW_BUG "ACPI:",
		"ACPI driver has detected an ACPI bug. This generally points to a "
		"bug in an ACPI table. Examine the kernel log for more details."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		FW_BUG "BIOS needs update for CPU frequency support", 
		"Having _PPC but missing frequencies (_PSS, _PCT) is a good hint "
		"that the BIOS is older than the CPU and does not know the CPU "
		"frequencies."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		FW_BUG "ERST.*ERST table is invalid",
		"The Error Record Serialization Table (ERST) seems to be invalid. "
		"This normally indicates that the ERST table header size is too "
		"small, or the table size (excluding header) is not a multiple of "
		"the ERST entries."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		FW_BUG "Invalid critical threshold",
		"ACPI _CRT (Critical Trip Point) is returning a threshold "
		"lower than zero degrees Celsius which is clearly incorrect."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,	
		FW_BUG "No valid trip found",
		"No valud ACPI _CRT (Critical Trip Point) was found."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		FW_BUG "_BCQ is usef instead of _BQC",
		"ACPI Method _BCQ was defined (typo) instead of _BQC - this should be fixed."
		"however the kernel has detected this and is working around this typo."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"defines _DOD but not _DOS", 
		"ACPI Method _DOD (Enumerate all devices attached to display adapter) "
		"is defined but we should also have _DOS (Enable/Disable output switching) "
		"defined but it's been omitted. This can cause display switching issues."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_MEDIUM,
		FW_BUG "Duplicate ACPI video bus",
		"Try video module parameter video.allow_duplicates=1 if the current driver does't work."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "PCI.*MMCONFIG.*not reserved in ACPI motherboard resources",
		"It appears that PCI config space has been configured for a specific device "
		"but does not appear to be reserved by the ACPI motherboard resources."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		FW_BUG "PCI.*not reserved in ACPI motherboard resources",
		"PCI firmware bug. Please see the kernel log for more details."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		FW_BUG,			/* Fall through to something vague */
		"The kernel has detected a Firmware bug in the BIOS or ACPI which needs investigating and fixing."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_MEDIUM,
		"PCI.*BIOS Bug:",
		NULL
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"ACPI Error.*Namespace lookup failure, AE_NOT_FOUND",
		"The kernel has detected an error trying to execute an Method and it cannot "
		"find an object. This is indicates a bug in the DSDT or SSDT AML code."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"ACPI Error.+psparse",
		"The ACPI parser has failed in executing some AML. "
		"The error message above lists the method that caused this error."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"ACPI Error",
		"The kernel has most probably detected an error while executing ACPI AML. "
		"The error lists the ACPI driver module and the line number where the "
		"bug has been caught and the method that caused the error."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"\\*\\*\\* Error.*Method execution failed",
		"Execution of an ACPI AML method failed."
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"\\*\\*\\* Error.*Method execution failed.*AE_AML_METHOD_LIMIT",
		"ACPI method reached maximum reentrancy limit of 255 - infinite recursion in AML in DSTD or SSDT",
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_CRITICAL,
		"\\*\\*\\* Error.*Method reached maximum reentrancy limit"
		"ACPI method has reached reentrancy limit, this is a recursion bug in the AML"
	},
	{
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"\\*\\*\\* Error.*Return object type is incorrect",
		"Return object type is not the correct type, this is an AML error in the DSDT or SSDT"
	},
	{
		FWTS_COMPARE_STRING,
		0,
		NULL,
		NULL
	}
};


/*
 *  Power management warnings
 */
static fwts_klog_pattern pm_error_warning_patterns[] = {
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Failed to prepare device",
		"dpm_prepare() failed to prepare all non-sys devices for "
		"a system PM transition. The device should be listed in the "
		"error message."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Some devices failed to power down",
		"dpm_suspend_noirq failed because some devices did not power down "
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Some system devices failed to power down",
		"sysdev_suspend failed because some system devices did not power down."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Error",
		NULL
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Some devices failed to power down",
		NULL
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Restore failed, recovering", 
		"A resume from hibernate failed when calling hibernation_restore()"
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Resume from disk failed",
		NULL
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Not enough free memory",
		"There was not enough physical memory to be able to "
		"generate a hibernation image before dumping it to disc."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Memory allocation failed",
		"swusp_alloc() failed trying to allocate highmem and failing "
		"that non-highmem pages for the suspend image. There is "
		"probably just not enough free physcial memory available."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Image mismatch",
		"Mismatch in kernel version, system type, kernel release "
		"version or machine id between suspended kernel and resumed "
		"kernel."
	},
        { 
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
	  	"PM: Some devices failed to power down",
		NULL,
	},
        { 
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Some devices failed to suspend", 
		NULL,
	},
        { 
		FWTS_COMPARE_STRING,
		LOG_LEVEL_MEDIUM,
		"PM: can't read",
		"Testing suspend cannot read RTC"
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_MEDIUM,
		"PM: can't set",
		"Testing suspend cannot set RTC"
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: suspend test failed, error",
		NULL
	},
        {	
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: can't test ", 						
		NULL
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_MEDIUM,
		"PM: no wakealarm-capable RTC driver is ready",
		NULL
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Adding page to bio failed at",
		"An attempt to write the hibernate image to disk failed "
		"because a write BIO operation failed. This is usually "
		"a result of some physical hardware problem."
	},
        {	
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Swap header not found", 
		"An attempt to write a hibernate image to disk failed because "
		"a valid swap device header could not be found. Make sure there is a "
		"formatted swap device on the machine and it is added using swapon -a."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Cannot find swap device", 
		"An attempt to write a hibernate image to disk failed because "
		"the swap device could not be found. Make sure there is a "
		"formatted swap device on the machine and it is added using swapon -a."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_CRITICAL,
		"PM: Not enough free swap",
		"Hibernate failed because the swap parition was probably too small."
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Image device not initialised",
		NULL
	},
        {
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"PM: Please power down manually",
		NULL
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"check_for_bios_corruption", 
		"The BIOS seems to be corrupting the first 64K of memory "
		"when doing suspend/resume. Setting bios_corruption_check=0 "
		"will disable this check."
	},
	{	
		FWTS_COMPARE_REGEX,
		LOG_LEVEL_HIGH,
		"WARNING: at.*hpet_next_event",
		"Possibly an Intel I/O controller hub HPET Write Timing issue: "
		"A read transaction that immediately follows a write transaction "
		"to the HPET TIMn_COMP Timer 0 (108h), HPET MAIN_CNT (0F0h), or "
		"TIMn_CONF.bit 6 (100h) may return an incorrect value.  This is "
		"known to cause issues when coming out of S3."
	},
	{
		FWTS_COMPARE_STRING,
		LOG_LEVEL_HIGH,
		"BUG: soft lockup.*stuck for 0s",
		"Softlock errors that occur when coming out of S3 may be tripped "
		"by TSC warping.  It may be worth trying the notsc kernel parameter "
		"and repeating S3 tests to see if this solves the problem."
	},
	{	
		FWTS_COMPARE_STRING,
		0,
		NULL,
		NULL
	}

};

void fwts_klog_scan_patterns(fwts_framework *fw, char *line, char *prevline, void *private, int *errors)
{
	fwts_klog_pattern *pattern = (fwts_klog_pattern *)private;
	int vector[1];
	static char *advice = 
		"This is a bug picked up by the kernel, but as yet, the "
		"firmware test suite has no diagnostic advice for this particular problem.";

	while (pattern->pattern != NULL) {
		switch (pattern->mode) {
		case FWTS_COMPARE_REGEX:
			if (pcre_exec(pattern->re, NULL, line, strlen(line), 0, 0, vector, 1) == 0) {
				fwts_failed_level(fw, pattern->level, "Kernel message: %s", line);
				(*errors)++;
				if (pattern->advice != NULL)
					fwts_advice(fw, "%s", pattern->advice);
				else
					fwts_advice(fw, advice);
				return;
			}
			break;
		case FWTS_COMPARE_STRING:
		default:
			if (strstr(line, pattern->pattern) != NULL) {
				fwts_failed_level(fw, pattern->level, "Kernel message: %s", line);
				(*errors)++;
				if (pattern->advice != NULL)
					fwts_advice(fw, "%s", pattern->advice);
				else
					fwts_advice(fw, advice);
				return;	
			}
		}
		pattern++;
	}
}

static void fwts_klog_compile(fwts_framework *fw, fwts_klog_pattern *pattern)
{
	const char *error;
	int erroffset;

	while (pattern->pattern != NULL) {
		if (pattern->re == NULL)
			if ((pattern->re = pcre_compile(pattern->pattern, 0, &error, &erroffset, NULL)) == NULL)
				fwts_log_error(fw, "Regex %s failed to compile: %s.", pattern->pattern, error);
		pattern++;
	}
}

static void fwts_klog_compile_free(fwts_klog_pattern *pattern)
{
	while (pattern->pattern != NULL) {
		pcre_free(pattern->re);
		pattern->re = NULL;
		pattern++;
	}
}

static int fwts_klog_check(fwts_framework *fw, fwts_klog_pattern *pattern, fwts_klog_progress_func progress, fwts_list *klog, int *errors)
{
	int ret;

	fwts_klog_compile(fw, pattern);
	ret = fwts_klog_scan(fw, klog, fwts_klog_scan_patterns, progress, pattern, errors);
	fwts_klog_compile_free(pattern);
	
	return ret;
}

int fwts_klog_firmware_check(fwts_framework *fw, fwts_klog_progress_func progress, fwts_list *klog, int *errors)
{	
	return fwts_klog_check(fw, firmware_error_warning_patterns, progress, klog, errors);
}

int fwts_klog_pm_check(fwts_framework *fw, fwts_klog_progress_func progress, fwts_list *klog, int *errors)
{
	return fwts_klog_check(fw, pm_error_warning_patterns, progress, klog, errors);
}

int fwts_klog_common_check(fwts_framework *fw, fwts_klog_progress_func progress, fwts_list *klog, int *errors)
{
	return fwts_klog_check(fw, common_error_warning_patterns, progress, klog, errors);
}

static void fwts_klog_regex_find_callback(fwts_framework *fw, char *line, char *prev, void *pattern, int *match)
{
	const char *error;
	int erroffset;
	pcre *re;
	int rc;
	int vector[1];

	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (re != NULL) {
		rc = pcre_exec(re, NULL, line, strlen(line), 0, 0, vector, 1);
		if (rc == 0)
			(*match)++;
		pcre_free(re);
	}
}

int fwts_klog_regex_find(fwts_framework *fw, fwts_list *klog, char *pattern)
{
	int found = 0;

	fwts_klog_scan(fw, klog, fwts_klog_regex_find_callback, NULL, pattern, &found);

	return found;
}
