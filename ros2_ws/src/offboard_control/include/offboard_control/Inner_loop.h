//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// Inner_loop.h
//
// Code generation for function 'Inner_loop'
//

#ifndef INNER_LOOP_H
#define INNER_LOOP_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void Inner_loop(const double rpy_angles[3], const double Omega[3],
                       const double Rd[9], const double Omegad[3],
                       const double dOmegad[3], double Fl,
                       const double gains[2], const double physics_param[6],
                       const double L_offset[3], const double ddxi_flat[3],
                       const double load_acc[3], double taub[3], double tau[3],
                       double rate_sp[3]);

#endif
// End of code generation (Inner_loop.h)
