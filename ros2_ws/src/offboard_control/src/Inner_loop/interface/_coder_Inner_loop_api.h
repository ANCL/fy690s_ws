//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: _coder_Inner_loop_api.h
//
// MATLAB Coder version            : 24.2
// C/C++ source code generated on  : 17-Aug-2026 21:58:17
//

#ifndef _CODER_INNER_LOOP_API_H
#define _CODER_INNER_LOOP_API_H

// Include Files
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"
#include <algorithm>
#include <cstring>

// Variable Declarations
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

// Function Declarations
void Inner_loop(real_T rpy_angles[3], real_T Omega[3], real_T Rd[9],
                real_T Omegad[3], real_T dOmegad[3], real_T Fl, real_T gains[4],
                real_T physics_param[6], real_T L_offset[3], real_T load_acc[3],
                real_T eI[3], real_T taub[3], real_T tau[3],
                real_T rate_sp_dt[3], real_T eI_dt[3]);

void Inner_loop_api(const mxArray *const prhs[11], int32_T nlhs,
                    const mxArray *plhs[4]);

void Inner_loop_atexit();

void Inner_loop_initialize();

void Inner_loop_terminate();

void Inner_loop_xil_shutdown();

void Inner_loop_xil_terminate();

#endif
//
// File trailer for _coder_Inner_loop_api.h
//
// [EOF]
//
