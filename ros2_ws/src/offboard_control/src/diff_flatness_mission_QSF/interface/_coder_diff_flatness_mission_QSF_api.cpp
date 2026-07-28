//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_diff_flatness_mission_QSF_api.cpp
//
// Code generation for function 'diff_flatness_mission_QSF'
//

// Include files
#include "_coder_diff_flatness_mission_QSF_api.h"
#include "_coder_diff_flatness_mission_QSF_mex.h"

// Variable Definitions
emlrtCTX emlrtRootTLSGlobal{nullptr};

emlrtContext emlrtContextGlobal{
    true,                                                 // bFirstTime
    false,                                                // bInitialized
    131643U,                                              // fVersionInfo
    nullptr,                                              // fErrorFunction
    "diff_flatness_mission_QSF",                          // fFunctionName
    nullptr,                                              // fRTCallStack
    false,                                                // bDebugMode
    {2045744189U, 2170104910U, 2743257031U, 4284093946U}, // fSigWrd
    nullptr                                               // fSigMem
};

// Function Declarations
static real_T (*b_emlrt_marshallIn(const emlrtStack &sp,
                                   const mxArray *b_nullptr,
                                   const char_T *identifier))[3];

static real_T (*b_emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                                   const emlrtMsgIdentifier *parentId))[3];

static real_T c_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                 const emlrtMsgIdentifier *msgId);

static real_T (*d_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                   const emlrtMsgIdentifier *msgId))[3];

static void emlrtExitTimeCleanupDtorFcn(const void *r);

static real_T emlrt_marshallIn(const emlrtStack &sp, const mxArray *b_nullptr,
                               const char_T *identifier);

static real_T emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                               const emlrtMsgIdentifier *parentId);

static const mxArray *emlrt_marshallOut(const real_T u[3]);

static const mxArray *emlrt_marshallOut(const real_T u);

// Function Definitions
static real_T (*b_emlrt_marshallIn(const emlrtStack &sp,
                                   const mxArray *b_nullptr,
                                   const char_T *identifier))[3]
{
  emlrtMsgIdentifier thisId;
  real_T(*y)[3];
  thisId.fIdentifier = const_cast<const char_T *>(identifier);
  thisId.fParent = nullptr;
  thisId.bParentIsCell = false;
  y = b_emlrt_marshallIn(sp, emlrtAlias(b_nullptr), &thisId);
  emlrtDestroyArray(&b_nullptr);
  return y;
}

static real_T (*b_emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                                   const emlrtMsgIdentifier *parentId))[3]
{
  real_T(*y)[3];
  y = d_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

static real_T c_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                 const emlrtMsgIdentifier *msgId)
{
  static const int32_T dims{0};
  real_T ret;
  emlrtCheckBuiltInR2012b((emlrtConstCTX)&sp, msgId, src, "double", false, 0U,
                          (const void *)&dims);
  ret = *static_cast<real_T *>(emlrtMxGetData(src));
  emlrtDestroyArray(&src);
  return ret;
}

static real_T (*d_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                   const emlrtMsgIdentifier *msgId))[3]
{
  static const int32_T dims{3};
  real_T(*ret)[3];
  int32_T i;
  boolean_T b{false};
  emlrtCheckVsBuiltInR2012b((emlrtConstCTX)&sp, msgId, src, "double", false, 1U,
                            (const void *)&dims, &b, &i);
  ret = (real_T(*)[3])emlrtMxGetData(src);
  emlrtDestroyArray(&src);
  return ret;
}

static void emlrtExitTimeCleanupDtorFcn(const void *r)
{
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

static real_T emlrt_marshallIn(const emlrtStack &sp, const mxArray *b_nullptr,
                               const char_T *identifier)
{
  emlrtMsgIdentifier thisId;
  real_T y;
  thisId.fIdentifier = const_cast<const char_T *>(identifier);
  thisId.fParent = nullptr;
  thisId.bParentIsCell = false;
  y = emlrt_marshallIn(sp, emlrtAlias(b_nullptr), &thisId);
  emlrtDestroyArray(&b_nullptr);
  return y;
}

static real_T emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                               const emlrtMsgIdentifier *parentId)
{
  real_T y;
  y = c_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

static const mxArray *emlrt_marshallOut(const real_T u[3])
{
  static const int32_T i{0};
  static const int32_T i1{3};
  const mxArray *m;
  const mxArray *y;
  y = nullptr;
  m = emlrtCreateNumericArray(1, (const void *)&i, mxDOUBLE_CLASS, mxREAL);
  emlrtMxSetData((mxArray *)m, (void *)&u[0]);
  emlrtSetDimensions((mxArray *)m, &i1, 1);
  emlrtAssign(&y, m);
  return y;
}

static const mxArray *emlrt_marshallOut(const real_T u)
{
  const mxArray *m;
  const mxArray *y;
  y = nullptr;
  m = emlrtCreateDoubleScalar(u);
  emlrtAssign(&y, m);
  return y;
}

void diff_flatness_mission_QSF_api(const mxArray *const prhs[10], int32_T nlhs,
                                   const mxArray *plhs[9])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  real_T(*L)[3];
  real_T(*Od)[3];
  real_T(*d2xipd)[3];
  real_T(*d3xipd)[3];
  real_T(*d4xipd)[3];
  real_T(*dOd)[3];
  real_T(*ddRL)[3];
  real_T(*dxipd)[3];
  real_T(*xipd)[3];
  real_T A;
  real_T B;
  real_T T;
  real_T Td_scaler;
  real_T g;
  real_T l;
  real_T mp;
  real_T mq;
  real_T psi;
  real_T t;
  st.tls = emlrtRootTLSGlobal;
  Od = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  dOd = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  xipd = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  dxipd = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  d2xipd = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  d3xipd = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  d4xipd = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  ddRL = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  // Marshall function inputs
  t = emlrt_marshallIn(st, emlrtAliasP(prhs[0]), "t");
  mp = emlrt_marshallIn(st, emlrtAliasP(prhs[1]), "mp");
  mq = emlrt_marshallIn(st, emlrtAliasP(prhs[2]), "mq");
  l = emlrt_marshallIn(st, emlrtAliasP(prhs[3]), "l");
  g = emlrt_marshallIn(st, emlrtAliasP(prhs[4]), "g");
  psi = emlrt_marshallIn(st, emlrtAliasP(prhs[5]), "psi");
  L = b_emlrt_marshallIn(st, emlrtAlias(prhs[6]), "L");
  T = emlrt_marshallIn(st, emlrtAliasP(prhs[7]), "T");
  A = emlrt_marshallIn(st, emlrtAliasP(prhs[8]), "A");
  B = emlrt_marshallIn(st, emlrtAliasP(prhs[9]), "B");
  // Invoke the target function
  diff_flatness_mission_QSF(t, mp, mq, l, g, psi, *L, T, A, B, *Od, *dOd, *xipd,
                            *dxipd, *d2xipd, *d3xipd, *d4xipd, *ddRL,
                            &Td_scaler);
  // Marshall function outputs
  plhs[0] = emlrt_marshallOut(*Od);
  if (nlhs > 1) {
    plhs[1] = emlrt_marshallOut(*dOd);
  }
  if (nlhs > 2) {
    plhs[2] = emlrt_marshallOut(*xipd);
  }
  if (nlhs > 3) {
    plhs[3] = emlrt_marshallOut(*dxipd);
  }
  if (nlhs > 4) {
    plhs[4] = emlrt_marshallOut(*d2xipd);
  }
  if (nlhs > 5) {
    plhs[5] = emlrt_marshallOut(*d3xipd);
  }
  if (nlhs > 6) {
    plhs[6] = emlrt_marshallOut(*d4xipd);
  }
  if (nlhs > 7) {
    plhs[7] = emlrt_marshallOut(*ddRL);
  }
  if (nlhs > 8) {
    plhs[8] = emlrt_marshallOut(Td_scaler);
  }
}

void diff_flatness_mission_QSF_atexit()
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtPushHeapReferenceStackR2021a(&st, false, nullptr,
                                    (void *)&emlrtExitTimeCleanupDtorFcn,
                                    nullptr, nullptr, nullptr);
  emlrtEnterRtStackR2012b(&st);
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
  diff_flatness_mission_QSF_xil_terminate();
  diff_flatness_mission_QSF_xil_shutdown();
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

void diff_flatness_mission_QSF_initialize()
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtClearAllocCountR2012b(&st, false, 0U, nullptr);
  emlrtEnterRtStackR2012b(&st);
  emlrtFirstTimeR2012b(emlrtRootTLSGlobal);
}

void diff_flatness_mission_QSF_terminate()
{
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
}

// End of code generation (_coder_diff_flatness_mission_QSF_api.cpp)
