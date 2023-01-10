/*
 * Copyright (C) 2020-2023 Canonical
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
#include "fwts_tpm.h"

/*
 *  fwts_tpm_data_hexdump
 *	hex dump of a tpm event log data
 */
void fwts_tpm_data_hexdump(
	fwts_framework *fw,
	const uint8_t *data,
	const size_t size,
	const char *str)
{
	size_t i;

	fwts_log_info_verbatim(fw, "%s: ", str);
	for (i = 0; i < size; i += 16) {
		char buffer[128];
		const size_t left = size - i;

		fwts_dump_raw_data(buffer, sizeof(buffer), data + i, i, left > 16 ? 16 : left);
		fwts_log_info_verbatim(fw, "%s", buffer + 2);
	}
}

/*
 *  fwts_tpm_evlog_type_to_string
 *	get hash size
 */
uint8_t fwts_tpm_get_hash_size(const TPM2_ALG_ID hash)
{
	uint8_t sz;

	switch (hash) {
	case TPM2_ALG_SHA1:
		sz = TPM2_SHA1_DIGEST_SIZE;
		break;
	case TPM2_ALG_SHA256:
		sz = TPM2_SHA256_DIGEST_SIZE;
		break;
	case TPM2_ALG_SHA384:
		sz = TPM2_SHA384_DIGEST_SIZE;
		break;
	case TPM2_ALG_SHA512:
		sz = TPM2_SHA512_DIGEST_SIZE;
		break;
	default:
		sz = 0;
		break;
	}

	return sz;
}
