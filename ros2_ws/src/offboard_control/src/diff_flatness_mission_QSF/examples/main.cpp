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
#include "diff_flatness_mission_QSF.h"
#include "diff_flatness_mission_QSF_terminate.h"
#include "rt_nonfinite.h"

// Function Declarations
static void argInit_3x1_real_T(double result[3]);

static double argInit_real_T();

// Function Definitions
static void argInit_3x1_real_T(double result[3])
{
  // Loop over the array to initialize each element.
  for (int idx0{0}; idx0 < 3; idx0++) {
    // Set the value of the array element.
    // Change this value to the value that the application requires.
    result[idx0] = argInit_real_T();
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
  main_diff_flatness_mission_QSF();
  // Terminate the application.
  // You do not need to do this more than one time.
  diff_flatness_mission_QSF_terminate();
  return 0;
}

void main_diff_flatness_mission_QSF()
{
  double Od[3];
  double d2xipd[3];
  double d3xipd[3];
  double d4xipd[3];
  double dOd[3];
  double ddRL[3];
  double dv[3];
  double dxipd[3];
  double xipd[3];
  double Td_scaler;
  double t_tmp;
  // Initialize function 'diff_flatness_mission_QSF' input arguments.
  t_tmp = argInit_real_T();
  // Initialize function input argument 'L'.
  // Call the entry-point 'diff_flatness_mission_QSF'.
  argInit_3x1_real_T(dv);
  diff_flatness_mission_QSF(t_tmp, t_tmp, t_tmp, t_tmp, t_tmp, t_tmp, dv, t_tmp,
                            t_tmp, t_tmp, Od, dOd, xipd, dxipd, d2xipd, d3xipd,
                            d4xipd, ddRL, &Td_scaler);
}

// End of code generation (main.cpp)
