/*
 * Copyright (C) 2010-2011 Canonical
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#include "fwts.h"

/* acpica headers */
#include "acpi.h"
#include "accommon.h"
#include "acparser.h"
#include "amlcode.h"
#include "acnamesp.h"
#include "acdebug.h"
#include "actables.h"
#include "acinterp.h"
#include "acapps.h"
#include <pthread.h>
#include <errno.h>

#define ACPI_MAX_INIT_TABLES	(32)
#define AEXEC_NUM_REGIONS   	(8)

#define MAX_SEMAPHORES		(1009)
#define HASH_FULL		(0xffffffff)

typedef struct {
	sem_t		*sem;	/* Semaphore handle */
	int		count;	/* count > 0 if acquired */
} sem_hash;

BOOLEAN AcpiGbl_IgnoreErrors = FALSE;
UINT8   AcpiGbl_RegionFillValue = 0;

/*
 *  Hash table of semaphore handles
 */
static sem_hash			sem_hash_table[MAX_SEMAPHORES];

static ACPI_TABLE_DESC		Tables[ACPI_MAX_INIT_TABLES];

/*
 *  Static copies of ACPI tables used by ACPICA execution engine
 */
static ACPI_TABLE_XSDT 		*fwts_acpica_XSDT;
static ACPI_TABLE_RSDT          *fwts_acpica_RSDT;
static ACPI_TABLE_RSDP		*fwts_acpica_RSDP;
static ACPI_TABLE_FADT		*fwts_acpica_FADT;
static void *			*fwts_acpica_DSDT;

static fwts_framework		*fwts_acpica_fw;			/* acpica context copy of fw */
static int			fwts_acpica_force_sem_timeout;		/* > 0, forces a semaphore timeout */
static bool			fwts_acpica_init_called;		/* > 0, ACPICA initialised */
static fwts_acpica_log_callback fwts_acpica_log_callback_func = NULL;	/* logging call back func */

/* Semaphore Tracking */

/*
 *  fwts_acpica_sem_count_clear()
 *	clear refs to semaphores and per semaphore count
 */
void fwts_acpica_sem_count_clear(void)
{
	int i;

	for (i=0;i<MAX_SEMAPHORES;i++) {
		sem_hash_table[i].sem = NULL;
		sem_hash_table[i].count = 0;
	}
}

/*
 *  fwts_acpica_sem_count_get()
 *	collect up number of semaphores that were acquire and release
 *	during a method evaluation
 */
void fwts_acpica_sem_count_get(int *acquired, int *released)
{
	int i;
	*acquired = 0;
	*released = 0;

	for (i=0;i<MAX_SEMAPHORES;i++) {
		if (sem_hash_table[i].sem != NULL) {
			(*acquired)++;
			if (sem_hash_table[i].count == 0)
				(*released)++;
		}
	}
}

/*
 *  fwts_acpica_simulate_sem_timeout()
 *	force a timeout on next semaphore acuire to see if
 *	we can break ACPI methods during testing.
 */
void fwts_acpica_simulate_sem_timeout(int timeout)
{
	fwts_acpica_force_sem_timeout = timeout;
}


/*
 *  hash_sem_handle()
 *	generate a simple hash based on semaphore handle
 */
static unsigned int hash_sem_handle(sem_t *sem)
{	
	unsigned int i = (unsigned int)((unsigned long)sem % MAX_SEMAPHORES);
	int j;

	for (j=0;j<MAX_SEMAPHORES;j++) {
		if (sem_hash_table[i].sem == sem)
			return i;
		if (sem_hash_table[i].sem == NULL)
			return i;
		i = (i+1) % MAX_SEMAPHORES;
	}
	return HASH_FULL;
}

/*
 *  hash_sem_inc_count()
 *	increment semaphore counter
 */
static void hash_sem_inc_count(sem_t *sem)
{
	unsigned int i = hash_sem_handle(sem);
	if (i != HASH_FULL) {
		sem_hash_table[i].sem = sem;
		sem_hash_table[i].count++;
	}
}

/*
 *  hash_sem_dec_count()
 *	decrement semaphore counter
 */
static void hash_sem_dec_count(sem_t *sem)
{
	unsigned int i = hash_sem_handle(sem);
	if (i != HASH_FULL) {
		sem_hash_table[i].sem = sem;
		sem_hash_table[i].count--;
	}
}

/* ACPICA Handlers */

/*
 *  fwtsNotifyHandler()
 *	Notify handler
 */
static void fwtsNotifyHandler(ACPI_HANDLE Device, UINT32 Value, void *Context)
{
	/* fwts_log_info(fwts_acpica_fw, "Received a notify 0x%X", Value); */
}

static UINT32 fwtsInterfaceHandler(ACPI_STRING InterfaceName, UINT32 Supported)
{
	return Supported;
}

static ACPI_STATUS fwtsTableHandler(UINT32 Event, void *Table, void *Context)
{
	return AE_OK;
}

static ACPI_STATUS fwtsExceptionHandler(ACPI_STATUS AmlStatus, ACPI_NAME Name,
    				      UINT16 Opcode, UINT32 AmlOffset, void *Context)
{
	char *exception = (char*)AcpiFormatException(AmlStatus);

	if (Name)
		fwts_log_info(fwts_acpica_fw, "[AcpiExec] Exception %s during execution of method %4.4s", exception, (char*)&Name);
	else
		fwts_log_info(fwts_acpica_fw, "[AcpiExec] Exception %s during execution at module level (table load)", exception);

	if (AmlStatus == AE_AML_INFINITE_LOOP)
		return AmlStatus;

   	return AE_OK;
}

static void fwtsDeviceNotifyHandler(ACPI_HANDLE device, UINT32 value, void *context)
{
    (void)AcpiEvaluateObject(device, "_NOT", NULL, NULL);
}

static void fwtsAttachedDataHandler(ACPI_HANDLE obj, void *data)
{
}

static ACPI_STATUS fwtsRegionInit(ACPI_HANDLE RegionHandle, UINT32 Function,
    				  void *HandlerContext, void **RegionContext)
{
    *RegionContext = RegionHandle;
    return (AE_OK);
}

extern ACPI_STATUS
AeRegionHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

/*
 *  AeLocalGetRootPointer()
 *	override ACPICA AeLocalGetRootPointer to return a local copy of the RSDP
 */
ACPI_PHYSICAL_ADDRESS AeLocalGetRootPointer(void)
{
	return ((ACPI_PHYSICAL_ADDRESS) fwts_acpica_RSDP);
}

/*
 *  fwts_acpica_set_log_callback()
 *	define logging callback function as used by fwts_acpica_vprintf()
 */
void fwts_acpica_set_log_callback(fwts_framework *fw, fwts_acpica_log_callback func)
{
	fwts_acpica_log_callback_func = func;
}

/*
 *  fwts_acpica_debug_command()
 *	run a debugging command, requires a logging callback function (or set to NULL for none).
 */
void fwts_acpica_debug_command(fwts_framework *fw, fwts_acpica_log_callback func, char *command)
{
	fwts_acpica_set_log_callback(fw, func);
	AcpiDbCommandDispatch(command, NULL, NULL);
}

/*
 *  fwts_acpica_vprintf()
 *	accumulate prints from ACPICA engine, emit them when we hit terminal '\n'
 */
void fwts_acpica_vprintf(const char *fmt, va_list args)
{
	static char *buffer;
	static int  buffer_len;

	char tmp[4096];
	int tmp_len;

	vsnprintf(tmp, sizeof(tmp), fmt, args);
	tmp_len = strlen(tmp);

	if (buffer_len == 0) {
		buffer_len = tmp_len + 1;
		buffer = malloc(buffer_len);
		strcpy(buffer, tmp);
	} else {
		buffer_len += tmp_len;
		buffer = realloc(buffer, buffer_len);
		strcat(buffer, tmp);
	}
	
	if (index(buffer, '\n') != NULL) {
		if (fwts_acpica_log_callback_func)
			fwts_acpica_log_callback_func(fwts_acpica_fw, buffer);
		free(buffer);
		buffer_len = 0;
	}
}

/*
 *  AcpiOsPrintf()
 *	Override ACPICA AcpiOsPrintf to write printfs to the fwts log
 *	rather than to stdout
 */
void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf (const char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	AcpiOsVprintf(fmt, args);
	va_end(args);
}

/*
 *  AcpiOsVprintf()
 *	Override ACPICA AcpiOsVprintf to write printfs to the fwts log
 *	rather than to stdout
 */
void AcpiOsVprintf(const char *fmt, va_list args)
{
	fwts_acpica_vprintf(fmt, args);
}

/*
 *  AcpiOsWaitSemaphore()
 *	Override ACPICA AcpiOsWaitSemaphore to keep track of semaphore acquires
 *	so that we can see if any methods are sloppy in their releases.
 */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_HANDLE handle, UINT32 Units, UINT16 Timeout)
{
	sem_t	*sem = (sem_t *)handle;
	struct timespec	tm;

	if (!handle)
		return AE_BAD_PARAMETER;

	if (fwts_acpica_force_sem_timeout)
		return AE_TIME;

	switch (Timeout) {
	case 0:
		if (sem_trywait(sem) < 0)
			return AE_TIME;
		break;
	case ACPI_WAIT_FOREVER:
		if (sem_wait(sem))
			return AE_TIME;
		break;
	default:
		tm.tv_sec = Timeout / 1000;
		tm.tv_nsec = (Timeout - (tm.tv_sec * 1000)) * 1000000;

		if (sem_timedwait(sem, &tm))
			return AE_TIME;
		break;
	}
	hash_sem_inc_count(sem);

	return AE_OK;
}

/*
 *  AcpiOsSignalSemaphore()
 *	Override ACPICA AcpiOsSignalSemaphore to keep track of semaphore releases
 *	so that we can see if any methods are sloppy in their releases.
 */
ACPI_STATUS AcpiOsSignalSemaphore (ACPI_HANDLE handle, UINT32 Units)
{
	sem_t *sem = (sem_t *)handle;

	if (!handle)
		return AE_BAD_PARAMETER;

	if (sem_post(sem) < 0)
		return AE_LIMIT;

	hash_sem_dec_count(sem);

	return AE_OK;
}

/*
 *  AcpiOsReadPort()
 *	Override ACPICA AcpiOsReadPort to fake port reads
 */
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS addr, UINT32 *value, UINT32 width)
{
	switch (width) {
    		case 8:
        		*value = 0xFF;
			break;
		case 16:
			*value = 0xFFFF;
			break;
		case 32:
			*value = 0xFFFFFFFF;
			break;
		default:
			return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

/*
 *  AcpiOsReadPciConfiguration()
 *	Override ACPICA AcpiOsReadPciConfiguration to fake PCI reads
 */
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *pciid, UINT32 reg, UINT64 *value, UINT32 width)
{
	switch (width) {
    		case 8:
        		*value = 0x00;
			break;
		case 16:
			*value = 0x0000;
			break;
		case 32:
			*value = 0x00000000;
			break;
		default:
			return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

void AeTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
    	if (strncmp(ExistingTable->Signature, ACPI_SIG_DSDT, 4) == 0)
		*NewTable = (ACPI_TABLE_HEADER*)fwts_acpica_DSDT;
}

ACPI_STATUS AcpiOsSignal (UINT32 function, void *info)
{
    switch (function) {
    case ACPI_SIGNAL_BREAKPOINT:
	fwts_warning(fwts_acpica_fw, "Method contains an ACPICA breakpoint: %s\n", info ? (char*)info : "No Information");
	AcpiGbl_CmSingleStep = FALSE;
        break;
    case ACPI_SIGNAL_FATAL:
    default:
        break;
    }
    return AE_OK;
}


/*
 *  fwts_acpica_init()
 *	Initialise ACPICA core engine
 */
int fwts_acpica_init(fwts_framework *fw)
{
	ACPI_HANDLE handle;
	int i;
	int n;
	UINT32 init_flags = ACPI_FULL_INITIALIZATION;
	fwts_acpi_table_info *table;
	ACPI_ADR_SPACE_TYPE SpaceIdList[] = {0, 1, 3, 4, 5, 6, 7, 0x80};

	/* Abort if already initialised */
	if (fwts_acpica_init_called)
		return FWTS_ERROR;

	fwts_acpica_fw = fw;

	AcpiDbgLevel = ACPI_NORMAL_DEFAULT;
	AcpiDbgLayer = 0x00000000;

	AcpiOsRedirectOutput(stderr);

	if (ACPI_FAILURE(AcpiInitializeSubsystem())) {
		fwts_log_error(fw, "Failed to initialise ACPICA subsystem.");
		return FWTS_ERROR;
	}

	/* Clone FADT, make sure it points to a DSDT in user space */
	if (fwts_acpi_find_table(fw, "FACP", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table != NULL) {
		fwts_acpi_table_info *tbl;

		fwts_acpica_FADT = fwts_low_calloc(1, table->length);
		memcpy(fwts_acpica_FADT, table->data, table->length);		

		if (fwts_acpi_find_table(fw, "DSDT", 0, &tbl) != FWTS_OK)
			return FWTS_ERROR;
		if (tbl) {
			fwts_acpica_DSDT = tbl->data;

			fwts_acpica_FADT->Dsdt = ACPI_PTR_TO_PHYSADDR(tbl->data);
			if (table->length >= 148)
				fwts_acpica_FADT->XDsdt = ACPI_PTR_TO_PHYSADDR(tbl->data);
		}

		if (fwts_acpi_find_table(fw, "FACS", 0, &tbl) != FWTS_OK)
			return FWTS_ERROR;
		if (tbl) {
			if (table->length >= 140)
				fwts_acpica_FADT->XFacs = ACPI_PTR_TO_PHYSADDR(tbl->data);
			fwts_acpica_FADT->Facs = ACPI_PTR_TO_PHYSADDR(tbl->data);
		}

		fwts_acpica_FADT->Header.Checksum = 0;
		fwts_acpica_FADT->Header.Checksum = (UINT8)-AcpiTbChecksum ((void*)fwts_acpica_FADT, table->length);
	}
	
	/* Clone XSDT, make it point to tables in user address space */
	if (fwts_acpi_find_table(fw, "XSDT", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table != NULL) {
		uint64_t *entries;

		fwts_acpica_XSDT = fwts_low_calloc(1, table->length);
		memcpy(fwts_acpica_XSDT, table->data, sizeof(ACPI_TABLE_HEADER));

		n = (table->length - sizeof(ACPI_TABLE_HEADER)) / sizeof(uint64_t);
		entries = (uint64_t*)(table->data + sizeof(ACPI_TABLE_HEADER));
		for (i=0; i<n; i++) {
			fwts_acpi_table_info *tbl;
			if (fwts_acpi_find_table_by_addr(fw, entries[i], &tbl) != FWTS_OK)
				return FWTS_ERROR;
			if (tbl) {
				if (strncmp(tbl->name, "FACP", 4) == 0) {
					fwts_acpica_XSDT->TableOffsetEntry[i] = ACPI_PTR_TO_PHYSADDR(fwts_acpica_FADT);
				} else {
					fwts_acpica_XSDT->TableOffsetEntry[i] = ACPI_PTR_TO_PHYSADDR(tbl->data);
				}
			} else {
				fwts_acpica_XSDT->TableOffsetEntry[i] = ACPI_PTR_TO_PHYSADDR(NULL);
			}
		}
		fwts_acpica_XSDT->Header.Checksum = 0;
		fwts_acpica_XSDT->Header.Checksum = (UINT8)-AcpiTbChecksum ((void*)fwts_acpica_XSDT, table->length);
	}

	/* Clone RSDT, make it point to tables in user address space */
	if (fwts_acpi_find_table(fw, "RSDT", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table) {
		uint32_t *entries;
	
		fwts_acpica_RSDT = fwts_low_calloc(1, table->length);
		memcpy(fwts_acpica_RSDT, table->data, sizeof(ACPI_TABLE_HEADER));
	
		n = (table->length - sizeof(ACPI_TABLE_HEADER)) / sizeof(uint32_t);
		entries = (uint32_t*)(table->data + sizeof(ACPI_TABLE_HEADER));
		for (i=0; i<n; i++) {
			fwts_acpi_table_info *tbl;
			if (fwts_acpi_find_table_by_addr(fw, entries[i], &tbl) != FWTS_OK)
				return FWTS_ERROR;
			if (tbl) {
				if (strncmp(tbl->name, "FACP", 4) == 0) {
					fwts_acpica_RSDT->TableOffsetEntry[i] = ACPI_PTR_TO_PHYSADDR(fwts_acpica_FADT);
				}
				else {
					fwts_acpica_RSDT->TableOffsetEntry[i] = ACPI_PTR_TO_PHYSADDR(tbl->data);
				}
			} else {
				fwts_acpica_RSDT->TableOffsetEntry[i] = ACPI_PTR_TO_PHYSADDR(NULL);
			}
		}
		fwts_acpica_RSDT->Header.Checksum = 0;
		fwts_acpica_RSDT->Header.Checksum = (UINT8)-AcpiTbChecksum ((void*)fwts_acpica_RSDT, table->length);
	}

	/* Clone RSDP, make it point to tables in user address space */
	if (fwts_acpi_find_table(fw, "RSDP", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table) {
		fwts_acpica_RSDP = fwts_low_calloc(1, table->length);
		memcpy(fwts_acpica_RSDP, table->data, table->length);

		if (table->length > 20)
			fwts_acpica_RSDP->XsdtPhysicalAddress = ACPI_PTR_TO_PHYSADDR(fwts_acpica_XSDT);
		fwts_acpica_RSDP->RsdtPhysicalAddress = ACPI_PTR_TO_PHYSADDR(fwts_acpica_RSDT);
		fwts_acpica_RSDP->Checksum = (UINT8)-AcpiTbChecksum ((void*)fwts_acpica_RSDP, ACPI_RSDP_CHECKSUM_LENGTH);
	}

	if (AcpiInitializeTables(Tables, ACPI_MAX_INIT_TABLES, TRUE) != AE_OK) {
		fwts_log_error(fw, "Failed to initialise tables.");
		return FWTS_ERROR;
	}
	if (AcpiReallocateRootTable() != AE_OK) {
		fwts_log_error(fw, "Failed to reallocate root table.");
		return FWTS_ERROR;
	}
	if (AcpiLoadTables() != AE_OK) {
		fwts_log_error(fw, "Failed to load tables.");
		return FWTS_ERROR;
	}

	/* Install handlers */
	if (AcpiInstallInterfaceHandler(fwtsInterfaceHandler) != AE_OK) {
		fwts_log_error(fw, "Failed to install interface handler.");
		return FWTS_ERROR;
	}
	if (AcpiInstallTableHandler(fwtsTableHandler, NULL) != AE_OK) {
		fwts_log_error(fw, "Failed to install table handler.");
		return FWTS_ERROR;
	}
	if (AcpiInstallExceptionHandler(fwtsExceptionHandler) != AE_OK) {
		fwts_log_error(fw, "Failed to install exception handler.");
		return FWTS_ERROR;
	}
	if (AcpiInstallNotifyHandler( ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, fwtsNotifyHandler, NULL) != AE_OK) {
		fwts_log_error(fw, "Failed to install notify handler.");
		return FWTS_ERROR;
	}
	if (AcpiInstallNotifyHandler( ACPI_ROOT_OBJECT, ACPI_DEVICE_NOTIFY, fwtsDeviceNotifyHandler, NULL) != AE_OK) {
		fwts_log_error(fw, "Failed to install notify handler.");
		return FWTS_ERROR;
	}
	if (AcpiGetHandle(NULL, "\\_SB", &handle) == AE_OK) {
#if 0
		if (AcpiInstallNotifyHandler(handle, ACPI_SYSTEM_NOTIFY, fwtsNotifyHandler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install notify handler.");
			return FWTS_ERROR;
		}
		if (AcpiRemoveNotifyHandler(handle, ACPI_SYSTEM_NOTIFY, fwtsNotifyHandler) != AE_OK) {
			fwts_log_error(fw, "Failed to remove notify handler.");
			return FWTS_ERROR;
		}
		if (AcpiInstallNotifyHandler(handle, ACPI_ALL_NOTIFY, fwtsNotifyHandler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install notify handler.");
			return FWTS_ERROR;
		}
		if (AcpiRemoveNotifyHandler(handle, ACPI_ALL_NOTIFY, fwtsNotifyHandler) != AE_OK) {
			fwts_log_error(fw, "Failed to remove notify handler.");
			return FWTS_ERROR;
		}
#endif
		if (AcpiInstallNotifyHandler(handle, ACPI_ALL_NOTIFY, fwtsNotifyHandler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install notify handler.");
			return FWTS_ERROR;
		}
#if 0
		if (AcpiAttachData(handle, fwtsAttachedDataHandler, handle) != AE_OK) {
			fwts_log_error(fw, "Failed to attach data handler.");
			return FWTS_ERROR;
		}
		if (AcpiDetachData(handle, fwtsAttachedDataHandler) != AE_OK) {
			fwts_log_error(fw, "Failed to detach data handler.");
			return FWTS_ERROR;
		}
#endif
		if (AcpiAttachData(handle, fwtsAttachedDataHandler, handle) != AE_OK) {
			fwts_log_error(fw, "Failed to attach data handler.");
			return FWTS_ERROR;
		}
	}

	for (i = 0; i < AEXEC_NUM_REGIONS; i++) {
        	AcpiRemoveAddressSpaceHandler (AcpiGbl_RootNode, SpaceIdList[i], AeRegionHandler);
        	if (AcpiInstallAddressSpaceHandler (AcpiGbl_RootNode,
                        SpaceIdList[i], AeRegionHandler, fwtsRegionInit, NULL) != AE_OK) {
                	fwts_log_error(fw, "Could not install an OpRegion handler for %s space(%u)",
                	AcpiUtGetRegionName((UINT8)SpaceIdList[i]), SpaceIdList[i]);
			return FWTS_ERROR;
		}
        }

	AcpiEnableSubsystem(init_flags);
	AcpiInitializeObjects(init_flags);

	AcpiDbCommandDispatch ("methods", NULL, NULL);

	fwts_acpica_init_called = true;

	return FWTS_OK;
}

#define FWTS_ACPICA_FREE(x)	\
	if (x) {		\
		fwts_low_free(x);	\
		x = NULL;	\
	}			\

/*
 *  fwts_acpica_deinit()
 *	De-initialise ACPICA core engine
 */
int fwts_acpica_deinit(void)
{
	if (!fwts_acpica_init_called)
		return FWTS_ERROR;

	AcpiTerminate();

	FWTS_ACPICA_FREE(fwts_acpica_XSDT);
	FWTS_ACPICA_FREE(fwts_acpica_RSDT);
	FWTS_ACPICA_FREE(fwts_acpica_RSDP);
	FWTS_ACPICA_FREE(fwts_acpica_FADT);

	fwts_acpica_init_called = false;

	return FWTS_OK;
}

/*
 *  fwts_acpi_walk_for_object_names()
 *  	append to list (passed in context) objects names that match a given
 * 	type, (callback from fwts_acpica_get_object_names())
 *
 */
static ACPI_STATUS fwts_acpi_walk_for_object_names(
	ACPI_HANDLE	objHandle,
	UINT32		nestingLevel,
	void		*context,
	void		**ret)
{
	fwts_list *list = (fwts_list*)context;

	ACPI_BUFFER	buffer;
	char tmpbuf[1024];

	buffer.Pointer = tmpbuf;
	buffer.Length  = sizeof(tmpbuf);

	if (!ACPI_FAILURE(AcpiNsHandleToPathname(objHandle, &buffer)))
		fwts_list_append(list, strdup((char *)buffer.Pointer));

	return AE_OK;
}

/*
 *  fwts_acpica_get_object_names()
 *	fetch a list of object names that match a specified type
 */
fwts_list *fwts_acpica_get_object_names(int type)
{
	fwts_list *list;

	if ((list = fwts_list_new()) != NULL)
		AcpiWalkNamespace (type, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,	
			fwts_acpi_walk_for_object_names, NULL, list, NULL);
		
	return list;
}
