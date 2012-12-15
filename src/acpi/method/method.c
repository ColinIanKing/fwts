/*
 * Copyright (C) 2010-2012 Canonical
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>

/* acpica headers */
#include "acpi.h"
#include "fwts_acpi_method.h"

/*
 * ACPI methods + objects used in Linux ACPI driver:
 *
 * Name	 APCI Spec.	Tested
 *	 section
 * _ACx  11.4.1		Y
 * _ADR  6.1.1, B.6.1,	Y
 * _AEI  5.6.5.2	Y
 * _ALx  11.4.2		N
 * _ALC  9.2.5		Y
 * _ALI  9.2.2		Y
 * _ALP  9.2.6		Y
 * _ALR  9.2.5		N
 * _ALT  9.2.3		Y
 * _ART  11.4.3		N
 * _BBN  6.5.5		Y
 * _BCL  B.6.2 		Y
 * _BCM  B.6.3		Y
 * _BCT  10.2.29	Y
 * _BDN  6.5.3		Y
 * _BFS  7.3.1		deprecated
 * _BIF  10.2.2.1	Y
 * _BIX  10.2.2.2	Y
 * _BLT  9.1.3		N
 * _BMA  10.2.2.4	Y
 * _BMC	 10.2.2.11	Y
 * _BMD  10.2.2.10	Y
 * _BMS  10.2.2.5	Y
 * _BQC  B.6.4		Y
 * _BST  10.2.2.6	Y
 * _BTM  10.2.2.8	N
 * _BTP  10.2.2.7	Y
 * _CBA  see PCI spec	N
 * _CDM  6.2.1		N
 * _CID  6.1.2		N
 * _CLS  6.1.3		N
 * _CPC  8.4.5		Y
 * _CRS  6.2.2		Y
 * _CRT  11.4.4		Y
 * _CSD  8.4.2.2	Y
 * _CST  8.4.2.1	N
 * _CWS  9.18.6		N
 * _DCK  6.5.2		Y
 * _DCS  B.6.6		Y
 * _DDC  B.6.5		Y
 * _DDN  6.1.4		Y
 * _DEP  6.5.8		N
 * _DGS  B.6.7		Y
 * _DIS  6.2.3		Y
 * _DLM  5.7.5		N
 * _DMA  6.2.4		Y
 * _DOD  B.4.2		Y
 * _DOS  B.4.1		Y
 * _DSM  9.14.1		N
 * _DSS  B.6.8		Y
 * _DSW  7.2.1		Y
 * _Exx  5.6.4.1	n/a
 * _EC   1.12		n/a
 * _EDL  6.3.1		Y
 * _EJD  6.3.2		Y
 * _EJx  6.3.3		Y
 * _EVT  5.6.5.3	N
 * _FDE  9.9.1		N
 * _FDI  9.9.2		N
 * _FDM  9.9.3		N
 * _FIF  11.3.1.1	Y
 * _FIX  6.2.5		N
 * _FPS  11.3.1.2	N
 * _FSL  11.3.1.3	Y
 * _FST  11.3.1.4	Y
 * _GAI  10.4.5		N
 * _GCP  9.18.2		Y
 * _GHL  10.4.7		N
 * _GL   5.7.1		N
 * _GLK  6.5.7		n/a
 * _GPD  B.4.4		Y
 * _GPE  5.3.1, 12.11	N
 * _GRT  9.18.3		Y
 * _GSB  6.2.6		Y
 * _GTF  9.8.1.1	N
 * _GTM  9.8.2.1.1	N
 * _GTS  7.3.3		deprecated
 * _GWS  9.18.5		Y
 * _HID  6.1.5		Y
 * _HOT  11.4.6		Y
 * _HPP  6.2.7		N
 * _HPX  6.2.8		N
 * _HRV  6.1.6		N
 * _IFT  19.5		N
 * _INI  6.5.1		N
 * _INT  19.1.8		N
 * _IRC  7.2.13		Y
 * _Lxx  5.6.4.1	n/a
 * _LCK  6.3.4		Y
 * _LID  9.4.1		Y
 * _MAT  6.2.9		N
 * _MBM  9.12.2.1	N
 * _MLS  6.1.7		N
 * _MSG  9.1.2		N
 * _MSM  9.12.2.2	N
 * _NTT  11.4.7		Y
 * _OFF  7.1.2		Y
 * _ON_  7.1.3		Y
 * _OS   5.7.3		N
 * _OSC  6.2.10		n/a
 * _OSI  5.7.2		n/a
 * _OST  6.3.5		N
 * _PAI  10.4.4		N
 * _PCL  10.3.2		N
 * _PCT  8.4.4.1	Y
 * _PDC  8.4.1		N
 * _PDL  8.4.4.6	Y
 * _PIC  5.8.1		N
 * _PIF  10.3.3		Y
 * _PLD  6.1.8		Y
 * _PMC  10.4.1		N
 * _PMD  10.4.8		N
 * _PMM  10.4.3		N
 * _PPC  8.4.4.3	Y
 * _PPE  8.4.6		Y
 * _PR   5.3.1		N
 * _PR0  7.2.8		Y
 * _PR1	 7.2.9		Y
 * _PR2  7.2.10		Y
 * _PR3  7.2.11		Y
 * _PRE  7.2.12		Y
 * _PRL  10.3.4		N
 * _PRS  6.2.11		N
 * _PRW  7.2.11		N
 * _PS0  7.2.2		Y
 * _PS1  7.2.3		Y
 * _PS2  7.2.4		Y
 * _PS3  7.2.5		Y
 * _PSC  7.2.6		Y
 * _PSD  8.4.4.5	N
 * _PSE  7.2.7		Y
 * _PSL  11.4.8		N
 * _PSR  10.3.1		Y
 * _PSS  8.4.4.2	Y
 * _PSV  11.4.9		Y
 * _PSW  7.2.12		Y
 * _PTC  8.4.3.1	N
 * _PTP  10.4.2		N
 * _PTS  7.3.2		Y
 * _PUR  8.5.11		N
 * _PXM  6.2.13 	Y
 * _Qxx  5.6.4.1	n/a
 * _REG  6.5.4		N
 * _REV  5.7.4		N
 * _RMV  6.3.6		Y
 * _ROM  B.4.3		Y
 * _RTV  11.4.10	Y
 * _S0_  7.3.4.1	N
 * _S1_  7.3.4.2	N
 * _S2_  7.3.4.3	N
 * _S3_  7.3.4.4	N
 * _S4_  7.3.4.5	N
 * _S5_  7.3.4.6	N
 * _S1D  7.2.16		Y
 * _S2D  7.2.17		Y
 * _S3D  7.2.18		Y
 * _S4D  7.2.19		Y
 * _S0W  7.2.20		Y
 * _S1W  7.2.21		Y
 * _S2W	 7.2.22		Y
 * _S3W  7.2.23		Y
 * _S4W  7.2.24		Y
 * _SB_  5.3.1		n/a
 * _SBS  10.1.3		Y
 * _SCP  11.4.11	Y
 * _SDD  9.8.3.3.1	N
 * _SEG  6.5.6		Y
 * _SHL  10.4.5		N
 * _SLI  6.2.14		N
 * _SPD  B.4.5		Y
 * _SRS  6.2.15		N
 * _SRT  9.18.4		N
 * _SST  9.1.1		N
 * _STA  6.3.7, 7.1.4	Y
 * _STM  9.8.2.1.2	N
 * _STP  9.18.7		N
 * _STR  6.1.9		Y
 * _STV  9.18.8		N
 * _SUB  6.1.9		Y
 * _SUN  6.1.8		N
 * _SWS  7.3.5		N
 * _T_x  18.2.1.1	n/a
 * _TC1  11.4.12	Y
 * _TC2  11.4.13	Y
 * _TDL  8.4.3.5	Y
 * _TIP  9.18.9		N
 * _TIV  9.18.10	N
 * _TMP  11.4.14	Y
 * _TPC  8.4.3.3	Y
 * _TPT  11.4.15	Y
 * _TRT  11.4.16	N
 * _TSD  8.4.3.4	Y
 * _TSP  11.4.17	Y
 * _TSS  8.4.3.2	Y
 * _TST  11.4.18	Y
 * _TTS  7.3.6		Y
 * _TZ_  5.3.1		N
 * _TZD  11.4.19	N
 * _TZP  11.4.21	Y
 * _TZM  11.4.20	N
 * _UID  6.1.9		Y
 * _UPC  9.13		N
 * _UPD  9.16.1		N
 * _UPP  9.16.2		N
 * _VPO  B.4.6		Y
 * _WAK  7.3.7 		Y
 * _Wxx  5.6.4.2.2	N
 * _WDG  WMI		N
 * _WED  WMI		N
 */

/* Test types */
#define	METHOD_MANDITORY	1
#define METHOD_OPTIONAL		2
#define METHOD_MOBILE		4

#define method_check_type(fw, name, buf, type) 			\
	method_check_type__(fw, name, buf, type, #type)

static bool fadt_mobile_platform;	/* True if a mobile platform */

#define method_test_integer(name, type)				\
static int method_test ## name(fwts_framework *fw)		\
{ 								\
	return method_evaluate_method(fw, type, # name,		\
		NULL, 0, method_test_integer_return, # name); 	\
}

typedef void (*method_test_return)(fwts_framework *fw, char *name,
	ACPI_BUFFER *ret_buff, ACPI_OBJECT *ret_obj, void *private);

/*
 * Helper functions to facilitate the evaluations
 */

/****************************************************************************/

/*
 *  method_init()
 *	initialize ACPI
 */
static int method_init(fwts_framework *fw)
{
	fwts_acpi_table_info *info;
	int i;
	bool got_fadt = false;

	fadt_mobile_platform = false;

	/* Some systems have multiple FADTs, sigh */
	for (i = 0; i < 256; i++) {
		int ret = fwts_acpi_find_table(fw, "FACP", i, &info);
		if (ret == FWTS_NULL_POINTER || info == NULL)
			break;
		fwts_acpi_table_fadt *fadt = (fwts_acpi_table_fadt*)info->data;
		got_fadt = true;
		if (fadt->preferred_pm_profile == 2) {
			fadt_mobile_platform = true;
			break;
		}
	}

	if (got_fadt && !fadt_mobile_platform) {
		fwts_log_info(fw,
			"FADT Preferred PM profile indicates this is not "
			"a Mobile Platform.");
	}

	if (fwts_acpi_init(fw) != FWTS_OK) {
		fwts_log_error(fw, "Cannot initialise ACPI.");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

/*
 *  method_deinit
 *	de-intialize ACPI
 */
static int method_deinit(fwts_framework *fw)
{
	return fwts_acpi_deinit(fw);
}

/*
 *  method_evaluate_found_method
 *	find a given object name and evaluate it
 */
static void method_evaluate_found_method(
	fwts_framework *fw,
	char *name,
	method_test_return check_func,
	void *private,
	ACPI_OBJECT_LIST *arg_list)
{
	ACPI_BUFFER       buf;
	ACPI_STATUS	  ret;
	int sem_acquired;
	int sem_released;

	fwts_acpica_sem_count_clear();

	ret = fwts_acpi_object_evaluate(fw, name, arg_list, &buf);

	if (ACPI_FAILURE(ret) != AE_OK) {
		fwts_acpi_object_evaluate_report_error(fw, name, ret);
	} else {
		if (check_func != NULL) {
			ACPI_OBJECT *obj = buf.Pointer;
			check_func(fw, name, &buf, obj, private);
		}
	}
	if (buf.Length && buf.Pointer)
		free(buf.Pointer);

	fwts_acpica_sem_count_get(&sem_acquired, &sem_released);
	if (sem_acquired != sem_released) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "AMLLocksAcquired",
			"%s left %d locks in an acquired state.",
			name, sem_acquired - sem_released);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_MUTEX);
		fwts_advice(fw,
			"Locks left in an acquired state generally indicates "
			"that the AML code is not releasing a lock. This can "
			"sometimes occur when a method hits an error "
			"condition and exits prematurely without releasing an "
			"acquired lock. It may be occurring in the method "
			"being tested or other methods used while evaluating "
			"the method.");
	}
}

/*
 *  method_evaluate_method
 *	find all matching object names and evaluate them,
 *	also run the callback check_func to sanity check
 *	any returned values
 */
static int method_evaluate_method(fwts_framework *fw,
	int test_type,  /* Manditory or optional */
	char *name,
	ACPI_OBJECT *args,
	int num_args,
	method_test_return check_func,
	void *private)
{
	fwts_list_link	*item;
	fwts_list *methods;
	size_t name_len = strlen(name);
	int found = 0;

	if ((methods = fwts_acpi_object_get_names()) != NULL) {
		fwts_list_foreach(item, methods) {
			char *method_name = fwts_list_data(char*, item);
			size_t len = strlen(method_name);
			if (strncmp(name, method_name + len - name_len, name_len) == 0) {
				ACPI_OBJECT_LIST  arg_list;

				found++;
				arg_list.Count   = num_args;
				arg_list.Pointer = args;
				fwts_acpica_simulate_sem_timeout(FWTS_FALSE);
				method_evaluate_found_method(fw, method_name,
					check_func, private, &arg_list);
			}
		}
	}

	if (found) {
		if ((test_type & METHOD_MOBILE) && (!fadt_mobile_platform)) {
			fwts_warning(fw,
				"The FADT indictates that this machine is not "
				"a mobile platform, however it has a mobile "
				"platform specific object %s defined. "
				"Either the FADT referred PM profile is "
				"incorrect or this machine has mobile "
				"platform objects defined when it should not.",
				name);
		}
		return FWTS_OK;
	} else {
		/* Manditory not-found test are a failure */
		if (test_type & METHOD_MANDITORY) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodNotExist",
				"Object %s did not exist.", name);
		}

		/* Mobile specific tests on non-mobile platform? */
		if ((test_type & METHOD_MOBILE) && (!fadt_mobile_platform)) {
			fwts_skipped(fw,
				"Machine is not a mobile platform, skipping "
				"test for non-existant mobile platform "
				"related object %s.", name);
		} else {
			fwts_skipped(fw,
				"Skipping test for non-existant object %s.",
				name);
		}

		return FWTS_NOT_EXIST;
	}
}

/*
 *  method_name_check
 *	sanity check object name conforms to ACPI specification
 */
static int method_name_check(fwts_framework *fw)
{
	fwts_list_link	*item;
	fwts_list *methods;
	int failed = 0;

 	if ((methods = fwts_acpi_object_get_names()) != NULL) {
		fwts_log_info(fw, "Found %d Objects\n", methods->len);

		fwts_list_foreach(item, methods) {
			char *ptr;

			for (ptr = fwts_list_data(char *, item); *ptr; ptr++) {
				if (!((*ptr == '\\') ||
				     (*ptr == '.') ||
				     (*ptr == '_') ||
				     (isdigit(*ptr)) ||
				     (isupper(*ptr))) ) {
					fwts_failed(fw, LOG_LEVEL_HIGH,
						"MethodIllegalName",
						"Method %s contains an illegal "
						"character: '%c'. This should "
						"be corrected.",
						fwts_list_data(char *, item),
						*ptr);
					fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD);
					failed++;
					break;
				}
			}
		}
		if (!failed)
			fwts_passed(fw, "Method names contain legal characters.");
	}

	return FWTS_OK;
}

/*
 *  method_check_type__
 *	check returned object type
 */
static int method_check_type__(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT_TYPE type,
	char *type_name)
{
	ACPI_OBJECT *obj;

	if ((buf == NULL) || (buf->Pointer == NULL)){
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodReturnNullObj",
			"Method %s returned a NULL object, and did not "
			"return %s.", name, type_name);
		return FWTS_ERROR;
	}

	obj = buf->Pointer;

	if (obj->Type != type) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodReturnBadType",
			"Method %s did not return %s.", name, type_name);
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

/*
 *  method_test_buffer_return
 *	check if a buffer object was returned
 */
static void method_test_buffer_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_BUFFER) == FWTS_OK)
		fwts_passed(fw, "%s correctly returned a buffer of %" PRIu32 " elements.",
			name, obj->Buffer.Length);
}

/*
 *  method_test_integer_return
 *	check if an integer object was returned
 */
static void method_test_integer_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(obj);
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw, "%s correctly returned an integer.", name);
}

/*
 *  method_test_string_return
 *	check if an string object was returned
 */
static void method_test_string_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(obj);
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_STRING) == FWTS_OK)
		fwts_passed(fw, "%s correctly returned a string.", name);
}

/*
 *  method_test_NULL_return
 *	check if no object was retuned
 */
static void method_test_NULL_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (buf->Length && buf->Pointer) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodShouldReturnNothing", "%s returned values, but was expected to return nothing.", name);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		fwts_log_info(fw, "Object returned:");
		fwts_acpi_object_dump(fw, obj);
		fwts_advice(fw,
			"This probably won't cause any errors, but it should "
			"be fixed as the AML code is not conforming to the "
			"expected behaviour as described in the ACPI "
			"specification.");
	} else
		fwts_passed(fw, "%s returned no values as expected.", name);
}

/*
 *  method_test_passed_failed_return
 *	check if 0 or 1 (false/true) integer is returned
 */
static void method_test_passed_failed_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	char *method = (char *)private;
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		uint32_t val = (uint32_t)obj->Integer.Value;
		if ((val == 0) || (val == 1))
			fwts_passed(fw,
				"%s correctly returned sane looking value "
				"0x%8.8" PRIx32 ".", method, val);
		else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"MethodReturnZeroOrOne",
				"%s returned 0x%8.8" PRIx32 ", should return 1 "
				"(success) or 0 (failed).", method, val);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw,
				"Method %s should be returning the correct "
				"1/0 success/failed return values. "
				"Unexpected behaviour may occur becauses of "
				"this error, the AML code does not conform to "
				"the ACPI specification and should be fixed.",
				method);
		}
	}
}

/*
 *  method_test_polling_return
 *	check if a returned polling time is valid
 */
static void method_test_polling_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		char *method = (char *)private;
		if (obj->Integer.Value < 36000) {
			fwts_passed(fw,
				"%s correctly returned sane looking value "
				"%f seconds", method,
				(float)obj->Integer.Value / 10.0);
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"MethodPollTimeTooLong",
				"%s returned a value %f seconds > (1 hour) "
				"which is probably incorrect.",
				method, (float)obj->Integer.Value / 10.0);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw,
				"The method is returning a polling interval "
				"which is very long and hence most probably "
				"incorrect.");
		}
	}
}

/****************************************************************************/

/*
 * Section 5.6 ACPI Event Programming Model
 */
static int method_test_AEI(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_AEI", NULL, 0, method_test_buffer_return, NULL);
}

/*
 * Section 6.1 Device Identification Objects
 */
static int method_test_DDN(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DDN", NULL, 0, method_test_string_return, NULL);
}

static bool method_valid_HID_string(char *str)
{
	if (strlen(str) == 7) {
		/* PNP ID, must be 3 capitals followed by 4 hex */
		if (!isupper(str[0]) ||
		    !isupper(str[1]) ||
		    !isupper(str[2])) return false;
		if (!isxdigit(str[3]) ||
		    !isxdigit(str[4]) ||
		    !isxdigit(str[5]) ||
		    !isxdigit(str[6])) return false;
		return true;
	}

	if (strlen(str) == 8) {
		/* ACPI ID, must be 4 capitals or digits followed by 4 hex */
		if ((!isupper(str[0]) && !isdigit(str[0])) ||
		    (!isupper(str[1]) && !isdigit(str[1])) ||
		    (!isupper(str[2]) && !isdigit(str[2])) ||
		    (!isupper(str[3]) && !isdigit(str[3]))) return false;
		if (!isxdigit(str[4]) ||
		    !isxdigit(str[5]) ||
		    !isxdigit(str[6]) ||
		    !isxdigit(str[7])) return false;
		return true;
	}

	return false;
}

static bool method_valid_EISA_ID(uint32_t id, char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%c%c%c%02" PRIX32 "%02" PRIX32,
		0x40 + ((id >> 2) & 0x1f),
		0x40 + ((id & 0x3) << 3) + ((id >> 13) & 0x7),
		0x40 + ((id >> 8) & 0x1f),
		(id >> 16) & 0xff, (id >> 24) & 0xff);

	/* 3 chars in EISA ID must be upper case */
	if (!isupper(buf[0]) ||
	    !isupper(buf[1]) ||
	    !isupper(buf[2])) return false;

	/* Last 4 digits are always going to be hex, so pass */
	return true;
}

static void method_test_HID_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	char tmp[8];

	FWTS_UNUSED(buf);
	FWTS_UNUSED(private);

	if (obj == NULL) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodReturnNullObj",
			"Method %s returned a NULL object, and did not "
			"return a buffer or integer.", name);
		return;
	}
	switch (obj->Type) {
	case ACPI_TYPE_STRING:
		if (obj->String.Pointer) {
			if (method_valid_HID_string(obj->String.Pointer))
				fwts_passed(fw,
					"Object _HID returned a string '%s' "
					"as expected.",
					obj->String.Pointer);
			else
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"MethodHIDInvalidString",
					"Object _HID returned a string '%s' "
					"but it was not a valid PNP ID or a "
					"valid ACPI ID.",
					obj->String.Pointer);
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_HIDNullString",
				"Object _HID returned a NULL string.");
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
		break;
	case ACPI_TYPE_INTEGER:
		if (method_valid_EISA_ID((uint32_t)obj->Integer.Value,
			tmp, sizeof(tmp)))
			fwts_passed(fw, "Object _HID returned an integer "
				"0x%8.8" PRIx64 " (EISA ID %s).",
				obj->Integer.Value,
				tmp);
		else
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"MethodHIDInvalidInteger",
				"Object _HID returned a integer 0x%8.8" PRIx64 " "
				"(EISA ID %s) but the this is not a valid "
				"EISA ID encoded PNP ID.",
				obj->Integer.Value,
				tmp);
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_HIDBadReturnType",
			"Method _HID did not return a string or an integer.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		break;
	}
}

static int method_test_HID(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_HID", NULL, 0, method_test_HID_return, NULL);
}

static int method_test_HRV(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_HRV", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_STR(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_STR", NULL, 0, method_test_string_return, NULL);
}

static void method_test_PLD_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* All elements in the package must be buffers */
	for (i = 0; i < obj->Package.Count; i++) {
		if (obj->Package.Elements[i].Type != ACPI_TYPE_BUFFER) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PLDElementType",
				"_PLD package element %" PRIu32 " was not a buffer.",
				i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
		/* We should sanity check the PLD further */
	}
}

static int method_test_PLD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PLD", NULL, 0, method_test_PLD_return, NULL);
}

static void method_test_SUB_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(buf);
	FWTS_UNUSED(private);

	if (obj == NULL) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodReturnNullObj",
			"Method %s returned a NULL object, and did not "
			"return a buffer or integer.", name);
		return;
	}
	if (obj->Type == ACPI_TYPE_STRING)
		if (obj->String.Pointer) {
			if (method_valid_HID_string(obj->String.Pointer))
				fwts_passed(fw,
					"Object _SUB returned a string '%s' "
					"as expected.",
					obj->String.Pointer);
			else
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"MethodSUBInvalidString",
					"Object _SUB returned a string '%s' "
					"but it was not a valid PNP ID or a "
					"valid ACPI ID.",
					obj->String.Pointer);
		} else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_SUBNullString",
				"Object _SUB returned a NULL string.");
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
	else {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_UIDBadReturnType",
			"Method _SUB did not return a string or an integer.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
	}
}


static int method_test_SUB(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_SUB", NULL, 0, method_test_SUB_return, NULL);
}

static int method_test_SUN(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_SUN", NULL, 0, method_test_integer_return, NULL);
}

static void method_test_UID_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(buf);
	FWTS_UNUSED(private);

	if (obj == NULL){
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "MethodReturnNullObj",
			"Method %s returned a NULL object, and did not "
			"return a buffer or integer.", name);
		return;
	}
	switch (obj->Type) {
	case ACPI_TYPE_STRING:
		if (obj->String.Pointer)
			fwts_passed(fw,
				"Object _UID returned a string '%s' as expected.",
				obj->String.Pointer);
		else {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_UIDNullString",
				"Object _UID returned a NULL string.");
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
		break;
	case ACPI_TYPE_INTEGER:
		fwts_passed(fw, "Object _UID returned an integer 0x%8.8" PRIx64 ".",
			obj->Integer.Value);
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_UIDBadReturnType",
			"Method _UID did not return a string or an integer.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		break;
	}
}

static int method_test_UID(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_UID", NULL, 0, method_test_UID_return, NULL);
}


/*
 *  Section 6.2 Device Configurations Objects
 */
static int method_test_CRS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MANDITORY,
		"_CRS", NULL, 0, method_test_buffer_return, NULL);
}

static int method_test_DMA(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DMA", NULL, 0, method_test_buffer_return, NULL);
}

static int method_test_DIS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DIS", NULL, 0, method_test_NULL_return, NULL);
}

static int method_test_GSB(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_GSB", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_PXM(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PXM", NULL, 0, method_test_integer_return, NULL);
}


/*
 * Section 6.3 Device Insertion, Removal and Status Objects
 */
static void method_test_EDL_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* All elements in the package must be references */
	for (i = 0; i < obj->Package.Count; i++) {
		if (obj->Package.Elements[i].Type != ACPI_TYPE_LOCAL_REFERENCE) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_EDLElementType",
				"_EDL package element %" PRIu32 " was not a reference.",
				i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
	}
}

static int method_test_EDL(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_EDL", NULL, 0, method_test_EDL_return, NULL);
}

static int method_test_EJD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_EJD", NULL, 0, method_test_string_return, NULL);
}

#define method_test_EJx(name)					\
static int method_test ## name(fwts_framework *fw)		\
{								\
	ACPI_OBJECT arg[1];					\
								\
	arg[0].Type = ACPI_TYPE_INTEGER;			\
	arg[0].Integer.Value = 1;				\
								\
	return method_evaluate_method(fw, METHOD_OPTIONAL,	\
		# name, arg, 1, method_test_NULL_return, # name); \
}

method_test_EJx(_EJ0)
method_test_EJx(_EJ1)
method_test_EJx(_EJ2)
method_test_EJx(_EJ3)
method_test_EJx(_EJ4)

static int method_test_LCK(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_LCK", arg, 1, method_test_NULL_return, NULL);
}

static int method_test_RMV(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_RMV",
		NULL, 0, method_test_passed_failed_return, "_RMV");
}

static void method_test_STA_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if ((obj->Integer.Value & 3) == 2) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_STAEnabledNotPresent",
				"_STA indicates that the device is enabled "
				"but not present, which is impossible.");
			failed = true;
		}
		if ((obj->Integer.Value & ~0x1f) != 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_STAReservedBitsSet",
				"_STA is returning non-zero reserved "
				"bits 5-31. These should be zero.");
			failed = true;
		}

		if (!failed)
			fwts_passed(fw,
				"_STA correctly returned sane looking "
				"value 0x%8.8" PRIx64, obj->Integer.Value);
	}
}

static int method_test_STA(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_STA",
		NULL, 0, method_test_STA_return, "_STA");
}


/*
 * Section 6.5 Other Objects and Controls
 */
static int method_test_BBN(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_BBN",
		NULL, 0, method_test_integer_return, "_BBN");
}

static int method_test_BDN(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE, "_BDN",
		NULL, 0, method_test_integer_return, "_BDN");
}

static int method_test_DCK(fwts_framework *fw)
{
	int i;

	for (i = 0; i <= 1; i++) {	/* Undock, Dock */
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;
		if (method_evaluate_method(fw, METHOD_MOBILE, "_DCK", arg,
			1, method_test_passed_failed_return, "_DCK") != FWTS_OK)
			break;
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static void method_test_SEG_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if ((obj->Integer.Value & 0xffff0000)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_SEGIllegalReserved",
				"_SEG returned value 0x%8.8" PRIx64 " and some of the "
				"upper 16 reserved bits are set when they "
				"should in fact be zero.",
				obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else
			fwts_passed(fw,
				"_SEG correctly returned sane looking "
				"value 0x%8.8" PRIx64, obj->Integer.Value);
	}
}

static int method_test_SEG(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_SEG",
		NULL, 0, method_test_SEG_return, "_SEG");
}

/*
 * Section 7.1 Declaring a Power Resource Object
 */
static int method_test_ON(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_ON", NULL, 0, method_test_NULL_return, NULL);
}

static int method_test_OFF(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_OFF", NULL, 0, method_test_NULL_return, NULL);
}


/*
 * Section 7.2  Device Power Management Objects
 */
static int method_test_DSW(fwts_framework *fw)
{
	ACPI_OBJECT arg[3];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 0;
	arg[2].Type = ACPI_TYPE_INTEGER;
	arg[2].Integer.Value = 3;

	return method_evaluate_method(fw, METHOD_OPTIONAL, "_DSW",
		arg, 3, method_test_NULL_return, NULL);
}

#define method_test_PSx(name)						\
static int method_test ## name(fwts_framework *fw)			\
{									\
	return method_evaluate_method(fw, METHOD_OPTIONAL,		\
		# name, NULL, 0, method_test_NULL_return, # name);	\
}

method_test_PSx(_PS0)
method_test_PSx(_PS1)
method_test_PSx(_PS2)
method_test_PSx(_PS3)

static int method_test_PSC(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PSC", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_PSE(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PSE", arg, 1, method_test_NULL_return, NULL);
}

static void method_test_power_resources_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* All elements in the package must be references */
	for (i = 0; i < obj->Package.Count; i++) {
		if (obj->Package.Elements[i].Type != ACPI_TYPE_LOCAL_REFERENCE) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PowerResourceElementType",
				"%s package element %" PRIu32 " was not a reference.",
				name, i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
	}
}

#define method_test_POWER(name)						\
static int method_test ## name(fwts_framework *fw)			\
{									\
	return method_evaluate_method(fw, METHOD_OPTIONAL,		\
		# name, NULL, 0, method_test_power_resources_return, # name);\
}

method_test_POWER(_PR0)
method_test_POWER(_PR1)
method_test_POWER(_PR2)
method_test_POWER(_PR3)
method_test_POWER(_PRE)

static int method_test_PSW(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PSW", arg, 1, method_test_NULL_return, NULL);
}

#define method_test_SxD(name)						\
static int method_test ## name(fwts_framework *fw)			\
{									\
	return method_evaluate_method(fw, METHOD_OPTIONAL,		\
		# name, NULL, 0, method_test_integer_return, # name);	\
}

method_test_SxD(_S1D)
method_test_SxD(_S2D)
method_test_SxD(_S3D)
method_test_SxD(_S4D)

#define method_test_SxW(name)						\
static int method_test ## name(fwts_framework *fw)			\
{									\
	return method_evaluate_method(fw, METHOD_OPTIONAL,		\
		# name, NULL, 0, method_test_integer_return, # name);	\
}

method_test_SxW(_S0W)
method_test_SxW(_S1W)
method_test_SxW(_S2W)
method_test_SxW(_S3W)
method_test_SxW(_S4W)

static int method_test_IRC(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_IRC", NULL, 0, method_test_NULL_return, NULL);
}

/*
 * Section 7.3 OEM Supplied System-Level Control Methods
 */
static void method_test_Sx__return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/*
	 * The ACPI spec states it should have 1 integer, with the
	 * values packed into each byte. However, nearly all BIOS
	 * vendors don't do this, instead they return a package of
	 * 2 or more integers with each integer lower byte containing
	 * the data we are interested in. The kernel handles this
	 * the non-compliant way. Doh. See drivers/acpi/acpica/hwxface.c
	 * for the kernel implementation and also
	 * source/components/hardware/hwxface.c in the reference ACPICA
	 * sources.
 	 */

	/* Something is really wrong if we don't have any elements in _Sx_ */
	if (obj->Package.Count < 1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "Method_SxElementCount",
			"The kernel expects a package of at least two "
			"integers, and %s only returned %" PRIu32
			" elements in the package.",
			name, obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/*
	 * Oh dear, BIOS is conforming to the spec but won't work in
	 * Linux
	 */
	if (obj->Package.Count == 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_SxElementCount",
			"The ACPI specification states that %s should "
			"return a package of a single integer which "
			"this firmware does do. However, nearly all of the "
			"BIOS vendors return the values in the low 8 bits "
			"in a package of 2 to 4 integers which is not "
			"compliant with the specification BUT is the way "
			"that the ACPICA reference engine and the kernel "
			"expect. So, while this is conforming to the ACPI "
			"specification it will in fact not work in the "
			"Linux kernel.", name);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/* Yes, we really want integers! */
	if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
	    (obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_SxElementType",
			"%s returned a package that did not contain "
			"an integer.", name);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	if (obj->Package.Elements[0].Integer.Value & 0xffffff00) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_SxElementValue",
			"%s package element 0 had upper 24 bits "
			"of bits that were non-zero.", name);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		failed = true;
	}

	if (obj->Package.Elements[1].Integer.Value & 0xffffff00) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_SxElementValue",
			"%s package element 1 had upper 24 bits "
			"of bits that were non-zero.", name);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		failed = true;
	}

	fwts_log_info(fw, "%s PM1a_CNT.SLP_TYP value: 0x%8.8" PRIx64, name,
		obj->Package.Elements[0].Integer.Value);
	fwts_log_info(fw, "%s PM1b_CNT.SLP_TYP value: 0x%8.8" PRIx64, name,
		obj->Package.Elements[1].Integer.Value);

	if (!failed)
		fwts_passed(fw, "%s correctly returned sane looking package.",
			name);
}

#define method_test_Sx_(name)						\
static int method_test ## name(fwts_framework *fw)			\
{									\
	return method_evaluate_method(fw, METHOD_OPTIONAL,		\
		# name, NULL, 0, method_test_Sx__return, # name);	\
}

method_test_Sx_(_S0_)
method_test_Sx_(_S1_)
method_test_Sx_(_S2_)
method_test_Sx_(_S3_)
method_test_Sx_(_S4_)
method_test_Sx_(_S5_)

static int method_test_SWS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_SWS", NULL, 0, method_test_integer_return, NULL);
}

/*
 * Section 8.4 Declaring Processors
 */
static void method_test_type_integer(
	fwts_framework *fw,
	bool *failed,
	ACPI_OBJECT *obj,
	int element,
	char *message)
{
	if (obj->Package.Elements[element].Type  != ACPI_TYPE_INTEGER) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_CPCElementType",
			"_CPC package element %d (%s) was not an integer.",
			element, message);
		*failed = true;
	}
}

static void method_test_type_buffer(
	fwts_framework *fw,
	bool *failed,
	ACPI_OBJECT *obj,
	int element,
	char *message)
{
	if (obj->Package.Elements[element].Type  != ACPI_TYPE_BUFFER) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_CPCElementType",
			"_CPC package element %d (%s) was not a buffer.",
			element, message);
		*failed = true;
	}
}

static void method_test_type_mixed(
	fwts_framework *fw,
	bool *failed,
	ACPI_OBJECT *obj,
	int element,
	char *message)
{
	if ((obj->Package.Elements[element].Type  != ACPI_TYPE_BUFFER) &&
	    (obj->Package.Elements[element].Type  != ACPI_TYPE_BUFFER)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_CPCElementType",
			"_CPC package element %d (%s) was not a buffer "
			"or an integer.", element, message);
		*failed = true;
	}
}

static void method_test_CPC_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* Something is really wrong if we don't have any elements in _PCT */
	if (obj->Package.Count != 17) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_CPCElementCount",
			"_CPC should return package of 17 elements, "
			"got %" PRIu32 " elements instead.",
			obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/* For now, just check types */

	method_test_type_integer(fw, &failed, obj, 0, "NumEntries");
	method_test_type_integer(fw, &failed, obj, 1, "Revision");
	method_test_type_mixed  (fw, &failed, obj, 2, "HighestPerformance");
	method_test_type_mixed  (fw, &failed, obj, 3, "NominalPerformance");
	method_test_type_mixed  (fw, &failed, obj, 4, "LowestNonlinerPerformance");
	method_test_type_mixed  (fw, &failed, obj, 5, "LowestPerformance");
	method_test_type_buffer (fw, &failed, obj, 6, "GuaranteedPerformanceRegister");
	method_test_type_buffer (fw, &failed, obj, 7, "DesiredPerformanceRegister");
	method_test_type_buffer (fw, &failed, obj, 8, "MinimumPerformanceRegister");
	method_test_type_buffer (fw, &failed, obj, 9, "MaximumPerformanceRegister");
	method_test_type_buffer (fw, &failed, obj, 10, "PerformanceReductionToleranceRegister");
	method_test_type_buffer (fw, &failed, obj, 11, "TimeWindowRegister");
	method_test_type_mixed  (fw, &failed, obj, 12, "CounterWraparoundTime");
	method_test_type_mixed  (fw, &failed, obj, 13, "NominalCounterRegister");
	method_test_type_mixed  (fw, &failed, obj, 14, "DeliveredCounterRegister");
	method_test_type_mixed  (fw, &failed, obj, 15, "PerformanceLimitedRegister");
	method_test_type_mixed  (fw, &failed, obj, 16, "EnableRegister");

	if (!failed)
		fwts_passed(fw, "_CPC correctly returned sane looking package.");
}

static int method_test_CPC(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_CPC", NULL,
		0, method_test_CPC_return, NULL);
}

static void method_test_CSD_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* Something is really wrong if we don't have any elements in _CSD */
	if (obj->Package.Count < 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_CSDElementCount",
			"_CSD should return package of at least 1 element, "
			"got %" PRIu32 " elements instead.",
			obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/* Could be one or more packages */
	for (i = 0; i < obj->Package.Count; i++) {
		ACPI_OBJECT *pkg;
		uint32_t j;
		bool elements_ok = true;

		if (obj->Package.Elements[i].Type != ACPI_TYPE_PACKAGE) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_CSDElementType",
				"_CSD package element %" PRIu32 " was not a package.",
				i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}

		pkg = &obj->Package.Elements[i];
		/*
		 *  Currently we expect a package of 6 integers.
		 */
		if (pkg->Package.Count != 6) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_CSDSubPackageElementCount",
				"_CSD sub-package %" PRIu32 " was expected to "
				"have 5 elements, got %" PRIu32 " elements instead.",
				i, pkg->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;
		}

		for (j = 0; j < 6; j++) {
			if (pkg->Package.Elements[j].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_CSDSubPackageElementCount",
					"_CSD sub-package %" PRIu32
					" element %" PRIu32 " is not "
					"an integer.",
					i, j);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				elements_ok = false;
			}
		}

		if (!elements_ok) {
			failed = true;
			continue;
		}

		/* Element 0 must equal the number elements in the package */
		if (pkg->Package.Elements[0].Integer.Value != pkg->Package.Count) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_CSDSubPackageElement0",
				"_CSD sub-package %d element 0 (NumEntries) "
				"was expected to have value 0x%" PRIx64 ".",
				i,
				pkg->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Element 1 should contain zero */
		if (pkg->Package.Elements[1].Integer.Value != 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_CSDSubPackageElement1",
				"_CSD sub-package %d element 1 (Revision) "
				"was expected to have value 1, instead it "
				"was 0x%" PRIx64 ".",
				i,
				pkg->Package.Elements[1].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Element 3 should contain 0xfc..0xfe */
		if ((pkg->Package.Elements[3].Integer.Value != 0xfc) &&
		    (pkg->Package.Elements[3].Integer.Value != 0xfd) &&
		    (pkg->Package.Elements[3].Integer.Value != 0xfe)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_CSDSubPackageElement1",
				"_CSD sub-package %d element 3 (CoordType) "
				"was expected to have value 0xfc (SW_ALL), "
				"0xfd (SW_ANY) or 0xfe (HW_ALL), instead it "
				"was 0x%" PRIx64 ".",
				i,
				pkg->Package.Elements[3].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Element 4 number of processors, skip check */
		/* Element 5 index, check */
	}

	if (!failed)
		fwts_passed(fw,
			"_CSD correctly returned sane looking package.");
}

static int method_test_CSD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_CSD", NULL, 0, method_test_CSD_return, NULL);
}

static void method_test_PCT_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* Something is really wrong if we don't have any elements in _PCT */
	if (obj->Package.Count < 2) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_PCTElementCount",
			"_PCT should return package of least 2 elements, "
			"got %" PRIu32 " elements instead.",
			obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	for (i = 0; i < obj->Package.Count; i++) {
		/*
		 * Fairly shallow checks here, should probably check
		 * for a register description in the buffer
		 */
		if (obj->Package.Elements[i].Type != ACPI_TYPE_BUFFER) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PCTElementType",
				"_PCT package element %" PRIu32
				" was not a buffer.", i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}
	}
	if (!failed)
		fwts_passed(fw,
			"_PCT correctly returned sane looking package.");
}

static int method_test_PCT(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_PCT", NULL,
		0, method_test_PCT_return, NULL);
}

static void method_test_PSS_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;
	bool failed = false;
	uint32_t max_freq = 0;
	uint32_t prev_power = 0;
	bool max_freq_valid = false;
	bool dump_elements = false;
	bool *element_ok;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* Something is really wrong if we don't have any elements in _PSS */
	if (obj->Package.Count < 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_PSSElementCount",
			"_PSS should return package of at least 1 element, "
			"got %" PRIu32 " elements instead.",
			obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	element_ok = calloc(obj->Package.Count, sizeof(bool));
	if (element_ok == NULL) {
		fwts_log_error(fw, "Cannot allocate an array. Test aborted.");
		return;
	}

	for (i = 0; i < obj->Package.Count; i++) {
		ACPI_OBJECT *pstate;

		if (obj->Package.Elements[i].Type != ACPI_TYPE_PACKAGE) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PSSElementType",
				"_PSS package element %" PRIu32
				" was not a package.", i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}

		pstate = &obj->Package.Elements[i];
		if (pstate->Package.Count != 6) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PSSSubPackageElementCount",
				"_PSS P-State sub-package %" PRIu32
				" was expected to "
				"have 6 elements, got %" PRIu32 " elements instead.",
				i, obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}

		/* Elements need to be all ACPI integer types */
		if ((pstate->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
		    (pstate->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
		    (pstate->Package.Elements[2].Type != ACPI_TYPE_INTEGER) ||
		    (pstate->Package.Elements[3].Type != ACPI_TYPE_INTEGER) ||
		    (pstate->Package.Elements[4].Type != ACPI_TYPE_INTEGER) ||
		    (pstate->Package.Elements[5].Type != ACPI_TYPE_INTEGER)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PSSSubPackageElementType",
				"_PSS P-State sub-package %" PRIu32 " was expected to "
				"have 6 Integer elements but didn't", i);
			failed = true;
			continue;
		}

		/*
	 	 *  Parses OK, so this element can be dumped out
		 */
		element_ok[i] = true;
		dump_elements = true;

		/*
		 * Collect maximum frequency.  The sub-packages are sorted in
		 * descending power dissipation order, so one would assume that
		 * the highest frequency is first.  However, it is not clear
		 * from the ACPI spec that this is necessarily an assumption we
		 * should make, so instead we should probably scan through all
		 * the valid sub-packages and find the highest frequency.
		 */
		if (max_freq < pstate->Package.Elements[0].Integer.Value) {
			max_freq = pstate->Package.Elements[0].Integer.Value;
			max_freq_valid = true;
		}

		/* Sanity check descending power dissipation levels */
		if ((i > 0) && (prev_power != 0) &&
		    (pstate->Package.Elements[1].Integer.Value > prev_power)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PSSSubPackagePowerNotDecending",
				"_PSS P-State sub-package %" PRIu32 " has a larger "
				"power dissipation setting than the previous "
				"sub-package.", i);
			fwts_advice(fw,
				"_PSS P-States must be ordered in decending "
				"order of power dissipation, so that the "
				"zero'th entry has the highest power "
				"dissipation level and the Nth has the "
				"lowest.");
			failed = true;
		}
		prev_power = pstate->Package.Elements[1].Integer.Value;
	}

	/*
	 *  If we have some valid data then dump it out, it is useful to see
	 */
	if (dump_elements) {
		fwts_log_info_verbatum(fw, "P-State  Freq     Power  Latency   Bus Master");
		fwts_log_info_verbatum(fw, "         (MHz)    (mW)    (us)    Latency (us)");
		for (i = 0; i < obj->Package.Count; i++) {
			ACPI_OBJECT *pstate = &obj->Package.Elements[i];
			if (element_ok[i]) {
				fwts_log_info_verbatum(fw, " %3d   %7" PRIu64 " %8" PRIu64
					" %5" PRIu64 "     %5" PRIu64,
					i,
					pstate->Package.Elements[0].Integer.Value,
					pstate->Package.Elements[1].Integer.Value,
					pstate->Package.Elements[2].Integer.Value,
					pstate->Package.Elements[3].Integer.Value);
			} else {
				fwts_log_info_verbatum(fw,
					" %3d      ----    -----    --        -- (invalid)", i);
			}
		}
	}

	free(element_ok);

	/*
	 * Sanity check maximum frequency.  We could also check the DMI data
	 * for a BIOS date (but this can be wrong) or check the CPU identity
	 * (which requires adding in new CPU identity checks) to make a decision
	 * on when it is reasonable to assume a CPU is modern and hence clocked
	 * incorrectly.  For now, just flag up a low level error that the
	 * frequency looks rather low rather than try to be intelligent (and
	 * possibly make a mistake).  I'd rather flag up a few false positives
	 * on older machines than miss flagging up bad _PSS settings on new
	 * machines.
	 */
	if (max_freq_valid && max_freq < 1000) {
		fwts_failed(fw, LOG_LEVEL_LOW, "Method_PSSSubPackageLowFreq",
			"Maximum CPU frequency is %dHz and this is low for "
			"a modern processor. This may indicate the _PSS "
			"P-States are incorrect\n", max_freq);
		fwts_advice(fw,
			"The _PSS P-States are used by the Linux CPU frequency "
			"driver to set the CPU frequencies according to system "
			"load.  Sometimes the firmware sets these incorrectly "
			"and the machine runs at a sub-optimal speed.  One can "
			"view the firmware defined CPU frequencies via "
			"/sys/devices/system/cpu/cpu*/cpufreq/"
			"scaling_available_frequencies");
		failed = true;
	}

	if (!failed)
		fwts_passed(fw,
			"_PSS correctly returned sane looking package.");
}

static int method_test_PSS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL, "_PSS", NULL, 0, method_test_PSS_return, NULL);
}

static int method_test_PPC(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PPC", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_PPE(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PPE", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_PDL(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PDL", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_TDL(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TDL", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_TPC(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TPC", NULL, 0, method_test_integer_return, NULL);
}

static void method_test_TSD_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* Something is really wrong if we don't have any elements in _TSD */
	if (obj->Package.Count < 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_TSDElementCount",
			"_TSD should return package of at least 1 element, "
			"got %" PRIu32 " elements instead.",
			obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/* Could be one or more packages */
	for (i = 0; i < obj->Package.Count; i++) {
		ACPI_OBJECT *pkg;
		uint32_t j;
		bool elements_ok = true;

		if (obj->Package.Elements[i].Type != ACPI_TYPE_PACKAGE) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSDElementType",
				"_TSD package element %" PRIu32
				" was not a package.", i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}

		pkg = &obj->Package.Elements[i];
		/*
		 *  Currently we expect a package of 5 integers.
		 */
		if (pkg->Package.Count != 5) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSDSubPackageElementCount",
				"_TSD sub-package %" PRIu32 " was expected to "
				"have 5 elements, got %" PRIu32 " elements instead.",
				i, pkg->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;
		}

		for (j = 0; j < 5; j++) {
			if (pkg->Package.Elements[j].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_TSDSubPackageElementCount",
					"_TSD sub-package %" PRIu32
					" element %" PRIu32 " is not "
					"an integer.", i, j);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				elements_ok = false;
			}
		}

		if (!elements_ok) {
			failed = true;
			continue;
		}

		/* Element 0 must equal the number elements in the package */
		if (pkg->Package.Elements[0].Integer.Value != pkg->Package.Count) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSDSubPackageElement0",
				"_TSD sub-package %" PRIu32
				" element 0 (NumEntries) "
				"was expected to have value 0x%" PRIx64 ".",
				i,
				pkg->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Element 1 should contain zero */
		if (pkg->Package.Elements[1].Integer.Value != 0) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSDSubPackageElement1",
				"_TSD sub-package %" PRIu32
				" element 1 (Revision) "
				"was expected to have value 1, instead it "
				"was 0x%" PRIx64 ".",
				i,
				pkg->Package.Elements[1].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Element 3 should contain 0xfc..0xfe */
		if ((pkg->Package.Elements[3].Integer.Value != 0xfc) &&
		    (pkg->Package.Elements[3].Integer.Value != 0xfd) &&
		    (pkg->Package.Elements[3].Integer.Value != 0xfe)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSDSubPackageElement1",
				"_TSD sub-package %" PRIu32
				" element 3 (CoordType) "
				"was expected to have value 0xfc (SW_ALL), "
				"0xfd (SW_ANY) or 0xfe (HW_ALL), instead it "
				"was 0x%" PRIx64 ".",
				i,
				pkg->Package.Elements[3].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Element 4 number of processors, skip check */
	}

	if (!failed)
		fwts_passed(fw,
			"_TSD correctly returned sane looking package.");
}

static int method_test_TSD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TSD", NULL, 0, method_test_TSD_return, NULL);
}

static void method_test_TSS_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t i;
	bool failed = false;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) != FWTS_OK)
		return;

	/* Something is really wrong if we don't have any elements in _TSS */
	if (obj->Package.Count < 1) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_TSSElementCount",
			"_TSS should return package of at least 1 element, "
			"got %" PRIu32 " elements instead.",
			obj->Package.Count);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/* Could be one or more packages */
	for (i = 0; i < obj->Package.Count; i++) {
		ACPI_OBJECT *pkg;
		uint32_t j;
		bool elements_ok = true;

		if (obj->Package.Elements[i].Type != ACPI_TYPE_PACKAGE) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSSElementType",
				"_TSS package element %" PRIu32
				" was not a package.", i);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}

		pkg = &obj->Package.Elements[i];
		/*
		 *  We expect a package of 5 integers.
		 */
		if (pkg->Package.Count != 5) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSSSubPackageElementCount",
				"_TSS sub-package %" PRIu32
				" was expected to have 5 elements, "
				"got %" PRIu32" elements instead.",
				i, pkg->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
			continue;	/* Skip processing sub-package */
		}

		for (j = 0; j < 5; j++) {
			if (pkg->Package.Elements[j].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_TSSSubPackageElementCount",
					"_TSS sub-package %" PRIu32
					" element %" PRIu32 " is not "
					"an integer.", i, j);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				elements_ok = false;
			}
		}
		if (!elements_ok) {
			failed = true;
			continue;
		}

		/* Element 0 must be 1..100 */
		if ((pkg->Package.Elements[0].Integer.Value < 1) ||
		    (pkg->Package.Elements[0].Integer.Value > 100)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_TSDSubPackageElement0",
				"_TSD sub-package %" PRIu32 " element 0"
				"was expected to have value 1..100, instead "
				"was %" PRIu64 ".",
				i,
				pkg->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed = true;
		}
		/* Skip checking elements 1..4 */

		fwts_log_info(fw, "TSS [%" PRIu32 "]:", i);
		fwts_log_info_verbatum(fw, "   CPU frequency: %" PRIu64 "%%",
			pkg->Package.Elements[0].Integer.Value);
		fwts_log_info_verbatum(fw, "   Power        : %" PRIu64 " (mW)",
			pkg->Package.Elements[1].Integer.Value);
		fwts_log_info_verbatum(fw, "   Latency      : %" PRIu64 " microseconds",
			pkg->Package.Elements[2].Integer.Value);
		fwts_log_info_verbatum(fw, "   Control      : 0x%" PRIx64,
			pkg->Package.Elements[3].Integer.Value);
		fwts_log_info_verbatum(fw, "   Status       : 0x%" PRIx64,
			pkg->Package.Elements[4].Integer.Value);
	}

	if (!failed)
		fwts_passed(fw,
			"_TSD correctly returned sane looking package.");
}

static int method_test_TSS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TSS", NULL, 0, method_test_TSS_return, NULL);
}


/*
 * Section 9.2 Ambient Light Sensor Device
 */
method_test_integer(_ALC, METHOD_OPTIONAL)
method_test_integer(_ALI, METHOD_OPTIONAL)
method_test_integer(_ALT, METHOD_OPTIONAL)

/* TODO _ALR */

static int method_test_ALP(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_ALP", NULL, 0, method_test_polling_return, "_ALP");
}


/*
 * Section 9.4 Lid control
 */
static void method_test_LID_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw,
			"_LID correctly returned sane looking value 0x%8.8" PRIx64,
			obj->Integer.Value);
}

static int method_test_LID(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_LID", NULL, 0, method_test_LID_return, NULL);
}


/*
 * Section 9.18 Wake Alarm Device
 */
static void method_test_GCP_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if (obj->Integer.Value & ~0xf) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_GCPReturn",
				"_GCP returned %" PRId64 ", should be between 0 and 15, "
				"one or more of the reserved bits 4..31 seem "
				"to be set.",
				obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			fwts_passed(fw,
				"_GCP correctly returned sane looking "
				"value 0x%8.8" PRIx64, obj->Integer.Value);
		}
	}
}

static int method_test_GCP(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_GCP", NULL, 0, method_test_GCP_return, "_GCP");
}

static void method_test_GRT_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_BUFFER) != FWTS_OK)
		return;

	if (obj->Buffer.Length != 16) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"Method_GRTBadBufferSize",
			"_GRT should return a buffer of 16 bytes, but "
			"instead just returned %" PRIu32,
			obj->Buffer.Length);
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		return;
	}

	/*
	 * Should sanity check this, but we can't read the
	 * the data in this emulated mode, so ignore
	 */
}

static int method_test_GRT(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_GRT", NULL, 0, method_test_GRT_return, NULL);
}

static void method_test_GWS_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if (obj->Integer.Value & ~0x3) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_GWSReturn",
				"_GWS returned %" PRIu64 ", should be between 0 and 3, "
				"one or more of the reserved bits 2..31 seem "
				"to be set.",
				obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			fwts_passed(fw,
				"_GWS correctly returned sane looking "
				"value 0x%8.8" PRIx64, obj->Integer.Value);
		}
	}
}

static int method_test_GWS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_GWS", arg, 1, method_test_GWS_return, "_GWS");
}

static int method_test_STP(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 0;	/* wake up instantly */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_STP", arg, 2, method_test_passed_failed_return, "_STP");
}

static int method_test_STV(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 100;	/* timer value */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_STV", arg, 2, method_test_passed_failed_return, "_STV");
}

static int method_test_TIP(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TIP", arg, 1, method_test_integer_return, NULL);
}

static int method_test_TIV(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;	/* DC timer */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TIV", arg, 1, method_test_integer_return, NULL);
}


/*
 * Section 10.1 Smart Battery
 */
static void method_test_SBS_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	static char *sbs_info[] = {
		"Maximum 1 Smart Battery, system manager/selector not present",
		"Maximum 1 Smart Battery, system manager/selector present",
		"Maximum 2 Smart Batteries, system manager/selector present",
		"Maximum 3 Smart Batteries, system manager/selector present",
		"Maximum 4 Smart Batteries, system manager/selector present"
	};

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		switch (obj->Integer.Value) {
		case 0 ... 4:
			fwts_passed(fw, "_SBS correctly returned value %" PRIu64 " %s",
				obj->Integer.Value,
				sbs_info[obj->Integer.Value]);
			break;
		default:
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_SBSReturn",
				"_SBS returned %" PRIu64 ", should be between 0 and 4.",
				obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			fwts_advice(fw,
				"Smart Battery _SBS is incorrectly informing "
				"the OS about the smart battery "
				"configuration. This is a bug and needs to be "
				"fixed.");
			break;
		}
	}
}

static int method_test_SBS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_SBS", NULL, 0, method_test_SBS_return, NULL);
}


/*
 * Section 10.2 Battery Control Methods
 */
static int method_test_BCT(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 50;	/* 50% */

	/*
	 * For now, just check that we get some integer back, values
	 * can be 0x00000000, 0x00000001-0xfffffffe and 0xffffffff,
	 * so anything is valid as long as it is an integer
	 */
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BCT", arg, 1, method_test_integer_return, NULL);
}

static void method_test_BIF_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		uint32_t i;
		int failed = 0;

		if (obj->Package.Count != 13) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BIFElementCount",
				"_BIF package should return 13 elements, "
				"got %" PRIu32 " instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}

		for (i = 0; (i < 9) && (i < obj->Package.Count); i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BIFBadType",
					"_BIF package element %" PRIu32
					" is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}
		for (i = 9; (i < 13) && (i < obj->Package.Count); i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_STRING) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BIFBadType",
					"_BIF package element %" PRIu32
					" is not of type STRING.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Power Unit */
		if (obj->Package.Elements[0].Integer.Value > 0x00000002) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BIFBadUnits",
				"_BIF: Expected Power Unit (Element 0) to be "
				"0 (mWh) or 1 (mAh), got 0x%8.8" PRIx64 ".",
				obj->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#ifdef FWTS_METHOD_PEDANDTIC
		/*
		 * Since this information may be evaluated by communicating with
		 * the EC we skip these checks as we can't do this from userspace
	 	 */
		/* Design Capacity */
		if (obj->Package.Elements[1].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIFBadCapacity",
				"_BIF: Design Capacity (Element 1) is "
				"unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[1].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Last Full Charge Capacity */
		if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIFChargeCapacity",
				"_BIF: Last Full Charge Capacity (Element 2) "
				"is unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[2].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#endif
		/* Battery Technology */
		if (obj->Package.Elements[3].Integer.Value > 0x00000002) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BIFBatTechUnit",
				"_BIF: Expected Battery Technology Unit "
				"(Element 3) to be 0 (Primary) or 1 "
				"(Secondary), got 0x%8.8" PRIx64 ".",
				obj->Package.Elements[3].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#ifdef FWTS_METHOD_PEDANDTIC
		/*
		 * Since this information may be evaluated by communicating with
		 * the EC we skip these checks as we can't do this from userspace
	 	 */
		/* Design Voltage */
		if (obj->Package.Elements[4].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIFDesignVoltage",
				"_BIF: Design Voltage (Element 4) is "
				"unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[4].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity warning */
		if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIFDesignCapacityE5",
				"_BIF: Design Capacity Warning (Element 5) "
				"is unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[5].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity low */
		if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIFDesignCapacityE6",
				"_BIF: Design Capacity Warning (Element 6) "
				"is unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[6].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#endif
		if (failed)
			fwts_advice(fw,
				"Battery _BIF package contains errors. It is "
				"worth running the firmware test suite "
				"interactive 'battery' test to see if this "
				"is problematic.  This is a bug an needs to "
				"be fixed.");
		else
			fwts_passed(fw, "Battery _BIF package looks sane.");
	}
}

static int method_test_BIF(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BIF", NULL, 0, method_test_BIF_return, NULL);
}

static void method_test_BIX_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		uint32_t i;
		int failed = 0;

		if (obj->Package.Count != 16) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BIXElementCount",
				"_BIX package should return 16 elements, "
				"got %" PRIu32 " instead.", obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}

		for (i = 0; (i < 16) && (i < obj->Package.Count); i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BIXBadType",
					"_BIX package element %" PRIu32
					" is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}
		for (i = 16; (i < 20) && (i < obj->Package.Count); i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_STRING) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BIXBadType",
					"_BIX package element %" PRIu32
					" is not of type STRING.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Power Unit */
		if (obj->Package.Elements[1].Integer.Value > 0x00000002) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BIXPowerUnit",
				"_BIX: Expected Power Unit (Element 1) to be "
				"0 (mWh) or 1 (mAh), got 0x%8.8" PRIx64 ".",
				obj->Package.Elements[1].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#ifdef FWTS_METHOD_PEDANDTIC
		/*
		 * Since this information may be evaluated by communicating with
		 * the EC we skip these checks as we can't do this from userspace
	 	 */
		/* Design Capacity */
		if (obj->Package.Elements[2].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIXDesignCapacity",
				"_BIX: Design Capacity (Element 2) is "
				"unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[2].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Last Full Charge Capacity */
		if (obj->Package.Elements[3].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIXFullChargeCapacity",
				"_BIX: Last Full Charge Capacity (Element 3) "
				"is unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[3].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#endif
		/* Battery Technology */
		if (obj->Package.Elements[4].Integer.Value > 0x00000002) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BIXBatteryTechUnit",
				"_BIX: Expected Battery Technology Unit "
				"(Element 4) to be 0 (Primary) or 1 "
				"(Secondary), got 0x%8.8" PRIx64 ".",
				obj->Package.Elements[4].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#ifdef FWTS_METHOD_PEDANDTIC
		/*
		 * Since this information may be evaluated by communicating with
		 * the EC we skip these checks as we can't do this from userspace
	 	 */
		/* Design Voltage */
		if (obj->Package.Elements[5].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIXDesignVoltage",
				"_BIX: Design Voltage (Element 5) is unknown: "
				"0x%8.8" PRIx64 ".",
				obj->Package.Elements[5].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity warning */
		if (obj->Package.Elements[6].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIXDesignCapacityE6",
				"_BIX: Design Capacity Warning (Element 6) "
				"is unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[6].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Design capacity low */
		if (obj->Package.Elements[7].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"Method_BIXDesignCapacityE7",
				 "_BIX: Design Capacity Warning (Element 7) "
				"is unknown: 0x%8.8" PRIx64 ".",
				obj->Package.Elements[7].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Cycle Count */
		if (obj->Package.Elements[10].Integer.Value > 0x7fffffff) {
			fwts_failed(fw, LOG_LEVEL_LOW, "Method_BIXCyleCount",
				"_BIX: Cycle Count (Element 10) is unknown: "
				"0x%8.8" PRIx64 ".",
				obj->Package.Elements[10].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
#endif
		if (failed)
			fwts_advice(fw,
				"Battery _BIX package contains errors. It is "
				"worth running the firmware test suite "
				"interactive 'battery' test to see if this "
				"is problematic.  This is a bug an needs to "
				"be fixed.");
		else
			fwts_passed(fw, "Battery _BIX package looks sane.");
	}
}

static int method_test_BIX(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BIX", NULL, 0, method_test_BIX_return, NULL);
}

static int method_test_BMA(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BMA", arg, 1, method_test_integer_return, NULL);
}

static int method_test_BMS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BMS", arg, 1, method_test_integer_return, NULL);
}

static void method_test_BST_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		uint32_t i;
		int failed = 0;

		if (obj->Package.Count != 4) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BSTElementCount",
				"_BST package should return 4 elements, "
				"got %" PRIu32" instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}

		for (i = 0; (i < 4) && (i < obj->Package.Count); i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BSTBadType",
					"_BST package element %" PRIu32
					" is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}

		/* Sanity check each field */
		/* Battery State */
		if ((obj->Package.Elements[0].Integer.Value) > 7) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BSTBadState",
				"_BST: Expected Battery State (Element 0) to "
				"be 0..7, got 0x%8.8" PRIx64 ".",
				obj->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Ensure bits 0 (discharging) and 1 (charging) are not both set, see 10.2.2.6 */
		if (((obj->Package.Elements[0].Integer.Value) & 3) == 3) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BSTBadState",
				"_BST: Battery State (Element 0) is "
				"indicating both charging and discharginng "
				"which is not allowed. Got value 0x%8.8" PRIx64 ".",
				obj->Package.Elements[0].Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}
		/* Battery Present Rate - cannot check, pulled from EC */
		/* Battery Remaining Capacity - cannot check, pulled from EC */
		/* Battery Present Voltage - cannot check, pulled from EC */
		if (failed)
			fwts_advice(fw,
				"Battery _BST package contains errors. It is "
				"worth running the firmware test suite "
				"interactive 'battery' test to see if this "
				"is problematic.  This is a bug an needs to "
				"be fixed.");
		else
			fwts_passed(fw, "Battery _BST package looks sane.");
	}
}

static int method_test_BST(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BST", NULL, 0, method_test_BST_return, NULL);
}

static int method_test_BTP(fwts_framework *fw)
{
	static int values[] = { 0, 1, 100, 200, 0x7fffffff };
	int i;

	for (i = 0; i < 5; i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		if (method_evaluate_method(fw, METHOD_MOBILE, "_BTP", arg, 1,
			method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static void method_test_PCL_return(fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(fw);
	FWTS_UNUSED(name);
	FWTS_UNUSED(buf);
	FWTS_UNUSED(obj);
	FWTS_UNUSED(private);

	/* FIXME */
}

static int method_test_PCL(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_PCL", NULL, 0, method_test_PCL_return, "_PCL");
}

static int method_test_BTM(fwts_framework *fw)
{
	static int values[] = { 0, 1, 100, 200, 0x7fffffff };
	int i;

	for (i=0 ; i < 5; i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		if (method_evaluate_method(fw, METHOD_MOBILE, "_BTM", arg, 1,
			method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static void method_test_BMD_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		uint32_t i;
		int failed = 0;

		fwts_acpi_object_dump(fw, obj);

		if (obj->Package.Count != 5) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BMDElementCount",
				"_BMD package should return 4 elements, "
				"got %" PRIu32 " instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		}

		for (i= 0; (i < 4) && (i < obj->Package.Count); i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BMDBadType",
					"_BMD package element %" PRIu32
					" is not of type DWORD Integer.", i);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}
		/* TODO: check return values */
	}
}

static int method_test_BMD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_MOBILE,
		"_BMD", NULL, 0, method_test_BMD_return, NULL);
}

static int method_test_BMC(fwts_framework *fw)
{
	static int values[] = { 0, 1, 2, 4 };
	int i;

	for (i = 0; i < 4; i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = values[i];
		if (method_evaluate_method(fw, METHOD_MOBILE, "_BMC", arg, 1,
			method_test_NULL_return, NULL) == FWTS_NOT_EXIST)
			break;
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}


/*
 * Section 10.3 AC Adapters and Power Sources Objects
 */
static void method_test_PSR_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		if (obj->Integer.Value > 2) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PSRZeroOrOne",
				"_PSR returned 0x%8.8" PRIx64 ", expected 0 "
				"(offline) or 1 (online)",
				obj->Integer.Value);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else
			fwts_passed(fw,
				"_PSR correctly returned sane looking "
				"value 0x%8.8" PRIx64, obj->Integer.Value);
	}
}

static int method_test_PSR(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PSR", NULL, 0, method_test_PSR_return, NULL);
}

static void method_test_PIF_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		fwts_acpi_object_dump(fw, obj);

		if (obj->Package.Count != 6) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_PIFElementCount",
				"_PIF should return package of 6 elements, "
				"got %" PRIu32 " elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_BUFFER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[3].Type != ACPI_TYPE_STRING) ||
			    (obj->Package.Elements[4].Type != ACPI_TYPE_STRING) ||
			    (obj->Package.Elements[5].Type != ACPI_TYPE_STRING)) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_PIFBadType",
					"_PIF should return package of 1 "
					"buffer, 2 integers and 3 strings.");
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			} else {
				fwts_passed(fw,
					"_PIF correctly returned sane "
					"looking package.");
			}
		}
	}
}

static int method_test_PIF(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_PIF", NULL, 0, method_test_PIF_return, NULL);
}


/*
 * Section 11.3 Fan Devices
 */
static void method_test_FIF_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		fwts_acpi_object_dump(fw, obj);

		if (obj->Package.Count != 4) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_FIFElementCount",
				"_FIF should return package of 4 elements, "
				"got %" PRIu32 " elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[3].Type != ACPI_TYPE_INTEGER)) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_FIFBadType",
					"_FIF should return package of 4 "
					"integers.");
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				fwts_advice(fw,
					"_FIF is not returning the correct "
					"fan information. It may be worth "
					"running the firmware test suite "
					"interactive 'fan' test to see if "
					"this affects the control and "
					"operation of the fan.");
			} else {
				fwts_passed(fw,
					"_FIF correctly returned sane "
					"looking package.");
			}
		}
	}
}

static int method_test_FIF(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_FIF", NULL, 0, method_test_FIF_return, NULL);
}

static int method_test_FSL(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 50;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_FSL", arg, 1, method_test_NULL_return, NULL);
}

static void method_test_FST_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		fwts_acpi_object_dump(fw, obj);

		if (obj->Package.Count != 3) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_FSTElementCount",
				"_FST should return package of 3 elements, "
				"got %" PRIu32 " elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER)) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_FSTBadType",
					"_FST should return package of 3 "
					"integers.");
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				fwts_advice(fw,
					"_FST is not returning the correct "
					"fan status information. It may be "
					"worth running the firmware test "
					"suite interactive 'fan' test to see "
					"if this affects the control and "
					"operation of the fan.");
			} else {
				fwts_passed(fw,
					"_FST correctly returned sane "
					"looking package.");
			}
		}
	}
}

static int method_test_FST(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_FST", NULL, 0, method_test_FST_return, NULL);
}


/*
 * Section 11.4 Thermal
 */
static void method_test_THERM_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		char *method = (char*)private;

		if (fwts_acpi_region_handler_called_get()) {
			/*
			 *  We accessed some memory or I/O region during the
			 *  evaluation which returns spoofed values, so we
			 *  should not test the value being returned. In this
			 *  case, just pass this as a valid return type.
			 */
			fwts_passed(fw,
				"%s correctly returned sane looking "
				"return type.", name);
		} else {
			/*
			 *  The evaluation probably was a hard-coded value,
			 *  so sanity check it
			 */
			if (obj->Integer.Value >= 2732)
				fwts_passed(fw,
					"%s correctly returned sane looking "
					"value 0x%8.8" PRIx64 " (%5.1f degrees K)",
					method,
					obj->Integer.Value,
					(float)((uint64_t)obj->Integer.Value) / 10.0);
			else {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"MethodBadTemp",
					"%s returned a dubious value below "
					"0 degrees C: 0x%8.8" PRIx64 " (%5.1f "
					"degrees K)",
					method,
					obj->Integer.Value,
					(float)((uint64_t)obj->Integer.Value) / 10.0);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				fwts_advice(fw,
					"The value returned was probably a "
					"hard-coded thermal value which is "
					"out of range because fwts did not "
					"detect any ACPI region handler "
					"accesses of I/O or system memeory "
					"to evaluate the thermal value. "
					"It is worth sanity checking these "
					"values in "
					"/sys/class/thermal/thermal_zone*.");
			}
		}
	}
}

#define method_test_THERM(name, type)			\
static int method_test ## name(fwts_framework *fw)	\
{							\
	fwts_acpi_region_handler_called_set(false);	\
	return method_evaluate_method(fw, type, # name, \
		NULL, 0, method_test_THERM_return, # name);	\
}

method_test_THERM(_CRT, METHOD_OPTIONAL)
method_test_THERM(_HOT, METHOD_OPTIONAL)
method_test_THERM(_TMP, METHOD_OPTIONAL)
method_test_THERM(_NTT, METHOD_OPTIONAL)
method_test_THERM(_PSV, METHOD_OPTIONAL)
method_test_THERM(_TST, METHOD_OPTIONAL)

static int method_test_TSP(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TSP", NULL, 0, method_test_polling_return, "_TSP");
}

static void method_test_TCx_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK) {
		char *method = (char *)private;
		fwts_passed(fw,
			"%s correctly returned sane looking value 0x%8.8x",
			method, (uint32_t)obj->Integer.Value);
	}
}

static int method_test_TC1(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TC1", NULL, 0, method_test_TCx_return, "_TC1");
}

static int method_test_TC2(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TC2", NULL, 0, method_test_TCx_return, "_TC1");
}

static int method_test_ACx(fwts_framework *fw)
{
	int i;

	for (i = 0; i < 10; i++) {
		char buffer[5];

		snprintf(buffer, sizeof(buffer), "AC%d", i);
		method_evaluate_method(fw, METHOD_OPTIONAL,
			buffer, NULL, 0, method_test_THERM_return, buffer);
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static int method_test_DTI(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 2732 + 800; /* 80 degrees C */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DTI", arg, 1, method_test_NULL_return, NULL);
}

static int method_test_SCP(fwts_framework *fw)
{
	int i;

	for (i = 0; i < 2; i++) {
		ACPI_OBJECT arg[3];

		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;		/* Mode */
		arg[1].Type = ACPI_TYPE_INTEGER;
		arg[1].Integer.Value = 5;		/* Acoustic limit */
		arg[2].Type = ACPI_TYPE_INTEGER;
		arg[2].Integer.Value = 5;		/* Power limit */

		if (method_evaluate_method(fw, METHOD_OPTIONAL,
			"_DTI", arg, 1, method_test_NULL_return,
			NULL) == FWTS_NOT_EXIST)
			break;
		fwts_log_nl(fw);

		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;		/* Mode */
		arg[1].Type = ACPI_TYPE_INTEGER;
		arg[1].Integer.Value = 1;		/* Acoustic limit */
		arg[2].Type = ACPI_TYPE_INTEGER;
		arg[2].Integer.Value = 1;		/* Power limit */

		if (method_evaluate_method(fw, METHOD_OPTIONAL,
			"_DTI", arg, 1, method_test_NULL_return,
			NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static void method_test_RTV_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_INTEGER) == FWTS_OK)
		fwts_passed(fw,
			"_RTV correctly returned sane looking value 0x%8.8" PRIx64,
			obj->Integer.Value);
}

static int method_test_RTV(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_RTV", NULL, 0, method_test_RTV_return, "_RTV");
}

static int method_test_TPT(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 2732 + 900; /* 90 degrees C */

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TPT", arg, 1, method_test_NULL_return, NULL);
}

static int method_test_TZP(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_TZP", NULL, 0, method_test_polling_return, "_TZP");
}


/*
 * Section 16 Waking and Sleeping
 */
static int method_test_PTS(fwts_framework *fw)
{
	int i;

	for (i = 1; i < 6; i++) {
		ACPI_OBJECT arg[1];

		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;

		fwts_log_info(fw, "Test _PTS(%d).", i);

		if (method_evaluate_method(fw, METHOD_MANDITORY, "_PTS", arg, 1,
			method_test_NULL_return, NULL) == FWTS_NOT_EXIST) {
			fwts_advice(fw,
				"Could not find _PTS. This method provides a "
				"mechanism to do housekeeping functions, such "
				"as write sleep state to the embedded "
				"controller before entering a sleep state. If "
				"the machine cannot suspend (S3), "
				"hibernate (S4) or shutdown (S5) then it "
				"could be because _PTS is missing.");
			break;
		}
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}

static int method_test_TTS(fwts_framework *fw)
{
	int i;

	if (fwts_acpi_object_exists("_BFS") != NULL) {
		for (i = 1; i < 6; i++) {
			ACPI_OBJECT arg[1];

			arg[0].Type = ACPI_TYPE_INTEGER;
			arg[0].Integer.Value = i;

			fwts_log_info(fw,
				"Test _TTS(%d) Transition To State S%d.", i, i);

			if (method_evaluate_method(fw, METHOD_MANDITORY,
				"_TTS", arg, 1, method_test_NULL_return,
				NULL) == FWTS_NOT_EXIST)
				break;
			fwts_log_nl(fw);
		}
	}
	else {
		fwts_skipped(fw,
			"Optional control method _TTS does not exist.");
	}
	return FWTS_OK;
}

static void method_test_Sx_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	char *method = (char *)private;

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		fwts_acpi_object_dump(fw, obj);

		if (obj->Package.Count != 3) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_SElementCount",
				"%s should return package of 3 integers, "
				"got %d elements instead.", method,
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		}
	}
}

#define method_test_Sx(name)					\
static int method_test ## name(fwts_framework *fw)		\
{								\
	return method_evaluate_method(fw, METHOD_OPTIONAL,	\
		# name, NULL, 0, method_test_Sx_return, # name);\
}

method_test_Sx(_S0)
method_test_Sx(_S1)
method_test_Sx(_S2)
method_test_Sx(_S3)
method_test_Sx(_S4)
method_test_Sx(_S5)

static void method_test_WAK_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	int failed = 0;

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		if (obj->Package.Count != 2) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_WAKElementCount",
				"_WAK should return package of 2 integers, "
				"got %" PRIu32 " elements instead.",
				obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			failed++;
		} else {
			if ((obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER) ||
			    (obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER))  {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_WAKBadType",
					"_WAK should return package of 2 "
					"integers, got %" PRIu32 " instead.",
					obj->Package.Count);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				failed++;
			}
		}
		if (!failed)
			fwts_passed(fw,
				"_WAK correctly returned sane "
				"looking package.");
	}
}

static int method_test_WAK(fwts_framework *fw)
{
	uint32_t i;

	for (i = 1; i < 6; i++) {
		ACPI_OBJECT arg[1];
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;
		fwts_log_info(fw, "Test _WAK(%d) System Wake, State S%d.", i, i);
		if (method_evaluate_method(fw, METHOD_MANDITORY, "_WAK", arg, 1,
			method_test_WAK_return, &i) == FWTS_NOT_EXIST)
			break;
		fwts_log_nl(fw);
	}
	return FWTS_OK;
}


/*
 * Appendix B ACPI Extensions for Display Adapters
 */
static int method_test_DOS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0 << 2 | 1;

	/*
	 * BIOS should toggle active display, BIOS controls brightness of
	 * LCD on AC/DC power changes
 	 */
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DOS", arg, 1, method_test_NULL_return, NULL);
}

static void method_test_DOD_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	int failed = 0;
	static char *dod_type[] = {
		"Other",
		"VGA, CRT or VESA Compatible Analog Monitor",
		"TV/HDTV or other Analog-Video Monitor",
		"External Digital Monitor",

		"Internal/Integrated Digital Flat Panel",
		"Reserved",
		"Reserved",
		"Reserved",

		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",

		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved"
	};

	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		uint32_t i;

		for (i = 0; i < obj->Package.Count; i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER)
				failed++;
			else {
				uint32_t val = obj->Package.Elements[i].Integer.Value;
				fwts_log_info_verbatum(fw, "Device %" PRIu32 ":", i);
				if ((val & 0x80000000)) {
					fwts_log_info_verbatum(fw, "  Video Chip Vendor Scheme %" PRIu32, val);
				} else {
					fwts_log_info_verbatum(fw, "  Instance:                %" PRIu32, val & 0xf);
					fwts_log_info_verbatum(fw, "  Display port attachment: %" PRIu32, (val >> 4) & 0xf);
					fwts_log_info_verbatum(fw, "  Type of display:         %" PRIu32 " (%s)",
						(val >> 8) & 0xf, dod_type[(val >> 8) & 0xf]);
					fwts_log_info_verbatum(fw, "  BIOS can detect device:  %" PRIu32, (val >> 16) & 1);
					fwts_log_info_verbatum(fw, "  Non-VGA device:          %" PRIu32, (val >> 17) & 1);
					fwts_log_info_verbatum(fw, "  Head or pipe ID:         %" PRIu32, (val >> 18) & 0x7);
				}
			}
		}
		if (failed) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_DODNoPackage",
				"Method _DOD did not return a package of "
				"%" PRIu32 " integers.", obj->Package.Count);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else
			fwts_passed(fw,
				"Method _DOD returned a sane package of "
				"%" PRIu32 " integers.", obj->Package.Count);
	}
}

static int method_test_DOD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DOD", NULL, 0, method_test_DOD_return, NULL);
}

static void method_test_ROM_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(obj);
	FWTS_UNUSED(private);

	method_check_type(fw, name, buf, ACPI_TYPE_BUFFER);
}

static int method_test_ROM(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0;
	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = 4096;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_ROM", arg, 2, method_test_ROM_return, NULL);
}

static int method_test_GPD(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_GPD", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_SPD(fwts_framework *fw)
{
	ACPI_OBJECT arg[2];
	int i;

	for (i = 0; i < 4; i++) {
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = i;	/* bits 00..11, post device */

		if (method_evaluate_method(fw, METHOD_OPTIONAL,
			"_SPD", arg, 1, method_test_passed_failed_return, NULL) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_VPO(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_VPO", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_ADR(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_ADR", NULL, 0, method_test_integer_return, NULL);
}

static void method_test_BCL_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	FWTS_UNUSED(private);

	if (method_check_type(fw, name, buf, ACPI_TYPE_PACKAGE) == FWTS_OK) {
		uint32_t i;
		int failed = 0;

		fwts_acpi_object_dump(fw, obj);

		for (i = 0; i < obj->Package.Count; i++) {
			if (obj->Package.Elements[i].Type != ACPI_TYPE_INTEGER)
				failed++;
		}
		if (failed) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_BCLNoPackage",
				"Method _BCL did not return a package of %" PRIu32
				" integers.", obj->Package.Count);
		} else {
			if (obj->Package.Count < 3) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"Method_BCLElementCount",
					"Method _BCL should return a package "
					"of more than 2 integers, got "
					"just %" PRIu32 ".", obj->Package.Count);
				fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
			} else {
				bool ascending_levels = false;

				if (obj->Package.Elements[0].Integer.Value <
				    obj->Package.Elements[1].Integer.Value) {
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"Method_BCLMaxLevel",
						"Brightness level when on full "
						" power (%" PRIu64 ") is less than "
						 "brightness level when on "
						"battery power (%" PRIu64 ").",
						obj->Package.Elements[0].Integer.Value,
						obj->Package.Elements[1].Integer.Value);
					fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
					failed++;
				}

				for (i = 2; i < obj->Package.Count - 1; i++) {
					if (obj->Package.Elements[i].Integer.Value >
					    obj->Package.Elements[i+1].Integer.Value) {
						fwts_log_info(fw,
							"Brightness level %" PRIu64
							" (index %" PRIu32 ") is greater "
							"than brightness level %" PRIu64
							" (index %d" PRIu32 "), should "
							"be in ascending order.",
							obj->Package.Elements[i].Integer.Value, i,
							obj->Package.Elements[i+1].Integer.Value, i+1);
						ascending_levels = true;
						failed++;
					}
				}
				if (ascending_levels) {
					fwts_failed(fw, LOG_LEVEL_MEDIUM,
						"Method_BCLAscendingOrder",
						"Some or all of the brightness "
						"level are not in ascending "
						"order which should be fixed "
						"in the firmware.");
					fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
				}

				if (failed)
					fwts_advice(fw,
						"Method _BCL seems to be "
						"misconfigured and is "
						"returning incorrect "
						"brightness levels."
						"It is worth sanity checking "
						"this with the firmware test "
						"suite interactive test "
						"'brightness' to see how "
						"broken this is. As it is, "
						"_BCL is broken and needs to "
						"be fixed.");
				else
					fwts_passed(fw,
						"Method _BCL returned a sane "
						"package of %" PRIu32 " integers.",
						obj->Package.Count);
			}
		}
	}
}

static int method_test_BCL(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_BCL", NULL, 0, method_test_BCL_return, NULL);
}

static int method_test_BCM(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_BCM", arg, 1, method_test_NULL_return, NULL);
}

static int method_test_BQC(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_BQC", NULL, 0, method_test_integer_return, NULL);
}

static void method_test_DDC_return(
	fwts_framework *fw,
	char *name,
	ACPI_BUFFER *buf,
	ACPI_OBJECT *obj,
	void *private)
{
	uint32_t requested = *(uint32_t*)private;

	FWTS_UNUSED(buf);

	if (obj == NULL){
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"MethodReturnNullObj",
			"Method %s returned a NULL object, and did not "
			"return a buffer or integer.", name);
		return;
	}
	switch (obj->Type) {
	case ACPI_TYPE_BUFFER:
		if (requested != obj->Buffer.Length) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"Method_DDCElementCount",
				"Method _DDC returned a buffer of %" PRIu32 " items, "
				"expected %" PRIu32 ".", obj->Buffer.Length, requested);
			fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		} else
			fwts_passed(fw,
				"Method _DDC returned a buffer of %d items "
				"as expected.", obj->Buffer.Length);
		break;
	case ACPI_TYPE_INTEGER:
			fwts_passed(fw,
				"Method _DDC could not return a buffer of %d "
					"items and instead returned an error "
					"status.",
				obj->Buffer.Length);
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "Method_DDCBadReturnType",
			"Method _DDC did not return a buffer or an integer.");
		fwts_tag_failed(fw, FWTS_TAG_ACPI_METHOD_RETURN);
		break;
	}
}

static int method_test_DDC(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];
	uint32_t i;

	for (i = 128; i <= 256; i <<= 1) {
		arg[0].Type = ACPI_TYPE_INTEGER;
		arg[0].Integer.Value = 128;

		if (method_evaluate_method(fw, METHOD_OPTIONAL,
			"_DDC", arg, 1, method_test_DDC_return,
			&i) == FWTS_NOT_EXIST)
			break;
	}
	return FWTS_OK;
}

static int method_test_DCS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DCS", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_DGS(fwts_framework *fw)
{
	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DGS", NULL, 0, method_test_integer_return, NULL);
}

static int method_test_DSS(fwts_framework *fw)
{
	ACPI_OBJECT arg[1];

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 0;

	return method_evaluate_method(fw, METHOD_OPTIONAL,
		"_DSS", arg, 1, method_test_NULL_return, NULL);
}


/*
 * Tests
 */
static fwts_framework_minor_test method_tests[] = {
	{ method_name_check, "Check Method Names." },

	/* Section 5.3 */
	/* { method_test_GPE, "Check _GPE (General Purpose Events)." }, */
	/* { method_test_PR , "Check _PR  (Processor)." }, */

	/* Section 5.6 ACPI Event Programming Model */
	/* { method_test_Wxx, "Check _Wxx (Wake Event)." }, */

	{ method_test_AEI, "Check _AEI." },
	/* { method_test_EVT, "Check _EVT (Event Method)." }, */

	/* Section 5.7 Predefined Objects */
	/* { method_test_DLM, "Check _DLM (Device Lock Mutex)." }, */
	/* { method_test_GL , "Check _GL  (Global Lock)." }, */
	/* { method_test_OS , "Check _OS  (Operating System)." }, */
	/* { method_test_REV, "Check _REV (Revision)." }, */

	/* Section 5.8 System Configuration Objects */
	/* { method_test_PIC, "Check _PIC (Inform AML of Interrupt Model)." }, */

	/* Section 6.1 Device Identification Objects  */

	/* { method_test_CID, "Check _CID (Compatible ID)." }, */
	/* { method_test_CLS, "Check _CLS (Class Code)." }, */
	{ method_test_DDN, "Check _DDN (DOS Device Name)." },
	{ method_test_HID, "Check _HID (Hardware ID)." },
	{ method_test_HRV, "Check _HRV (Hardware Revision Number)." },
	/* { method_test_MLS, "Check _MLS (Multiple Language String)." }, */
	{ method_test_PLD, "Check _PLD (Physical Device Location)." },
	{ method_test_SUB, "Check _SUB (Subsystem ID)." },
	{ method_test_SUN, "Check _SUN (Slot User Number)." },
	{ method_test_STR, "Check _STR (String)." },
	{ method_test_UID, "Check _UID (Unique ID)." },

	/* Section 6.2 Device Configurations Objects */

	/* { method_test_CDM, "Check _CDM (Clock Domain)." }, */
	{ method_test_CRS, "Check _CRS (Current Resource Settings)." },
	{ method_test_DIS, "Check _DIS (Disable)." },
	{ method_test_DMA, "Check _DMA (Direct Memory Access)." },
	/* { method_test_FIX, "Check _FIX (Fixed Register Resource Provider)." }, */
	{ method_test_GSB, "Check _GSB (Global System Interrupt Base)." },
	/* { method_test_HPP, "Check _HPP (Hot Plug Parameters)." }, */
	/* { method_test_HPX, "Check _HPX (Hot Plug Extensions)." }, */
	/* { method_test_MAT, "Check _MAT (Multiple APIC Table Entry)." }, */
	/* { method_test_PRS, "Check _PRS (Possible Resource Settings)." }, */
	{ method_test_PXM, "Check _PXM (Proximity)." },
	/* { method_test_SLI, "Check _SLI (System Locality Information)." }, */
	/* { method_test_SRS, "Check _SRS (Set Resource Settings)." }, */

	/* Section 6.3 Device Insertion, Removal and Status Objects */

	{ method_test_EDL, "Check _EDL (Eject Device List)." },
	{ method_test_EJD, "Check _EJD (Ejection Dependent Device)." },
	{ method_test_EJ0, "Check _EJ0 (Eject)." },
	{ method_test_EJ1, "Check _EJ1 (Eject)." },
	{ method_test_EJ2, "Check _EJ2 (Eject)." },
	{ method_test_EJ3, "Check _EJ3 (Eject)." },
	{ method_test_EJ4, "Check _EJ4 (Eject)." },
	{ method_test_LCK, "Check _LCK (Lock)." },
	/* { method_test_OST, "Check _OST (OSPM Status Indication)." }, */
	{ method_test_RMV, "Check _RMV (Remove)." },
	{ method_test_STA, "Check _STA (Status)." },

	/* Section 6.4 Resource Data Types for ACPI */

	/* Section 6.5 Other Objects and Controls */

	/* { method_test_DEP, "Check _DEP (Operational Region Dependencies)." }, */
	{ method_test_BDN, "Check _BDN (BIOS Dock Name)." },
	{ method_test_BBN, "Check _BBN (Base Bus Number)." },
	{ method_test_DCK, "Check _DCK (Dock)." },
	/* { method_test_INI, "Check _INI (Initialize)." }, */
	/* { method_test_GLK, "Check _GLK (Global Lock)." }, */
	/* { method_test_REG, "Check _REG (Region)." }, */
	{ method_test_SEG, "Check _SEG (Segment)." },

	/* Section 7.1 Declaring a Power Resource Object */

	{ method_test_OFF, "Check _OFF (Set resource off)." },
	{ method_test_ON,  "Check _ON  (Set resource on)." },

	/* Section 7.2 Device Power Management Objects */

	{ method_test_DSW, "Check _DSW (Device Sleep Wake)." },
	{ method_test_IRC, "Check _IRC (In Rush Current)." },
	{ method_test_PRE, "Check _PRE (Power Resources for Enumeration)." },
	{ method_test_PR0, "Check _PR0 (Power Resources for D0)." },
	{ method_test_PR1, "Check _PR1 (Power Resources for D1)." },
	{ method_test_PR2, "Check _PR2 (Power Resources for D2)." },
	{ method_test_PR3, "Check _PR3 (Power Resources for D3)." },
	/* { method_test_PRW, "Check _PRW (Power Resources for Wake)." }, */
	{ method_test_PS0, "Check _PS0 (Power State 0)." },
	{ method_test_PS1, "Check _PS1 (Power State 1)." },
	{ method_test_PS2, "Check _PS2 (Power State 2)." },
	{ method_test_PS3, "Check _PS3 (Power State 3)." },
	{ method_test_PSC, "Check _PSC (Power State Current)." },
	{ method_test_PSE, "Check _PSE (Power State for Enumeration)." },
	{ method_test_PSW, "Check _PSW (Power State Wake)." },
	{ method_test_S1D, "Check _S1D (S1 Device State)." },
	{ method_test_S2D, "Check _S2D (S2 Device State)." },
	{ method_test_S3D, "Check _S3D (S3 Device State)." },
	{ method_test_S4D, "Check _S4D (S4 Device State)." },
	{ method_test_S0W, "Check _S0W (S0 Device Wake State)." },
	{ method_test_S1W, "Check _S1W (S1 Device Wake State)." },
	{ method_test_S2W, "Check _S2W (S2 Device Wake State)." },
	{ method_test_S3W, "Check _S3W (S3 Device Wake State)." },
	{ method_test_S4W, "Check _S4W (S4 Device Wake State)." },

	/* Section 7.3 OEM-Supplied System-Level Control Methods */
	{ method_test_S0_, "Check _S0_ (S0 System State)." },
	{ method_test_S1_, "Check _S1_ (S1 System State)." },
	{ method_test_S2_, "Check _S2_ (S2 System State)." },
	{ method_test_S3_, "Check _S3_ (S3 System State)." },
	{ method_test_S4_, "Check _S4_ (S4 System State)." },
	{ method_test_S5_, "Check _S5_ (S5 System State)." },
	{ method_test_SWS, "Check _SWS (System Wake Source)." },

	/* Section 8.4 Declaring Processors */

	{ method_test_PSS, "Check _PSS (Performance Supported States)." },
	{ method_test_CPC, "Check _CPC (Continuous Performance Control)." },
	{ method_test_CSD, "Check _CSD (C State Dependencies)." },
	/* { method_test_CST, "Check _CST (C States)." }, */
	{ method_test_PCT, "Check _PCT (Performance Control)." },
	/* { method_test_PDC, "Check _PDC (Processor Driver Capabilities)." }, */
	{ method_test_PDL, "Check _PDL (P-State Depth Limit)." },
	{ method_test_PPC, "Check _PPC (Performance Present Capabilities)." },
	{ method_test_PPE, "Check _PPE (Polling for Platform Error)." },
	/* { method_test_PSD, "Check _PSD (Power State Dependencies)." }, */
	/* { method_test_PTC, "Check _PTC (Processor Throttling Control)." }, */
	{ method_test_TDL, "Check _TDL (T-State Depth Limit)." },
	{ method_test_TPC, "Check _TPC (Throttling Present Capabilities)." },
	{ method_test_TSD, "Check _TSD (Throttling State Dependencies)." },
	{ method_test_TSS, "Check _TSS (Throttling Supported States)." },

	/* Section 8.5 Processor Aggregator Device */

	/* Section 9.1 System Indicators */
	/* { method_test_CWS, "Check _CWS (Clear Wake Status)." }, */
	/* { method_test_MSG, "Check _MSG (Message)." }, */
	/* { method_test_PUR, "Check _PUR (Processor Utilization Request)." }, */
	/* { method_test_SST, "Check _SST (System Status)." }, */
	/* { method_test_STP, "Check _STP (Set Expired Timer Wake Policy)." }, */
	/* { method_test_STV, "Check _STV (Set Timer Value)." }, */
	/* { method_test_TIP, "Check _TIP (Expired Timer Wake Policy)." }, */
	/* { method_test_TIV, "Check _TIV (Timer Values)." }, */

	/* Section 9.2 Ambient Light Sensor Device */

	{ method_test_ALC, "Check _ALC (Ambient Light Colour Chromaticity)." },
	{ method_test_ALI, "Check _ALI (Ambient Light Illuminance)." },
	{ method_test_ALT, "Check _ALT (Ambient Light Temperature)." },
	{ method_test_ALP, "Check _ALP (Ambient Light Polling). "},
	/* { method_test_ALR, "Check _ALR (Ambient Light Response). "}, */

	/* Section 9.3 Battery Device */

	/* Section 9.4 Lid Device */

	{ method_test_LID, "Check _LID (Lid Status)." },

	/* Section 9.8 ATA Controllers */
	/* { method_test_GTF, "Check _GTF (Get Task File)." }, */
	/* { method_test_GTM, "Check _GTM (Get Timing Mode)." }, */
	/* { method_test_SDD, "Check _SDD (Set Device Data)." }, */
	/* { method_test_STM, "Check _STM (Set Timing Mode)." }, */

	/* Section 9.9 Floppy Controllers */
	/* { method_test_FDE, "Check _FDE (Floppy Disk Enumerate)." }, */
	/* { method_test_FDI, "Check _FDI (Floppy Drive Information)." }, */
	/* { method_test_FDM, "Check _FDM (Floppy Drive Mode)." }, */

	/* Section 9.12 Memory Devices */
	/* { method_test_MBM, "Check _MBM (Memory Bandwidth Monitoring Data)." }, */
	/* { method_test_MSM, "Check _MSM (Memory Set Monitoring)." }, */

	/* Section 9.13 USB Port Capabilities */
	/* { method_test_UPC, "Check _UPC (USB Port Capabilities)." }, */

	/* Section 9.14 Device Object Name Collision */
	/* { method_test_DSM, "Check _DSM (Device Specific Method)." }, */

	/* Section 9.16 User Presence Detection Device */
	/* { method_test_UPD, "Check _UPD (User Presence Detect)." }, */
	/* { method_test_UPP, "Check _UPP (User Presence Polling)." }, */

	/* Section 9.18 Wake Alarm Device */

	{ method_test_GCP, "Check _GCP (Get Capabilities)." },
	{ method_test_GRT, "Check _GRT (Get Real Time)." },
	{ method_test_GWS, "Check _GWS (Get Wake Status)." },
	/* { method_test_SRT, "Check _SRT (Set Real Time)." }, */
	{ method_test_STP, "Check _STP (Set Expired Timer Wake Policy)." },
	{ method_test_STV, "Check _STV (Set Timer Value)." },
	{ method_test_TIP, "Check _TIP (Expired Timer Wake Policy)." },
	{ method_test_TIV, "Check _TIV (Timer Values)." },

	/* Section 10.1 Smart Battery */

	{ method_test_SBS, "Check _SBS (Smart Battery Subsystem)." },

	/* Section 10.2 Battery Controls */

	{ method_test_BCT, "Check _BCT (Battery Charge Time)." },
	{ method_test_BIF, "Check _BIF (Battery Information)." },
	{ method_test_BIX, "Check _BIX (Battery Information Extended)." },
	{ method_test_BMA, "Check _BMA (Battery Measurement Averaging)." },
	{ method_test_BMC, "Check _BMC (Battery Maintenance Control)." },
	{ method_test_BMD, "Check _BMD (Battery Maintenance Data)." },
	{ method_test_BMS, "Check _BMS (Battery Measurement Sampling Time)." },
	{ method_test_BST, "Check _BST (Battery Status)." },
	{ method_test_BTP, "Check _BTP (Battery Trip Point)." },
	{ method_test_BTM, "Check _BTM (Battery Time)." },
	/* { method_test_BLT, "Check _BLT (Battery Level Threshold)." }, */
	{ method_test_PCL, "Check _PCL (Power Consumer List)." },

	/* Section 10.3 AC Adapters and Power Source Objects */

	/* { method_test_PCL, "Check _PCL (Power Consumer List)." }, */
	{ method_test_PIF, "Check _PIF (Power Source Information)." },
	/* { method_test_PRL, "Check _PRL (Power Source Redundancy List)." }, */
	{ method_test_PSR, "Check _PSR (Power Source)." },

	/* Section 10.4 Power Meters */
	/* { method_test_GAI, "Check _GAI (Get Averaging Level)." }, */
	/* { method_test_GHL, "Check _GHL (Get Harware Limit)." }, */
	/* { method_test_PAI, "Check _PAI (Power Averaging Interval)." }, */
	/* { method_test_PMC, "Check _PMC (Power Meter Capabilities)." }, */
	/* { method_test_PMD, "Check _PMD (Power Meter Devices)." }, */
	/* { method_test_PMM, "Check _PMM (Power Meter Measurement)." }, */
	/* { method_test_PTP, "Check _PTP (Power Trip Points)." }, */
	/* { method_test_SHL, "Check _SHL (Set Hardware Limit)." }, */

	/* Section 11.3 Fan Devices */

	{ method_test_FIF, "Check _FIF (Fan Information)." },
	/* { method_test_FPS, "Check _FPS (Fan Performance States)." }, */
	{ method_test_FSL, "Check _FSL (Fan Set Level)." },
	{ method_test_FST, "Check _FST (Fan Status)." },

	/* Section 11.4 Thermal Objects */

	{ method_test_ACx, "Check _ACx (Active Cooling)." },
	/* { method_test_ART, "Check _ART (Active Cooling Relationship Table)." }, */
	/* { method_test_ALx, "Check _ALx (Active List)". }, */
	{ method_test_CRT, "Check _CRT (Critical Trip Point)." },
	{ method_test_DTI, "Check _DTI (Device Temperature Indication)." },
	{ method_test_HOT, "Check _HOT (Hot Temperature)." },
	{ method_test_NTT, "Check _NTT (Notification Temp Threshold)." },
	/* { method_test_PSL, "Check _PSL (Passive List)." }, */
	{ method_test_PSV, "Check _PSV (Passive Temp)." },
	{ method_test_RTV, "Check _RTV (Relative Temp Values)." },
	{ method_test_SCP, "Check _SCP (Set Cooling Policy)." },
	{ method_test_TC1, "Check _TC1 (Thermal Constant 1)." },
	{ method_test_TC2, "Check _TC2 (Thermal Constant 2)." },
	{ method_test_TMP, "Check _TMP (Thermal Zone Current Temp)." },
	{ method_test_TPT, "Check _TPT (Trip Point Temperature)." },
	/* { method_test_TRT, "Check _TRT (Thermal Relationship Table)." }, */
	{ method_test_TSP, "Check _TSP (Thermal Sampling Period)." },
	{ method_test_TST, "Check _TST (Temperature Sensor Threshold)." },
	/* { method_test_TZD, "Check _TZD (Thermal Zone Devices)." }, */
	{ method_test_TZP, "Check _TZP (Thermal Zone Polling)." },

	/* Section 16 Waking and Sleeping */

	{ method_test_PTS, "Check _PTS (Prepare to Sleep)." },
	{ method_test_TTS, "Check _TTS (Transition to State)." },
	{ method_test_S0,  "Check _S0  (System S0 State)." },
	{ method_test_S1,  "Check _S1  (System S1 State)." },
	{ method_test_S2,  "Check _S2  (System S2 State)." },
	{ method_test_S3,  "Check _S3  (System S3 State)." },
	{ method_test_S4,  "Check _S4  (System S4 State)." },
	{ method_test_S5,  "Check _S5  (System S5 State)." },
	{ method_test_WAK, "Check _WAK (System Wake)." },

	/* Section 19 */
	/* { method_test_IFT, "Check _IFT (IPMI Interface Type)." }, */
	/* { method_test_INT, "Check _INT (Interrupts)." }, */

	/* Appendix B, ACPI Extensions for Display Adapters */

	{ method_test_ADR, "Check _ADR (Return Unique ID for Device)." },
	{ method_test_BCL, "Check _BCL (Query List of Brightness Control Levels Supported)." },
	{ method_test_BCM, "Check _BCM (Set Brightness Level)." },
	{ method_test_BQC, "Check _BQC (Brightness Query Current Level)." },
	{ method_test_DCS, "Check _DCS (Return the Status of Output Device)." },
	{ method_test_DDC, "Check _DDC (Return the EDID for this Device)." },
	{ method_test_DSS, "Check _DSS (Device Set State)." },
	{ method_test_DGS, "Check _DGS (Query Graphics State)." },
	{ method_test_DOD, "Check _DOD (Enumerate All Devices Attached to Display Adapter)." },
	{ method_test_DOS, "Check _DOS (Enable/Disable Output Switching)." },
	{ method_test_GPD, "Check _GPD (Get POST Device)." },
	{ method_test_ROM, "Check _ROM (Get ROM Data)." },
	{ method_test_SPD, "Check _SPD (Set POST Device)." },
	{ method_test_VPO, "Check _VPO (Video POST Options)." },

	/* From PCI Specification */
	/* { method_test_CBA, "Check _CBA (Configuration Base Address)." }, */

	/* End! */

	{ NULL, NULL }
};

static fwts_framework_ops method_ops = {
	.description = "ACPI DSDT Method Semantic Tests.",
	.init        = method_init,
	.deinit      = method_deinit,
	.minor_tests = method_tests
};

FWTS_REGISTER(method, &method_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH);
