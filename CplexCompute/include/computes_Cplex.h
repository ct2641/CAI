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
#include "problem_description.hpp"

int regular_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC);
int hull_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC);
int random_selected_constraints_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC, int num_constraints, int num_iter);
int prove_bounds(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC);
int sensitivity_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC);

#endif
