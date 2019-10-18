// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "PDParser/PDparser.h"
#include "ERfromPD.h"
#include "PDParser/utilities.h"

// this will only do permutation
// return the permuted result if is results in a valid subset, otherwise return 1<<num_rvs
int do_permutation(int number, char* perm, int numRV)
{
	// take the permutation and give the permuted index for input number
	int newnumber = 0;
	for (int j = 0; j < numRV; j++) {
		//need to be more careful here since some permutation has non-mapped slot: check if perm[j]>=0
		if (((number >> j) & 1) == 1) {
			if (perm[j] >= 0)
				newnumber = newnumber | (1 << perm[j]);
			else { // if not, this permutation leads to a subset of random variables not in the valid mask (restricted set)
				newnumber = -1;
				break;
			}
		}
	}
	return(newnumber);
}

// this will expand the given subset based on dependence relation and permutation, and find the unique min-valued permutation
// note: perhaps not the most efficient way to do so
// this should always be a valid expanded value
int expand(int number, problem_description* PD)
{
	int oldnumber;
	// first: expand according to the dependency relation
	do {
		oldnumber = number;
		for (int i = 0; i < PD->num_dependency; i++) {
			if ((PD->list_dependency[i * 2 + 1] & number) == PD->list_dependency[i * 2 + 1])
				number = (number | PD->list_dependency[i * 2]);
		}
	} while (oldnumber != number);

	// second do permutation:
	int tempA = number;
	int tempB;
	for (int i = 1; i < PD->num_symmetrymap; i++) {
		tempB = do_permutation(number, PD->list_symmetry[i], PD->num_rvs);
		if (tempB < tempA &&tempB>0) // Note: if it is a valid subset, then it must be less than 1<<num_rvs, so won't be replaced
			tempA = tempB;
	}
	return(tempA);
}

int reduce(int number, problem_description* PD)
{
	while (1) {
		int oldnumber = number;
		for (int i = PD->num_dependency - 1; i >= 0; i--)
		{
			if ((PD->list_dependency[i * 2 + 1] & number) == PD->list_dependency[i * 2 + 1])
				number = (number&(~PD->list_dependency[i * 2])) | PD->list_dependency[i * 2 + 1];
		}
		if (oldnumber == number)
			return(number);
	}
}

void initializeER(extracted_reductions* ER)
{
	ER->num_effetive = 0;
	ER->num_pairs = 0;
	ER->forward_mappingtable = NULL;
	ER->inverse_mappingtable = NULL;
	ER->pairs = NULL;
}

void freeER(extracted_reductions* ER)
{
	if (ER->num_effetive > 0) {
		free(ER->forward_mappingtable);
		free(ER->inverse_mappingtable);
	}
	if (ER->num_pairs > 0) {
		free(ER->pairs[0]);
		free(ER->pairs[1]);
		free(ER->pairs);
	}

	ER->num_effetive = 0;
	ER->num_pairs = 0;
	ER->forward_mappingtable = NULL;
	ER->inverse_mappingtable = NULL;
	ER->pairs = NULL;
}


// this function will make the mapping table: note that the memory will be allocated here, and should be released after use.
// two arrays are produced: one is the mappingtable for each subset, the other is the reverse mapping table
// the first one is used in generating the LP< but the reverse mapping table is useful when interpreting the result
void make_mappingtable(problem_description* PD, extracted_reductions* ER)
{
	int length = (1 << PD->num_rvs);
	int* mappingtable = (int*)calloc(length, sizeof(int));
	int* mT = mappingtable;
	printf("Total number of elements to reduce: %d\n", length);
	int count = 0;
	int min_nonset = 1;
	int num_effective = 0;

	while (min_nonset < length) {
		count++;
		//if(((count>>15)<<15)==count) // print a message every (1<<8) iterations
		//	printf("Current minimum noneset value = %8d, current processed total = %8d.\n", min_nonset, count);

		int expanded_value = expand(min_nonset, PD);
		mT[min_nonset] = expanded_value;
		for (int i = 1; i < PD->num_symmetrymap; i++) {
			int next = do_permutation(min_nonset, PD->list_symmetry[i], PD->num_rvs);
			if (next < 0) // if it goes out of range, skip this one
				continue;
			if (mT[next] == 0) {
				mT[next] = expanded_value;
				count++;
			}
		}

		while (min_nonset < length)
		{
			if (mT[min_nonset] > 0)
				min_nonset++;
			else
				break;
		}
	}

	int* new_mappingtable = (int*)calloc(length + PD->num_add_LPvars, sizeof(int));
	new_mappingtable[0] = 0;

	for (int i = 1; i < length; i++) {
		if (mT[i] == i) {
			num_effective++;
			new_mappingtable[i] = num_effective;
		}
	}
	ER->inverse_mappingtable = (int*)calloc(num_effective + 1 + PD->num_add_LPvars, sizeof(int));

	for (int i = 1; i < length; i++) {
		if (new_mappingtable[i] != 0)
			ER->inverse_mappingtable[new_mappingtable[i]] = i;
		new_mappingtable[i] = new_mappingtable[mT[i]];
	}

	for (int i = 0; i < PD->num_add_LPvars; i++) {
		num_effective++;
		new_mappingtable[i + length] = num_effective;
		ER->inverse_mappingtable[num_effective] = i + length;
	}

	free(mappingtable);
	ER->forward_mappingtable = new_mappingtable;
	ER->num_effetive = num_effective;
}

void makepairs(problem_description* PD, extracted_reductions* ER)
{
	char* ij_matrix = (char*)calloc(PD->num_rvs*PD->num_rvs, sizeof(char));
	int count = 0;

	for (int i = PD->num_rvs - 1; i >= 1; i--) {
		for (int j = i - 1; j >= 0; j--) {

			if (ij_matrix[i*PD->num_rvs + j] == 0) {
				ij_matrix[i*PD->num_rvs + j] = 1; //1 = the leader, 0=new item, 2=non-leader
				count++;
				//int number = (1<<i)|(1<<j);
				for (int k = 1; k < PD->num_symmetrymap; k++) {
					char I = PD->list_symmetry[k][i];
					char J = PD->list_symmetry[k][j];
					if (I >= 0 && J >= 0 && ij_matrix[I*PD->num_rvs + J] == 0)
						ij_matrix[I*PD->num_rvs + J] = 2;
				}
			}
		}
	}
	ER->pairs = (int**)malloc(sizeof(int*) * 2);
	ER->pairs[0] = (int*)malloc(sizeof(int)*count);
	ER->pairs[1] = (int*)malloc(sizeof(int)*count);

	count = 0;
	for (int i = PD->num_rvs - 1; i >= 1; i--) {
		for (int j = i - 1; j >= 0; j--) {
			if (ij_matrix[i*PD->num_rvs + j] == 1) {
				ER->pairs[0][count] = i;
				ER->pairs[1][count] = j;
				count++;
			}
		}
	}

	free(ij_matrix);
	ER->num_pairs = count;
}

int map_compress(int* positions, double* values, int num_initial, extracted_reductions* ER)
{
	for (int k = 0; k < num_initial; k++)
	{
		if (positions[k] == 0)
		{
			values[k] = 0;
			continue;
		}
		positions[k] = ER->forward_mappingtable[positions[k]];
		for (int r = k - 1; r >= 0; r--) {
			if (positions[r] == positions[k]) // repeated positions
			{
				values[r] += values[k];
				positions[k] = 0;
				values[k] = 0;
				break;
			}
		}
	}
	//processing the valid positions
	int num_effective = 0;
	int bound_upper = num_initial;
	for (int k = 0; k < bound_upper; k++) {
		if (fabs(values[k]) < TOLERANCE_LPGENERATION || positions[k] == 0) {
			for (int r = k + 1; r < num_initial; r++) {
				positions[r - 1] = positions[r];
				values[r - 1] = values[r];
			}
			k--;
			bound_upper--;
			continue;
		}
		else
			num_effective++;
	}
	return(num_effective);
}
