// An Open-Source Toolbox for Computer-Aided Investigation
// on the Fundamental Limits of Information Systems, Version 0.1
//
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
//
#ifndef UTILITIES_PD
#define UTILITIES_PD

#include <vector>
#include "problem_description.hpp"
#include "disjoint_set.hpp"


class Extracted_Reductions
{
  public:

        /* Need a constructor to set num_effective and num_pairs.
           Otherwise, we don't need destructor, copy or move construtors, because
           the defaults will work just fine. */

	Extracted_Reductions();

        void Make_Mappingtable(const CAI::Problem_Description_C_Style* PD);
        void Make_Pairs(const CAI::Problem_Description_C_Style* PD);
        int Map_Compress(int* positions, double* values, int num_initial) const;

	int num_effective;
	std::vector <int> forward_mappingtable;
	std::vector <int> inverse_mappingtable;
	int num_pairs;
	std::vector < std::vector <int> > pairs;
        Disjoint_Set DS;                           // The disjoint sets of equivalent subsets.

};

int do_permutation(int number, const char* perm, int numRV);
int reduce(int number, const CAI::Problem_Description_C_Style* PD);

#endif
