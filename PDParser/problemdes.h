// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef PROBLEM_DESCRIPTION
#define PROBLEM_DESCRIPTION

typedef struct
{
	// for the 7 group of keywords information
	int num_rvs;
	char** list_rvs;

	// sometimes we need additional LP variables; these are kept here
	// same format as for the random variables
	// they are only allowed to show up on the right-hand side of the constant contraints
	int num_add_LPvars;
	char** list_add_LPvars;

	// how many terms in the objective: their positions and values
	int num_items_in_objective;
	unsigned int* list_pos_in_objective;
	double* list_value_in_objective;

	// each dependency always involve 2 terms: only positions
	int num_dependency;
	int* list_dependency;

	// the number of items in this type of constaints can vary
	// depending on the type, the last one or two positions can be zero
	int num_independence;
	unsigned int** list_independence;
	char* list_type_independence;
	int* list_num_items_in_independence;

	// set a conditional entropy or mutual information to be >=, <= or =
	// then a constant, another entropy or mutual information, or an additional LP bound
	int num_constantbounds;
	int* list_num_items_in_constantbounds;
	unsigned int** list_pos_constantbounds;
	double** list_value_constantbounds;
	char* list_type_constantbounds;
	double* list_rhs;

	// bounds to prove
	int num_bounds;
	int* list_num_items_in_bounds;
	unsigned int** list_pos_bounds;
	double** list_value_bounds;
	char* list_type_bounds;
	double* list_rhs_2prove;

	// each row is a permutation, ie., a sequence of numbers in [0,num_rvs-1].
	int num_symmetrymap;
	char** list_symmetry;

} problem_description;


#endif
