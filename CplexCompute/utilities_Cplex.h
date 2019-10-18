// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef CPLEX_UTILITY
#define CPLEX_UTILITY
#include <ilcplex/cplex.h>

int set_defaultLP(CPXENVptr* p_env, CPXLPptr* p_lp, int num_columns, char* problem_name, CPXINT SCRIND = CPX_ON, CPXINT DATACHECK = CPX_ON);
int freeLP(CPXENVptr env, CPXLPptr lp);
int free_Lp_env(CPXENVptr env);
int make_dual(CPXENVptr env, CPXLPptr primalLP, CPXLPptr* p_dualLP, CPXLPptr* p_MIP, char* problemName);

#endif
