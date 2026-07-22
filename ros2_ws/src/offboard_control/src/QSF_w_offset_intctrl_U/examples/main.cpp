//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// main.cpp
//
// Code generation for function 'main'
//

/*************************************************************************/
/* This automatically generated example C++ main file shows how to call  */
/* entry-point functions that MATLAB Coder generated. You must customize */
/* this file for your application. Do not modify this file directly.     */
/* Instead, make a copy of this file, modify it, and integrate it into   */
/* your development environment.                                         */
/*                                                                       */
/* This file initializes entry-point function arguments to a default     */
/* size and value before calling the entry-point functions. It does      */
/* not store or use any values returned from the entry-point functions.  */
/* If necessary, it does pre-allocate memory for returned values.        */
/* You can use this file as a starting point for a main function that    */
/* you can deploy in your application.                                   */
/*                                                                       */
/* After you copy the file, and before you deploy it, you must make the  */
/* following changes:                                                    */
/* * For variable-size function arguments, change the example sizes to   */
/* the sizes that your application requires.                             */
/* * Change the example values of function arguments to the values that  */
/* your application requires.                                            */
/* * If the entry-point functions return values, store these values or   */
/* otherwise use them as required by your application.                   */
/*                                                                       */
/*************************************************************************/

// Include files
#include "main.h"
#include "QSF_w_offset_intctrl_U.h"
#include "QSF_w_offset_intctrl_U_terminate.h"
#include "rt_nonfinite.h"

// Function Declarations
static void argInit_12x1_real_T(double result[12]);

static void argInit_15x1_real_T(double result[15]);

static void argInit_1x3_real_T(double result[3]);

static void argInit_1x5_real_T(double result[5]);

static double argInit_real_T();

// Function Definitions
static void argInit_12x1_real_T(double result[12])
{
  // Loop over the array to initialize each element.
  for (int idx0{0}; idx0 < 12; idx0++) {
    // Set the value of the array element.
    // Change this value to the value that the application requires.
    result[idx0] = argInit_real_T();
  }
}

static void argInit_15x1_real_T(double result[15])
{
  // Loop over the array to initialize each element.
  for (int idx0{0}; idx0 < 15; idx0++) {
    // Set the value of the array element.
    // Change this value to the value that the application requires.
    result[idx0] = argInit_real_T();
  }
}

static void argInit_1x3_real_T(double result[3])
{
  // Loop over the array to initialize each element.
  for (int idx1{0}; idx1 < 3; idx1++) {
    // Set the value of the array element.
    // Change this value to the value that the application requires.
    result[idx1] = argInit_real_T();
  }
}

static void argInit_1x5_real_T(double result[5])
{
  // Loop over the array to initialize each element.
  for (int idx1{0}; idx1 < 5; idx1++) {
    // Set the value of the array element.
    // Change this value to the value that the application requires.
    result[idx1] = argInit_real_T();
  }
}

static double argInit_real_T()
{
  return 0.0;
}

int main(int, char **)
{
  // The initialize function is being called automatically from your entry-point
  // function. So, a call to initialize is not included here. Invoke the
  // entry-point functions.
  // You can call entry-point functions multiple times.
  main_QSF_w_offset_intctrl_U();
  // Terminate the application.
  // You do not need to do this more than one time.
  QSF_w_offset_intctrl_U_terminate();
  return 0;
}

void main_QSF_w_offset_intctrl_U()
{
  double dv1[15];
  double dv2[12];
  double Rbd[9];
  double K1_tmp[5];
  double dv[3];
  double dv3[3];
  double Fld_scaler;
  double mq_tmp;
  double phid;
  double thetad;
  // Initialize function 'QSF_w_offset_intctrl_U' input arguments.
  mq_tmp = argInit_real_T();
  // Initialize function input argument 'K1'.
  argInit_1x5_real_T(K1_tmp);
  // Initialize function input argument 'K2'.
  // Initialize function input argument 'K3'.
  // Initialize function input argument 'ref_traj'.
  // Initialize function input argument 'states'.
  // Initialize function input argument 'Int'.
  // Call the entry-point 'QSF_w_offset_intctrl_U'.
  argInit_1x3_real_T(dv);
  argInit_15x1_real_T(dv1);
  argInit_12x1_real_T(dv2);
  argInit_1x3_real_T(dv3);
  QSF_w_offset_intctrl_U(mq_tmp, mq_tmp, mq_tmp, K1_tmp, K1_tmp, dv, dv1, dv2,
                         mq_tmp, mq_tmp, dv3, Rbd, &Fld_scaler, &phid, &thetad);
}

// End of code generation (main.cpp)
