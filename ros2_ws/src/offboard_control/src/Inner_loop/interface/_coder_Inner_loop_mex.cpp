//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_Inner_loop_mex.cpp
//
// Code generation for function 'Inner_loop'
//

// Include files
#include "_coder_Inner_loop_mex.h"
#include "_coder_Inner_loop_api.h"

// Function Definitions
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&Inner_loop_atexit);
  // Module initialization.
  Inner_loop_initialize();
  // Dispatch the entry-point.
  unsafe_Inner_loop_mexFunction(nlhs, plhs, nrhs, prhs);
  // Module termination.
  Inner_loop_terminate();
}

emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           nullptr, "windows-1252", true);
  return emlrtRootTLSGlobal;
}

void unsafe_Inner_loop_mexFunction(int32_T nlhs, mxArray *plhs[3], int32_T nrhs,
                                   const mxArray *prhs[11])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *b_prhs[11];
  const mxArray *outputs[3];
  int32_T i1;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 11) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 11, 4,
                        10, "Inner_loop");
  }
  if (nlhs > 3) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 10,
                        "Inner_loop");
  }
  // Call the function.
  for (int32_T i{0}; i < 11; i++) {
    b_prhs[i] = prhs[i];
  }
  Inner_loop_api(b_prhs, nlhs, outputs);
  // Copy over outputs to the caller.
  if (nlhs < 1) {
    i1 = 1;
  } else {
    i1 = nlhs;
  }
  emlrtReturnArrays(i1, &plhs[0], &outputs[0]);
}

// End of code generation (_coder_Inner_loop_mex.cpp)
