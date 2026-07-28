//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// diff_flatness_mission_QSF.h
//
// Code generation for function 'diff_flatness_mission_QSF'
//

#ifndef DIFF_FLATNESS_MISSION_QSF_H
#define DIFF_FLATNESS_MISSION_QSF_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void
diff_flatness_mission_QSF(double t, double mp, double mq, double l, double g,
                          double psi, const double L[3], double T, double A,
                          double B, double Od[3], double dOd[3], double xipd[3],
                          double dxipd[3], double d2xipd[3], double d3xipd[3],
                          double d4xipd[3], double ddRL[3], double *Td_scaler);

#endif
// End of code generation (diff_flatness_mission_QSF.h)
