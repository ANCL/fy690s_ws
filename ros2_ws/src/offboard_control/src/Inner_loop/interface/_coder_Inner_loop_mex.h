//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: _coder_Inner_loop_mex.h
//
// MATLAB Coder version            : 24.2
// C/C++ source code generated on  : 17-Aug-2026 21:58:17
//

#ifndef _CODER_INNER_LOOP_MEX_H
#define _CODER_INNER_LOOP_MEX_H

// Include Files
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"

// Function Declarations
MEXFUNCTION_LINKAGE void mexFunction(int32_T nlhs, mxArray *plhs[],
                                     int32_T nrhs, const mxArray *prhs[]);

emlrtCTX mexFunctionCreateRootTLS();

void unsafe_Inner_loop_mexFunction(int32_T nlhs, mxArray *plhs[4], int32_T nrhs,
                                   const mxArray *prhs[11]);

#endif
//
// File trailer for _coder_Inner_loop_mex.h
//
// [EOF]
//
