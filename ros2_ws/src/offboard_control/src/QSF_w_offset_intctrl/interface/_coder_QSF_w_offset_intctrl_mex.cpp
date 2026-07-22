//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_QSF_w_offset_intctrl_mex.cpp
//
// Code generation for function 'QSF_w_offset_intctrl'
//

// Include files
#include "_coder_QSF_w_offset_intctrl_mex.h"
#include "_coder_QSF_w_offset_intctrl_api.h"

// Function Definitions
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&QSF_w_offset_intctrl_atexit);
  // Module initialization.
  QSF_w_offset_intctrl_initialize();
  // Dispatch the entry-point.
  unsafe_QSF_w_offset_intctrl_mexFunction(nlhs, plhs, nrhs, prhs);
  // Module termination.
  QSF_w_offset_intctrl_terminate();
}

emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           nullptr, "windows-1252", true);
  return emlrtRootTLSGlobal;
}

void unsafe_QSF_w_offset_intctrl_mexFunction(int32_T nlhs, mxArray *plhs[4],
                                             int32_T nrhs,
                                             const mxArray *prhs[12])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *b_prhs[12];
  const mxArray *outputs[4];
  int32_T i1;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 12) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 12, 4,
                        20, "QSF_w_offset_intctrl");
  }
  if (nlhs > 4) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 20,
                        "QSF_w_offset_intctrl");
  }
  // Call the function.
  for (int32_T i{0}; i < 12; i++) {
    b_prhs[i] = prhs[i];
  }
  QSF_w_offset_intctrl_api(b_prhs, nlhs, outputs);
  // Copy over outputs to the caller.
  if (nlhs < 1) {
    i1 = 1;
  } else {
    i1 = nlhs;
  }
  emlrtReturnArrays(i1, &plhs[0], &outputs[0]);
}

// End of code generation (_coder_QSF_w_offset_intctrl_mex.cpp)
