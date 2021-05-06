/*
 * pfmlib_amd64_msr.c : AMD MSR PMU
 *
 * Copyright (c) 2020 Inria
 * Copyright (c) 2020 University of Lille
 * Contributed by Guillaume Fieni <guillaume.fieni@inria.fr>
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
 * AMD MSR PMU
 */

#include "pfmlib_priv.h"
#include "pfmlib_amd64_priv.h"

extern pfmlib_pmu_t amd64_msr_support;

static const amd64_entry_t amd64_msr_generic_pe[] = {
  { .name   = "TSC",
    .desc   = "Time Stamp Counter",
    .code   = 0x0,
  },
  { .name   = "APERF",
    .desc   = "Actual Performance",
    .code   = 0x1,
  },
  { .name   = "MPERF",
    .desc   = "Maximum Performance",
    .code   = 0x2,
  },
  { .name   = "PPERF",
    .desc   = "Productive Performance Count",
    .code   = 0x3,
  },
  { .name   = "SMI",
    .desc   = "System Management Interrupt",
    .code   = 0x4,
  },
  { .name   = "PTSC",
    .desc   = "Performance Time Stamp Counter",
    .code   = 0x5,
  },
};

static int
pfm_amd64_msr_detect(void *this)
{
        int ret;

        ret = pfm_amd64_detect(this);
        if (ret != PFM_SUCCESS)
                return ret;

	/* TODO: proper model/arch filter */
        return PFM_SUCCESS;
}

static int
pfm_amd64_msr_get_encoding(void *this, pfmlib_event_desc_t *e)
{
	const amd64_entry_t *pe;

	pe = this_pe(this);

	e->fstr[0] = '\0';

	e->codes[0] = pe[e->event].code;
	e->count = 1;
	evt_strcat(e->fstr, "%s", pe[e->event].name);

	__pfm_vbprintf("[0x%"PRIx64" event=0x%x] %s\n",
		       e->codes[0],
		       e->codes[0], e->fstr);

	return PFM_SUCCESS;
}

/*
 * number modifiers for MSR
 * define an empty modifier to avoid firing the
 * sanity pfm_intel_x86_validate_table(). We are
 * using this function to avoid duplicating code.
 */
static const pfmlib_attr_desc_t amd64_msr_mods[] = { { 0, } };

pfmlib_pmu_t amd64_msr_support={
        .desc                   = "AMD64 MSR",
        .name                   = "amd64_msr",
        .perf_name              = "msr",
        .pmu                    = PFM_PMU_AMD64_MSR,
        .pme_count              = LIBPFM_ARRAY_SIZE(amd64_msr_generic_pe),
        .type                   = PFM_PMU_TYPE_CORE,
        .supported_plm          = PFM_PLM0,
        .num_cntrs              = 0,
        .num_fixed_cntrs        = 6,
        .max_encoding           = 1,
        .pe                     = amd64_msr_generic_pe,
        .pmu_detect             = pfm_amd64_msr_detect,
        .atdesc                 = amd64_msr_mods,

        .get_event_encoding[PFM_OS_NONE] = pfm_amd64_msr_get_encoding,
        PFMLIB_ENCODE_PERF(pfm_amd64_get_perf_encoding),
        .get_event_first        = pfm_amd64_get_event_first,
        .get_event_next         = pfm_amd64_get_event_next,
        .event_is_valid         = pfm_amd64_event_is_valid,
        .validate_table         = pfm_amd64_validate_table,
        .get_event_info         = pfm_amd64_get_event_info,
        .get_event_attr_info    = pfm_amd64_get_event_attr_info,
        PFMLIB_VALID_PERF_PATTRS(pfm_amd64_perf_validate_pattrs),
        .get_event_nattrs       = pfm_amd64_get_event_nattrs,
};
