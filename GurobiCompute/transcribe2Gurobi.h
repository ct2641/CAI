// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef MY_LP_H
#define MY_LP_H
#include "../PDParser/PDparser.h"
#include "../ERfromPD.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "utilities_Gurobi.h"
#include "transcribe2Gurobi.h"
#include "../ERfromPD.h"
#include "../PDParser/utilities.h"
#include "gurobi_c.h"

// the elementary inequalities, however, this is done after symmetry and dependence reduction to improve the efficiency

int populate_Gurobirows_generic(GRBmodel* lp, problem_description* PD, extracted_reductions* ER);
int add_Gurobirows_problemspecific(GRBmodel* lp, problem_description* PD, extracted_reductions* ER);
int populate_Gurobirows_random(GRBmodel* lp, problem_description* PD, extracted_reductions* ER, int num_constraints);
void print_Gurobiinequalities(GRBmodel* primallp, GRBmodel* duallp, problem_description* PD, extracted_reductions* ER);
#endif
