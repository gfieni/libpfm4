/*
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
 * This file is part of libpfm, a performance monitoring support library for
 * applications on Linux.
 *
 * PMU: icx_unc_pcu (IcelakeX Uncore PCU)
 * Based on Intel JSON event table version   : 1.21
 * Based on Intel JSON event table published : 06/06/2023
 */

static const intel_x86_umask_t icx_unc_p_power_state_occupancy[]={
  { .uname   = "CORES_C0",
    .udesc   = "C0 and C1 (experimental)",
    .ucode   = 0x4000ull,
    .uflags  = INTEL_X86_NCOMBO,
  },
  { .uname   = "CORES_C3",
    .udesc   = "C3 (experimental)",
    .ucode   = 0x8000ull,
    .uflags  = INTEL_X86_NCOMBO,
  },
  { .uname   = "CORES_C6",
    .udesc   = "C6 and C7 (experimental)",
    .ucode   = 0xc000ull,
    .uflags  = INTEL_X86_NCOMBO,
  },
};

static const intel_x86_entry_t intel_icx_unc_pcu_pe[]={
  { .name   = "UNC_P_CLOCKTICKS",
    .desc   = "Clockticks of the power control unit (PCU)",
    .code   = 0x0000,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_CORE_TRANSITION_CYCLES",
    .desc   = "UNC_P_CORE_TRANSITION_CYCLES (experimental)",
    .code   = 0x0060,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_DEMOTIONS",
    .desc   = "UNC_P_DEMOTIONS (experimental)",
    .code   = 0x0030,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FIVR_PS_PS0_CYCLES",
    .desc   = "Phase Shed 0 Cycles (experimental)",
    .code   = 0x0075,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FIVR_PS_PS1_CYCLES",
    .desc   = "Phase Shed 1 Cycles (experimental)",
    .code   = 0x0076,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FIVR_PS_PS2_CYCLES",
    .desc   = "Phase Shed 2 Cycles (experimental)",
    .code   = 0x0077,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FIVR_PS_PS3_CYCLES",
    .desc   = "Phase Shed 3 Cycles (experimental)",
    .code   = 0x0078,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FREQ_CLIP_AVX256",
    .desc   = "AVX256 Frequency Clipping (experimental)",
    .code   = 0x0049,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FREQ_CLIP_AVX512",
    .desc   = "AVX512 Frequency Clipping (experimental)",
    .code   = 0x004a,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FREQ_MAX_LIMIT_THERMAL_CYCLES",
    .desc   = "Thermal Strongest Upper Limit Cycles (experimental)",
    .code   = 0x0004,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FREQ_MAX_POWER_CYCLES",
    .desc   = "Power Strongest Upper Limit Cycles (experimental)",
    .code   = 0x0005,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FREQ_MIN_IO_P_CYCLES",
    .desc   = "IO P Limit Strongest Lower Limit Cycles (experimental)",
    .code   = 0x0073,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_FREQ_TRANS_CYCLES",
    .desc   = "Cycles spent changing Frequency (experimental)",
    .code   = 0x0074,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_MEMORY_PHASE_SHEDDING_CYCLES",
    .desc   = "Memory Phase Shedding Cycles (experimental)",
    .code   = 0x002f,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_PKG_RESIDENCY_C0_CYCLES",
    .desc   = "Package C State Residency - C0 (experimental)",
    .code   = 0x002a,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_PKG_RESIDENCY_C2E_CYCLES",
    .desc   = "Package C State Residency - C2E (experimental)",
    .code   = 0x002b,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_PKG_RESIDENCY_C3_CYCLES",
    .desc   = "Package C State Residency - C3 (experimental)",
    .code   = 0x002c,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_PKG_RESIDENCY_C6_CYCLES",
    .desc   = "Package C State Residency - C6 (experimental)",
    .code   = 0x002d,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_PMAX_THROTTLED_CYCLES",
    .desc   = "UNC_P_PMAX_THROTTLED_CYCLES (experimental)",
    .code   = 0x0006,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_POWER_STATE_OCCUPANCY",
    .desc   = "Number of cores in C-State",
    .code   = 0x0080,
    .modmsk = ICX_UNC_PCU_OCC_ATTRS,
    .cntmsk = 0xfull,
    .ngrp   = 1,
    .numasks= LIBPFM_ARRAY_SIZE(icx_unc_p_power_state_occupancy),
    .umasks = icx_unc_p_power_state_occupancy,
  },
  { .name   = "UNC_P_PROCHOT_EXTERNAL_CYCLES",
    .desc   = "External Prochot (experimental)",
    .code   = 0x000a,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_PROCHOT_INTERNAL_CYCLES",
    .desc   = "Internal Prochot (experimental)",
    .code   = 0x0009,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_TOTAL_TRANSITION_CYCLES",
    .desc   = "Total Core C State Transition Cycles (experimental)",
    .code   = 0x0072,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
  { .name   = "UNC_P_VR_HOT_CYCLES",
    .desc   = "VR Hot (experimental)",
    .code   = 0x0042,
    .modmsk = ICX_UNC_PCU_ATTRS,
    .cntmsk = 0xfull,
  },
};
/* 24 events available */
