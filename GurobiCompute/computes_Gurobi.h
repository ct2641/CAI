// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef CPLEXCOMPUTES
#define CPLEXCOMPUTES

#define TOL_ALG 1e-6
//#include <stdio.h>
#include "../PDParser/PDparser.h"

int regular_compute(problem_description* PD);
int hull_compute(problem_description* PD);
int random_selected_constraints_compute(problem_description* PD, int num_constraints, int num_iter);
int prove_bounds(problem_description* PD);

#endif
