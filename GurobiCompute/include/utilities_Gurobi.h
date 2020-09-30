// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef GUROBI_UTILITY
#define GUROBI_UTILITY
#include "gurobi_c.h"

int set_defaultLP(GRBenv** p_env, GRBmodel** p_lp, int num_columns, char* problem_name);
int make_dual(GRBenv* env, GRBmodel* primalLP, GRBmodel** p_dualLP, GRBmodel** p_MIP);

#endif
