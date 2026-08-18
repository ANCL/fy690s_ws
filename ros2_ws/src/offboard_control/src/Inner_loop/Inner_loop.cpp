//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: Inner_loop.cpp
//
// MATLAB Coder version            : 24.2
// C/C++ source code generated on  : 17-Aug-2026 21:58:17
//

// Include Files
#include "Inner_loop.h"
#include "diag.h"
#include "mldivide.h"
#include <cmath>
#include <emmintrin.h>

// Function Definitions
//
// extract
//
// Arguments    : const double rpy_angles[3]
//                const double Omega[3]
//                const double Rd[9]
//                const double Omegad[3]
//                const double dOmegad[3]
//                double Fl
//                const double gains[4]
//                const double physics_param[6]
//                const double L_offset[3]
//                const double load_acc[3]
//                const double eI[3]
//                double taub[3]
//                double tau[3]
//                double rate_sp_dt[3]
//                double eI_dt[3]
// Return Type  : void
//
void Inner_loop(const double rpy_angles[3], const double Omega[3],
                const double Rd[9], const double Omegad[3],
                const double dOmegad[3], double Fl, const double gains[4],
                const double physics_param[6], const double L_offset[3],
                const double load_acc[3], const double eI[3], double taub[3],
                double tau[3], double rate_sp_dt[3], double eI_dt[3])
{
  __m128d r;
  __m128d r1;
  double J[9];
  double R[9];
  double SOmega_tmp[9];
  double b_physics_param[9];
  double eR_raw[9];
  double eR_raw_tmp[9];
  double eR_raw_tmp_tmp[9];
  double F[3];
  double b[3];
  double b_F[3];
  double b_y[3];
  double eR[3];
  double y[3];
  double R_tmp;
  double b_R_tmp;
  double c_R_tmp;
  double d;
  double d1;
  double d2;
  double d3;
  double d_R_tmp;
  double e_R_tmp;
  double f_R_tmp;
  double g_R_tmp;
  int F_tmp;
  R_tmp = std::cos(rpy_angles[1]);
  b_R_tmp = std::sin(rpy_angles[1]);
  c_R_tmp = std::cos(rpy_angles[2]);
  d_R_tmp = std::sin(rpy_angles[2]);
  e_R_tmp = std::sin(rpy_angles[0]);
  f_R_tmp = std::cos(rpy_angles[0]);
  R[0] = R_tmp * c_R_tmp;
  R[1] = R_tmp * d_R_tmp;
  R[2] = -b_R_tmp;
  g_R_tmp = e_R_tmp * b_R_tmp;
  R[3] = g_R_tmp * c_R_tmp - f_R_tmp * d_R_tmp;
  R[4] = g_R_tmp * d_R_tmp + f_R_tmp * c_R_tmp;
  R[5] = e_R_tmp * R_tmp;
  b_R_tmp *= f_R_tmp;
  R[6] = b_R_tmp * c_R_tmp + e_R_tmp * d_R_tmp;
  R[7] = b_R_tmp * d_R_tmp - e_R_tmp * c_R_tmp;
  R[8] = f_R_tmp * R_tmp;
  //  Rbi
  R_tmp = L_offset[2] * L_offset[2];
  b_R_tmp = L_offset[1] * L_offset[1];
  J[0] = (b_R_tmp + R_tmp) * physics_param[0] + physics_param[3];
  c_R_tmp = -physics_param[0] * L_offset[0];
  d_R_tmp = c_R_tmp * L_offset[1];
  J[3] = d_R_tmp;
  c_R_tmp *= L_offset[2];
  J[6] = c_R_tmp;
  J[1] = d_R_tmp;
  d_R_tmp = L_offset[0] * L_offset[0];
  J[4] = physics_param[4] + physics_param[0] * (d_R_tmp + R_tmp);
  R_tmp = -physics_param[0] * L_offset[1] * L_offset[2];
  J[7] = R_tmp;
  J[2] = c_R_tmp;
  J[5] = R_tmp;
  J[8] = physics_param[5] + physics_param[0] * (d_R_tmp + b_R_tmp);
  //  skew
  SOmega_tmp[0] = 0.0;
  SOmega_tmp[3] = -Omega[2];
  SOmega_tmp[6] = Omega[1];
  SOmega_tmp[1] = Omega[2];
  SOmega_tmp[4] = 0.0;
  SOmega_tmp[7] = -Omega[0];
  SOmega_tmp[2] = -Omega[1];
  SOmega_tmp[5] = Omega[0];
  SOmega_tmp[8] = 0.0;
  //  thrust force vector
  y[0] = 0.0;
  y[1] = 0.0;
  y[2] = Fl;
  //  approximated suspension point acc.
  d = physics_param[0];
  for (int i{0}; i < 3; i++) {
    d1 = 0.0;
    for (int i1{0}; i1 < 3; i1++) {
      F_tmp = i + 3 * i1;
      d1 += R[F_tmp] * y[i1];
      eR_raw[F_tmp] = (d * R[i] * SOmega_tmp[3 * i1] +
                       d * R[i + 3] * SOmega_tmp[3 * i1 + 1]) +
                      d * R[i + 6] * SOmega_tmp[3 * i1 + 2];
    }
    F[i] = d1;
    d1 = eR_raw[i];
    d2 = eR_raw[i + 3];
    d3 = eR_raw[i + 6];
    for (int i1{0}; i1 < 3; i1++) {
      b_physics_param[i + 3 * i1] =
          (d1 * SOmega_tmp[3 * i1] + d2 * SOmega_tmp[3 * i1 + 1]) +
          d3 * SOmega_tmp[3 * i1 + 2];
    }
  }
  //  errors
  d = L_offset[0];
  d1 = L_offset[1];
  d2 = L_offset[2];
  for (int i{0}; i < 3; i++) {
    eR_raw_tmp_tmp[3 * i] = R[i];
    eR_raw_tmp_tmp[3 * i + 1] = R[i + 3];
    eR_raw_tmp_tmp[3 * i + 2] = R[i + 6];
    y[i] = (b_physics_param[i] * d + b_physics_param[i + 3] * d1) +
           b_physics_param[i + 6] * d2;
  }
  for (int i{0}; i < 3; i++) {
    d = eR_raw_tmp_tmp[i];
    d1 = eR_raw_tmp_tmp[i + 3];
    d2 = eR_raw_tmp_tmp[i + 6];
    for (int i1{0}; i1 < 3; i1++) {
      int b_eR_raw_tmp_tmp;
      int i2;
      F_tmp = 3 * i1 + 1;
      i2 = 3 * i1 + 2;
      d3 = (d * Rd[3 * i1] + d1 * Rd[F_tmp]) + d2 * Rd[i2];
      b_eR_raw_tmp_tmp = i + 3 * i1;
      eR_raw_tmp[b_eR_raw_tmp_tmp] = d3;
      eR_raw[b_eR_raw_tmp_tmp] =
          ((Rd[3 * i] * R[3 * i1] + Rd[3 * i + 1] * R[F_tmp]) +
           Rd[3 * i + 2] * R[i2]) -
          d3;
    }
  }
  r = _mm_loadu_pd(&eR_raw[0]);
  r1 = _mm_set1_pd(0.5);
  _mm_storeu_pd(&eR_raw[0], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw[2]);
  _mm_storeu_pd(&eR_raw[2], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw[4]);
  _mm_storeu_pd(&eR_raw[4], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw[6]);
  _mm_storeu_pd(&eR_raw[6], _mm_mul_pd(r1, r));
  //  before vee map
  eR[0] = eR_raw[5];
  eR[1] = eR_raw[6];
  eR[2] = eR_raw[1];
  //  inner loop control laws
  b_F[0] = (F[0] - load_acc[0] * physics_param[1]) + y[0];
  b_F[1] = (F[1] - physics_param[1] * load_acc[1]) + y[1];
  b_F[2] = (F[2] - (physics_param[1] * load_acc[2] -
                    physics_param[1] * physics_param[2])) +
           y[2];
  y[0] = 0.0;
  y[1] = 0.0;
  R_tmp = physics_param[0] * physics_param[2];
  y[2] = R_tmp;
  d = Omegad[0];
  d1 = Omegad[1];
  d2 = Omegad[2];
  r = _mm_loadu_pd(&eR_raw_tmp[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(d));
  r1 = _mm_loadu_pd(&eR_raw_tmp[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d1));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&eR_raw_tmp[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d2));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&Omega[0]);
  r = _mm_sub_pd(r1, r);
  _mm_storeu_pd(&eI_dt[0], r);
  r = _mm_loadu_pd(&b_F[0]);
  r1 = _mm_loadu_pd(&y[0]);
  r = _mm_add_pd(r, r1);
  r = _mm_div_pd(r, _mm_set1_pd(physics_param[0]));
  _mm_storeu_pd(&b_F[0], r);
  eI_dt[2] = Omega[2] -
             ((eR_raw_tmp[2] * d + eR_raw_tmp[5] * d1) + eR_raw_tmp[8] * d2);
  b_F[2] = (b_F[2] + y[2]) / physics_param[0];
  d = physics_param[0];
  d1 = b_F[0];
  d2 = b_F[1];
  d3 = b_F[2];
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(d));
  r = _mm_mul_pd(r, _mm_set1_pd(d1));
  r1 = _mm_loadu_pd(&eR_raw_tmp_tmp[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d));
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d2));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&eR_raw_tmp_tmp[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d));
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d3));
  r = _mm_add_pd(r, r1);
  _mm_storeu_pd(&b[0], r);
  b[2] = (eR_raw_tmp_tmp[2] * d * d1 + eR_raw_tmp_tmp[5] * d * d2) +
         eR_raw_tmp_tmp[8] * d * d3;
  F[0] = L_offset[1] * b[2] - b[1] * L_offset[2];
  F[1] = b[0] * L_offset[2] - L_offset[0] * b[2];
  F[2] = L_offset[0] * b[1] - b[0] * L_offset[1];
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(0.0));
  r1 = _mm_loadu_pd(&eR_raw_tmp_tmp[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(0.0));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&eR_raw_tmp_tmp[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(R_tmp));
  r = _mm_add_pd(r, r1);
  _mm_storeu_pd(&b[0], r);
  b[2] = (eR_raw_tmp_tmp[2] * 0.0 + eR_raw_tmp_tmp[5] * 0.0) +
         eR_raw_tmp_tmp[8] * R_tmp;
  b_y[0] = L_offset[1] * b[2] - b[1] * L_offset[2];
  b_y[1] = b[0] * L_offset[2] - L_offset[0] * b[2];
  b_y[2] = L_offset[0] * b[1] - b[0] * L_offset[1];
  d = Omega[0];
  d1 = Omega[1];
  d2 = Omega[2];
  r = _mm_loadu_pd(&J[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(d));
  r1 = _mm_loadu_pd(&J[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d1));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&J[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d2));
  r = _mm_add_pd(r, r1);
  _mm_storeu_pd(&b[0], r);
  b[2] = (J[2] * d + J[5] * d1) + J[8] * d2;
  y[0] = physics_param[3];
  y[1] = physics_param[4];
  y[2] = physics_param[5];
  coder::diag(y, R);
  b_F[0] = Omega[1] * b[2] - b[1] * Omega[2];
  b_F[1] = b[0] * Omega[2] - Omega[0] * b[2];
  b_F[2] = Omega[0] * b[1] - b[0] * Omega[1];
  for (int i{0}; i < 3; i++) {
    d = SOmega_tmp[i];
    d1 = SOmega_tmp[i + 3];
    d2 = SOmega_tmp[i + 6];
    for (int i1{0}; i1 < 3; i1++) {
      b_physics_param[i + 3 * i1] =
          (d * eR_raw_tmp_tmp[3 * i1] + d1 * eR_raw_tmp_tmp[3 * i1 + 1]) +
          d2 * eR_raw_tmp_tmp[3 * i1 + 2];
    }
    d = b_physics_param[i];
    d1 = b_physics_param[i + 3];
    d2 = b_physics_param[i + 6];
    d3 = 0.0;
    R_tmp = 0.0;
    for (int i1{0}; i1 < 3; i1++) {
      b_R_tmp = (d * Rd[3 * i1] + d1 * Rd[3 * i1 + 1]) + d2 * Rd[3 * i1 + 2];
      F_tmp = i + 3 * i1;
      SOmega_tmp[F_tmp] = b_R_tmp;
      d3 += b_R_tmp * Omegad[i1];
      R_tmp += eR_raw_tmp[F_tmp] * dOmegad[i1];
    }
    b[i] = d3 - R_tmp;
  }
  __m128d r2;
  __m128d r3;
  __m128d r4;
  double d4;
  //  CoM torque
  //  desired angular acceleration from dynamics
  //  integral derivative
  d = gains[0];
  d1 = gains[1];
  d2 = gains[2];
  d3 = gains[3];
  R_tmp = b[0];
  b_R_tmp = b[1];
  c_R_tmp = b[2];
  d_R_tmp = Omegad[0];
  e_R_tmp = Omegad[1];
  f_R_tmp = Omegad[2];
  r = _mm_loadu_pd(&eR[0]);
  r1 = _mm_mul_pd(_mm_set1_pd(-d), r);
  r2 = _mm_loadu_pd(&eI_dt[0]);
  r3 = _mm_mul_pd(_mm_set1_pd(d1), r2);
  r1 = _mm_sub_pd(r1, r3);
  r3 = _mm_loadu_pd(&eI[0]);
  r3 = _mm_mul_pd(_mm_set1_pd(d2), r3);
  r1 = _mm_sub_pd(r1, r3);
  r3 = _mm_loadu_pd(&b_F[0]);
  r1 = _mm_add_pd(r1, r3);
  r3 = _mm_loadu_pd(&R[0]);
  r3 = _mm_mul_pd(r3, _mm_set1_pd(R_tmp));
  r4 = _mm_loadu_pd(&R[3]);
  r4 = _mm_mul_pd(r4, _mm_set1_pd(b_R_tmp));
  r3 = _mm_add_pd(r3, r4);
  r4 = _mm_loadu_pd(&R[6]);
  r4 = _mm_mul_pd(r4, _mm_set1_pd(c_R_tmp));
  r3 = _mm_add_pd(r3, r4);
  r1 = _mm_sub_pd(r1, r3);
  r3 = _mm_loadu_pd(&F[0]);
  r1 = _mm_sub_pd(r1, r3);
  r3 = _mm_loadu_pd(&b_y[0]);
  r1 = _mm_add_pd(r1, r3);
  _mm_storeu_pd(&taub[0], r1);
  r1 = _mm_loadu_pd(&J[0]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d_R_tmp));
  r3 = _mm_loadu_pd(&J[3]);
  r3 = _mm_mul_pd(r3, _mm_set1_pd(e_R_tmp));
  r1 = _mm_add_pd(r1, r3);
  r3 = _mm_loadu_pd(&J[6]);
  r3 = _mm_mul_pd(r3, _mm_set1_pd(f_R_tmp));
  r1 = _mm_add_pd(r1, r3);
  _mm_storeu_pd(&b[0], r1);
  r = _mm_mul_pd(_mm_set1_pd(d3), r);
  r = _mm_add_pd(r2, r);
  _mm_storeu_pd(&eI_dt[0], r);
  g_R_tmp = eR[2];
  d4 = eI_dt[2];
  taub[2] = (((((-d * g_R_tmp - d1 * d4) - d2 * eI[2]) + b_F[2]) -
              ((R[2] * R_tmp + R[5] * b_R_tmp) + R[8] * c_R_tmp)) -
             F[2]) +
            b_y[2];
  b[2] = (J[2] * d_R_tmp + J[5] * e_R_tmp) + J[8] * f_R_tmp;
  d4 += d3 * g_R_tmp;
  eI_dt[2] = d4;
  tau[0] = taub[0] + (L_offset[1] * Fl - 0.0 * L_offset[2]);
  tau[1] = taub[1] + (0.0 * L_offset[2] - L_offset[0] * Fl);
  tau[2] = taub[2] + (L_offset[0] * 0.0 - 0.0 * L_offset[1]);
  y[0] = ((taub[0] - (Omegad[1] * b[2] - b[1] * Omegad[2])) + F[0]) - b_y[0];
  y[1] = ((taub[1] - (b[0] * Omegad[2] - Omegad[0] * b[2])) + F[1]) - b_y[1];
  y[2] = ((taub[2] - (Omegad[0] * b[1] - b[0] * Omegad[1])) + F[2]) - b_y[2];
  coder::mldivide(J, y, rate_sp_dt);
}

//
// File trailer for Inner_loop.cpp
//
// [EOF]
//
