//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_diff_flatness_mission_QSF_mex.cpp
//
// Code generation for function 'diff_flatness_mission_QSF'
//

// Include files
#include "_coder_diff_flatness_mission_QSF_mex.h"
#include "_coder_diff_flatness_mission_QSF_api.h"

// Function Definitions
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&diff_flatness_mission_QSF_atexit);
  // Module initialization.
  diff_flatness_mission_QSF_initialize();
  // Dispatch the entry-point.
  unsafe_diff_flatness_mission_QSF_mexFunction(nlhs, plhs, nrhs, prhs);
  // Module termination.
  diff_flatness_mission_QSF_terminate();
}

emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           nullptr, "windows-1252", true);
  return emlrtRootTLSGlobal;
}

void unsafe_diff_flatness_mission_QSF_mexFunction(int32_T nlhs,
                                                  mxArray *plhs[9],
                                                  int32_T nrhs,
                                                  const mxArray *prhs[10])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *b_prhs[10];
  const mxArray *outputs[9];
  int32_T i1;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 10) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 10, 4,
                        25, "diff_flatness_mission_QSF");
  }
  if (nlhs > 9) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 25,
                        "diff_flatness_mission_QSF");
  }
  // Call the function.
  for (int32_T i{0}; i < 10; i++) {
    b_prhs[i] = prhs[i];
  }
  diff_flatness_mission_QSF_api(b_prhs, nlhs, outputs);
  // Copy over outputs to the caller.
  if (nlhs < 1) {
    i1 = 1;
  } else {
    i1 = nlhs;
  }
  emlrtReturnArrays(i1, &plhs[0], &outputs[0]);
}

// End of code generation (_coder_diff_flatness_mission_QSF_mex.cpp)
