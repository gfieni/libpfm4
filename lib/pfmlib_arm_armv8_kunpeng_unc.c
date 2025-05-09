/*
 * pfmlib_arm_armv8_kunpeng_unc.c : support for HiSilicon Kunpeng uncore PMUs
 *
 * Copyright (c) 2024 Google Inc. All rights reserved
 * Contributed by Stephane Eranian <eranian@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

/* private headers */
#include "pfmlib_priv.h"			/* library private */
#include "pfmlib_arm_priv.h"
#include "pfmlib_arm_armv8_unc_priv.h"

#include "events/arm_hisilicon_kunpeng_unc_events.h" /* Hisilicon Kunpeng PMU uncore tables */

static int
pfm_arm_detect_hisilicon_kunpeng(void *this)
{
	/* Hisilicon Kunpeng */
	arm_cpuid_t attr = { .impl = 0x48, .arch = 8, .part = 0xd01 };
	return pfm_arm_detect(&attr, NULL);
}

static void
display_com(void *this, pfmlib_event_desc_t *e, void *val)
{
	const arm_entry_t *pe = this_pe(this);
	kunpeng_unc_data_t *reg = val;

	__pfm_vbprintf("[UNC=0x%"PRIx64"] %s\n",
			reg->val,
			pe[e->event].name);
}

static void
display_reg(void *this, pfmlib_event_desc_t *e, kunpeng_unc_data_t reg)
{
	pfmlib_pmu_t *pmu = this;
	if (pmu->display_reg)
		pmu->display_reg(this, e, &reg);
	else
		display_com(this, e, &reg);
}

int
pfm_kunpeng_unc_get_event_encoding(void *this, pfmlib_event_desc_t *e)
{
	//from pe field in for the uncore, get the array with all the event defs
	const arm_entry_t *event_list = this_pe(this);
	kunpeng_unc_data_t reg;
	//get code for the event from the table
	reg.val = event_list[e->event].code;
	//pass the data back to the caller
	e->codes[0] = reg.val;
	e->count = 1;
	evt_strcat(e->fstr, "%s", event_list[e->event].name);
	display_reg(this, e, reg);
	return PFM_SUCCESS;
}



/* Hisilicon Kunpeng support */
// For uncore, each socket has a separate perf name, otherwise they are the same, use macro

#define DEFINE_KUNPENG_DDRC(n,m) \
pfmlib_pmu_t arm_hisilicon_kunpeng_sccl##n##_ddrc##m##_support={ \
	.desc           = "Hisilicon Kunpeng SCCL"#n" DDRC"#m, \
	.name           = "hisi_sccl"#n"_ddrc"#m, \
	.perf_name      = "hisi_sccl"#n"_ddrc"#m, \
	.pmu            = PFM_PMU_ARM_KUNPENG_UNC_SCCL##n##_DDRC##m, \
	.pme_count      = LIBPFM_ARRAY_SIZE(arm_kunpeng_unc_ddrc_pe), \
	.type           = PFM_PMU_TYPE_UNCORE, \
	.pe             = arm_kunpeng_unc_ddrc_pe, \
	.pmu_detect     = pfm_arm_detect_hisilicon_kunpeng, \
	.max_encoding   = 1, \
	.num_cntrs      = 4, \
	.get_event_encoding[PFM_OS_NONE] = pfm_kunpeng_unc_get_event_encoding, \
	 PFMLIB_ENCODE_PERF(pfm_kunpeng_unc_get_perf_encoding), \
	.get_event_first	= pfm_arm_get_event_first, \
	.get_event_next		= pfm_arm_get_event_next, \
	.event_is_valid		= pfm_arm_event_is_valid, \
	.validate_table		= pfm_arm_validate_table, \
	.get_event_info		= pfm_arm_get_event_info, \
	.get_event_attr_info	= pfm_arm_get_event_attr_info, \
	 PFMLIB_VALID_PERF_PATTRS(pfm_arm_perf_validate_pattrs), \
	.get_event_nattrs	= pfm_arm_get_event_nattrs, \
};

DEFINE_KUNPENG_DDRC(1,0);
DEFINE_KUNPENG_DDRC(1,1);
DEFINE_KUNPENG_DDRC(1,2);
DEFINE_KUNPENG_DDRC(1,3);
DEFINE_KUNPENG_DDRC(3,0);
DEFINE_KUNPENG_DDRC(3,1);
DEFINE_KUNPENG_DDRC(3,2);
DEFINE_KUNPENG_DDRC(3,3);
DEFINE_KUNPENG_DDRC(5,0);
DEFINE_KUNPENG_DDRC(5,1);
DEFINE_KUNPENG_DDRC(5,2);
DEFINE_KUNPENG_DDRC(5,3);
DEFINE_KUNPENG_DDRC(7,0);
DEFINE_KUNPENG_DDRC(7,1);
DEFINE_KUNPENG_DDRC(7,2);
DEFINE_KUNPENG_DDRC(7,3);

#define DEFINE_KUNPENG_HHA(n,m) \
pfmlib_pmu_t arm_hisilicon_kunpeng_sccl##n##_hha##m##_support={ \
	.desc           = "Hisilicon Kunpeng SCCL"#n" HHA"#m, \
	.name           = "hisi_sccl"#n"_hha"#m, \
	.perf_name      = "hisi_sccl"#n"_hha"#m, \
	.pmu            = PFM_PMU_ARM_KUNPENG_UNC_SCCL##n##_HHA##m, \
	.pme_count      = LIBPFM_ARRAY_SIZE(arm_kunpeng_unc_hha_pe), \
	.type           = PFM_PMU_TYPE_UNCORE, \
	.pe             = arm_kunpeng_unc_hha_pe, \
	.pmu_detect     = pfm_arm_detect_hisilicon_kunpeng, \
	.max_encoding   = 1, \
	.num_cntrs      = 4, \
	.get_event_encoding[PFM_OS_NONE] = pfm_kunpeng_unc_get_event_encoding, \
	 PFMLIB_ENCODE_PERF(pfm_kunpeng_unc_get_perf_encoding), \
	.get_event_first	= pfm_arm_get_event_first, \
	.get_event_next		= pfm_arm_get_event_next, \
	.event_is_valid		= pfm_arm_event_is_valid, \
	.validate_table		= pfm_arm_validate_table, \
	.get_event_info		= pfm_arm_get_event_info, \
	.get_event_attr_info	= pfm_arm_get_event_attr_info, \
	 PFMLIB_VALID_PERF_PATTRS(pfm_arm_perf_validate_pattrs), \
	.get_event_nattrs	= pfm_arm_get_event_nattrs, \
};

DEFINE_KUNPENG_HHA(1,2);
DEFINE_KUNPENG_HHA(1,3);
DEFINE_KUNPENG_HHA(3,0);
DEFINE_KUNPENG_HHA(3,1);
DEFINE_KUNPENG_HHA(5,6);
DEFINE_KUNPENG_HHA(5,7);
DEFINE_KUNPENG_HHA(7,4);
DEFINE_KUNPENG_HHA(7,5);

#define DEFINE_KUNPENG_L3C(n,m) \
pfmlib_pmu_t arm_hisilicon_kunpeng_sccl##n##_l3c##m##_support={ \
	.desc           = "Hisilicon Kunpeng SCCL"#n" L3C"#m, \
	.name           = "hisi_sccl"#n"_l3c"#m, \
	.perf_name      = "hisi_sccl"#n"_l3c"#m, \
	.pmu            = PFM_PMU_ARM_KUNPENG_UNC_SCCL##n##_L3C##m, \
	.pme_count      = LIBPFM_ARRAY_SIZE(arm_kunpeng_unc_l3c_pe), \
	.type           = PFM_PMU_TYPE_UNCORE, \
	.pe             = arm_kunpeng_unc_l3c_pe, \
	.pmu_detect     = pfm_arm_detect_hisilicon_kunpeng, \
	.max_encoding   = 1, \
	.num_cntrs      = 4, \
	.get_event_encoding[PFM_OS_NONE] = pfm_kunpeng_unc_get_event_encoding, \
	 PFMLIB_ENCODE_PERF(pfm_kunpeng_unc_get_perf_encoding), \
	.get_event_first	= pfm_arm_get_event_first, \
	.get_event_next		= pfm_arm_get_event_next, \
	.event_is_valid		= pfm_arm_event_is_valid, \
	.validate_table		= pfm_arm_validate_table, \
	.get_event_info		= pfm_arm_get_event_info, \
	.get_event_attr_info	= pfm_arm_get_event_attr_info, \
	 PFMLIB_VALID_PERF_PATTRS(pfm_arm_perf_validate_pattrs), \
	.get_event_nattrs	= pfm_arm_get_event_nattrs, \
};

DEFINE_KUNPENG_L3C(1,10);
DEFINE_KUNPENG_L3C(1,11);
DEFINE_KUNPENG_L3C(1,12);
DEFINE_KUNPENG_L3C(1,13);
DEFINE_KUNPENG_L3C(1,14);
DEFINE_KUNPENG_L3C(1,15);
DEFINE_KUNPENG_L3C(1,8);
DEFINE_KUNPENG_L3C(1,9);
DEFINE_KUNPENG_L3C(3,0);
DEFINE_KUNPENG_L3C(3,1);
DEFINE_KUNPENG_L3C(3,2);
DEFINE_KUNPENG_L3C(3,3);
DEFINE_KUNPENG_L3C(3,4);
DEFINE_KUNPENG_L3C(3,5);
DEFINE_KUNPENG_L3C(3,6);
DEFINE_KUNPENG_L3C(3,7);
DEFINE_KUNPENG_L3C(5,24);
DEFINE_KUNPENG_L3C(5,25);
DEFINE_KUNPENG_L3C(5,26);
DEFINE_KUNPENG_L3C(5,27);
DEFINE_KUNPENG_L3C(5,28);
DEFINE_KUNPENG_L3C(5,29);
DEFINE_KUNPENG_L3C(5,30);
DEFINE_KUNPENG_L3C(5,31);
DEFINE_KUNPENG_L3C(7,16);
DEFINE_KUNPENG_L3C(7,17);
DEFINE_KUNPENG_L3C(7,18);
DEFINE_KUNPENG_L3C(7,19);
DEFINE_KUNPENG_L3C(7,20);
DEFINE_KUNPENG_L3C(7,21);
DEFINE_KUNPENG_L3C(7,22);
DEFINE_KUNPENG_L3C(7,23);
