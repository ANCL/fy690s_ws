//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: Inner_loop.h
//
// MATLAB Coder version            : 24.2
// C/C++ source code generated on  : 17-Aug-2026 21:58:17
//

#ifndef INNER_LOOP_H
#define INNER_LOOP_H

// Include Files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void Inner_loop(const double rpy_angles[3], const double Omega[3],
                       const double Rd[9], const double Omegad[3],
                       const double dOmegad[3], double Fl,
                       const double gains[4], const double physics_param[6],
                       const double L_offset[3], const double load_acc[3],
                       const double eI[3], double taub[3], double tau[3],
                       double rate_sp_dt[3], double eI_dt[3]);

#endif
//
// File trailer for Inner_loop.h
//
// [EOF]
//
