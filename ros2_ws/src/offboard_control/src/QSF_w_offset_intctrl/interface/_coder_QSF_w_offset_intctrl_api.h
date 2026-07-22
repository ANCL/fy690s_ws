//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_QSF_w_offset_intctrl_api.h
//
// Code generation for function 'QSF_w_offset_intctrl'
//

#ifndef _CODER_QSF_W_OFFSET_INTCTRL_API_H
#define _CODER_QSF_W_OFFSET_INTCTRL_API_H

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
void QSF_w_offset_intctrl(real_T mp, real_T mq, real_T l, real_T g,
                          real_T K1[5], real_T K2[5], real_T K3[3],
                          real_T ref_traj[15], real_T states[12], real_T psi,
                          real_T aL[3], real_T Int[3], real_T Rbd[9],
                          real_T *Fld_scaler, real_T *phid, real_T *thetad);

void QSF_w_offset_intctrl_api(const mxArray *const prhs[12], int32_T nlhs,
                              const mxArray *plhs[4]);

void QSF_w_offset_intctrl_atexit();

void QSF_w_offset_intctrl_initialize();

void QSF_w_offset_intctrl_terminate();

void QSF_w_offset_intctrl_xil_shutdown();

void QSF_w_offset_intctrl_xil_terminate();

#endif
// End of code generation (_coder_QSF_w_offset_intctrl_api.h)
