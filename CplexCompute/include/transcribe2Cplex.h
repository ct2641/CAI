// An Open-Source Toolbox for Computer-Aided Investigation
// on the Fundamental Limits of Information Systems, Version 0.1
//
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
//
#ifndef MY_LP_H
#define MY_LP_H
#include <ilcplex/cplex.h>
#include "problem_description.hpp"
#include "ERfromPD.h"

int populate_Cplexrows_generic(CPXENVptr env, CPXLPptr lp, CAI::Problem_Description_C_Style* PD, Extracted_Reductions* ER);
int populate_Cplexrows_random(CPXENVptr env, CPXLPptr lp, CAI::Problem_Description_C_Style* PD, Extracted_Reductions* ER, int num_constraints);
int add_Cplexrows_problemspecific(CPXENVptr env, CPXLPptr lp, CAI::Problem_Description_C_Style* PD, Extracted_Reductions* ER);
void print_Cplexinequalities(CPXENVptr env, CPXLPptr dualLP, CAI::Problem_Description_C_Style* PD, Extracted_Reductions* ER);

#endif
