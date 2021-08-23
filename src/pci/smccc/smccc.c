/*
 *
 * Copyright (C) 2021 Canonical
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

#ifdef FWTS_ARCH_AARCH64

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "smccc_test.h"

/*
 * ARM SMCCC tests,
 *   https://developer.arm.com/documentation/den0115/latest
 */

/* SMCCC conduit types */
enum {
	FWTS_SMCCC_CONDUIT_NONE,
	FWTS_SMCCC_CONDUIT_SMC,
	FWTS_SMCCC_CONDUIT_HVC,
};

/* SMCCC API function ids */
#define PCI_VERSION	 (0x84000130)
#define PCI_FEATURES	 (0x84000131)
#define PCI_READ	 (0x84000132)
#define PCI_WRITE	 (0x84000133)
#define PCI_GET_SEG_INFO (0x84000134)

/* SMCCC API id to name mapping */
typedef struct {
	const uint32_t	pci_func_id;
	const char	*pci_func_id_name;
	bool		implemented;
} pci_func_id_t;

static pci_func_id_t pci_func_ids[] = {
	{ PCI_VERSION,		"PCI_VERSION",		false },
	{ PCI_FEATURES,		"PCI_FEATURES",		false },
	{ PCI_READ,		"PCI_READ",		false },
	{ PCI_WRITE,		"PCI_WRITE",		false },
	{ PCI_GET_SEG_INFO,	"PCI_GET_SEG_INFO",	false },
};

static const char *module_name = "smccc_test";
static const char *dev_name = "/dev/smccc_test";
static bool module_loaded;
static int smccc_fd = -1;

static int smccc_init(fwts_framework *fw)
{
	if (fwts_module_load(fw, module_name) != FWTS_OK) {
		module_loaded = false;
		smccc_fd = -1;
		return FWTS_ERROR;
	}

	smccc_fd = open(dev_name, O_RDWR);
	if (smccc_fd < 0) {
		smccc_fd = -1;
		fwts_log_error(fw, "Cannot open %s, errno=%d (%s)\n",
			dev_name, errno, strerror(errno));
	}

	return FWTS_OK;
}

static int smccc_deinit(fwts_framework *fw)
{
	if (smccc_fd >= 0) {
		close(smccc_fd);
		smccc_fd = -1;
	}

	if (module_loaded && fwts_module_unload(fw, module_name) != FWTS_OK)
		return FWTS_ERROR;

	module_loaded = true;

	return FWTS_OK;
}

/*
 *  smccc_pci_conduit_name()
 *	map the conduit number to human readable string
 */
static char *smccc_pci_conduit_name(struct smccc_test_arg *arg)
{
	static char unknown[32];

	switch (arg->conduit) {
	case FWTS_SMCCC_CONDUIT_NONE:
		return "SMCCC_CONDUIT_NONE";
	case FWTS_SMCCC_CONDUIT_HVC:
		return "SMCCC_CONDUIT_HVC";
	case FWTS_SMCCC_CONDUIT_SMC:
		return "SMCCC_CONDUIT_SMC";
	default:
		break;
	}

	snprintf(unknown, sizeof(unknown), "Unknown: 0x%x", arg->conduit);
	return unknown;
}

/*
 *  smccc_pci_conduit_check()
 *	check if conduit number is valid
 */
static int smccc_pci_conduit_check(fwts_framework *fw, struct smccc_test_arg *arg)
{
	switch (arg->conduit) {
	case FWTS_SMCCC_CONDUIT_HVC:
		return FWTS_OK;
	case FWTS_SMCCC_CONDUIT_SMC:
		return FWTS_OK;
	default:
		fwts_log_error(fw, "Invalid SMCCC conduit used: %s\n",
			       smccc_pci_conduit_name(arg));
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

/*
 *  smccc_pci_func_implemented()
 *	return true if function has been implemented, only valid
 *	once smccc_pci_features_test has been run.
 */
static bool smccc_pci_func_implemented(const uint32_t pci_func_id)
{
	size_t i;

	for (i = 0; i < FWTS_ARRAY_SIZE(pci_func_ids); i++) {
		if (pci_func_ids[i].pci_func_id == pci_func_id)
			return pci_func_ids[i].implemented;
	}
	return false;
}

/*
 *  smccc_pci_version_test()
 *	test SMCCC function PCI_VERSION
 */
static int smccc_pci_version_test(fwts_framework *fw)
{
	int ret;
	struct smccc_test_arg arg = { };

	arg.size = sizeof(arg);
	arg.w[0] = PCI_VERSION;

	ret = ioctl(smccc_fd, SMCCC_TEST_PCI_VERSION, &arg);
	if (ret < 0) {
		fwts_log_error(fw, "SMCCC test driver ioctl SMCCC_TEST_PCI_VERSION "
			"failed, errno=%d (%s)\n", errno, strerror(errno));
		return FWTS_ERROR;
	}
	if (smccc_pci_conduit_check(fw, &arg) != FWTS_OK)
		return FWTS_ERROR;

	fwts_log_info_verbatim(fw, "  SMCCC conduit type: 0x%x ('%s')",
		arg.conduit, smccc_pci_conduit_name(&arg));

	fwts_log_info_verbatim(fw, "  Major Rev: 0x%" PRIx16 ", Minor Rev: 0x%" PRIx16,
		(arg.w[0] >> 16) & 0xffff, arg.w[0] & 0xffff);
	fwts_passed(fw, "SMCCC v1.0 PCI_VERSION passed");

	return FWTS_OK;
}

/*
 *  smccc_pci_features_test()
 *	test SMCCC function PCI_FEATURES
 */
static int smccc_pci_features_test(fwts_framework *fw)
{
	struct smccc_test_arg arg = { };
	int ret, implemented_funcs = 0;
	bool passed = true;
	static const char *test = "SMCCC v1.0 PCI_FEATURES";
	size_t i;

	/*
	 *  Check SMCCC functions are implemented in the firmware
	 */
	for (i = 0; i < FWTS_ARRAY_SIZE(pci_func_ids); i++) {
		memset(&arg, 0, sizeof(arg));

		/* Assume it is not implemented */
		pci_func_ids[i].implemented = false;

		arg.size = sizeof(arg);
		arg.w[0] = PCI_FEATURES;
		arg.w[1] = pci_func_ids[i].pci_func_id;
		ret = ioctl(smccc_fd, SMCCC_TEST_PCI_FEATURES, &arg);
		if (ret < 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH, "SMCCC_PCI_VERSION",
				"SMCCC test driver ioctl SMCCC_TEST_PCI_FEATURES "
				"failed, errno=%d (%s)\n", errno, strerror(errno));
		} else {
			const bool implemented = (int)arg.w[0] >= 0;

			fwts_log_info_verbatim(fw, "  function 0x%x %-18.18s: %simplemented (%x)",
				pci_func_ids[i].pci_func_id,
				pci_func_ids[i].pci_func_id_name,
				implemented ? "" : "not ",
				arg.w[0]);
			if (implemented) {
				pci_func_ids[i].implemented = true;
				implemented_funcs++;
			}
		}
	}
	if (implemented_funcs == 0)
		fwts_log_warning(fw, "Note: No PCI functions were implemented");

	if (passed)
		fwts_passed(fw, "%s", test);

	return FWTS_OK;
}

/*
 *  smccc_pci_get_seg_info()
 *	test SMCCC function PCI_GET_SEG_INFO
 */
static int smccc_pci_get_seg_info(fwts_framework *fw)
{
	struct smccc_test_arg arg = { };
	int ret, segments = 0;
	bool passed = true;
	static const char *test = "SMCCC v1.0 PCI_GET_SEG_INFO";
	int i;

	if (!smccc_pci_func_implemented(PCI_GET_SEG_INFO)) {
		fwts_skipped(fw, "%s: not enabled on this platform", test);
		return EXIT_SUCCESS;
	}

	/*
	 *  Scan over all potential 65536 segment infos..
	 */
	for (i = 0; i <= 0xffff; i++) {
		memset(&arg, 0, sizeof(arg));

		arg.size = sizeof(arg);
		arg.w[0] = PCI_GET_SEG_INFO;
		arg.w[1] = i & 0xffff;
		ret = ioctl(smccc_fd, SMCCC_TEST_PCI_GET_SEG_INFO, &arg);
		if (ret < 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH, "SMCCC_PCI_VERSION",
				"SMCCC test driver ioctl PCI_GET_SEG_INFO "
				"failed, errno=%d (%s)\n", errno, strerror(errno));
			break;
		} else {
			if (arg.w[0] == 0) {
				const uint8_t pci_bus_start = arg.w[1] & 0xff;
				const uint8_t pci_bus_end = (arg.w[1] >> 8) & 0xff;
				const uint32_t next_seg = arg.w[2];

				fwts_log_info_verbatim(fw, "  PCI segment %4x: Bus 0x%2.2x .. 0x%2.2x",
					i, (int)pci_bus_start, (int)pci_bus_end);
				segments++;

				/*
				 * a zero next segment id marks the end
				 * of the segment information structs
				 */
				if (next_seg == 0)
					break;

				/* if next_seg is valid skip to this */
				if (next_seg <= 0xffff)
					i = next_seg;
			} else {
				fwts_log_info_verbatim(fw, "  PCI segment %4x: error return: %x\n", i, arg.w[0]);
				break;
			}
		}
	}
	if (segments == 0)
		fwts_log_warning(fw, "No PCI segments were found");

	if (passed)
		fwts_passed(fw, "%s", test);

	return FWTS_OK;
}

static fwts_framework_minor_test smccc_tests[] = {
	{ smccc_pci_version_test,	"Test PCI_VERSION" },
	{ smccc_pci_features_test,	"Test PCI_FEATURES" },
	{ smccc_pci_get_seg_info,	"Test PCI_GET_SEG_INFO" },
	{ NULL, NULL }
};

static fwts_framework_ops smcccops = {
	.description = "ARM64 PCI SMMCCC tests.",
	.init        = smccc_init,
	.deinit      = smccc_deinit,
	.minor_tests = smccc_tests
};

FWTS_REGISTER("smccc", &smcccops, FWTS_TEST_ANYTIME, FWTS_FLAG_UNSAFE | FWTS_FLAG_ROOT_PRIV)

#endif
