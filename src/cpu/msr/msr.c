/*
 * Copyright (C) 2011 Canonical
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

#ifdef FWTS_ARCH_INTEL

typedef void (*msr_callback_check)(fwts_framework *fw, uint64_t val);

static int ncpus;
static bool intel_cpu;
static bool amd_cpu;
static fwts_cpuinfo_x86 *cpuinfo;

static int msr_init(fwts_framework *fw)
{
	if ((cpuinfo = fwts_cpu_get_info(0)) == NULL) {
		fwts_log_error(fw, "Cannot get CPU info");
		return FWTS_ERROR;
	}
	if (cpuinfo->vendor_id == NULL) {
		fwts_log_error(fw, "Cannot get CPU vendor_id");
		return FWTS_ERROR;
	}
	intel_cpu = strstr(cpuinfo->vendor_id, "Intel") != NULL;
	amd_cpu = strstr(cpuinfo->vendor_id, "AuthenticAMD") != NULL;

	if ((ncpus = fwts_cpu_enumerate()) == FWTS_ERROR) {
		fwts_log_error(fw, "Cannot detect the number of CPUs on this machine.");
		return FWTS_ABORTED;
	}
	return FWTS_OK;
}

static int msr_deinit(fwts_framework *fw)
{
	fwts_cpu_free_info(cpuinfo);

	return FWTS_OK;
}

static int msr_consistent(const uint32_t msr,
	const int shift,
	const uint64_t mask,
	uint64_t *vals,
	int *inconsistent_count,
	bool *inconsistent)
{
	int cpu;

	*inconsistent_count = 0;

	for (cpu=0; cpu<ncpus; cpu++) {
		uint64_t val;
		if (fwts_cpu_readmsr(cpu, msr, &val) != FWTS_OK) {
			return FWTS_ERROR;
		}
		val >>= shift;
		val &= mask;
		vals[cpu] = val;
	}

	for (cpu=0; cpu<ncpus; cpu++) {
		if (vals[0] != vals[cpu]) {
			(*inconsistent_count)++;
			inconsistent[cpu] = true;
		} else {
			inconsistent[cpu] = false;
		}
	}
	return FWTS_OK;
}

static int msr_consistent_check(fwts_framework *fw,
	const fwts_log_level level,
	const char *msr_name,
	const uint32_t msr,
	const int shift,
	const uint64_t mask,
	const msr_callback_check callback)
{
	uint64_t *vals;
	bool *inconsistent;
	int inconsistent_count;
	int cpu;

	if ((vals = calloc(ncpus, sizeof(uint64_t))) == NULL) {
		fwts_log_error(fw, "Out of memory allocating msr value buffers.");
		return FWTS_ERROR;
	}
	if ((inconsistent = calloc(ncpus, sizeof(bool))) == NULL) {
		fwts_log_error(fw, "Out of memory allocating msr value buffers.");
		free(vals);
		return FWTS_ERROR;
	}

	if (msr_consistent(msr, shift, mask,
		vals, &inconsistent_count, inconsistent) != FWTS_OK) {
		//fwts_log_info(fw, "Cannot read MSR %s (0x%x).", msr_name, msr);
		free(inconsistent);
		free(vals);
		return FWTS_ERROR;
	}

	if (inconsistent_count > 0) {
		fwts_failed(fw, level, "MSRCPUsInconsistent",
			"MSR %s (0x%x) has %d inconsistent values across %d CPUs for (shift: %d mask: 0x%llx).",
			msr_name, (int)msr,
			inconsistent_count, ncpus, shift,
			(unsigned long long)mask);

		for (cpu=1; cpu<ncpus; cpu++) {
			if (inconsistent[cpu])
				fwts_log_info(fw, "MSR CPU 0 -> 0x%llx vs CPU %d -> 0x%llx",
					(unsigned long long)vals[0], cpu,
					(unsigned long long)vals[cpu]);
		}
	} else {
		fwts_passed(fw, "MSR %s (0x%x) (mask:%llx) was consistent across %d CPUs.",
			msr_name, (int)msr, (unsigned long long)mask, ncpus);
		if (callback)
			callback(fw, vals[0]);
	}


	free(inconsistent);
	free(vals);

	return FWTS_OK;
}

static int msr_pstate_ratios(fwts_framework *fw)
{
	if (intel_cpu) {
		msr_consistent_check(fw, LOG_LEVEL_HIGH, "Minimum P-State", 0xce, 8, 0xff, NULL);
		msr_consistent_check(fw, LOG_LEVEL_HIGH, "Maximum P-State", 0xce, 40, 0xff, NULL);
	} else
		fwts_skipped(fw, "Non-Intel CPU, test skipped.");

	return FWTS_OK;
}

static void msr_c1_c3_autodemotion_check(fwts_framework *fw, uint64_t val)
{
	fwts_log_info(fw, "C1 and C3 Autodemotion %s.",
		val == 3 ? "enabled" : "disabled");
}


static int msr_c1_c3_autodemotion(fwts_framework *fw)
{
	if (intel_cpu) {
		msr_consistent_check(fw, LOG_LEVEL_HIGH, "C1 and C3 Autodemotion", 0xe2, 25, 0x03, msr_c1_c3_autodemotion_check);
	} else
		fwts_skipped(fw, "Non-Intel CPU, test skipped.");

	return FWTS_OK;
}

static int msr_smrr(fwts_framework *fw)
{
	uint64_t val;

	if (intel_cpu) {
		if (fwts_cpu_readmsr(0, 0xfe, &val) != FWTS_OK) {
			fwts_skipped(fw, "Cannot read MSR 0xfe.");
			return FWTS_ERROR;
		}

		if (((val >> 11) & 1) == 0)
			fwts_log_info(fw, "SMRR not supported by this CPU.");
		else {
			msr_consistent_check(fw, LOG_LEVEL_HIGH, "SMRR_PHYSBASE", 0x1f2, 12, 0xfffff, NULL);
			msr_consistent_check(fw, LOG_LEVEL_HIGH, "SMRR_TYPE", 0x1f2, 0, 0x7, NULL);
			msr_consistent_check(fw, LOG_LEVEL_HIGH, "SMRR_PHYSMASK", 0x1f3, 12, 0xfffff, NULL);
			msr_consistent_check(fw, LOG_LEVEL_HIGH, "SMRR_VALID", 0x1f3, 11, 0x1, NULL);

			if (fwts_cpu_readmsr(0, 0x1f2, &val) == FWTS_OK) {
				uint64_t physbase = val & 0xfffff000;
				uint64_t type = val & 7;
				if ((physbase % 0x7fffff) != 0)
					fwts_failed(fw, LOG_LEVEL_HIGH, "MSRSMRR_PHYSBASE8MBBoundary",
						"SMRR: SMRR_PHYSBASE is NOT on an 8MB boundary: %llx.", (unsigned long long)physbase);
				if (type != 6)
					fwts_failed(fw, LOG_LEVEL_HIGH, "MSRSMRR_TYPE",
						"SMRR: SMRR_TYPE is 0x%x, should be 0x6 (Write-Back).", (int)type);
			}
			if (fwts_cpu_readmsr(0, 0x1f2, &val) == FWTS_OK) {
				uint64_t physmask = val & 0xfffff000;
				uint64_t valid = (val >> 11) & 1;

				if (physmask < 0x80000) {
					fwts_failed(fw, LOG_LEVEL_HIGH, "MSRSMRRRegion",
						"SMRR: region needs to be at least 8MB, SMRR_PHYSMASK = %llx.",
						(unsigned long long) physmask);
				}
				if (!valid)
					fwts_failed(fw, LOG_LEVEL_HIGH, "MSRSMRRValidBit",
						"SMRR: valid bit is 0, it should be 1 (enabled).");
			}
		}
	} else
		fwts_skipped(fw, "Non-Intel CPU, test skipped.");

	return FWTS_OK;
}

typedef struct {
	const char *name;
	const uint32_t msr;
	const int shift;
	const uint64_t mask;
	const msr_callback_check callback;
} msr_info;


/* From AMD Architecture Programmer's Manual, Volume 2: System Programming, Appending A */
static msr_info AMD_MSRs[] = {
	{ "MTRRCAP",		0x000000fe,	0, 0xfffULL, NULL },
	{ "SYSENTER_CS",	0x00000174,	0, 0xffffULL, NULL },
	{ "SYSENTER_ESP",	0x00000175,	0, ~0, NULL },
	{ "SYSENTER_EIP",	0x00000176,	0, ~0, NULL },
	{ "MCG_CAP",		0x00000179,	0, 0x1ff0fffULL, NULL },
	{ "MCG_STATUS",		0x0000017a,	0, ~0, NULL },
	{ "MCG_CTL",		0x0000017b,	0, ~0, NULL },
	{ "MTRR_PHYSBASE0",	0x00000200,	0, ~0, NULL },
	{ "MTRR_PHYSMASK0",	0x00000201,	0, ~0, NULL },
	{ "MTRR_PHYSBASE1",	0x00000202,	0, ~0, NULL },
	{ "MTRR_PHYSMASK1",	0x00000203,	0, ~0, NULL },
	{ "MTRR_PHYSBASE2",	0x00000204,	0, ~0, NULL },
	{ "MTRR_PHYSMASK2",	0x00000205,	0, ~0, NULL },
	{ "MTRR_PHYSBASE3",	0x00000206,	0, ~0, NULL },
	{ "MTRR_PHYSMASK3",	0x00000207,	0, ~0, NULL },
	{ "MTRR_PHYSBASE4",	0x00000208,	0, ~0, NULL },
	{ "MTRR_PHYSMASK4",	0x00000209,	0, ~0, NULL },
	{ "MTRR_PHYSBASE5",	0x0000020a,	0, ~0, NULL },
	{ "MTRR_PHYSMASK5",	0x0000020b,	0, ~0, NULL },
	{ "MTRR_PHYSBASE6",	0x0000020c,	0, ~0, NULL },
	{ "MTRR_PHYSMASK6",	0x0000020d,	0, ~0, NULL },
	{ "MTRR_PHYSBASE7",	0x0000020e,	0, ~0, NULL },
	{ "MTRR_PHYSMASK7",	0x0000020f,	0, ~0, NULL },
	{ "MTRR_FIX64K_000",	0x00000250,	0, ~0, NULL },
	{ "MTRR_FIX16K_800",	0x00000258,	0, ~0, NULL },
	{ "MTRR_FIX16K_a00",	0x00000259,	0, ~0, NULL },
	{ "MTRR_FIX4K_C000",	0x00000268,	0, ~0, NULL },
	{ "MTRR_FIX4K_C800",	0x00000269,	0, ~0, NULL },
	{ "MTRR_FIX4K_D000",	0x0000026a,	0, ~0, NULL },
	{ "MTRR_FIX4K_D800",	0x0000026b,	0, ~0, NULL },
	{ "MTRR_FIX4K_E000",	0x0000026c,	0, ~0, NULL },
	{ "MTRR_FIX4K_E800",	0x0000026d,	0, ~0, NULL },
	{ "MTRR_FIX4K_F000",	0x0000026e,	0, ~0, NULL },
	{ "MTRR_FIX4K_F800",	0x0000026f,	0, ~0, NULL },
	{ "PAT",		0x00000277,	0, 0x707070707070703ULL, NULL },
	{ "MTRR_DEF_TYPE",	0x000002ff,	0, 0xc0fULL, NULL },
	{ "EFER",		0xc0000080,	0, 0xd01ULL, NULL },
	{ "STAR",		0xc0000081,	0, ~0, NULL },
	{ "LSTAR",		0xc0000082,	0, ~0, NULL },
	{ "FMASK",		0xc0000084,	0, ~0, NULL },
	//{ "FS_BASE",		0xc0000100,	0, ~0, NULL },
	//{ "GS_BASE",		0xc0000101, 	0, ~0, NULL },
	{ "KERNEL_GS_BASE",	0xc0000102, 	0, ~0, NULL },
	//{ "TSC_AUX",		0xc0000103, 	0, 0xffffffffULL, NULL },
	{ "SYSCFG",		0xc0010010,	0, ~0, NULL },
	{ "IORRBase0",		0xc0010016,	0, ~0, NULL },
	{ "IORRMask0",		0xc0010017,	0, ~0, NULL },
	{ "IORRBase1",		0xc0010018,	0, ~0, NULL },
	{ "IORRMask1",		0xc0010019,	0, ~0, NULL },
	{ "TOP_MEM",		0xc001001a,	0, ~0, NULL },
	{ "TOP_MEM2",		0xc001001d,	0, ~0, NULL },
	{ "Processor_Name_String", 0xc0010030,	0, ~0, NULL },
	{ "Processor_Name_String", 0xc0010031,	0, ~0, NULL },
	{ "Processor_Name_String", 0xc0010032,	0, ~0, NULL },
	{ "Processor_Name_String", 0xc0010033,	0, ~0, NULL },
	{ "Processor_Name_String", 0xc0010034,	0, ~0, NULL },
	{ "Processor_Name_String", 0xc0010035,	0, ~0, NULL },
	{ "TSC Ratio",		0xc0010104,	0, ~0, NULL },
	//{ "SMBASE",		0xc0010111,	0, ~0, NULL },
	{ "SMM_ADDR",		0xc0010112,	0, ~0, NULL },
	{ "SMM_MASK",		0xc0010113,	0, ~0, NULL },
	{ "VM_CR",		0xc0010114,	0, ~0, NULL },
	{ "IGNNE",		0xc0010115,	0, ~0, NULL },
	{ "SMM_CTL",		0xc0010116,	0, ~0, NULL },
	{ "VM_HSAVE_PA",	0xc0010117,	0, ~0, NULL },
	{ "SVM_KEY_MSR",	0xc0010118,	0, ~0, NULL },
	{ "OSVW_ID_Length",	0xc0010140,	0, ~0, NULL },
	{ NULL,			0x00000000,	0, 0 , NULL }
};

static msr_info IA32_MSRs[] = {
	//{ "P5_MC_ADDR",		0x00000000,	0, ~0, NULL },
	{ "P5_MC_TYPE",		0x00000001,	0, ~0, NULL },
	{ "MONITOR_FILTER_SIZE",0x00000006,	0, ~0, NULL },
	//{ "TIME_STAMP_COUNTER",	0x00000010,	0, ~0, NULL },
	{ "PLATFORM_ID",	0x00000017,	0, 0x1c000000000000ULL, NULL },
	{ "EBL_CR_POWERON",	0x0000002a,	0, ~0, NULL },
	{ "APIC_BASE",		0x0000001b,	0, 0xfffffffffffffeffULL, NULL },
	{ "FEATURE_CONTROL",	0x0000003a,	0, 0xff07ULL, NULL },
	{ "BIOS_SIGN_ID",	0x0000008b,	0, 0xffffffff00000000ULL, NULL },
	{ "MTRRCAP",		0x000000fe,	0, 0xfffULL, NULL },
	{ "SYSENTER_CS",	0x00000174,	0, 0xffffULL, NULL },
	{ "SYSENTER_ESP",	0x00000175,	0, ~0, NULL },
	{ "SYSENTER_EIP",	0x00000176,	0, ~0, NULL },
	{ "MCG_CAP",		0x00000179,	0, 0x1ff0fffULL, NULL },
	{ "MCG_STATUS",		0x0000017a,	0, ~0, NULL },
	{ "MCG_CTL",		0x0000017b,	0, ~0, NULL },
	{ "CLOCK_MODULATION",	0x0000019a,	0, 0x1fULL, NULL },
	{ "THERM_INTERRUPT",	0x0000019b,	0, 0x180801fULL, NULL },
	//{ "THERM_STATUS",	0x0000019c,	0, 0x80000fffULL, NULL },
	{ "MISC_ENABLE",	0x000001a0,	0, 0x400c51889ULL, NULL },
	{ "PACKAGE_THERM_INTERRUPT", 0x000001b2,0, 0x1ffff17ULL, NULL },
	{ "SMRR_PHYSBASE",	0x000001f2,	0, 0xfffff0ffULL, NULL },
	{ "SMRR_PHYSMASK",	0x000001f3,	0, 0xfffff800ULL, NULL },
	{ "PLATFORM_DCA_CAP",	0x000001f8,	0, ~0, NULL },
	{ "CPU_DCA_CAP",	0x000001f9,	0, ~0, NULL },
	{ "DCA_O_CAP",		0x000001fa,	0, 0x501e7ffULL, NULL },
	{ "MTRR_PHYSBASE0",	0x00000200,	0, ~0, NULL },
	{ "MTRR_PHYSMASK0",	0x00000201,	0, ~0, NULL },
	{ "MTRR_PHYSBASE1",	0x00000202,	0, ~0, NULL },
	{ "MTRR_PHYSMASK1",	0x00000203,	0, ~0, NULL },
	{ "MTRR_PHYSBASE2",	0x00000204,	0, ~0, NULL },
	{ "MTRR_PHYSMASK2",	0x00000205,	0, ~0, NULL },
	{ "MTRR_PHYSBASE3",	0x00000206,	0, ~0, NULL },
	{ "MTRR_PHYSMASK3",	0x00000207,	0, ~0, NULL },
	{ "MTRR_PHYSBASE4",	0x00000208,	0, ~0, NULL },
	{ "MTRR_PHYSMASK4",	0x00000209,	0, ~0, NULL },
	{ "MTRR_PHYSBASE5",	0x0000020a,	0, ~0, NULL },
	{ "MTRR_PHYSMASK5",	0x0000020b,	0, ~0, NULL },
	{ "MTRR_PHYSBASE6",	0x0000020c,	0, ~0, NULL },
	{ "MTRR_PHYSMASK6",	0x0000020d,	0, ~0, NULL },
	{ "MTRR_PHYSBASE7",	0x0000020e,	0, ~0, NULL },
	{ "MTRR_PHYSMASK7",	0x0000020f,	0, ~0, NULL },
	{ "MTRR_PHYSBASE8",	0x00000210,	0, ~0, NULL },
	{ "MTRR_PHYSMASK8",	0x00000211,	0, ~0, NULL },
	{ "MTRR_PHYSBASE9",	0x00000212,	0, ~0, NULL },
	{ "MTRR_PHYSMASK9",	0x00000213,	0, ~0, NULL },
	{ "MTRR_FIX64K_000",	0x00000250,	0, ~0, NULL },
	{ "MTRR_FIX16K_800",	0x00000258,	0, ~0, NULL },
	{ "MTRR_FIX16K_a00",	0x00000259,	0, ~0, NULL },
	{ "MTRR_FIX4K_C000",	0x00000268,	0, ~0, NULL },
	{ "MTRR_FIX4K_C800",	0x00000269,	0, ~0, NULL },
	{ "MTRR_FIX4K_D000",	0x0000026a,	0, ~0, NULL },
	{ "MTRR_FIX4K_D800",	0x0000026b,	0, ~0, NULL },
	{ "MTRR_FIX4K_E000",	0x0000026c,	0, ~0, NULL },
	{ "MTRR_FIX4K_E800",	0x0000026d,	0, ~0, NULL },
	{ "MTRR_FIX4K_F000",	0x0000026e,	0, ~0, NULL },
	{ "MTRR_FIX4K_F800",	0x0000026f,	0, ~0, NULL },
	{ "PAT",		0x00000277,	0, 0x707070707070703ULL, NULL },
	{ "MC0_CTL2",		0x00000280,	0, 0x40007fffULL, NULL },
	{ "MC1_CTL2",		0x00000281,	0, 0x40007fffULL, NULL },
	{ "MC2_CTL2",		0x00000282,	0, 0x40007fffULL, NULL },
	{ "MC3_CTL2",		0x00000283,	0, 0x40007fffULL, NULL },
	{ "MC4_CTL2",		0x00000284,	0, 0x40007fffULL, NULL },
	{ "MC5_CTL2",		0x00000285,	0, 0x40007fffULL, NULL },
	{ "MC6_CTL2",		0x00000286,	0, 0x40007fffULL, NULL },
	{ "MC7_CTL2",		0x00000287,	0, 0x40007fffULL, NULL },
	{ "MC8_CTL2",		0x00000288,	0, 0x40007fffULL, NULL },
	{ "MC9_CTL2",		0x00000289,	0, 0x40007fffULL, NULL },
	{ "MC10_CTL2",		0x0000028a,	0, 0x40007fffULL, NULL },
	{ "MC11_CTL2",		0x0000028b,	0, 0x40007fffULL, NULL },
	{ "MC12_CTL2",		0x0000028c,	0, 0x40007fffULL, NULL },
	{ "MC13_CTL2",		0x0000028d,	0, 0x40007fffULL, NULL },
	{ "MC14_CTL2",		0x0000028e,	0, 0x40007fffULL, NULL },
	{ "MC15_CTL2",		0x0000028f,	0, 0x40007fffULL, NULL },
	{ "MC16_CTL2",		0x00000290,	0, 0x40007fffULL, NULL },
	{ "MC17_CTL2",		0x00000291,	0, 0x40007fffULL, NULL },
	{ "MC18_CTL2",		0x00000292,	0, 0x40007fffULL, NULL },
	{ "MC19_CTL2",		0x00000293,	0, 0x40007fffULL, NULL },
	{ "MC20_CTL2",		0x00000294,	0, 0x40007fffULL, NULL },
	{ "MC21_CTL2",		0x00000295,	0, 0x40007fffULL, NULL },
	{ "MTRR_DEF_TYPE",	0x000002ff,	0, 0xc0fULL, NULL },
	{ "PEBS_ENABLE",	0x000003f1,	0, 0x1ULL, NULL },

	{ "VMX_BASIC",		0x00000480,	0, ~0, NULL },
	{ "VMX_PINPASED_CTLS",	0x00000481,	0, ~0, NULL },
	{ "VMX_PROCBASED_CTLS",	0x00000482,	0, ~0, NULL },
	{ "VMX_EXIT_CTLS",	0x00000483,	0, ~0, NULL },
	{ "VMX_ENTRY_CTLS",	0x00000484,	0, ~0, NULL },
	{ "VMX_MISC",		0x00000485,	0, ~0, NULL },
	{ "VMX_CR0_FIXED0",	0x00000486,	0, ~0, NULL },
	{ "VMX_CR0_FIXED1",	0x00000487,	0, ~0, NULL },
	{ "VMX_CR4_FIXED0",	0x00000488,	0, ~0, NULL },
	{ "VMX_CR4_FIXED1",	0x00000489,	0, ~0, NULL },
	{ "VMX_VMX_VMCS_ENUM",	0x0000048a,	0, ~0, NULL },
	{ "VMX_PROCBASED_CTLS2",0x0000048b,	0, ~0, NULL },
	{ "VMX_EPT_VPID_CAP",	0x0000048c,	0, ~0, NULL },
	{ "VMX_TRUE_PINBASED_CTLS",0x0000048d,	0, ~0, NULL },
	{ "VMX_TRUE_PROCBASED_CTLS",0x0000048e,	0, ~0, NULL },
	{ "VMX_TRUE_EXIT_CTLS",	0x0000048f,	0, ~0, NULL },
	{ "VMX_TRUE_ENTRY_CTLS",0x00000490,	0, ~0, NULL },

	{ "A_PMC0",		0x000004c1,	0, ~0, NULL },
	{ "A_PMC1",		0x000004c2,	0, ~0, NULL },
	{ "A_PMC2",		0x000004c3,	0, ~0, NULL },
	{ "A_PMC3",		0x000004c4,	0, ~0, NULL },
	{ "A_PMC4",		0x000004c5,	0, ~0, NULL },
	{ "A_PMC5",		0x000004c6,	0, ~0, NULL },
	{ "A_PMC6",		0x000004c7,	0, ~0, NULL },
	{ "A_PMC7",		0x000004c8,	0, ~0, NULL },

	{ "EFER",		0xc0000080,	0, 0xd01ULL, NULL },
	{ "STAR",		0xc0000081,	0, ~0, NULL },
	{ "LSTAR",		0xc0000082,	0, ~0, NULL },
	{ "FMASK",		0xc0000084,	0, ~0, NULL },
	//{ "FS_BASE",		0xc0000100,	0, ~0, NULL },
	//{ "GS_BASE",		0xc0000101, 	0, ~0, NULL },
	{ "KERNEL_GS_BASE",	0xc0000102, 	0, ~0, NULL },
	{ "TSC_AUX",		0xc0000103, 	0, 0xffffffffULL, NULL },
	{ NULL,			0x00000000,	0, 0 , NULL },
};

static msr_info IA32_atom_MSRs[] = {
	{ "BIOS_UPDT_TRIG",	0x00000079,	0, ~0, NULL },
	{ "BIOS_SIGN_ID",	0x0000008b,	0, ~0, NULL },
	{ "MSR_FSB_FREQ",	0x000000cd,	0, 0x7ULL, NULL },
	{ "MSR_BBL_CR_CTL3",	0x0000011e,	0, 0x800101ULL, NULL },
	{ "PERFEVTSEL0",	0x00000186,	0, ~0, NULL },
	{ "PERFEVTSEL1",	0x00000187,	0, ~0, NULL },
	{ "CLOCK_MODULATION",	0x0000019a,	0, ~0, NULL },
	{ "MSR_THERM2_CTL",	0x0000019d,	0, 0x10000ULL, NULL },
	{ "MC0_CTL",		0x00000400,	0, ~0, NULL },
	{ "MC0_STATUS",		0x00000401,	0, ~0, NULL },
	{ "MC0_ADDR",		0x00000402,	0, ~0, NULL },
	{ "MC1_CTL",		0x00000404,	0, ~0, NULL },
	{ "MC1_STATUS",		0x00000405,	0, ~0, NULL },
	{ "MC2_CTL",		0x00000408,	0, ~0, NULL },
	{ "MC2_STATUS",		0x00000409,	0, ~0, NULL },
	{ "MC2_ADDR",		0x0000040a,	0, ~0, NULL },
	{ "MC3_CTL",		0x0000040c,	0, ~0, NULL },
	{ "MC3_STATUS",		0x0000040d,	0, ~0, NULL },
	{ "MC3_ADDR",		0x0000040e,	0, ~0, NULL },
	{ "MC4_CTL",		0x00000410,	0, ~0, NULL },
	{ "MC4_STATUS",		0x00000411,	0, ~0, NULL },
	{ "MC4_ADDR",		0x00000412,	0, ~0, NULL },
	{ NULL,			0x00000000,	0, 0 , NULL },
};

static msr_info IA32_nehalem_MSRs[] = {
	{ "BIOS_UPDT_TRIG",		0x00000079,	0, ~0, NULL },
	{ "MSR_PLATFORM_INFO",		0x000000ce,	0, 0xff003001ff00ULL, NULL },
	{ "MSR_PKG_CST_CONFIG_CONTROL",	0x000000e2,	0, 0x7008407ULL, NULL },
	{ "MSR_PMG_IO_CAPTURE_BASE",	0x000000e4,	0, 0x7ffffULL, NULL },
	{ "CLOCK_MODULATION",		0x0000019a,	0, 0x1fULL, NULL },
	{ "MSR_TEMPERATURE_TARGET",	0x000001a2,	0, 0xff0000, NULL },
	{ "MSR_OFFCORE_RSP_0",		0x000001a6,	0, ~0, NULL },
	{ "MSR_MISC_PWR_MGMT",		0x000001aa,	0, 0x3ULL, NULL },
	{ "MSR_TURBO_POWER_CURRENT_LIMIT",0x000001ac,	0, 0xffffffffULL, NULL },
	{ "MSR_TURBO_RATIO_LIMIT",	0x000001ad,	0, 0xffffffffULL, NULL },
	{ "MSR_POWER_CTL",		0x000001fc,	0, 0x2ULL, NULL },
	{ NULL,				0x00000000,	0, 0 , NULL },
};

static msr_info IA32_sandybridge_MSRs[] = {
	{ "BIOS_UPDT_TRIG",		0x00000079,	0, ~0, NULL },
	{ "BIOS_SIGN_ID",		0x0000008b,	0, ~0, NULL },
	{ "MSR_PLATFORM_INFO",		0x000000ce,	0, 0xff003001ff00ULL, NULL },
	{ "MSR_PKG_CST_CONFIG_CONTROL",	0x000000e2,	0, 0x7008407ULL, NULL },
	{ "MSR_PMG_IO_CAPTURE_BASE",	0x000000e4,	0, 0x7ffffULL, NULL },
	{ "CLOCK_MODULATION",		0x0000019a,	0, 0x1fULL, NULL },
	{ "MSR_TEMPERATURE_TARGET",	0x000001a2,	0, 0xff0000, NULL },
	{ "MSR_OFFCORE_RSP_0",		0x000001a6,	0, ~0, NULL },
	{ "MSR_TURBO_RATIO_LIMIT",	0x000001ad,	0, 0xffffffffULL, NULL },
	{ "MSR_POWER_CTL",		0x000001fc,	0, 0x2ULL, NULL },
	{ "MSR_PKGC3_IRTL",		0x0000060a,	0, 0x9fffULL, NULL },
	{ "MSR_PKGC6_IRTL",		0x0000060b,	0, 0x9fffULL, NULL },
	{ "MSR_PKGC7_IRTL",		0x0000060c,	0, 0x9fffULL, NULL },
	{ "MSR_PKG_RAPL_POWER_LIMIT",	0x00000610,	0, ~0, NULL },
	{ "MSR_PKG_RAPL_POWER_INFO",	0x00000614,	0, ~0, NULL },
	{ "MSR_PP0_POWER_LIMIT",	0x00000638,	0, ~0, NULL },
	{ "MSR_PP0_POLICY",		0x0000063a,	0, ~0, NULL },
	{ "MSR_PP1_POWER_LIMIT",	0x00000640,	0, ~0, NULL },
	{ "MSR_PP1_POLICY",		0x00000642,	0, ~0, NULL },
	{ NULL,				0x00000000,	0, 0 , NULL },
};

static int msr_table_check(fwts_framework *fw, msr_info *info)
{
	int i;

	for (i=0; info[i].name != NULL; i++)
		msr_consistent_check(fw, LOG_LEVEL_MEDIUM,
			info[i].name, info[i].msr, info[i].shift, info[i].mask, info[i].callback);

	return FWTS_OK;
}

static int msr_cpu_generic(fwts_framework *fw)
{
	if (intel_cpu)
		msr_table_check(fw, IA32_MSRs);
	else if (amd_cpu)
		msr_table_check(fw, AMD_MSRs);
	else
		fwts_skipped(fw, "Not an AMD or Intel CPU, test skipped.");

	return FWTS_OK;
}

static int msr_cpu_specific(fwts_framework *fw)
{
	if (intel_cpu) {
        	switch (cpuinfo->x86_model) {
		case 0x1A: /* Core i7, Xeon 5500 series */
		case 0x1E: /* Core i7 and i5 Processor - Lynnfield Jasper Forest */
		case 0x1F: /* Core i7 and i5 Processor - Nehalem */
		case 0x2E: /* Nehalem-EX Xeon */
		case 0x2F: /* Westmere-EX Xeon */
		case 0x25: /* Westmere */
		case 0x2C: /* Westmere */
			msr_table_check(fw, IA32_nehalem_MSRs);
			printf("Nehalem\n");
			break;
		case 0x1C: /* Atom Processor */
		case 0x26: /* Lincroft Atom Processor */
			msr_table_check(fw, IA32_atom_MSRs);
			break;
		case 0x2A: /* Sandybridge */
		case 0x2D: /* Ssandybridge Xeon */
			msr_table_check(fw, IA32_sandybridge_MSRs);
			break;
		default:
			fwts_log_info(fw, "No model specific tests for model 0x%x.", cpuinfo->x86_model);
			break;
		}
	} else
		fwts_skipped(fw, "Non-Intel CPU, test skipped.");

	return FWTS_OK;
}


static fwts_framework_minor_test msr_tests[] = {
	{ msr_cpu_generic,		"Check CPU generic MSRs." },
	{ msr_cpu_specific,		"Check CPU specific model MSRs." },
	{ msr_pstate_ratios, 		"Check all P State Ratios." },
	{ msr_c1_c3_autodemotion, 	"Check C1 and C3 autodemotion." },
	{ msr_smrr,			"Check SMRR MSR registers." },
	{ NULL, NULL }
};

static fwts_framework_ops msr_ops = {
	.description = "MSR register tests.",
	.init        = msr_init,
	.deinit	     = msr_deinit,
	.minor_tests = msr_tests
};

FWTS_REGISTER(msr, &msr_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
