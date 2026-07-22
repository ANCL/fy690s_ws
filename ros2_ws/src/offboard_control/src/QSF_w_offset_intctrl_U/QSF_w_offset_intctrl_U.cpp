//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// QSF_w_offset_intctrl_U.cpp
//
// Code generation for function 'QSF_w_offset_intctrl_U'
//

// Include files
#include "QSF_w_offset_intctrl_U.h"
#include "rt_nonfinite.h"
#include <cmath>
#include <emmintrin.h>

// Function Declarations
static double rt_powd_snf(double u0, double u1);

// Function Definitions
static double rt_powd_snf(double u0, double u1)
{
  double y;
  if (std::isnan(u0) || std::isnan(u1)) {
    y = rtNaN;
  } else {
    double d;
    double d1;
    d = std::abs(u0);
    d1 = std::abs(u1);
    if (std::isinf(u1)) {
      if (d == 1.0) {
        y = 1.0;
      } else if (d > 1.0) {
        if (u1 > 0.0) {
          y = rtInf;
        } else {
          y = 0.0;
        }
      } else if (u1 > 0.0) {
        y = 0.0;
      } else {
        y = rtInf;
      }
    } else if (d1 == 0.0) {
      y = 1.0;
    } else if (d1 == 1.0) {
      if (u1 > 0.0) {
        y = u0;
      } else {
        y = 1.0 / u0;
      }
    } else if (u1 == 2.0) {
      y = u0 * u0;
    } else if ((u1 == 0.5) && (u0 >= 0.0)) {
      y = std::sqrt(u0);
    } else if ((u0 < 0.0) && (u1 > std::floor(u1))) {
      y = rtNaN;
    } else {
      y = std::pow(u0, u1);
    }
  }
  return y;
}

void QSF_w_offset_intctrl_U(double mq, double l, double g, const double K1[5],
                            const double K2[5], const double K3[3],
                            const double ref_traj[15], const double states[12],
                            double psi, double Td_scaler, const double Int[3],
                            double Rbd[9], double *Fld_scaler, double *phid,
                            double *thetad)
{
  __m128d r;
  double A[9];
  double b_Int[5];
  double Fld[3];
  double dr[3];
  double Int_tmp;
  double U_idx_1;
  double a;
  double a_tmp;
  double absxk;
  double ddnu3;
  double de3;
  double dnu3;
  double e3;
  double nu3;
  double scale;
  double t;
  int r1;
  int r2;
  int r3;
  //  refs
  //  states
  //  xp: payload position
  //  q: payload angle
  //  vp: payload velocity
  //  w: payload angluar velocity
  //  extract elements
  //  error of y3 = act -des
  e3 = states[2] - ref_traj[10];
  de3 = states[8] - ref_traj[11];
  //  integral controller
  nu3 = ((-K3[0] * Int[2] + -K3[1] * e3) + -K3[2] * de3) + ref_traj[12];
  U_idx_1 = nu3 - ref_traj[12];
  dnu3 = ((-K3[0] * e3 + -K3[1] * de3) + -K3[2] * U_idx_1) + ref_traj[13];
  ddnu3 = ((-K3[0] * de3 + -K3[1] * U_idx_1) + -K3[2] * (dnu3 - ref_traj[13])) +
          ref_traj[14];
  //  4th time derivative of y1&y2
  dr[0] = states[5] * states[10] - states[4] * states[11];
  dr[1] = states[3] * states[11] - states[5] * states[9];
  dr[2] = states[4] * states[9] - states[3] * states[10];
  //  error of y1&y2
  //  nu1 and nu2
  //  input u
  a_tmp = nu3 - g;
  U_idx_1 = states[5] * states[5];
  a = l * U_idx_1 / a_tmp;
  b_Int[0] = Int[0];
  b_Int[1] = states[0] - ref_traj[0];
  b_Int[2] = states[6] - ref_traj[1];
  e3 = states[3] / states[5];
  b_Int[3] = e3 * a_tmp - ref_traj[2];
  de3 = dr[0] / states[5] - dr[2] * states[3] / U_idx_1;
  b_Int[4] = (e3 * dnu3 + de3 * a_tmp) - ref_traj[3];
  nu3 = 0.0;
  for (r1 = 0; r1 < 5; r1++) {
    nu3 += -K1[r1] * b_Int[r1];
  }
  b_Int[0] = Int[1];
  b_Int[1] = states[1] - ref_traj[5];
  b_Int[2] = states[7] - ref_traj[6];
  Int_tmp = states[4] / states[5];
  b_Int[3] = Int_tmp * a_tmp - ref_traj[7];
  scale = dr[1] / states[5] - dr[2] * states[4] / U_idx_1;
  b_Int[4] = (Int_tmp * dnu3 + scale * a_tmp) - ref_traj[8];
  absxk = 0.0;
  for (r1 = 0; r1 < 5; r1++) {
    absxk += -K2[r1] * b_Int[r1];
  }
  double K1_idx_0_tmp;
  t = dr[2] * dr[2];
  K1_idx_0_tmp = rt_powd_snf(states[5], 3.0);
  nu3 =
      (nu3 + ref_traj[4]) -
      ((e3 * ddnu3 + 2.0 * dnu3 * de3) +
       2.0 * (states[3] * t / K1_idx_0_tmp - dr[0] * dr[2] / U_idx_1) * a_tmp);
  e3 = (absxk + ref_traj[9]) -
       ((Int_tmp * ddnu3 + 2.0 * dnu3 * scale) +
        2.0 * (states[4] * t / K1_idx_0_tmp - dr[1] * dr[2] / U_idx_1) * a_tmp);
  //  back to virtual input U = ddp - ge3
  scale = 3.3121686421112381E-170;
  absxk = std::abs(states[9]);
  if (absxk > 3.3121686421112381E-170) {
    de3 = 1.0;
    scale = absxk;
  } else {
    t = absxk / 3.3121686421112381E-170;
    de3 = t * t;
  }
  absxk = std::abs(states[10]);
  if (absxk > scale) {
    t = scale / absxk;
    de3 = de3 * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    de3 += t * t;
  }
  absxk = std::abs(states[11]);
  if (absxk > scale) {
    t = scale / absxk;
    de3 = de3 * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    de3 += t * t;
  }
  de3 = scale * std::sqrt(de3);
  A[0] = states[3];
  A[3] = states[4];
  A[6] = states[5];
  A[1] = 0.0;
  A[4] = states[5];
  A[7] = -states[4];
  A[2] = -states[5];
  A[5] = 0.0;
  A[8] = states[3];
  dr[0] = a_tmp / states[5] + l * (de3 * de3);
  dr[1] = a * 0.0 * nu3 + -a * e3;
  dr[2] = a * nu3 + a * 0.0 * e3;
  r1 = 0;
  r2 = 1;
  r3 = 2;
  if (std::abs(-states[5]) > std::abs(states[3])) {
    r1 = 2;
    r3 = 0;
  }
  A[1] = 0.0 / A[r1];
  A[r3] /= A[r1];
  A[4] -= A[1] * A[r1 + 3];
  A[r3 + 3] -= A[r3] * A[r1 + 3];
  A[7] -= A[1] * A[r1 + 6];
  A[r3 + 6] -= A[r3] * A[r1 + 6];
  if (std::abs(A[r3 + 3]) > std::abs(A[4])) {
    r2 = r3;
    r3 = 1;
  }
  A[r3 + 3] /= A[r2 + 3];
  A[r3 + 6] -= A[r3 + 3] * A[r2 + 6];
  U_idx_1 = dr[r2] - dr[r1] * A[r2];
  e3 = ((dr[r3] - dr[r1] * A[r3]) - U_idx_1 * A[r3 + 3]) / A[r3 + 6];
  U_idx_1 -= e3 * A[r2 + 6];
  U_idx_1 /= A[r2 + 3];
  // ddxi to Fld % FT(39)
  Fld[0] = mq * (((dr[r1] - e3 * A[r1 + 6]) - U_idx_1 * A[r1 + 3]) / A[r1]) +
           Td_scaler * states[3];
  Fld[1] = mq * U_idx_1 + Td_scaler * states[4];
  Fld[2] = (mq * (e3 + g) + Td_scaler * states[5]) - mq * g;
  // output FT(41)
  e3 = std::sin(psi);
  de3 = std::cos(psi);
  *thetad = std::atan((Fld[0] * de3 + Fld[1] * e3) / Fld[2]);
  Int_tmp = std::cos(*thetad);
  *phid = -std::atan((-Fld[0] * e3 + Fld[1] * de3) * Int_tmp / Fld[2]);
  //  geometric decoupler
  scale = 3.3121686421112381E-170;
  absxk = std::abs(Fld[0]);
  if (absxk > 3.3121686421112381E-170) {
    e3 = 1.0;
    scale = absxk;
  } else {
    t = absxk / 3.3121686421112381E-170;
    e3 = t * t;
  }
  absxk = std::abs(Fld[1]);
  if (absxk > scale) {
    t = scale / absxk;
    e3 = e3 * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    e3 += t * t;
  }
  absxk = std::abs(Fld[2]);
  if (absxk > scale) {
    t = scale / absxk;
    e3 = e3 * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    e3 += t * t;
  }
  e3 = scale * std::sqrt(e3);
  *Fld_scaler = -e3;
  r = _mm_loadu_pd(&Fld[0]);
  _mm_storeu_pd(&Fld[0],
                _mm_div_pd(_mm_mul_pd(r, _mm_set1_pd(-1.0)), _mm_set1_pd(e3)));
  Fld[2] = -Fld[2] / e3;
  dr[2] = -std::sin(*thetad);
  //  force psi -> 0
  nu3 = Fld[1] * dr[2] - Fld[2] * 0.0;
  U_idx_1 = Int_tmp * Fld[2] - Fld[0] * dr[2];
  e3 = Fld[0] * 0.0 - Int_tmp * Fld[1];
  scale = 3.3121686421112381E-170;
  absxk = std::abs(nu3);
  if (absxk > 3.3121686421112381E-170) {
    de3 = 1.0;
    scale = absxk;
  } else {
    t = absxk / 3.3121686421112381E-170;
    de3 = t * t;
  }
  absxk = std::abs(U_idx_1);
  if (absxk > scale) {
    t = scale / absxk;
    de3 = de3 * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    de3 += t * t;
  }
  absxk = std::abs(e3);
  if (absxk > scale) {
    t = scale / absxk;
    de3 = de3 * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    de3 += t * t;
  }
  de3 = scale * std::sqrt(de3);
  dr[0] = -(Fld[1] * e3 - U_idx_1 * Fld[2]) / de3;
  dr[1] = -(nu3 * Fld[2] - Fld[0] * e3) / de3;
  dr[2] = -(Fld[0] * U_idx_1 - nu3 * Fld[1]) / de3;
  Rbd[3] = Fld[1] * dr[2] - dr[1] * Fld[2];
  Rbd[4] = dr[0] * Fld[2] - Fld[0] * dr[2];
  Rbd[5] = Fld[0] * dr[1] - dr[0] * Fld[1];
  Rbd[0] = dr[0];
  Rbd[6] = Fld[0];
  Rbd[1] = dr[1];
  Rbd[7] = Fld[1];
  Rbd[2] = dr[2];
  Rbd[8] = Fld[2];
}

// End of code generation (QSF_w_offset_intctrl_U.cpp)
