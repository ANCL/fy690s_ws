//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// Inner_loop.cpp
//
// Code generation for function 'Inner_loop'
//

// Include files
#include "Inner_loop.h"
#include <cmath>
#include <emmintrin.h>

// Function Definitions
void Inner_loop(const double rpy_angles[3], const double Omega[3],
                const double Rd[9], const double Omegad[3],
                const double dOmegad[3], double Fl, const double gains[2],
                const double physics_param[6], const double L_offset[3],
                const double ddxi_flat[3], const double load_acc[3],
                double taub[3], double tau[3], double rate_sp[3])
{
  __m128d b_r2;
  __m128d b_r3;
  __m128d r;
  __m128d r1;
  double J[9];
  double R[9];
  double eR_raw[9];
  double eR_raw_tmp[9];
  double eR_raw_tmp_tmp[9];
  double F[3];
  double b_Omega[3];
  double b_gains[3];
  double b_y[3];
  double c_Omega[3];
  double c_y[3];
  double dv[3];
  double y[3];
  double J_tmp;
  double R_tmp;
  double a21;
  double b_J_tmp;
  double b_R_tmp;
  double d;
  double d1;
  double d2;
  double ddxi_idx_0;
  double ddxi_idx_1;
  double ddxi_idx_2;
  double maxval;
  int b_r1;
  int r2;
  int r3;
  int rtemp;
  //  extract
  a21 = std::cos(rpy_angles[1]);
  maxval = std::sin(rpy_angles[1]);
  ddxi_idx_0 = std::cos(rpy_angles[2]);
  ddxi_idx_1 = std::sin(rpy_angles[2]);
  R_tmp = std::cos(rpy_angles[0]);
  ddxi_idx_2 = std::sin(rpy_angles[0]);
  R[0] = a21 * ddxi_idx_0;
  R[1] = a21 * ddxi_idx_1;
  R[2] = -maxval;
  b_R_tmp = ddxi_idx_2 * maxval;
  R[3] = b_R_tmp * ddxi_idx_0 - R_tmp * ddxi_idx_1;
  R[4] = b_R_tmp * ddxi_idx_1 + R_tmp * ddxi_idx_0;
  R[5] = ddxi_idx_2 * a21;
  maxval *= R_tmp;
  R[6] = maxval * ddxi_idx_0 + ddxi_idx_2 * ddxi_idx_1;
  R[7] = maxval * ddxi_idx_1 - ddxi_idx_2 * ddxi_idx_0;
  R[8] = R_tmp * a21;
  //  Rbi
  a21 = L_offset[2] * L_offset[2];
  maxval = L_offset[1] * L_offset[1];
  J[0] = (maxval + a21) * physics_param[0] + physics_param[3];
  J_tmp = -physics_param[0] * L_offset[0];
  b_J_tmp = J_tmp * L_offset[1];
  J[3] = b_J_tmp;
  J_tmp *= L_offset[2];
  J[6] = J_tmp;
  J[1] = b_J_tmp;
  ddxi_idx_0 = L_offset[0] * L_offset[0];
  J[4] = physics_param[4] + physics_param[0] * (ddxi_idx_0 + a21);
  a21 = -physics_param[0] * L_offset[1] * L_offset[2];
  J[7] = a21;
  J[2] = J_tmp;
  J[5] = a21;
  J[8] = physics_param[5] + physics_param[0] * (ddxi_idx_0 + maxval);
  //  skew
  //  exact form with ddRbi from flatenss
  //  errors
  for (rtemp = 0; rtemp < 3; rtemp++) {
    a21 = R[rtemp];
    maxval = a21 * 0.0;
    eR_raw_tmp_tmp[3 * rtemp] = a21;
    a21 = R[rtemp + 3];
    maxval += a21 * 0.0;
    eR_raw_tmp_tmp[3 * rtemp + 1] = a21;
    a21 = R[rtemp + 6];
    maxval += a21 * Fl;
    eR_raw_tmp_tmp[3 * rtemp + 2] = a21;
    F[rtemp] = maxval;
  }
  ddxi_idx_0 = ((F[0] - load_acc[0] * physics_param[1]) + ddxi_flat[0]) /
               physics_param[0];
  ddxi_idx_1 = ((F[1] - physics_param[1] * load_acc[1]) + ddxi_flat[1]) /
               physics_param[0];
  R_tmp = physics_param[0] * physics_param[2];
  ddxi_idx_2 = (((F[2] - (physics_param[1] * load_acc[2] -
                          physics_param[1] * physics_param[2])) +
                 ddxi_flat[2]) +
                R_tmp) /
               physics_param[0];
  for (rtemp = 0; rtemp < 3; rtemp++) {
    a21 = eR_raw_tmp_tmp[rtemp];
    maxval = eR_raw_tmp_tmp[rtemp + 3];
    b_R_tmp = eR_raw_tmp_tmp[rtemp + 6];
    for (int i{0}; i < 3; i++) {
      b_r1 = 3 * i + 1;
      r2 = 3 * i + 2;
      d = (a21 * Rd[3 * i] + maxval * Rd[b_r1]) + b_R_tmp * Rd[r2];
      r3 = rtemp + 3 * i;
      eR_raw_tmp[r3] = d;
      eR_raw[r3] = ((Rd[3 * rtemp] * R[3 * i] + Rd[3 * rtemp + 1] * R[b_r1]) +
                    Rd[3 * rtemp + 2] * R[r2]) -
                   d;
    }
  }
  //  before vee map
  //  inner loop control laws
  r = _mm_loadu_pd(&eR_raw[0]);
  r1 = _mm_set1_pd(0.5);
  _mm_storeu_pd(&eR_raw[0], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[0]);
  b_r2 = _mm_set1_pd(physics_param[0]);
  _mm_storeu_pd(&R[0], _mm_mul_pd(r, b_r2));
  r = _mm_loadu_pd(&eR_raw[2]);
  _mm_storeu_pd(&eR_raw[2], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[2]);
  _mm_storeu_pd(&R[2], _mm_mul_pd(r, b_r2));
  r = _mm_loadu_pd(&eR_raw[4]);
  _mm_storeu_pd(&eR_raw[4], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[4]);
  _mm_storeu_pd(&R[4], _mm_mul_pd(r, b_r2));
  r = _mm_loadu_pd(&eR_raw[6]);
  _mm_storeu_pd(&eR_raw[6], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[6]);
  _mm_storeu_pd(&R[6], _mm_mul_pd(r, b_r2));
  R[8] = physics_param[0] * eR_raw_tmp_tmp[8];
  dv[2] = physics_param[2];
  a21 = Omegad[0];
  maxval = Omegad[1];
  b_R_tmp = Omegad[2];
  r = _mm_loadu_pd(&R[0]);
  r1 = _mm_mul_pd(r, _mm_set1_pd(ddxi_idx_0));
  r = _mm_mul_pd(r, _mm_set1_pd(0.0));
  b_r2 = _mm_loadu_pd(&R[3]);
  b_r3 = _mm_mul_pd(b_r2, _mm_set1_pd(ddxi_idx_1));
  r1 = _mm_add_pd(r1, b_r3);
  b_r2 = _mm_mul_pd(b_r2, _mm_set1_pd(0.0));
  r = _mm_add_pd(r, b_r2);
  b_r2 = _mm_loadu_pd(&R[6]);
  b_r3 = _mm_mul_pd(b_r2, _mm_set1_pd(ddxi_idx_2));
  r1 = _mm_add_pd(r1, b_r3);
  b_r2 = _mm_mul_pd(b_r2, _mm_set1_pd(dv[2]));
  r = _mm_add_pd(r, b_r2);
  _mm_storeu_pd(&c_y[0], r);
  _mm_storeu_pd(&b_y[0], r1);
  r = _mm_loadu_pd(&J[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(Omega[0]));
  r1 = _mm_loadu_pd(&J[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(Omega[1]));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&J[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(Omega[2]));
  r = _mm_add_pd(r, r1);
  _mm_storeu_pd(&y[0], r);
  r = _mm_loadu_pd(&eR_raw_tmp[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(a21));
  r1 = _mm_loadu_pd(&eR_raw_tmp[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(maxval));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&eR_raw_tmp[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(b_R_tmp));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&Omega[0]);
  r = _mm_sub_pd(r1, r);
  _mm_storeu_pd(&b_Omega[0], r);
  d = R[2];
  d1 = d * ddxi_idx_0;
  d2 = d * 0.0;
  d = R[5];
  d1 += d * ddxi_idx_1;
  d2 += d * 0.0;
  d = R[8];
  d1 += d * ddxi_idx_2;
  d2 += d * dv[2];
  c_y[2] = d2;
  b_y[2] = d1;
  y[2] = (Omega[0] * J[2] + Omega[1] * J[5]) + Omega[2] * J[8];
  b_Omega[2] = Omega[2] - ((eR_raw_tmp[2] * a21 + eR_raw_tmp[5] * maxval) +
                           eR_raw_tmp[8] * b_R_tmp);
  b_gains[0] = -gains[0] * eR_raw[5];
  b_gains[1] = -gains[0] * eR_raw[6];
  b_gains[2] = -gains[0] * eR_raw[1];
  c_Omega[0] = Omega[1] * y[2] - y[1] * Omega[2];
  c_Omega[1] = y[0] * Omega[2] - Omega[0] * y[2];
  c_Omega[2] = Omega[0] * y[1] - y[0] * Omega[1];
  R[0] = 0.0;
  R[3] = -Omega[2];
  R[6] = Omega[1];
  R[1] = Omega[2];
  R[4] = 0.0;
  R[7] = -Omega[0];
  R[2] = -Omega[1];
  R[5] = Omega[0];
  R[8] = 0.0;
  for (rtemp = 0; rtemp < 3; rtemp++) {
    a21 = R[rtemp];
    maxval = R[rtemp + 3];
    b_R_tmp = R[rtemp + 6];
    for (int i{0}; i < 3; i++) {
      eR_raw[rtemp + 3 * i] =
          (a21 * eR_raw_tmp_tmp[3 * i] + maxval * eR_raw_tmp_tmp[3 * i + 1]) +
          b_R_tmp * eR_raw_tmp_tmp[3 * i + 2];
    }
    a21 = eR_raw[rtemp];
    maxval = eR_raw[rtemp + 3];
    b_R_tmp = eR_raw[rtemp + 6];
    d = 0.0;
    d1 = 0.0;
    for (int i{0}; i < 3; i++) {
      d2 = (a21 * Rd[3 * i] + maxval * Rd[3 * i + 1]) + b_R_tmp * Rd[3 * i + 2];
      b_r1 = rtemp + 3 * i;
      R[b_r1] = d2;
      d += d2 * Omegad[i];
      d1 += eR_raw_tmp[b_r1] * dOmegad[i];
    }
    dv[rtemp] = d - d1;
  }
  __m128d r4;
  __m128d r5;
  double d3;
  double d4;
  double d5;
  y[0] = L_offset[1] * b_y[2] - b_y[1] * L_offset[2];
  y[1] = b_y[0] * L_offset[2] - L_offset[0] * b_y[2];
  y[2] = L_offset[0] * b_y[1] - b_y[0] * L_offset[1];
  b_y[0] = L_offset[1] * c_y[2] - c_y[1] * L_offset[2];
  b_y[1] = c_y[0] * L_offset[2] - L_offset[0] * c_y[2];
  b_y[2] = L_offset[0] * c_y[1] - c_y[0] * L_offset[1];
  a21 = dv[0];
  maxval = dv[1];
  b_R_tmp = dv[2];
  d = Omegad[0];
  d1 = Omegad[1];
  d2 = Omegad[2];
  r = _mm_loadu_pd(&b_Omega[0]);
  r = _mm_mul_pd(_mm_set1_pd(gains[1]), r);
  r1 = _mm_loadu_pd(&b_gains[0]);
  r = _mm_sub_pd(r1, r);
  r1 = _mm_loadu_pd(&c_Omega[0]);
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&J[0]);
  b_r2 = _mm_mul_pd(r1, _mm_set1_pd(a21));
  b_r3 = _mm_loadu_pd(&J[3]);
  r4 = _mm_mul_pd(b_r3, _mm_set1_pd(maxval));
  b_r2 = _mm_add_pd(b_r2, r4);
  r4 = _mm_loadu_pd(&J[6]);
  r5 = _mm_mul_pd(r4, _mm_set1_pd(b_R_tmp));
  b_r2 = _mm_add_pd(b_r2, r5);
  r = _mm_sub_pd(r, b_r2);
  b_r2 = _mm_loadu_pd(&y[0]);
  r = _mm_sub_pd(r, b_r2);
  b_r2 = _mm_loadu_pd(&b_y[0]);
  r = _mm_add_pd(r, b_r2);
  _mm_storeu_pd(&taub[0], r);
  r = _mm_mul_pd(r1, _mm_set1_pd(d));
  r1 = _mm_mul_pd(b_r3, _mm_set1_pd(d1));
  r = _mm_add_pd(r, r1);
  r1 = _mm_mul_pd(r4, _mm_set1_pd(d2));
  r = _mm_add_pd(r, r1);
  _mm_storeu_pd(&y[0], r);
  d3 = J[2];
  d4 = J[5];
  d5 = J[8];
  taub[2] = ((((b_gains[2] - gains[1] * b_Omega[2]) + c_Omega[2]) -
              ((d3 * a21 + d4 * maxval) + d5 * b_R_tmp)) -
             y[2]) +
            b_y[2];
  y[2] = (d3 * d + d4 * d1) + d5 * d2;
  tau[0] = taub[0] + (L_offset[1] * Fl - L_offset[2] * 0.0);
  tau[1] = taub[1] + (L_offset[2] * 0.0 - L_offset[0] * Fl);
  tau[2] = taub[2] + (L_offset[0] * 0.0 - L_offset[1] * 0.0);
  a21 = physics_param[0];
  r = _mm_loadu_pd(&eR_raw_tmp_tmp[0]);
  r1 = _mm_mul_pd(r, _mm_set1_pd(a21));
  r1 = _mm_mul_pd(r1, _mm_set1_pd(ddxi_idx_0));
  r = _mm_mul_pd(r, _mm_set1_pd(0.0));
  b_r2 = _mm_loadu_pd(&eR_raw_tmp_tmp[3]);
  b_r3 = _mm_mul_pd(b_r2, _mm_set1_pd(a21));
  b_r3 = _mm_mul_pd(b_r3, _mm_set1_pd(ddxi_idx_1));
  r1 = _mm_add_pd(r1, b_r3);
  b_r2 = _mm_mul_pd(b_r2, _mm_set1_pd(0.0));
  r = _mm_add_pd(r, b_r2);
  b_r2 = _mm_loadu_pd(&eR_raw_tmp_tmp[6]);
  b_r3 = _mm_mul_pd(b_r2, _mm_set1_pd(a21));
  b_r3 = _mm_mul_pd(b_r3, _mm_set1_pd(ddxi_idx_2));
  r1 = _mm_add_pd(r1, b_r3);
  b_r2 = _mm_mul_pd(b_r2, _mm_set1_pd(R_tmp));
  r = _mm_add_pd(r, b_r2);
  _mm_storeu_pd(&c_y[0], r);
  _mm_storeu_pd(&b_y[0], r1);
  maxval = eR_raw_tmp_tmp[2];
  b_R_tmp = maxval * a21 * ddxi_idx_0;
  d = maxval * 0.0;
  maxval = eR_raw_tmp_tmp[5];
  b_R_tmp += maxval * a21 * ddxi_idx_1;
  d += maxval * 0.0;
  maxval = eR_raw_tmp_tmp[8];
  b_R_tmp += maxval * a21 * ddxi_idx_2;
  d += maxval * R_tmp;
  c_y[2] = d;
  b_y[2] = b_R_tmp;
  F[0] = ((taub[0] - (Omegad[1] * y[2] - y[1] * Omegad[2])) +
          (L_offset[1] * b_y[2] - b_y[1] * L_offset[2])) -
         (L_offset[1] * c_y[2] - c_y[1] * L_offset[2]);
  F[1] = ((taub[1] - (y[0] * Omegad[2] - Omegad[0] * y[2])) +
          (b_y[0] * L_offset[2] - L_offset[0] * b_y[2])) -
         (c_y[0] * L_offset[2] - L_offset[0] * c_y[2]);
  F[2] = ((taub[2] - (Omegad[0] * y[1] - y[0] * Omegad[1])) +
          (L_offset[0] * b_y[1] - b_y[0] * L_offset[1])) -
         (L_offset[0] * c_y[1] - c_y[0] * L_offset[1]);
  b_r1 = 0;
  r2 = 1;
  r3 = 2;
  maxval = std::abs(J[0]);
  a21 = std::abs(b_J_tmp);
  if (a21 > maxval) {
    maxval = a21;
    b_r1 = 1;
    r2 = 0;
  }
  if (std::abs(J_tmp) > maxval) {
    b_r1 = 2;
    r2 = 1;
    r3 = 0;
  }
  J[r2] /= J[b_r1];
  J[r3] /= J[b_r1];
  J[r2 + 3] -= J[r2] * J[b_r1 + 3];
  J[r3 + 3] -= J[r3] * J[b_r1 + 3];
  J[r2 + 6] -= J[r2] * J[b_r1 + 6];
  J[r3 + 6] -= J[r3] * J[b_r1 + 6];
  if (std::abs(J[r3 + 3]) > std::abs(J[r2 + 3])) {
    rtemp = r2;
    r2 = r3;
    r3 = rtemp;
  }
  J[r3 + 3] /= J[r2 + 3];
  J[r3 + 6] -= J[r3 + 3] * J[r2 + 6];
  rate_sp[1] = F[r2] - F[b_r1] * J[r2];
  rate_sp[2] = (F[r3] - F[b_r1] * J[r3]) - rate_sp[1] * J[r3 + 3];
  rate_sp[2] /= J[r3 + 6];
  rate_sp[0] = F[b_r1] - rate_sp[2] * J[b_r1 + 6];
  rate_sp[1] -= rate_sp[2] * J[r2 + 6];
  rate_sp[1] /= J[r2 + 3];
  rate_sp[0] -= rate_sp[1] * J[b_r1 + 3];
  rate_sp[0] /= J[b_r1];
}

// End of code generation (Inner_loop.cpp)
