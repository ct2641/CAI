// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef UTILITIES_PD
#define UTILITIES_PD

#include "PDParser/PDparser.h"


typedef struct
{
	int num_effetive;
	int* forward_mappingtable;
	int* inverse_mappingtable;

	int num_pairs;
	int** pairs;

} extracted_reductions;

int do_permutation(int number, char* perm, int numRV);
int expand(int number, problem_description* PD);
int reduce(int number, problem_description* PD);
void initializeER(extracted_reductions* ER);
void freeER(extracted_reductions* ER);
void make_mappingtable(problem_description* PD, extracted_reductions* ER);
void makepairs(problem_description* PD, extracted_reductions* ER);
int map_compress(int* positions, double* values, int num_initial, extracted_reductions* ER);

#endif
