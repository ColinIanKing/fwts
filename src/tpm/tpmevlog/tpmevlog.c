/*
 * Copyright (C) 2020-2021 Canonical
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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fwts_tpm.h"

#define FWTS_TPM_LOG_DIR_PATH	"/sys/kernel/security"

static int tpmevlog_pcrindex_value_check(fwts_framework *fw, const uint32_t pcr)
{
	/*
	 * Current PCRs defined from 0 to 16, and 23 is for application support,
	 * defined from TCG PC Client Platform Firmware Profile Specification
	 * https://trustedcomputinggroup.org/resource/pc-client-specific-platform-firmware-profile-specification/
	 * 2.3.4 PCR Usage
	 */
	if ((pcr > 16) && (pcr != 23)) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "PCRIndexValue",
			"The PCR Index value is undefined, 0x%8.8" PRIx32 ".",
			pcr);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int tpmevlog_eventtype_check(fwts_framework *fw, const fwts_tpmlog_event_type event_type)
{
	switch (event_type) {
	case EV_PREBOOT_CERT:
	case EV_POST_CODE:
	case EV_UNUSED:
	case EV_NO_ACTION:
	case EV_SEPARATOR:
	case EV_ACTION:
	case EV_EVENT_TAG:
	case EV_S_CRTM_CONTENTS:
	case EV_S_CRTM_VERSION:
	case EV_CPU_MICROCODE:
	case EV_PLATFORM_CONFIG_FLAGS:
	case EV_TABLE_OF_DEVICES:
	case EV_IPL:
	case EV_IPL_PARTITION_DATA:
	case EV_NONHOST_CODE:
	case EV_NONHOST_CONFIG:
	case EV_NONHOST_INFO:
	case EV_OMIT_BOOT_DEVICE_EVENTS:
	case EV_EFI_EVENT_BASE:
	case EV_EFI_VARIABLE_DRIVER_CONFIG:
	case EV_EFI_VARIABLE_BOOT:
	case EV_EFI_BOOT_SERVICES_APPLICATION:
	case EV_EFI_BOOT_SERVICES_DRIVER:
	case EV_EFI_RUNTIME_SERVICES_DRIVER:
	case EV_EFI_GPT_EVENT:
	case EV_EFI_ACTION:
	case EV_EFI_PLATFORM_FIRMWARE_BLOB:
	case EV_EFI_HANDOFF_TABLES:
	case EV_EFI_HCRTM_EVENT:
	case EV_EFI_VARIABLE_AUTHORITY:
		return FWTS_OK;
	default:
		fwts_failed(fw, LOG_LEVEL_HIGH, "PCREventType",
			"The Event Type is undefined, 0x%8.8" PRIx32 ".",
			event_type);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int tpmevlog_algid_check(fwts_framework *fw, const TPM2_ALG_ID hash)
{
	switch (hash) {
	case TPM2_ALG_RSA:
	case TPM2_ALG_TDES:
	case TPM2_ALG_SHA1:
	case TPM2_ALG_HMAC:
	case TPM2_ALG_AES:
	case TPM2_ALG_MGF1:
	case TPM2_ALG_KEYEDHASH:
	case TPM2_ALG_XOR:
	case TPM2_ALG_SHA256:
	case TPM2_ALG_SHA384:
	case TPM2_ALG_SHA512:
	case TPM2_ALG_NULL:
	case TPM2_ALG_SM3_256:
	case TPM2_ALG_SM4:
	case TPM2_ALG_RSASSA:
	case TPM2_ALG_RSAES:
	case TPM2_ALG_RSAPSS:
	case TPM2_ALG_OAEP:
	case TPM2_ALG_ECDSA:
	case TPM2_ALG_ECDH:
	case TPM2_ALG_ECDAA:
	case TPM2_ALG_SM2:
	case TPM2_ALG_ECSCHNORR:
	case TPM2_ALG_ECMQV:
	case TPM2_ALG_KDF1_SP800_56A:
	case TPM2_ALG_KDF2:
	case TPM2_ALG_KDF1_SP800_108:
	case TPM2_ALG_ECC:
	case TPM2_ALG_SYMCIPHER:
	case TPM2_ALG_CAMELLIA:
	case TPM2_ALG_CMAC:
	case TPM2_ALG_CTR:
	case TPM2_ALG_SHA3_256:
	case TPM2_ALG_SHA3_384:
	case TPM2_ALG_SHA3_512:
	case TPM2_ALG_OFB:
	case TPM2_ALG_CBC:
	case TPM2_ALG_CFB:
	case TPM2_ALG_ECB:
		return FWTS_OK;
	default:
		fwts_failed(fw, LOG_LEVEL_HIGH, "AlgorithmID",
			"The AlgorithmID is undefined, 0x%4.4" PRIx16 ".",
			hash);
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int tpmevlog_v2_check(
	fwts_framework *fw,
	uint8_t *data,
	const size_t len)
{
	int ret = FWTS_OK;
	size_t len_remain = len;
	uint8_t *pdata = data;
	int i = 0;
	uint8_t vendor_info_size = 0;
	fwts_pc_client_pcr_event *pc_event;
	fwts_efi_spec_id_event *specid_evcent;
	fwts_spec_id_event_alg_sz *alg_sz;

	/* specid_event_check */
	if (len < sizeof(fwts_pc_client_pcr_event)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SpecidEventLength",
				"The length of the Specid event is %zd bytes "
				"is smaller than the PCClientPCREvent %zd bytes.",
				len_remain,
				sizeof(fwts_pc_client_pcr_event));
		return FWTS_ERROR;
	}

	pc_event = (fwts_pc_client_pcr_event *)pdata;
	ret = tpmevlog_pcrindex_value_check(fw, pc_event->pcr_index);
	if (ret != FWTS_OK)
		return ret;
	ret = tpmevlog_eventtype_check(fw, pc_event->event_type);
	if (ret != FWTS_OK)
		return ret;
	for (i = 0; i < TPM2_SHA1_DIGEST_SIZE; i++) {
		if (pc_event->digest[i] != 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "SpecIdEvDigest",
				"The digest filed of SpecId event should be all zero.");
			fwts_tpm_data_hexdump(fw, pc_event->digest, sizeof(pc_event->digest), "Digest");
		}
	}

	pdata += sizeof(fwts_pc_client_pcr_event);
	len_remain -= sizeof(fwts_pc_client_pcr_event);

	/* check the data length specid event */
	if (len_remain < sizeof(fwts_efi_spec_id_event)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SpecidEventLength",
				"The length of the Specid event is %zd bytes "
				"is smaller than the SpecId event %zd bytes.",
				len_remain,
				sizeof(fwts_efi_spec_id_event));
		return FWTS_ERROR;
	}

	specid_evcent = (fwts_efi_spec_id_event *)pdata;
	if (strcmp((char *)specid_evcent->signature, FWTS_TPM_EVENTLOG_V2_SIGNATURE) != 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SpecIdEvSignature",
			"The signature of SpecId event is not the same as expected "
			"Spec ID Event03, got %s.",
			(char *)specid_evcent->signature);
		return FWTS_ERROR;
	}

	/*
	 * Check the platform class value which defined in TCG ACPI Specification,
	 * 0 for client platforms, 1 for server platforms.
	 */
	if (specid_evcent->platform_class > 1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SpecIdEvPlatformClass",
			"The PlatformClass value of SpecId event is unexpected "
			"0 for client platforms, 1 for server platforms, "
			"got 0x%8.8" PRIx32 ".", specid_evcent->platform_class);
		return FWTS_ERROR;
	}

	if (specid_evcent->uintn_size < 1 || specid_evcent->uintn_size > 2) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SpecIdEvUINTNFields",
			"The size of the UINTN fieldsof SpecId event is unexpected "
			"0x01 indicates UINT32 and 0x02 indicates UINT64, "
			"got 0x%" PRIx8 ".", specid_evcent->uintn_size);
		return FWTS_ERROR;
	}

	if (specid_evcent->number_of_alg < 1) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "SpecIdEvAlgNumber",
			"The number of Hash algorithms of SpecId event must "
			"be set to a value of 0x01 or greater "
			"got 0x%" PRIx8 ".", specid_evcent->number_of_alg);
		return FWTS_ERROR;
	}

	pdata += sizeof(fwts_efi_spec_id_event);
	len_remain -= sizeof(fwts_efi_spec_id_event);
	alg_sz = (fwts_spec_id_event_alg_sz *)pdata;
	for (i = 0; i < specid_evcent->number_of_alg; i++) {
		if (len_remain < sizeof(fwts_spec_id_event_alg_sz)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "SpecidEventLength",
					"The length of the Specid event is %zd bytes "
					"is smaller than AlgorithmSize %zd bytes.",
					len_remain,
					sizeof(fwts_spec_id_event_alg_sz));
			return FWTS_ERROR;
		}

		ret = tpmevlog_algid_check(fw, alg_sz->algorithm_id);
		if (ret != FWTS_OK)
			return ret;

		pdata += sizeof(fwts_spec_id_event_alg_sz);
		len_remain -= sizeof(fwts_spec_id_event_alg_sz);
		alg_sz = (fwts_spec_id_event_alg_sz *)pdata;
	}

	vendor_info_size = *(uint8_t *)pdata;
	pdata += sizeof(vendor_info_size);
	len_remain -= sizeof(vendor_info_size);
	if (vendor_info_size > 0) {
		if (len_remain < vendor_info_size) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "SpecidEventLength",
					"The remain length of the Specid event is "
					"is too small (%zd bytes) for "
					"vendor info size.",
					len_remain);
			return FWTS_ERROR;
		}
		len_remain -= vendor_info_size;
		pdata += vendor_info_size;
	}

	/* Check the Crypto agile log format event */
	while (len_remain > 0) {
		uint32_t event_size;
		fwts_tcg_pcr_event2 *pcr_event2;

		if (len_remain < sizeof(fwts_tcg_pcr_event2)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "EventV2Length",
					"The length of the event2 is %zd bytes "
					"is smaller than the tcg pcr event2 %zd bytes.",
					len_remain,
					sizeof(fwts_tcg_pcr_event2));
			return FWTS_ERROR;
		}

		pcr_event2 = (fwts_tcg_pcr_event2 *)pdata;
		ret = tpmevlog_pcrindex_value_check(fw, pcr_event2->pcr_index);
		if (ret != FWTS_OK)
			return ret;
		ret = tpmevlog_eventtype_check(fw, pcr_event2->event_type);
		if (ret != FWTS_OK)
			return ret;

		pdata += sizeof(fwts_tcg_pcr_event2);
		len_remain -= sizeof(fwts_tcg_pcr_event2);
		for (i = 0; i < pcr_event2->digests_count; i++) {
			uint8_t hash_size;

			TPM2_ALG_ID alg_id = *(TPM2_ALG_ID *)pdata;

			ret = tpmevlog_algid_check(fw, alg_id);
			if (ret != FWTS_OK)
				return ret;

			pdata += sizeof(TPM2_ALG_ID);
			len_remain -= sizeof(TPM2_ALG_ID);

			hash_size = fwts_tpm_get_hash_size(alg_id);
			if (!hash_size) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM, "EventV2HashSize",
						"The hash size of the event2 is %zd bytes "
						"is smaller than the tcg pcr event2 %zd bytes.",
						len_remain,
						sizeof(fwts_tcg_pcr_event2));
				return FWTS_ERROR;
			}

			pdata += hash_size;
			len_remain -= hash_size;
		}

		event_size = *(uint32_t *)pdata;
		if (len_remain < event_size) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "EventV2Length",
					"The remain length of the event2 is %zd bytes "
					"is smaller than required event2 length %zd bytes.",
					len_remain,
					sizeof(event_size));
			return FWTS_ERROR;
		}
		pdata += (event_size + sizeof(event_size));
		len_remain -= (event_size + sizeof(event_size));

	}
	fwts_passed(fw, "Check TPM crypto agile event log test passed.");
	return FWTS_OK;
}

static int tpmevlog_check(fwts_framework *fw, uint8_t *data, size_t len)
{
	uint8_t *pdata = data;
	fwts_pc_client_pcr_event *pc_event = NULL;

	do {
		int ret;

		if (len < sizeof(fwts_pc_client_pcr_event)) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "EventLength",
					"The length of the event is %zd bytes "
					"is smaller than the PCClientPCREvent %zd bytes.",
					len,
					sizeof(fwts_pc_client_pcr_event));
			return FWTS_ERROR;
		}

		pc_event = (fwts_pc_client_pcr_event *)pdata;
		ret = tpmevlog_pcrindex_value_check(fw, pc_event->pcr_index);
		if (ret != FWTS_OK)
			return ret;
		ret = tpmevlog_eventtype_check(fw, pc_event->event_type);
		if (ret != FWTS_OK)
			return ret;

		if ((len - sizeof(fwts_pc_client_pcr_event)) < pc_event->event_data_size) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM, "EventLength",
					"The remain length of the event is %zd bytes "
					"is smaller than required event length "
					"%" PRIu32 " bytes.",
					len - sizeof(fwts_pc_client_pcr_event),
					pc_event->event_data_size);
			return FWTS_ERROR;
		}

		pdata += (sizeof(fwts_pc_client_pcr_event) + pc_event->event_data_size);
		len -= (sizeof(fwts_pc_client_pcr_event) + pc_event->event_data_size);
	} while (len > 0);

	fwts_passed(fw, "Check TPM event SHA1 log test passed.");
	return FWTS_OK;
}


static uint8_t *tpmevlog_load_file(const int fd, size_t *length)
{
	uint8_t *ptr = NULL, *tmp;
	size_t size = 0;
	char buffer[4096];

	*length = 0;

	for (;;) {
		const ssize_t n = read(fd, buffer, sizeof(buffer));

		if (n == 0)
			break;
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				free(ptr);
				return NULL;
			}
			continue;
		}
		if (n > (ssize_t)sizeof(buffer))
			goto err;
		if (size + n > 0xffffffff)
			goto err;

		if ((tmp = (uint8_t*)realloc(ptr, size + n + 1)) == NULL) {
			free(ptr);
			return NULL;
		}
		ptr = tmp;
		memcpy(ptr + size, buffer, n);
		size += n;
	}

	if (!ptr || !size)
		goto err_no_data;

	*length = size;
	return ptr;

err:
	free(ptr);
err_no_data:
	*length = 0;
	return NULL;
}

static int tpmevlog_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *tpmdir;
	bool tpm_logfile_found = false;

	if (!(dir = opendir(FWTS_TPM_LOG_DIR_PATH))) {
		fwts_log_info(fw, "Cannot find the TPM event log. Aborted.");
		return FWTS_ABORTED;
	}

	do {
		tpmdir = readdir(dir);
		if (tpmdir && strstr(tpmdir->d_name, "tpm")) {
			char path[PATH_MAX];
			int fd;
			size_t length;

			fwts_log_nl(fw);
			fwts_log_info_verbatim(fw, "%s", tpmdir->d_name);

			snprintf(path, sizeof(path), FWTS_TPM_LOG_DIR_PATH "/%s/binary_bios_measurements", tpmdir->d_name);

			if ((fd = open(path, O_RDONLY)) >= 0) {
				uint8_t *data;

				data = tpmevlog_load_file(fd, &length);
				tpm_logfile_found = true;
				if (data == NULL) {
					fwts_log_info(fw, "Cannot load the TPM event logs. Aborted.");
					(void)closedir(dir);
					(void)close(fd);
					return FWTS_ABORTED;
				} else {
					/* check if the TPM2 eventlog */
					if (strstr((char *)(data + sizeof(fwts_pc_client_pcr_event)), FWTS_TPM_EVENTLOG_V2_SIGNATURE)) {
						fwts_log_info_verbatim(fw, "Crypto agile log format (TPM2.0):");
						tpmevlog_v2_check(fw, data, length);
					} else {
						fwts_log_info_verbatim(fw, "SHA1 log format (TPM1.2):");
						tpmevlog_check(fw, data, length);
					}

					free(data);
				}
				(void)close(fd);
			}
		}
	} while (tpmdir);

	(void)closedir(dir);

	if (!tpm_logfile_found) {
		fwts_log_info(fw, "Cannot find the TPM event log. Aborted.");
		return FWTS_ABORTED;
	}
	return FWTS_OK;
}

static fwts_framework_minor_test tpmevlog_tests[] = {
	{ tpmevlog_test1, "Sanity check TPM event log." },
	{ NULL, NULL }
};

static fwts_framework_ops tpmevlog_ops = {
	.description = "Sanity check TPM event log.",
	.minor_tests = tpmevlog_tests
};

FWTS_REGISTER("tpmevlog", &tpmevlog_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV)
