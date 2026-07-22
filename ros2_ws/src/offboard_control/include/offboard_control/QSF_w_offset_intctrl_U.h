//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// QSF_w_offset_intctrl_U.h
//
// Code generation for function 'QSF_w_offset_intctrl_U'
//

#ifndef QSF_W_OFFSET_INTCTRL_U_H
#define QSF_W_OFFSET_INTCTRL_U_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void QSF_w_offset_intctrl_U(
    double mq, double l, double g, const double K1[5], const double K2[5],
    const double K3[3], const double ref_traj[15], const double states[12],
    double psi, double Td_scaler, const double Int[3], double Rbd[9],
    double *Fld_scaler, double *phid, double *thetad);

#endif
// End of code generation (QSF_w_offset_intctrl_U.h)
