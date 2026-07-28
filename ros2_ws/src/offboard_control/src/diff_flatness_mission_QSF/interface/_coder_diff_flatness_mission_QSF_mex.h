//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_diff_flatness_mission_QSF_mex.h
//
// Code generation for function 'diff_flatness_mission_QSF'
//

#ifndef _CODER_DIFF_FLATNESS_MISSION_QSF_MEX_H
#define _CODER_DIFF_FLATNESS_MISSION_QSF_MEX_H

// Include files
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"

// Function Declarations
MEXFUNCTION_LINKAGE void mexFunction(int32_T nlhs, mxArray *plhs[],
                                     int32_T nrhs, const mxArray *prhs[]);

emlrtCTX mexFunctionCreateRootTLS();

void unsafe_diff_flatness_mission_QSF_mexFunction(int32_T nlhs,
                                                  mxArray *plhs[9],
                                                  int32_T nrhs,
                                                  const mxArray *prhs[10]);

#endif
// End of code generation (_coder_diff_flatness_mission_QSF_mex.h)
