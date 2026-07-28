//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_diff_flatness_mission_QSF_api.h
//
// Code generation for function 'diff_flatness_mission_QSF'
//

#ifndef _CODER_DIFF_FLATNESS_MISSION_QSF_API_H
#define _CODER_DIFF_FLATNESS_MISSION_QSF_API_H

// Include files
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"
#include <algorithm>
#include <cstring>

// Variable Declarations
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

// Function Declarations
void diff_flatness_mission_QSF(real_T t, real_T mp, real_T mq, real_T l,
                               real_T g, real_T psi, real_T L[3], real_T T,
                               real_T A, real_T B, real_T Od[3], real_T dOd[3],
                               real_T xipd[3], real_T dxipd[3],
                               real_T d2xipd[3], real_T d3xipd[3],
                               real_T d4xipd[3], real_T ddRL[3],
                               real_T *Td_scaler);

void diff_flatness_mission_QSF_api(const mxArray *const prhs[10], int32_T nlhs,
                                   const mxArray *plhs[9]);

void diff_flatness_mission_QSF_atexit();

void diff_flatness_mission_QSF_initialize();

void diff_flatness_mission_QSF_terminate();

void diff_flatness_mission_QSF_xil_shutdown();

void diff_flatness_mission_QSF_xil_terminate();

#endif
// End of code generation (_coder_diff_flatness_mission_QSF_api.h)
