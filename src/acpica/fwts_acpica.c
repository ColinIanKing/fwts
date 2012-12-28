/*
 * Copyright (C) 2010-2013 Canonical
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
#include <pthread.h>

static pthread_mutex_t mutex_lock_count;
static pthread_mutex_t mutex_thread_info;

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

#define MAX_THREADS		(128)

typedef struct {
	sem_t		*sem;	/* Semaphore handle */
	int		count;	/* count > 0 if acquired */
} sem_hash;

typedef void * (*PTHREAD_CALLBACK)(void *);

BOOLEAN AcpiGbl_IgnoreErrors = FALSE;
UINT8   AcpiGbl_RegionFillValue = 0;

/*
 *  Hash table of semaphore handles
 */
static sem_hash			sem_hash_table[MAX_SEMAPHORES];

static ACPI_TABLE_DESC		Tables[ACPI_MAX_INIT_TABLES];

static bool			region_handler_called;

/*
 *  Used to account for threads used by AcpiOsExecute
 */
typedef struct {
	bool		used;	/* true if the thread accounting info in use by a thread */
	pthread_t	thread;	/* thread info */
} fwts_thread;

static fwts_thread	threads[MAX_THREADS];

/*
 *  Static copies of ACPI tables used by ACPICA execution engine
 */
static ACPI_TABLE_XSDT 		*fwts_acpica_XSDT;
static ACPI_TABLE_RSDT          *fwts_acpica_RSDT;
static ACPI_TABLE_RSDP		*fwts_acpica_RSDP;
static ACPI_TABLE_FADT		*fwts_acpica_FADT;
static void 			*fwts_acpica_DSDT;

static fwts_framework		*fwts_acpica_fw;			/* acpica context copy of fw */
static int			fwts_acpica_force_sem_timeout;		/* > 0, forces a semaphore timeout */
static bool			fwts_acpica_init_called;		/* > 0, ACPICA initialised */
static fwts_acpica_log_callback fwts_acpica_log_callback_func = NULL;	/* logging call back func */

#define ACPI_ADR_SPACE_USER_DEFINED1        0x80
#define ACPI_ADR_SPACE_USER_DEFINED2        0xE4

static ACPI_ADR_SPACE_TYPE fwts_space_id_list[] =
{
	ACPI_ADR_SPACE_SYSTEM_MEMORY,
	ACPI_ADR_SPACE_SYSTEM_IO,
	ACPI_ADR_SPACE_EC,
	ACPI_ADR_SPACE_SMBUS,
	ACPI_ADR_SPACE_CMOS,
	ACPI_ADR_SPACE_GSBUS,
	ACPI_ADR_SPACE_GPIO,
	ACPI_ADR_SPACE_PCI_BAR_TARGET,
	ACPI_ADR_SPACE_IPMI,
	ACPI_ADR_SPACE_FIXED_HARDWARE,
	ACPI_ADR_SPACE_USER_DEFINED1,
	ACPI_ADR_SPACE_USER_DEFINED2
};

/* Semaphore Tracking */

/*
 *  fwts_acpica_sem_count_clear()
 *	clear refs to semaphores and per semaphore count
 */
void fwts_acpica_sem_count_clear(void)
{
	int i;

	pthread_mutex_lock(&mutex_lock_count);

	for (i=0;i<MAX_SEMAPHORES;i++) {
		sem_hash_table[i].sem = NULL;
		sem_hash_table[i].count = 0;
	}

	pthread_mutex_unlock(&mutex_lock_count);
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

	/* Wait for any pending threads to complete */

	for (i = 0; i < MAX_THREADS; i++) {
		pthread_mutex_lock(&mutex_thread_info);
		if (threads[i].used) {
			pthread_mutex_unlock(&mutex_thread_info);

			/* Wait for thread to complete */
			pthread_join(threads[i].thread, NULL);

			pthread_mutex_lock(&mutex_thread_info);
			threads[i].used = false;
			pthread_mutex_unlock(&mutex_thread_info);
		}
		pthread_mutex_unlock(&mutex_thread_info);
	}

	/*
	 * All threads (such as Notify() calls now complete, so
	 * we can now do the semaphore accounting calculations
	 */
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

	for (j = 0; j<MAX_SEMAPHORES; j++) {
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
		pthread_mutex_lock(&mutex_lock_count);
		sem_hash_table[i].sem = sem;
		sem_hash_table[i].count++;
		pthread_mutex_unlock(&mutex_lock_count);
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
		pthread_mutex_lock(&mutex_lock_count);
		sem_hash_table[i].sem = sem;
		sem_hash_table[i].count--;
		pthread_mutex_unlock(&mutex_lock_count);
	}
}

/* ACPICA Handlers */

/*
 *  fwts_notify_handler()
 *	Notify handler
 */
static void fwts_notify_handler(ACPI_HANDLE Device, UINT32 Value, void *Context)
{
	(void)AcpiEvaluateObject(Device, "_NOT", NULL, NULL);
}

static void fwts_device_notify_handler(ACPI_HANDLE Device, UINT32 Value, void *Context)
{
	(void)AcpiEvaluateObject(Device, "_NOT", NULL, NULL);
}

static UINT32 fwts_interface_handler(ACPI_STRING InterfaceName, UINT32 Supported)
{
	return Supported;
}

static ACPI_STATUS fwts_table_handler(UINT32 Event, void *Table, void *Context)
{
	return AE_OK;
}

static UINT32 fwts_event_handler(void *Context)
{
	return 0;
}

static ACPI_STATUS fwts_exception_handler(
	ACPI_STATUS AmlStatus,
	ACPI_NAME Name,
	UINT16 Opcode,
	UINT32 AmlOffset,
	void *Context)
{
	char *exception = (char*)AcpiFormatException(AmlStatus);

	if (Name)
		fwts_log_info(fwts_acpica_fw,
			"ACPICA Exception %s during execution of method %4.4s",
			exception, (char*)&Name);
	else
		fwts_log_info(fwts_acpica_fw,
			"ACPICA Exception %s during execution at module level (table load)",
			exception);

	return AmlStatus;
}

static void fwts_attached_data_handler(ACPI_HANDLE obj, void *data)
{
}

static ACPI_STATUS fwts_region_init(
	ACPI_HANDLE RegionHandle,
	UINT32 Function,
	void *HandlerContext,
	void **RegionContext)
{
	*RegionContext = RegionHandle;
	return AE_OK;
}

void fwts_acpi_region_handler_called_set(bool val)
{
	region_handler_called = val;
}

bool fwts_acpi_region_handler_called_get(void)
{
	return region_handler_called;
}

static ACPI_STATUS fwts_region_handler(
	UINT32                  function,
	ACPI_PHYSICAL_ADDRESS   address,
	UINT32                  bitwidth,
	UINT64                  *value,
	void                    *handlercontext,
	void                    *regioncontext)
{
	ACPI_OPERAND_OBJECT     *regionobject = ACPI_CAST_PTR(ACPI_OPERAND_OBJECT, regioncontext);
	UINT8                   *buffer = ACPI_CAST_PTR(UINT8, value);
	ACPI_SIZE               length;
	UINT32                  bytewidth;
	ACPI_CONNECTION_INFO    *context;
	int			i;

	if (regionobject->Region.Type != ACPI_TYPE_REGION)
		return AE_OK;

	fwts_acpi_region_handler_called_set(true);

	context = ACPI_CAST_PTR (ACPI_CONNECTION_INFO, handlercontext);
	length = (ACPI_SIZE)regionobject->Region.Length;

	switch (regionobject->Region.SpaceId) {
	case ACPI_ADR_SPACE_SYSTEM_IO:
		switch (function & ACPI_IO_MASK) {
		case ACPI_READ:
			*value = 0;
			break;
		case ACPI_WRITE:
			break;
		default:
			return AE_BAD_PARAMETER;
			break;
		}
		break;

	case ACPI_ADR_SPACE_SMBUS:
	case ACPI_ADR_SPACE_GSBUS:  /* ACPI 5.0 */
		length = 0;

		switch (function & ACPI_IO_MASK) {
		case ACPI_READ:
			switch (function >> 16) {
			case AML_FIELD_ATTRIB_QUICK:
			case AML_FIELD_ATTRIB_SEND_RCV:
			case AML_FIELD_ATTRIB_BYTE:
				length = 1;
				break;
			case AML_FIELD_ATTRIB_WORD:
			case AML_FIELD_ATTRIB_WORD_CALL:
				length = 2;
				break;
			case AML_FIELD_ATTRIB_BLOCK:
			case AML_FIELD_ATTRIB_BLOCK_CALL:
				length = 32;
				break;
			case AML_FIELD_ATTRIB_MULTIBYTE:
			case AML_FIELD_ATTRIB_RAW_BYTES:
			case AML_FIELD_ATTRIB_RAW_PROCESS:
				length = context->AccessLength - 2;
				break;
			default:
				break;
			}
			break;

		case ACPI_WRITE:
			switch (function >> 16) {
			case AML_FIELD_ATTRIB_QUICK:
			case AML_FIELD_ATTRIB_SEND_RCV:
			case AML_FIELD_ATTRIB_BYTE:
			case AML_FIELD_ATTRIB_WORD:
			case AML_FIELD_ATTRIB_BLOCK:
				length = 0;
				break;
			case AML_FIELD_ATTRIB_WORD_CALL:
				length = 2;
				break;
			case AML_FIELD_ATTRIB_BLOCK_CALL:
				length = 32;
				break;
			case AML_FIELD_ATTRIB_MULTIBYTE:
			case AML_FIELD_ATTRIB_RAW_BYTES:
			case AML_FIELD_ATTRIB_RAW_PROCESS:
				length = context->AccessLength - 2;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		for (i = 0; i < length; i++)
			buffer[i+2] = (UINT8)(0xA0 + i);
		buffer[0] = 0x7A;
		buffer[1] = (UINT8)length;
		return AE_OK;

	case ACPI_ADR_SPACE_IPMI: /* ACPI 4.0 */
		buffer[0] = 0;       /* Status byte */
		buffer[1] = 64;      /* Return buffer data length */
		buffer[2] = 0;       /* Completion code */
		buffer[3] = 0;       /* Reserved */
		for (i = 4; i < 66; i++)
			buffer[i] = (UINT8) (i);
		return AE_OK;

	case ACPI_ADR_SPACE_SYSTEM_MEMORY:
	case ACPI_ADR_SPACE_EC:
	case ACPI_ADR_SPACE_CMOS:
	case ACPI_ADR_SPACE_GPIO:
	case ACPI_ADR_SPACE_PCI_BAR_TARGET:
	case ACPI_ADR_SPACE_FIXED_HARDWARE:
	case ACPI_ADR_SPACE_USER_DEFINED1:
	case ACPI_ADR_SPACE_USER_DEFINED2:
	default:
		bytewidth = (bitwidth / 8);
		if (bitwidth % 8)
			bytewidth += 1;

		switch (function) {
		case ACPI_READ:
			/* Fake it, return all zeros */
			memset(value, 0, bytewidth);
		break;
		case ACPI_WRITE:
			/* Fake it, do nothing */
			break;
		default:
			return AE_BAD_PARAMETER;
		}
		return AE_OK;
	}

	return AE_OK;
}

/*
 *  AeLocalGetRootPointer()
 *	override ACPICA AeLocalGetRootPointer to return a local copy of the RSDP
 */
ACPI_PHYSICAL_ADDRESS AeLocalGetRootPointer(void)
{
	return (ACPI_PHYSICAL_ADDRESS)fwts_acpica_RSDP;
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
	static size_t buffer_len;

	char tmp[4096];
	size_t tmp_len;

	vsnprintf(tmp, sizeof(tmp), fmt, args);
	tmp_len = strlen(tmp);

	if (buffer_len == 0) {
		buffer_len = tmp_len + 1;
		buffer = malloc(buffer_len);
		if (buffer)
			strcpy(buffer, tmp);
		else
			buffer_len = 0;
	} else {
		buffer_len += tmp_len;
		buffer = realloc(buffer, buffer_len);
		if (buffer)
			strcat(buffer, tmp);
		else
			buffer_len = 0;
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
 *  AcpiOsCreateSemaphore()
 *	Override ACPICA AcpiOsCreateSemaphore
 */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits,
	UINT32 InitialUnits,
	ACPI_HANDLE *OutHandle)
{
	sem_t *sem;

	if (!OutHandle)
		return AE_BAD_PARAMETER;

	sem = AcpiOsAllocate(sizeof(sem_t));
	if (!sem)
		return AE_NO_MEMORY;

	if (sem_init(sem, 0, InitialUnits) == -1) {
		AcpiOsFree(sem);
		return AE_BAD_PARAMETER;
	}

	*OutHandle = (ACPI_HANDLE)sem;

	return AE_OK;
}

/*
 *  AcpiOsDeleteSemaphore()
 *	Override ACPICA AcpiOsDeleteSemaphore to fix semaphore memory freeing
 */
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_HANDLE Handle)
{
	sem_t *sem = (sem_t *)Handle;

	if (!sem)
		return AE_BAD_PARAMETER;

	if (sem_destroy(sem) == -1)
		return AE_BAD_PARAMETER;

	AcpiOsFree(sem);

	return AE_OK;
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
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_HANDLE handle, UINT32 Units)
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

typedef struct {
	PTHREAD_CALLBACK	func;
	void *			context;
	int			thread_index;
} fwts_func_wrapper_context;

/*
 *  fwts_pthread_func_wrapper()
 *	wrap the AcpiOsExecute function so we can mark the thread
 *	accounting free once the function has completed.
 */
void *fwts_pthread_func_wrapper(fwts_func_wrapper_context *ctx)
{
	void *ret;

	ret = ctx->func(ctx->context);

	pthread_mutex_lock(&mutex_thread_info);
	threads[ctx->thread_index].used = false;
	pthread_mutex_unlock(&mutex_thread_info);

	free(ctx);

	return ret;
}

ACPI_STATUS AcpiOsExecute(
	ACPI_EXECUTE_TYPE       type,
	ACPI_OSD_EXEC_CALLBACK  function,
	void                    *func_context)
{
	int	ret;
	int 	i;

	pthread_mutex_lock(&mutex_thread_info);

	/* Find a free slot to do per-thread join tracking */
	for (i = 0; i < MAX_THREADS; i++) {
		if (!threads[i].used) {
			fwts_func_wrapper_context *ctx;

			/* We need some context to pass through to the thread wrapper */
			if ((ctx = malloc(sizeof(fwts_func_wrapper_context))) == NULL) {
				pthread_mutex_unlock(&mutex_thread_info);
				return AE_NO_MEMORY;
			}

			ctx->func = (PTHREAD_CALLBACK)function;
			ctx->context = func_context;
			ctx->thread_index = i;
			threads[i].used = true;

			ret = pthread_create(&threads[i].thread, NULL,
				(PTHREAD_CALLBACK)fwts_pthread_func_wrapper, ctx);
			pthread_mutex_unlock(&mutex_thread_info);

			if (ret)
				return AE_ERROR;

			return AE_OK;
		}
	}

	/* No free slots, failed! */
	pthread_mutex_unlock(&mutex_thread_info);
	return AE_NO_MEMORY;
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

ACPI_STATUS AcpiOsSignal(UINT32 function, void *info)
{
	switch (function) {
	case ACPI_SIGNAL_BREAKPOINT:
		fwts_warning(fwts_acpica_fw,
			"Method contains an ACPICA breakpoint: %s\n",
			info ? (char*)info : "No Information");
		AcpiGbl_CmSingleStep = FALSE;
		break;
	case ACPI_SIGNAL_FATAL:
	default:
		break;
	}
	return AE_OK;
}

void AcpiOsSleep(UINT64 milliseconds)
{
}

int fwtsInstallLateHandlers(fwts_framework *fw)
{
	int i;

	if (!AcpiGbl_ReducedHardware) {
		if (AcpiInstallFixedEventHandler(ACPI_EVENT_GLOBAL, fwts_event_handler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install global event handler.");
			return FWTS_ERROR;
		}
		if (AcpiInstallFixedEventHandler(ACPI_EVENT_RTC, fwts_event_handler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install RTC event handler.");
			return FWTS_ERROR;
		}
	}

	for (i = 0; i < ACPI_ARRAY_LENGTH(fwts_space_id_list); i++) {
		if (AcpiInstallAddressSpaceHandler(AcpiGbl_RootNode,
		    fwts_space_id_list[i], fwts_region_handler, fwts_region_init, NULL) != AE_OK) {
			fwts_log_error(fw,
				"Failed to install handler for %s space(%u)",
				AcpiUtGetRegionName((UINT8)fwts_space_id_list[i]),
				fwts_space_id_list[i]);
			return FWTS_ERROR;
		}
	}

	return FWTS_OK;
}

int fwtsInstallEarlyHandlers(fwts_framework *fw)
{
	int i;
	ACPI_HANDLE	handle;

	if (AcpiInstallInterfaceHandler(fwts_interface_handler) != AE_OK) {
		fwts_log_error(fw, "Failed to install interface handler.");
		return FWTS_ERROR;
	}

	if (AcpiInstallTableHandler(fwts_table_handler, NULL) != AE_OK) {
		fwts_log_error(fw, "Failed to install table handler.");
		return FWTS_ERROR;
	}

	if (AcpiInstallExceptionHandler(fwts_exception_handler) != AE_OK) {
		fwts_log_error(fw, "Failed to install exception handler.");
		return FWTS_ERROR;
	}

	if (AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, fwts_notify_handler, NULL) != AE_OK) {
		fwts_log_error(fw, "Failed to install system notify handler.");
		return FWTS_ERROR;
	}

	if (AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_DEVICE_NOTIFY, fwts_device_notify_handler, NULL) != AE_OK) {
		fwts_log_error(fw, "Failed to install device notify handler.");
		return FWTS_ERROR;
	}

	if (AcpiGetHandle(NULL, "\\_SB", &handle) == AE_OK) {
		if (AcpiInstallNotifyHandler(handle, ACPI_SYSTEM_NOTIFY, fwts_notify_handler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install notify handler.");
			return FWTS_ERROR;
		}

		if (AcpiRemoveNotifyHandler(handle, ACPI_SYSTEM_NOTIFY, fwts_notify_handler) != AE_OK) {
			fwts_log_error(fw, "Failed to remove notify handler.");
			return FWTS_ERROR;
		}

		if (AcpiInstallNotifyHandler(handle, ACPI_ALL_NOTIFY, fwts_notify_handler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install notify handler.");
			return FWTS_ERROR;
		}

		if (AcpiRemoveNotifyHandler(handle, ACPI_ALL_NOTIFY, fwts_notify_handler) != AE_OK) {
			fwts_log_error(fw, "Failed to remove notify handler.");
			return FWTS_ERROR;
		}

		if (AcpiInstallNotifyHandler(handle, ACPI_ALL_NOTIFY, fwts_notify_handler, NULL) != AE_OK) {
			fwts_log_error(fw, "Failed to install notify handler.");
			return FWTS_ERROR;
		}

		if (AcpiAttachData(handle, fwts_attached_data_handler, handle) != AE_OK) {
			fwts_log_error(fw, "Failed to attach data handler.");
			return FWTS_ERROR;
		}
		if (AcpiDetachData(handle, fwts_attached_data_handler) != AE_OK) {
			fwts_log_error(fw, "Failed to detach data handler.");
			return FWTS_ERROR;
		}

		if (AcpiAttachData(handle, fwts_attached_data_handler, handle) != AE_OK) {
			fwts_log_error(fw, "Failed to attach data handler.");
			return FWTS_ERROR;
		}
	}

	for (i = 0; i < ACPI_ARRAY_LENGTH(fwts_space_id_list); i++) {
		if (AcpiInstallAddressSpaceHandler(AcpiGbl_RootNode,
		    fwts_space_id_list[i], fwts_region_handler, fwts_region_init, NULL) != AE_OK) {
			fwts_log_error(fw,
				"Could not install an OpRegion handler for %s space(%u)",
				AcpiUtGetRegionName((UINT8)fwts_space_id_list[i]),
				fwts_space_id_list[i]);
			return FWTS_ERROR;
		}
	}
	return FWTS_OK;
}


/*
 *  fwts_acpica_init()
 *	Initialise ACPICA core engine
 */
int fwts_acpica_init(fwts_framework *fw)
{
	int i;
	int n;
	UINT32 init_flags = ACPI_FULL_INITIALIZATION;
	fwts_acpi_table_info *table;

	/* Abort if already initialised */
	if (fwts_acpica_init_called)
		return FWTS_ERROR;

	pthread_mutex_init(&mutex_lock_count, NULL);
	pthread_mutex_init(&mutex_thread_info, NULL);

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
		if (fwts_acpica_FADT == NULL) {
			fwts_log_error(fw, "Out of memory allocating FADT.");
			return FWTS_ERROR;
		}
		memcpy(fwts_acpica_FADT, table->data, table->length);

		if (fwts_acpi_find_table(fw, "DSDT", 0, &tbl) != FWTS_OK)
			return FWTS_ERROR;
		if (tbl) {
			fwts_acpica_DSDT = (void *)tbl->data;

			fwts_acpica_FADT->Dsdt = ACPI_PTR_TO_PHYSADDR(tbl->data);
			if (table->length >= 148)
				fwts_acpica_FADT->XDsdt = ACPI_PTR_TO_PHYSADDR(tbl->data);
		} else {
			fwts_acpica_DSDT = NULL;
			fwts_acpica_FADT->Dsdt = 0;
			fwts_acpica_FADT->XDsdt = 0;
		}

		if (fwts_acpi_find_table(fw, "FACS", 0, &tbl) != FWTS_OK)
			return FWTS_ERROR;
		if (tbl) {
			fwts_acpica_FADT->Facs = ACPI_PTR_TO_PHYSADDR(tbl->data);
			if (table->length >= 140)
				fwts_acpica_FADT->XFacs = ACPI_PTR_TO_PHYSADDR(tbl->data);
		} else {
			fwts_acpica_FADT->Facs = 0;
			fwts_acpica_FADT->XFacs = 0;
		}

		fwts_acpica_FADT->Header.Checksum = 0;
		fwts_acpica_FADT->Header.Checksum = (UINT8)-AcpiTbChecksum ((void*)fwts_acpica_FADT, table->length);
	} else {
		fwts_acpica_FADT = NULL;
	}

	/* Clone XSDT, make it point to tables in user address space */
	if (fwts_acpi_find_table(fw, "XSDT", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table != NULL) {
		uint64_t *entries;

		fwts_acpica_XSDT = fwts_low_calloc(1, table->length);
		if (fwts_acpica_XSDT == NULL) {
			fwts_log_error(fw, "Out of memory allocating XSDT.");
			return FWTS_ERROR;
		}
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
	} else {
		fwts_acpica_XSDT = NULL;
	}

	/* Clone RSDT, make it point to tables in user address space */
	if (fwts_acpi_find_table(fw, "RSDT", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table) {
		uint32_t *entries;

		fwts_acpica_RSDT = fwts_low_calloc(1, table->length);
		if (fwts_acpica_RSDT == NULL) {
			fwts_log_error(fw, "Out of memory allocating RSDT.");
			return FWTS_ERROR;
		}
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
	} else {
		fwts_acpica_RSDT = NULL;
	}

	/* Clone RSDP, make it point to tables in user address space */
	if (fwts_acpi_find_table(fw, "RSDP", 0, &table) != FWTS_OK)
		return FWTS_ERROR;
	if (table) {
		fwts_acpica_RSDP = fwts_low_calloc(1, table->length);
		if (fwts_acpica_RSDP == NULL) {
			fwts_log_error(fw, "Out of memory allocating RSDP.");
			return FWTS_ERROR;
		}
		memcpy(fwts_acpica_RSDP, table->data, table->length);

		if (table->length > 20)
			fwts_acpica_RSDP->XsdtPhysicalAddress = ACPI_PTR_TO_PHYSADDR(fwts_acpica_XSDT);
		fwts_acpica_RSDP->RsdtPhysicalAddress = ACPI_PTR_TO_PHYSADDR(fwts_acpica_RSDT);
		fwts_acpica_RSDP->Checksum = (UINT8)-AcpiTbChecksum ((void*)fwts_acpica_RSDP, ACPI_RSDP_CHECKSUM_LENGTH);
	} else {
		fwts_acpica_RSDP = NULL;
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


	(void)fwtsInstallEarlyHandlers(fw);
	AcpiEnableSubsystem(init_flags);
	AcpiInitializeObjects(init_flags);
	(void)fwtsInstallLateHandlers(fw);

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
	pthread_mutex_destroy(&mutex_lock_count);
	pthread_mutex_destroy(&mutex_thread_info);

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
		AcpiWalkNamespace(type, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,
			fwts_acpi_walk_for_object_names, NULL, list, NULL);

	return list;
}
