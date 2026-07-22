//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// QSF_w_offset_intctrl.cpp
//
// Code generation for function 'QSF_w_offset_intctrl'
//

// Include files
#include "QSF_w_offset_intctrl.h"
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

void QSF_w_offset_intctrl(double mp, double mq, double l, double g,
                          const double K1[5], const double K2[5],
                          const double K3[3], const double ref_traj[15],
                          const double states[12], double psi,
                          const double aL[3], const double Int[3],
                          double Rbd[9], double *Fld_scaler, double *phid,
                          double *thetad)
{
  __m128d r;
  __m128d r1;
  double b_states[9];
  double b_Int[5];
  double Fld[3];
  double b_nu3[3];
  double Fld_tmp;
  double Int_tmp;
  double a;
  double a_tmp;
  double absxk;
  double b_Int_tmp;
  double b_K2;
  double b_a_tmp;
  double ddnu3;
  double de3;
  double dnu3;
  double dr_idx_0;
  double dr_idx_1;
  double dr_idx_2;
  double e3;
  double nu3;
  double t;
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
  Fld_tmp = nu3 - ref_traj[12];
  dnu3 = ((-K3[0] * e3 + -K3[1] * de3) + -K3[2] * Fld_tmp) + ref_traj[13];
  ddnu3 = ((-K3[0] * de3 + -K3[1] * Fld_tmp) + -K3[2] * (dnu3 - ref_traj[13])) +
          ref_traj[14];
  //  4th time derivative of y1&y2
  dr_idx_0 = states[5] * states[10] - states[4] * states[11];
  dr_idx_1 = states[3] * states[11] - states[5] * states[9];
  dr_idx_2 = states[4] * states[9] - states[3] * states[10];
  //  error of y1&y2
  //  nu1 and nu2
  //  input u
  a_tmp = nu3 - g;
  nu3 = states[5] * states[5];
  b_a_tmp = mq * l;
  a = b_a_tmp * nu3 / a_tmp;
  b_Int[0] = Int[0];
  b_Int[1] = states[0] - ref_traj[0];
  b_Int[2] = states[6] - ref_traj[1];
  e3 = states[3] / states[5];
  b_Int[3] = e3 * a_tmp - ref_traj[2];
  de3 = dr_idx_0 / states[5] - dr_idx_2 * states[3] / nu3;
  b_Int[4] = (e3 * dnu3 + de3 * a_tmp) - ref_traj[3];
  Fld_tmp = 0.0;
  for (int k{0}; k < 5; k++) {
    Fld_tmp += -K1[k] * b_Int[k];
  }
  b_Int[0] = Int[1];
  b_Int[1] = states[1] - ref_traj[5];
  b_Int[2] = states[7] - ref_traj[6];
  Int_tmp = states[4] / states[5];
  b_Int[3] = Int_tmp * a_tmp - ref_traj[7];
  b_Int_tmp = dr_idx_1 / states[5] - dr_idx_2 * states[4] / nu3;
  b_Int[4] = (Int_tmp * dnu3 + b_Int_tmp * a_tmp) - ref_traj[8];
  b_K2 = 0.0;
  for (int k{0}; k < 5; k++) {
    b_K2 += -K2[k] * b_Int[k];
  }
  absxk = dr_idx_2 * dr_idx_2;
  t = rt_powd_snf(states[5], 3.0);
  Fld_tmp = (Fld_tmp + ref_traj[4]) -
            ((e3 * ddnu3 + 2.0 * dnu3 * de3) +
             2.0 * (states[3] * absxk / t - dr_idx_0 * dr_idx_2 / nu3) * a_tmp);
  e3 = (b_K2 + ref_traj[9]) -
       ((Int_tmp * ddnu3 + 2.0 * dnu3 * b_Int_tmp) +
        2.0 * (states[4] * absxk / t - dr_idx_1 * dr_idx_2 / nu3) * a_tmp);
  //  back to physical input F
  ddnu3 = 3.3121686421112381E-170;
  absxk = std::abs(states[9]);
  if (absxk > 3.3121686421112381E-170) {
    de3 = 1.0;
    ddnu3 = absxk;
  } else {
    t = absxk / 3.3121686421112381E-170;
    de3 = t * t;
  }
  absxk = std::abs(states[10]);
  if (absxk > ddnu3) {
    t = ddnu3 / absxk;
    de3 = de3 * t * t + 1.0;
    ddnu3 = absxk;
  } else {
    t = absxk / ddnu3;
    de3 += t * t;
  }
  absxk = std::abs(states[11]);
  if (absxk > ddnu3) {
    t = ddnu3 / absxk;
    de3 = de3 * t * t + 1.0;
    ddnu3 = absxk;
  } else {
    t = absxk / ddnu3;
    de3 += t * t;
  }
  de3 = ddnu3 * std::sqrt(de3);
  b_nu3[0] = a_tmp / states[5];
  b_nu3[1] = a * 0.0 * Fld_tmp + -a * e3;
  b_nu3[2] = a * Fld_tmp + a * 0.0 * e3;
  Fld_tmp = mp + mq;
  Fld[0] = b_a_tmp / Fld_tmp * (de3 * de3);
  Fld[1] = 0.0;
  Fld[2] = 0.0;
  b_states[0] = states[3] * Fld_tmp;
  b_states[3] = -states[4] * states[3] / states[5];
  b_states[6] = (states[3] * states[3] - 1.0) / states[5];
  b_states[1] = states[4] * Fld_tmp;
  b_states[4] = (-(states[4] * states[4]) + 1.0) / states[5];
  b_states[7] = states[3] * states[4] / states[5];
  b_states[2] = states[5] * Fld_tmp;
  b_states[5] = -states[4];
  b_states[8] = states[3];
  r = _mm_loadu_pd(&b_nu3[0]);
  r1 = _mm_loadu_pd(&Fld[0]);
  _mm_storeu_pd(&b_nu3[0], _mm_add_pd(r, r1));
  // output FT(41)
  Int_tmp = std::sin(psi);
  b_Int_tmp = std::cos(psi);
  //  geometric decoupler
  b_K2 = 0.0;
  ddnu3 = 3.3121686421112381E-170;
  de3 = b_nu3[0];
  Fld_tmp = b_nu3[1];
  nu3 = b_nu3[2];
  for (int k{0}; k < 3; k++) {
    e3 = ((b_states[k] * de3 + b_states[k + 3] * Fld_tmp) +
          b_states[k + 6] * nu3) -
         mq * aL[k];
    Fld[k] = e3;
    absxk = std::abs(e3);
    if (absxk > ddnu3) {
      t = ddnu3 / absxk;
      b_K2 = b_K2 * t * t + 1.0;
      ddnu3 = absxk;
    } else {
      t = absxk / ddnu3;
      b_K2 += t * t;
    }
  }
  *thetad = std::atan((Fld[0] * b_Int_tmp + Fld[1] * Int_tmp) / Fld[2]);
  e3 = std::cos(*thetad);
  *phid = -std::atan((-Fld[0] * Int_tmp + Fld[1] * b_Int_tmp) * e3 / Fld[2]);
  b_K2 = ddnu3 * std::sqrt(b_K2);
  *Fld_scaler = -b_K2;
  r = _mm_loadu_pd(&Fld[0]);
  _mm_storeu_pd(
      &Fld[0], _mm_div_pd(_mm_mul_pd(r, _mm_set1_pd(-1.0)), _mm_set1_pd(b_K2)));
  Fld[2] = -Fld[2] / b_K2;
  dr_idx_2 = -std::sin(*thetad);
  //  force psi -> 0
  nu3 = Fld[1] * dr_idx_2 - Fld[2] * 0.0;
  Fld_tmp = e3 * Fld[2] - Fld[0] * dr_idx_2;
  e3 = Fld[0] * 0.0 - e3 * Fld[1];
  ddnu3 = 3.3121686421112381E-170;
  absxk = std::abs(nu3);
  if (absxk > 3.3121686421112381E-170) {
    de3 = 1.0;
    ddnu3 = absxk;
  } else {
    t = absxk / 3.3121686421112381E-170;
    de3 = t * t;
  }
  absxk = std::abs(Fld_tmp);
  if (absxk > ddnu3) {
    t = ddnu3 / absxk;
    de3 = de3 * t * t + 1.0;
    ddnu3 = absxk;
  } else {
    t = absxk / ddnu3;
    de3 += t * t;
  }
  absxk = std::abs(e3);
  if (absxk > ddnu3) {
    t = ddnu3 / absxk;
    de3 = de3 * t * t + 1.0;
    ddnu3 = absxk;
  } else {
    t = absxk / ddnu3;
    de3 += t * t;
  }
  de3 = ddnu3 * std::sqrt(de3);
  dr_idx_0 = -(Fld[1] * e3 - Fld_tmp * Fld[2]) / de3;
  dr_idx_1 = -(nu3 * Fld[2] - Fld[0] * e3) / de3;
  dr_idx_2 = -(Fld[0] * Fld_tmp - nu3 * Fld[1]) / de3;
  Rbd[3] = Fld[1] * dr_idx_2 - dr_idx_1 * Fld[2];
  Rbd[4] = dr_idx_0 * Fld[2] - Fld[0] * dr_idx_2;
  Rbd[5] = Fld[0] * dr_idx_1 - dr_idx_0 * Fld[1];
  Rbd[0] = dr_idx_0;
  Rbd[6] = Fld[0];
  Rbd[1] = dr_idx_1;
  Rbd[7] = Fld[1];
  Rbd[2] = dr_idx_2;
  Rbd[8] = Fld[2];
}

// End of code generation (QSF_w_offset_intctrl.cpp)
