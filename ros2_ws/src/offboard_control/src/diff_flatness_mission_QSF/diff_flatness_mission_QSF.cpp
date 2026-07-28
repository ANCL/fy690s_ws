//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// diff_flatness_mission_QSF.cpp
//
// Code generation for function 'diff_flatness_mission_QSF'
//

// Include files
#include "diff_flatness_mission_QSF.h"
#include "rt_nonfinite.h"
#include <cmath>
#include <emmintrin.h>

// Function Declarations
static double rt_powd_snf(double u0, double u1);

static void unitvec_dt(const double v[3], const double dv[3],
                       const double ddv[3], const double dddv[3],
                       const double ddddv[3], double du[3], double ddu[3],
                       double d3u[3], double d4u[3]);

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

static void unitvec_dt(const double v[3], const double dv[3],
                       const double ddv[3], const double dddv[3],
                       const double ddddv[3], double du[3], double ddu[3],
                       double d3u[3], double d4u[3])
{
  static const signed char b_I[9]{1, 0, 0, 0, 1, 0, 0, 0, 1};
  __m128d r;
  __m128d r1;
  __m128d r2;
  __m128d r3;
  __m128d r4;
  double P[9];
  double b_ddu[9];
  double c_u[9];
  double d3u_tmp[9];
  double dP[9];
  double b_P[3];
  double u[3];
  double a;
  double a_tmp;
  double absxk;
  double b;
  double b_a;
  double b_a_tmp;
  double b_b;
  double b_du;
  double b_tmp;
  double b_u;
  double c_a;
  double c_a_tmp;
  double d;
  double d1;
  double d2;
  double d3;
  double d4;
  double d5;
  double d6;
  double d7;
  double d8;
  double d9;
  double d_a;
  double d_a_tmp;
  double dds;
  double e_a;
  double f_a;
  double s;
  double scale;
  double t;
  int P_tmp;
  //  helper functions
  scale = 3.3121686421112381E-170;
  absxk = std::abs(v[0]);
  if (absxk > 3.3121686421112381E-170) {
    s = 1.0;
    scale = absxk;
  } else {
    t = absxk / 3.3121686421112381E-170;
    s = t * t;
  }
  absxk = std::abs(v[1]);
  if (absxk > scale) {
    t = scale / absxk;
    s = s * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    s += t * t;
  }
  absxk = std::abs(v[2]);
  if (absxk > scale) {
    t = scale / absxk;
    s = s * t * t + 1.0;
    scale = absxk;
  } else {
    t = absxk / scale;
    s += t * t;
  }
  s = scale * std::sqrt(s);
  r = _mm_set1_pd(s);
  _mm_storeu_pd(&u[0], _mm_div_pd(_mm_loadu_pd(&v[0]), r));
  u[2] = v[2] / s;
  b_u = 0.0;
  for (int i{0}; i < 3; i++) {
    P[3 * i] = static_cast<double>(b_I[3 * i]) - u[0] * u[i];
    P_tmp = 3 * i + 1;
    P[P_tmp] = static_cast<double>(b_I[P_tmp]) - u[1] * u[i];
    P_tmp = 3 * i + 2;
    P[P_tmp] = static_cast<double>(b_I[P_tmp]) - u[2] * u[i];
    b_u += u[i] * dv[i];
  }
  a_tmp = 1.0 / s;
  b_a_tmp = b_u / s;
  c_a_tmp = 2.0 * b_a_tmp;
  d_a_tmp = 0.0;
  scale = 3.3121686421112381E-170;
  d = dv[0];
  d1 = dv[1];
  d2 = dv[2];
  for (P_tmp = 0; P_tmp < 3; P_tmp++) {
    d3 = a_tmp * ((P[P_tmp] * d + P[P_tmp + 3] * d1) + P[P_tmp + 6] * d2);
    du[P_tmp] = d3;
    absxk = std::abs(d3);
    if (absxk > scale) {
      t = scale / absxk;
      d_a_tmp = d_a_tmp * t * t + 1.0;
      scale = absxk;
    } else {
      t = absxk / scale;
      d_a_tmp += t * t;
    }
  }
  d_a_tmp = scale * std::sqrt(d_a_tmp);
  b_tmp = d_a_tmp * d_a_tmp;
  d = ddv[0];
  d1 = ddv[1];
  d2 = ddv[2];
  for (int i{0}; i < 3; i++) {
    dP[3 * i] = du[0] * u[i];
    c_u[3 * i] = u[0] * du[i];
    P_tmp = 3 * i + 1;
    dP[P_tmp] = du[1] * u[i];
    c_u[P_tmp] = u[1] * du[i];
    P_tmp = 3 * i + 2;
    dP[P_tmp] = du[2] * u[i];
    c_u[P_tmp] = u[2] * du[i];
    ddu[i] = (a_tmp * ((P[i] * d + P[i + 3] * d1) + P[i + 6] * d2) -
              c_a_tmp * du[i]) -
             u[i] * b_tmp;
  }
  r1 = _mm_loadu_pd(&dP[0]);
  r2 = _mm_loadu_pd(&c_u[0]);
  r3 = _mm_set1_pd(-1.0);
  _mm_storeu_pd(&dP[0], _mm_mul_pd(_mm_add_pd(r1, r2), r3));
  r1 = _mm_loadu_pd(&dP[2]);
  r2 = _mm_loadu_pd(&c_u[2]);
  _mm_storeu_pd(&dP[2], _mm_mul_pd(_mm_add_pd(r1, r2), r3));
  r1 = _mm_loadu_pd(&dP[4]);
  r2 = _mm_loadu_pd(&c_u[4]);
  _mm_storeu_pd(&dP[4], _mm_mul_pd(_mm_add_pd(r1, r2), r3));
  r1 = _mm_loadu_pd(&dP[6]);
  r2 = _mm_loadu_pd(&c_u[6]);
  _mm_storeu_pd(&dP[6], _mm_mul_pd(_mm_add_pd(r1, r2), r3));
  dP[8] = -(dP[8] + c_u[8]);
  dds = ((du[0] * dv[0] + du[1] * dv[1]) + du[2] * dv[2]) +
        ((u[0] * ddv[0] + u[1] * ddv[1]) + u[2] * ddv[2]);
  //  Helper scalars
  d_a_tmp = s * s;
  absxk = b_u * b_u;
  a = dds / s - absxk / d_a_tmp;
  //  db = a;
  //  Third derivative
  r1 = _mm_loadu_pd(&P[0]);
  r2 = _mm_loadu_pd(&dP[0]);
  r4 = _mm_set1_pd(b_a_tmp);
  _mm_storeu_pd(&d3u_tmp[0], _mm_sub_pd(r2, _mm_mul_pd(r4, r1)));
  r1 = _mm_loadu_pd(&P[2]);
  r2 = _mm_loadu_pd(&dP[2]);
  _mm_storeu_pd(&d3u_tmp[2], _mm_sub_pd(r2, _mm_mul_pd(r4, r1)));
  r1 = _mm_loadu_pd(&P[4]);
  r2 = _mm_loadu_pd(&dP[4]);
  _mm_storeu_pd(&d3u_tmp[4], _mm_sub_pd(r2, _mm_mul_pd(r4, r1)));
  r1 = _mm_loadu_pd(&P[6]);
  r2 = _mm_loadu_pd(&dP[6]);
  _mm_storeu_pd(&d3u_tmp[6], _mm_sub_pd(r2, _mm_mul_pd(r4, r1)));
  d3u_tmp[8] = dP[8] - b_a_tmp * P[8];
  b_a = 2.0 * a;
  c_a = 2.0 * ((du[0] * ddu[0] + du[1] * ddu[1]) + du[2] * ddu[2]);
  //  fourth derivative
  d_a = 2.0 / s;
  b = b_u / d_a_tmp;
  e_a = dds / d_a_tmp;
  b_b = absxk / rt_powd_snf(s, 3.0);
  absxk = 0.0;
  t = 0.0;
  d = 0.0;
  f_a = 4.0 * a;
  scale = 0.0;
  b_a_tmp = 0.0;
  b_du = 0.0;
  d1 = ddddv[0];
  d2 = ddddv[1];
  d3 = ddddv[2];
  for (int i{0}; i < 3; i++) {
    double d10;
    d4 = P[i + 3];
    d5 = P[i + 6];
    d6 = P[i];
    d7 = du[i];
    d8 = ddu[i];
    d9 = u[i];
    d10 = ((((a_tmp * ((d6 * dddv[0] + d4 * dddv[1]) + d5 * dddv[2]) +
              ((a_tmp * d3u_tmp[i] * ddv[0] + a_tmp * d3u_tmp[i + 3] * ddv[1]) +
               a_tmp * d3u_tmp[i + 6] * ddv[2])) -
             b_a * d7) -
            c_a_tmp * d8) -
           c_a * d9) -
          d7 * b_tmp;
    d3u[i] = d10;
    absxk += d9 * dddv[i];
    t += d8 * dv[i];
    d += 2.0 * d7 * ddv[i];
    scale += d7 * d8;
    b_a_tmp += d8 * d8;
    b_du += d7 * d10;
    b_ddu[3 * i] = ddu[0] * d9 + 2.0 * (du[0] * du[i]);
    c_u[3 * i] = u[0] * ddu[i];
    P_tmp = 3 * i + 1;
    b_ddu[P_tmp] = ddu[1] * u[i] + 2.0 * (du[1] * du[i]);
    c_u[P_tmp] = u[1] * ddu[i];
    P_tmp = 3 * i + 2;
    b_ddu[P_tmp] = ddu[2] * u[i] + 2.0 * (du[2] * du[i]);
    c_u[P_tmp] = u[2] * ddu[i];
    b_P[i] = (d6 * d1 + d4 * d2) + d5 * d3;
  }
  __m128d r10;
  __m128d r5;
  __m128d r6;
  __m128d r7;
  __m128d r8;
  __m128d r9;
  a = 2.0 * ((((absxk + t) + d) / s - dds * b_u / d_a_tmp) - c_a_tmp * a);
  b_a = 4.0 * scale;
  absxk = b_a_tmp + b_du;
  r1 = _mm_loadu_pd(&dP[0]);
  r2 = _mm_loadu_pd(&b_ddu[0]);
  r4 = _mm_loadu_pd(&c_u[0]);
  r5 = _mm_loadu_pd(&P[0]);
  r6 = _mm_set1_pd(-2.0);
  r7 = _mm_set1_pd(b);
  r8 = _mm_set1_pd(e_a);
  r9 = _mm_set1_pd(2.0);
  r10 = _mm_set1_pd(b_b);
  _mm_storeu_pd(
      &dP[0],
      _mm_add_pd(
          _mm_sub_pd(
              _mm_add_pd(_mm_mul_pd(_mm_mul_pd(r6, r1), r7),
                         _mm_div_pd(_mm_mul_pd(_mm_add_pd(r2, r4), r3), r)),
              _mm_mul_pd(r8, r5)),
          _mm_mul_pd(_mm_mul_pd(r5, r9), r10)));
  r1 = _mm_loadu_pd(&dP[2]);
  r2 = _mm_loadu_pd(&b_ddu[2]);
  r4 = _mm_loadu_pd(&c_u[2]);
  r5 = _mm_loadu_pd(&P[2]);
  _mm_storeu_pd(
      &dP[2],
      _mm_add_pd(
          _mm_sub_pd(
              _mm_add_pd(_mm_mul_pd(_mm_mul_pd(r6, r1), r7),
                         _mm_div_pd(_mm_mul_pd(_mm_add_pd(r2, r4), r3), r)),
              _mm_mul_pd(r8, r5)),
          _mm_mul_pd(_mm_mul_pd(r5, r9), r10)));
  r1 = _mm_loadu_pd(&dP[4]);
  r2 = _mm_loadu_pd(&b_ddu[4]);
  r4 = _mm_loadu_pd(&c_u[4]);
  r5 = _mm_loadu_pd(&P[4]);
  _mm_storeu_pd(
      &dP[4],
      _mm_add_pd(
          _mm_sub_pd(
              _mm_add_pd(_mm_mul_pd(_mm_mul_pd(r6, r1), r7),
                         _mm_div_pd(_mm_mul_pd(_mm_add_pd(r2, r4), r3), r)),
              _mm_mul_pd(r8, r5)),
          _mm_mul_pd(_mm_mul_pd(r5, r9), r10)));
  r1 = _mm_loadu_pd(&dP[6]);
  r2 = _mm_loadu_pd(&b_ddu[6]);
  r4 = _mm_loadu_pd(&c_u[6]);
  r5 = _mm_loadu_pd(&P[6]);
  _mm_storeu_pd(
      &dP[6],
      _mm_add_pd(
          _mm_sub_pd(
              _mm_add_pd(_mm_mul_pd(_mm_mul_pd(r6, r1), r7),
                         _mm_div_pd(_mm_mul_pd(_mm_add_pd(r2, r4), r3), r)),
              _mm_mul_pd(r8, r5)),
          _mm_mul_pd(_mm_mul_pd(r5, r9), r10)));
  dP[8] = ((-2.0 * dP[8] * b + -(b_ddu[8] + c_u[8]) / s) - e_a * P[8]) +
          P[8] * 2.0 * b_b;
  d = dddv[0];
  d1 = dddv[1];
  d2 = dddv[2];
  d3 = ddv[0];
  d4 = ddv[1];
  d5 = ddv[2];
  r = _mm_loadu_pd(&b_P[0]);
  r = _mm_mul_pd(_mm_set1_pd(a_tmp), r);
  r1 = _mm_loadu_pd(&d3u_tmp[0]);
  r1 = _mm_mul_pd(_mm_set1_pd(d_a), r1);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d));
  r2 = _mm_loadu_pd(&d3u_tmp[3]);
  r2 = _mm_mul_pd(_mm_set1_pd(d_a), r2);
  r2 = _mm_mul_pd(r2, _mm_set1_pd(d1));
  r1 = _mm_add_pd(r1, r2);
  r2 = _mm_loadu_pd(&d3u_tmp[6]);
  r2 = _mm_mul_pd(_mm_set1_pd(d_a), r2);
  r2 = _mm_mul_pd(r2, _mm_set1_pd(d2));
  r1 = _mm_add_pd(r1, r2);
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&dP[0]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d3));
  r2 = _mm_loadu_pd(&dP[3]);
  r2 = _mm_mul_pd(r2, _mm_set1_pd(d4));
  r1 = _mm_add_pd(r1, r2);
  r2 = _mm_loadu_pd(&dP[6]);
  r2 = _mm_mul_pd(r2, _mm_set1_pd(d5));
  r1 = _mm_add_pd(r1, r2);
  _mm_storeu_pd(&b_P[0], r1);
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&du[0]);
  r2 = _mm_mul_pd(_mm_set1_pd(a), r1);
  r = _mm_sub_pd(r, r2);
  r2 = _mm_loadu_pd(&ddu[0]);
  r3 = _mm_mul_pd(_mm_set1_pd(f_a), r2);
  r = _mm_sub_pd(r, r3);
  r3 = _mm_loadu_pd(&d3u[0]);
  r3 = _mm_mul_pd(_mm_set1_pd(c_a_tmp), r3);
  r = _mm_sub_pd(r, r3);
  r2 = _mm_mul_pd(r2, _mm_set1_pd(b_tmp));
  r = _mm_sub_pd(r, r2);
  r1 = _mm_mul_pd(_mm_set1_pd(b_a), r1);
  r = _mm_sub_pd(r, r1);
  r1 = _mm_loadu_pd(&u[0]);
  r1 = _mm_mul_pd(_mm_set1_pd(2.0), r1);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(absxk));
  r = _mm_sub_pd(r, r1);
  _mm_storeu_pd(&d4u[0], r);
  d6 = a_tmp * b_P[2] +
       ((d_a * d3u_tmp[2] * d + d_a * d3u_tmp[5] * d1) + d_a * d3u_tmp[8] * d2);
  d7 = (dP[2] * d3 + dP[5] * d4) + dP[8] * d5;
  d8 = du[2];
  d9 = ddu[2];
  d6 = ((((((d6 + d7) - a * d8) - f_a * d9) - c_a_tmp * d3u[2]) - d9 * b_tmp) -
        b_a * d8) -
       2.0 * u[2] * absxk;
  d4u[2] = d6;
}

void diff_flatness_mission_QSF(double t, double mp, double mq, double l,
                               double g, double psi, const double L[3],
                               double T, double A, double B, double Od[3],
                               double dOd[3], double xipd[3], double dxipd[3],
                               double d2xipd[3], double d3xipd[3],
                               double d4xipd[3], double ddRL[3],
                               double *Td_scaler)
{
  __m128d r;
  __m128d r1;
  __m128d r2;
  double b_dRb[9];
  double d2Rb[9];
  double dRb[9];
  double y_tmp[9];
  double Fd[3];
  double b3c[3];
  double b_T[3];
  double b_d2T[3];
  double b_dT[3];
  double b_mp[3];
  double c_T[3];
  double c_mp[3];
  double d2T[3];
  double d3q[3];
  double d4q[3];
  double dT[3];
  double dc[3];
  double ddc[3];
  double b1c_idx_2;
  double b1c_tmp;
  double b_t;
  double d2xpd;
  double d2ypd;
  double d3xpd;
  double d3ypd;
  double d4xpd;
  double d4ypd;
  double d5xpd;
  double d5ypd;
  double d6xpd;
  double d6ypd;
  double dxpd;
  double dypd;
  double scale;
  double xpd;
  double ypd;
  double zpd;
  xpd = 0.0;
  ypd = 0.0;
  zpd = -1.0;
  //  default
  dxpd = 0.0;
  d2xpd = 0.0;
  d3xpd = 0.0;
  d4xpd = 0.0;
  d5xpd = 0.0;
  d6xpd = 0.0;
  dypd = 0.0;
  d2ypd = 0.0;
  d3ypd = 0.0;
  d4ypd = 0.0;
  d5ypd = 0.0;
  d6ypd = 0.0;
  if (!(t <= 3.0)) {
    if ((t > 3.0) && (t <= 18.0)) {
      //  setpoint 1
      ypd = 1.0;
    } else if ((t > 18.0) && (t <= 33.0)) {
      //  setpoint 2
      xpd = 1.0;
    } else if ((t > 33.0) && (t <= 48.0)) {
      //  setpoint 3
      xpd = 0.5;
      ypd = -0.5;
      zpd = -1.5;
    } else if (((!(t > 48.0)) || (!(t <= 63.0))) && (t > 63.0) &&
               (t <= 105.0)) {
      //  fig8
      d6ypd = 6.2831853071795862 * (t - 63.0) / T;
      d5ypd = std::sin(d6ypd);
      xpd = A * d5ypd;
      scale = 12.566370614359172 * (t - 63.0) / T;
      b_t = std::sin(scale);
      ypd = B * b_t;
      d6ypd = std::cos(d6ypd);
      d2ypd = A * d6ypd;
      dxpd = d2ypd * (6.2831853071795862 / T);
      d3ypd = 6.2831853071795862 / T;
      d5ypd *= -A;
      d2xpd = d5ypd * (d3ypd * d3ypd);
      d3xpd = -A * d6ypd * rt_powd_snf(6.2831853071795862 / T, 3.0);
      d4xpd = xpd * rt_powd_snf(6.2831853071795862 / T, 4.0);
      d5xpd = d2ypd * rt_powd_snf(6.2831853071795862 / T, 5.0);
      d6xpd = d5ypd * rt_powd_snf(6.2831853071795862 / T, 6.0);
      d6ypd = std::cos(scale);
      d5ypd = B * d6ypd;
      dypd = d5ypd * (12.566370614359172 / T);
      d3ypd = 12.566370614359172 / T;
      scale = -B * b_t;
      d2ypd = scale * (d3ypd * d3ypd);
      d3ypd = -B * d6ypd * rt_powd_snf(12.566370614359172 / T, 3.0);
      d4ypd = ypd * rt_powd_snf(12.566370614359172 / T, 4.0);
      d5ypd *= rt_powd_snf(12.566370614359172 / T, 5.0);
      d6ypd = scale * rt_powd_snf(12.566370614359172 / T, 6.0);
    } else {
      //  setpoint 0
    }
  } else {
    //  setpoint 0
  }
  //  1) desired tension force vector
  d2xipd[0] = d2xpd;
  d2xipd[1] = d2ypd;
  d2xipd[2] = 0.0;
  d3xipd[0] = d3xpd;
  d3xipd[1] = d3ypd;
  d3xipd[2] = 0.0;
  d4xipd[0] = d4xpd;
  d4xipd[1] = d4ypd;
  d4xipd[2] = 0.0;
  r = _mm_loadu_pd(&d2xipd[0]);
  r1 = _mm_set1_pd(mp);
  _mm_storeu_pd(&dc[0], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&d3xipd[0]);
  _mm_storeu_pd(&dT[0], _mm_mul_pd(r1, r));
  r = _mm_loadu_pd(&d4xipd[0]);
  _mm_storeu_pd(&d2T[0], _mm_mul_pd(r1, r));
  dT[2] = mp * 0.0;
  d2T[2] = mp * 0.0;
  b_T[0] = dc[0];
  b_T[1] = dc[1];
  b_T[2] = mp * 0.0 - mp * g;
  //  2) get q
  //  q = -T/norm(T); % q := p3
  r = _mm_loadu_pd(&b_T[0]);
  r1 = _mm_set1_pd(-1.0);
  _mm_storeu_pd(&c_T[0], _mm_mul_pd(r, r1));
  r = _mm_loadu_pd(&dT[0]);
  _mm_storeu_pd(&b_dT[0], _mm_mul_pd(r, r1));
  r = _mm_loadu_pd(&d2T[0]);
  _mm_storeu_pd(&b_d2T[0], _mm_mul_pd(r, r1));
  c_T[2] = -b_T[2];
  b_dT[2] = -dT[2];
  b_d2T[2] = -d2T[2];
  b_mp[0] = -(mp * d5xpd);
  b_mp[1] = -(mp * d5ypd);
  b_mp[2] = -(mp * 0.0);
  c_mp[0] = -(mp * d6xpd);
  c_mp[1] = -(mp * d6ypd);
  c_mp[2] = -(mp * 0.0);
  unitvec_dt(c_T, b_dT, b_d2T, b_mp, c_mp, dc, ddc, d3q, d4q);
  //  since q = -T/norm(T) := p3
  //  3) find suspension point acc. (p = pl - q*l)
  r = _mm_loadu_pd(&ddc[0]);
  r2 = _mm_set1_pd(l);
  _mm_storeu_pd(&ddc[0], _mm_mul_pd(r, r2));
  r = _mm_loadu_pd(&d3q[0]);
  _mm_storeu_pd(&d3q[0], _mm_mul_pd(r, r2));
  r = _mm_loadu_pd(&d4q[0]);
  _mm_storeu_pd(&d4q[0], _mm_mul_pd(r, r2));
  ddc[2] *= l;
  d3q[2] *= l;
  d4q[2] *= l;
  //  4) find Rbi (remaining are the same)
  Fd[0] = b_T[0] + mq * (d2xpd - ddc[0]);
  Fd[1] = b_T[1] + mq * (d2ypd - ddc[1]);
  Fd[2] = (b_T[2] + mq * (0.0 - ddc[2])) - mq * g;
  d5ypd = std::atan((Fd[0] * std::cos(psi) + Fd[1] * std::sin(psi)) / Fd[2]);
  scale = 3.3121686421112381E-170;
  d2ypd = std::abs(Fd[0]);
  if (d2ypd > 3.3121686421112381E-170) {
    d6ypd = 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / 3.3121686421112381E-170;
    d6ypd = b_t * b_t;
  }
  d2ypd = std::abs(Fd[1]);
  if (d2ypd > scale) {
    b_t = scale / d2ypd;
    d6ypd = d6ypd * b_t * b_t + 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / scale;
    d6ypd += b_t * b_t;
  }
  d2ypd = std::abs(Fd[2]);
  if (d2ypd > scale) {
    b_t = scale / d2ypd;
    d6ypd = d6ypd * b_t * b_t + 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / scale;
    d6ypd += b_t * b_t;
  }
  d6ypd = scale * std::sqrt(d6ypd);
  r = _mm_loadu_pd(&Fd[0]);
  _mm_storeu_pd(&b3c[0], _mm_div_pd(_mm_mul_pd(r, r1), _mm_set1_pd(d6ypd)));
  b3c[2] = -Fd[2] / d6ypd;
  d6xpd = std::cos(d5ypd);
  d6ypd = -std::sin(d5ypd);
  //  directly by reference
  d5ypd = b3c[1] * d6ypd - 0.0 * b3c[2];
  d5xpd = d6xpd * b3c[2] - b3c[0] * d6ypd;
  d2xpd = b3c[0] * 0.0 - d6xpd * b3c[1];
  scale = 3.3121686421112381E-170;
  d2ypd = std::abs(d5ypd);
  if (d2ypd > 3.3121686421112381E-170) {
    b1c_tmp = 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / 3.3121686421112381E-170;
    b1c_tmp = b_t * b_t;
  }
  d2ypd = std::abs(d5xpd);
  if (d2ypd > scale) {
    b_t = scale / d2ypd;
    b1c_tmp = b1c_tmp * b_t * b_t + 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / scale;
    b1c_tmp += b_t * b_t;
  }
  d2ypd = std::abs(d2xpd);
  if (d2ypd > scale) {
    b_t = scale / d2ypd;
    b1c_tmp = b1c_tmp * b_t * b_t + 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / scale;
    b1c_tmp += b_t * b_t;
  }
  b1c_tmp = scale * std::sqrt(b1c_tmp);
  scale = -(b3c[1] * d2xpd - d5xpd * b3c[2]) / b1c_tmp;
  b_t = -(d5ypd * b3c[2] - b3c[0] * d2xpd) / b1c_tmp;
  b1c_idx_2 = -(b3c[0] * d5xpd - d5ypd * b3c[1]) / b1c_tmp;
  b_dT[0] = -(dT[0] + mq * (d3xpd - d3q[0]));
  b_dT[1] = -(dT[1] + mq * (d3ypd - d3q[1]));
  b_dT[2] = -(dT[2] + mq * (0.0 - d3q[2]));
  b_d2T[0] = -(d2T[0] + mq * (d4xpd - d4q[0]));
  b_d2T[1] = -(d2T[1] + mq * (d4ypd - d4q[1]));
  b_d2T[2] = -(d2T[2] + mq * (0.0 - d4q[2]));
  r = _mm_loadu_pd(&Fd[0]);
  _mm_storeu_pd(&b_mp[0], _mm_mul_pd(r, r1));
  r = _mm_set1_pd(0.0);
  _mm_storeu_pd(&c_mp[0], r);
  _mm_storeu_pd(&c_T[0], r);
  b_mp[2] = -Fd[2];
  c_mp[2] = 0.0;
  c_T[2] = 0.0;
  unitvec_dt(b_mp, b_dT, b_d2T, c_mp, c_T, d2T, d3q, dc, ddc);
  //  since b3 = -F/norm(F)
  //  1) assume derivatives of b1d = 0
  //  get 1-4th derivative of b1
  dc[0] = d2T[1] * d6ypd - 0.0 * d2T[2];
  dc[1] = d6xpd * d2T[2] - d2T[0] * d6ypd;
  dc[2] = d2T[0] * 0.0 - d6xpd * d2T[1];
  ddc[0] = d3q[1] * d6ypd - 0.0 * d3q[2];
  ddc[1] = d6xpd * d3q[2] - d3q[0] * d6ypd;
  ddc[2] = d3q[0] * 0.0 - d6xpd * d3q[1];
  d2ypd = ((d5ypd * dc[0] + d5xpd * dc[1]) + d2xpd * dc[2]) / b1c_tmp;
  d6ypd = d2ypd / b1c_tmp;
  dT[0] =
      (-(d2T[1] * d2xpd - d5xpd * d2T[2]) - (b3c[1] * dc[2] - dc[1] * b3c[2])) /
          b1c_tmp -
      d6ypd * scale;
  dT[1] =
      (-(d5ypd * d2T[2] - d2T[0] * d2xpd) - (dc[0] * b3c[2] - b3c[0] * dc[2])) /
          b1c_tmp -
      d6ypd * b_t;
  dT[2] =
      (-(d2T[0] * d5xpd - d5ypd * d2T[1]) - (b3c[0] * dc[1] - dc[0] * b3c[1])) /
          b1c_tmp -
      d6ypd * b1c_idx_2;
  d3ypd = 2.0 * d6ypd;
  d6ypd = ((((dc[0] * dc[0] + dc[1] * dc[1]) + dc[2] * dc[2]) +
            ((d5ypd * ddc[0] + d5xpd * ddc[1]) + d2xpd * ddc[2])) -
           d2ypd * d2ypd) /
          b1c_tmp / b1c_tmp;
  Fd[0] = (((-(d3q[1] * d2xpd - d5xpd * d3q[2]) -
             2.0 * (d2T[1] * dc[2] - dc[1] * d2T[2])) -
            (b3c[1] * ddc[2] - ddc[1] * b3c[2])) /
               b1c_tmp -
           d3ypd * dT[0]) -
          d6ypd * scale;
  Fd[1] = (((-(d5ypd * d3q[2] - d3q[0] * d2xpd) -
             2.0 * (dc[0] * d2T[2] - d2T[0] * dc[2])) -
            (ddc[0] * b3c[2] - b3c[0] * ddc[2])) /
               b1c_tmp -
           d3ypd * dT[1]) -
          d6ypd * b_t;
  Fd[2] = (((-(d3q[0] * d5xpd - d5ypd * d3q[1]) -
             2.0 * (d2T[0] * dc[1] - dc[0] * d2T[1])) -
            (b3c[0] * ddc[1] - ddc[0] * b3c[1])) /
               b1c_tmp -
           d3ypd * dT[2]) -
          d6ypd * b1c_idx_2;
  //  2) derivatives of b1d != 0
  //  dFd = dT + mq*d3xi;
  //  d2Fd = d2T + mq*d4xi;
  //  N = Fd(1)*cos(psi) + Fd(2)*sin(psi); % partition
  //  D = Fd(3);
  //  Nd  = dFd(1)*cos(psi) + dFd(2)*sin(psi) + dpsi*(-Fd(1)*sin(psi) +
  //  Fd(2)*cos(psi)); Dd  = dFd(3); Ndd = d2Fd(1)*cos(psi) + d2Fd(2)*sin(psi)
  //  ...
  //      + ddpsi*(-Fd(1)*sin(psi) + Fd(2)*cos(psi)) ...
  //      + (dpsi^2)*(-Fd(1)*cos(psi) - Fd(2)*sin(psi)) ...
  //      + 2*dpsi*(-dFd(1)*sin(psi) + dFd(2)*cos(psi));
  //  Ddd = d2Fd(3);
  //  A  = Nd*D - N*Dd;
  //  B  = D^2 + N^2;
  //  dthetad = A/B;
  //  ddthetad = ((D*Ndd - N*Ddd)*B - A*2*(D*Dd + N*Nd)) / (B^2);
  //  db1d = [-sin(thetad)*dthetad; 0; -cos(thetad)*dthetad];
  //  ddb1d = [-cos(thetad)*thetad^2 + -sin(thetad)*ddthetad; 0;
  //  sin(thetad)*thetad^2 - cos(thetad)*ddthetad]; c = cross(b3c,b1d); s =
  //  norm(c); dc = cross(db3,b1d) + cross(b3c,db1d); dw = -cross(db3,c) -
  //  cross(b3c,dc); ds = c'*dc/s; db1 = dw/s - (ds/s)*b1c; ddc =
  //  cross(ddb3,b1d) + 2*cross(db3,db1d) + 0*cross(b3c,ddb1d); % change coeff
  //  to remove ddb1d ddw = -cross(ddb3,c) - 2*cross(db3,dc) - cross(b3c,ddc);
  //  dds = (dc'*dc + c'*ddc - ds^2)/s;
  //  ddb1 = ddw/s - 2*(ds/s)*db1 - (dds/s)*b1c;
  //  db2 = cross(db3,b1c) + cross(b3c,db1);
  //  d2b2 = cross(ddb3,b1c) + cross(b3c,ddb1) + 2*cross(db3,db1);
  //  desired Omega
  y_tmp[1] = b3c[1] * b1c_idx_2 - b_t * b3c[2];
  y_tmp[4] = scale * b3c[2] - b3c[0] * b1c_idx_2;
  y_tmp[7] = b3c[0] * b_t - scale * b3c[1];
  dRb[0] = dT[0];
  dRb[3] =
      (d2T[1] * b1c_idx_2 - b_t * d2T[2]) + (b3c[1] * dT[2] - dT[1] * b3c[2]);
  dRb[6] = d2T[0];
  y_tmp[0] = scale;
  y_tmp[2] = b3c[0];
  dRb[1] = dT[1];
  dRb[4] =
      (scale * d2T[2] - d2T[0] * b1c_idx_2) + (dT[0] * b3c[2] - b3c[0] * dT[2]);
  dRb[7] = d2T[1];
  y_tmp[3] = b_t;
  y_tmp[5] = b3c[1];
  dRb[2] = dT[2];
  dRb[5] = (d2T[0] * b_t - scale * d2T[1]) + (b3c[0] * dT[1] - dT[0] * b3c[1]);
  dRb[8] = d2T[2];
  y_tmp[6] = b1c_idx_2;
  y_tmp[8] = b3c[2];
  for (int i{0}; i < 3; i++) {
    d6ypd = y_tmp[i];
    d5ypd = y_tmp[i + 3];
    d2ypd = y_tmp[i + 6];
    for (int i1{0}; i1 < 3; i1++) {
      d2Rb[i + 3 * i1] = (d6ypd * dRb[3 * i1] + d5ypd * dRb[3 * i1 + 1]) +
                         d2ypd * dRb[3 * i1 + 2];
    }
  }
  Od[0] = d2Rb[5];
  Od[1] = d2Rb[6];
  Od[2] = d2Rb[1];
  c_T[0] = d3q[1] * b1c_idx_2 - b_t * d3q[2];
  c_T[1] = scale * d3q[2] - d3q[0] * b1c_idx_2;
  c_T[2] = d3q[0] * b_t - scale * d3q[1];
  b_mp[0] = b3c[1] * Fd[2] - Fd[1] * b3c[2];
  b_mp[1] = Fd[0] * b3c[2] - b3c[0] * Fd[2];
  b_mp[2] = b3c[0] * Fd[1] - Fd[0] * b3c[1];
  c_mp[0] = 2.0 * (d2T[1] * dT[2] - dT[1] * d2T[2]);
  c_mp[1] = 2.0 * (dT[0] * d2T[2] - d2T[0] * dT[2]);
  c_mp[2] = 2.0 * (d2T[0] * dT[1] - dT[0] * d2T[1]);
  for (int i{0}; i < 3; i++) {
    d2Rb[i] = Fd[i];
    d2Rb[i + 3] = (c_T[i] + b_mp[i]) + c_mp[i];
    d2Rb[i + 6] = d3q[i];
    for (int i1{0}; i1 < 3; i1++) {
      b_dRb[i + 3 * i1] =
          (dRb[3 * i] * dRb[3 * i1] + dRb[3 * i + 1] * dRb[3 * i1 + 1]) +
          dRb[3 * i + 2] * dRb[3 * i1 + 2];
    }
  }
  for (int i{0}; i < 3; i++) {
    d6ypd = y_tmp[i];
    d5ypd = y_tmp[i + 3];
    d2ypd = y_tmp[i + 6];
    for (int i1{0}; i1 < 3; i1++) {
      dRb[i + 3 * i1] = (d6ypd * d2Rb[3 * i1] + d5ypd * d2Rb[3 * i1 + 1]) +
                        d2ypd * d2Rb[3 * i1 + 2];
    }
  }
  r = _mm_loadu_pd(&b_dRb[0]);
  r1 = _mm_loadu_pd(&dRb[0]);
  _mm_storeu_pd(&b_dRb[0], _mm_add_pd(r, r1));
  r = _mm_loadu_pd(&b_dRb[2]);
  r1 = _mm_loadu_pd(&dRb[2]);
  _mm_storeu_pd(&b_dRb[2], _mm_add_pd(r, r1));
  r = _mm_loadu_pd(&b_dRb[4]);
  r1 = _mm_loadu_pd(&dRb[4]);
  _mm_storeu_pd(&b_dRb[4], _mm_add_pd(r, r1));
  r = _mm_loadu_pd(&b_dRb[6]);
  r1 = _mm_loadu_pd(&dRb[6]);
  _mm_storeu_pd(&b_dRb[6], _mm_add_pd(r, r1));
  dOd[0] = b_dRb[5];
  dOd[1] = b_dRb[6];
  dOd[2] = b_dRb[1];
  //  output for QSF
  //  ddR_term = mq*d2Rb*L; % alternative way but feed d2Rb term only,
  //  originonal
  scale = 3.3121686421112381E-170;
  d2ypd = std::abs(b_T[0]);
  if (d2ypd > 3.3121686421112381E-170) {
    d6ypd = 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / 3.3121686421112381E-170;
    d6ypd = b_t * b_t;
  }
  d2ypd = std::abs(b_T[1]);
  if (d2ypd > scale) {
    b_t = scale / d2ypd;
    d6ypd = d6ypd * b_t * b_t + 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / scale;
    d6ypd += b_t * b_t;
  }
  d2ypd = std::abs(b_T[2]);
  if (d2ypd > scale) {
    b_t = scale / d2ypd;
    d6ypd = d6ypd * b_t * b_t + 1.0;
    scale = d2ypd;
  } else {
    b_t = d2ypd / scale;
    d6ypd += b_t * b_t;
  }
  d6ypd = scale * std::sqrt(d6ypd);
  *Td_scaler = -d6ypd;
  d6ypd = L[0];
  d5ypd = L[1];
  d2ypd = L[2];
  r = _mm_loadu_pd(&d2Rb[0]);
  r = _mm_mul_pd(r, _mm_set1_pd(d6ypd));
  r1 = _mm_loadu_pd(&d2Rb[3]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d5ypd));
  r = _mm_add_pd(r, r1);
  r1 = _mm_loadu_pd(&d2Rb[6]);
  r1 = _mm_mul_pd(r1, _mm_set1_pd(d2ypd));
  r = _mm_add_pd(r, r1);
  _mm_storeu_pd(&ddRL[0], r);
  ddRL[2] = (d2Rb[2] * d6ypd + d2Rb[5] * d5ypd) + d2Rb[8] * d2ypd;
  xipd[0] = xpd;
  xipd[1] = ypd;
  xipd[2] = zpd;
  dxipd[0] = dxpd;
  dxipd[1] = dypd;
  dxipd[2] = 0.0;
}

// End of code generation (diff_flatness_mission_QSF.cpp)
