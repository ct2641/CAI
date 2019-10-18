// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
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

int populate_Gurobirows_generic(GRBmodel* lp, problem_description* PD, extracted_reductions* ER)
{
	int positions[4];
	double initial_values[4] = { 1,1,-1,-1 };
	double doublevalues[4];
	double rhs = 0;
	//char sense = 'G';
	int status = 0;
	int count = 0;
	//int last_count = 1;
	int comp_length = 1 << (PD->num_rvs - 2);

	/* Now add the generic constraints.  */
	for (int cur_pair = 0; cur_pair < ER->num_pairs; cur_pair++)
	{
		int i = ER->pairs[0][cur_pair];
		int j = ER->pairs[1][cur_pair];
		for (int base = 0; base < comp_length; base++)
		{
			// note: must have i>j
			// set base_shifted as the conditioned random variables
			int base_shifted = ((base >> (i - 1)) << (i + 1));
			base_shifted = base_shifted | (((base&((1 << (i - 1)) - 1)) >> j) << (j + 1));
			base_shifted = base_shifted | (base&((1 << j) - 1));

			positions[0] = base_shifted | (1 << i);
			positions[1] = base_shifted | (1 << j);
			positions[2] = positions[1] | positions[0];
			positions[3] = base_shifted;

			memcpy(doublevalues, initial_values, sizeof(double) * 4);
			int num_validpos = map_compress(positions, doublevalues, 4, ER);

			if (num_validpos != 0) { // adding new constraints
				if ((status = status | GRBaddconstr(lp, num_validpos, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL)))
					printf("Something wrong in add rows\n");
				count++;

				if (((count >> 20) << 20) == count) {
					printf("%d constraints.\n", count); // provide some information as the problem is being generated
					//last_count = count;
					fflush(stdout);
					if (count >= (1 << 30)) {
						printf("Too many constraints, early turning off to avoid runnning out of momery\n");
					}
				}
			}
			if (status)
				return(status);
		}
	}
	for (int i = 0; i < PD->num_rvs; i++) {
		positions[0] = (1 << PD->num_rvs) - 1;
		positions[1] = positions[0] ^ (1 << i);
		doublevalues[0] = 1.0;
		doublevalues[1] = -1.0;
		int num_validpos = map_compress(positions, doublevalues, 2, ER);
		if (num_validpos != 0) { // adding new constraints
			if ((status = status | GRBaddconstr(lp, num_validpos, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL)))
				printf("Something wrong in add rows\n");
			count++;
		}
		if (status)
			return(status);
	}
	GRBupdatemodel(lp);
	return(0);
}

int add_Gurobirows_problemspecific(GRBmodel* lp, problem_description* PD, extracted_reductions* ER)
{
	int positions[MAXINDPITEMS];
	double doublevalues[MAXINDPITEMS];
	double rhs = 0;
	int status = 0;

	// to add objective
	if (PD->num_items_in_objective > 0 && PD->num_items_in_objective <= MAXINDPITEMS) {
		memcpy(doublevalues, PD->list_value_in_objective, sizeof(double)*PD->num_items_in_objective);
		memcpy(positions, PD->list_pos_in_objective, sizeof(int)*PD->num_items_in_objective);
		int num_effective = map_compress(positions, doublevalues, PD->num_items_in_objective, ER);
		if (num_effective > 0) {
			//**********************************************************************/
			//*********************specific for Cplex*******************************/
			//**********************************************************************/
			status = GRBsetdblattrlist(lp, "Obj", num_effective, positions, doublevalues);
		}
		else {
			printf("Invalid objective function. Equal to 0.\n");
			return 1;
		}
	}
	else {
		printf("Too many items in the objective functions. \n");
		return 1;
	}

	// to add independence
	if (PD->num_independence > 0) {
		for (int i = 0; i < PD->num_independence; i++) {
			for (int j = 0; j < PD->list_num_items_in_independence[i]; j++)
				doublevalues[j] = 1;
			doublevalues[PD->list_num_items_in_independence[i] - 1] = -1;
			if (PD->list_type_independence[i] == 1)
				doublevalues[PD->list_num_items_in_independence[i] - 2] = -(PD->list_num_items_in_independence[i] - 3);
			memcpy(positions, PD->list_independence[i], sizeof(int)*PD->list_num_items_in_independence[i]);
			int num_effective = map_compress(positions, doublevalues, PD->list_num_items_in_independence[i], ER);
			if (num_effective > 0) {
				//**********************************************************************/
				//*********************specific for Cplex*******************************/
				//**********************************************************************/
				status = GRBaddconstr(lp, num_effective, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL);
				// also add the negative term, this is useful in generating the proof
				for (int j = 0; j < num_effective; j++)
					doublevalues[j] = -doublevalues[j];
				status = status | GRBaddconstr(lp, num_effective, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL);
				if (status)
					printf("Something wrong in add rows in problem specific conditions: independence.\n");
			}
			else
				printf("Warning: An independence relation is not effective under the current problem setting. Possible input error.\n");
		}
	}

	// to add constant bounds
	if (PD->num_constantbounds > 0) {
		for (int i = 0; i < PD->num_constantbounds; i++) {
			if (PD->list_type_constantbounds[i] == 0 || PD->list_type_constantbounds[i] == 2) {
				rhs = PD->list_rhs[i];
				memcpy(doublevalues, PD->list_value_constantbounds[i], PD->list_num_items_in_constantbounds[i] * sizeof(double));
				memcpy(positions, PD->list_pos_constantbounds[i], PD->list_num_items_in_constantbounds[i] * sizeof(int));
				int num_effective = map_compress(positions, doublevalues, PD->list_num_items_in_constantbounds[i], ER);
				if (num_effective > 0) {
					//**********************************************************************/
					//*********************specific for Cplex*******************************/
					//**********************************************************************/
					status = GRBaddconstr(lp, num_effective, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL);
					if (PD->list_type_constantbounds[i] == 2) {
						for (int j = 0; j < num_effective; j++)
							doublevalues[j] = -doublevalues[j];
						rhs = -rhs;
						status = status | GRBaddconstr(lp, num_effective, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL);
					}

					if (status)
						printf("Something wrong in add rows in \n");
				}
				//else
				//	printf("Warning: A constant value bound is not effective under the current problem setting. Possible input error.\n");
			}
			else
				printf("Unrecognized type of constant bound type. Possible parsing error.\n");
		}
	}
	GRBupdatemodel(lp);
	return(0);
}

int populate_Gurobirows_random(GRBmodel* lp, problem_description* PD, extracted_reductions* ER, int num_constraints)
{
	int positions[4];
	double initial_values[4] = { 1,1,-1,-1 };
	double doublevalues[4];
	double rhs = 0;
	//char sense = 'G';
	//int rmatbeg = 0;
	int status = 0;
	int count = 0;
	//int last_count = 1;
	int comp_length = 1 << (PD->num_rvs - 2);
	//int* chosen = (int*)malloc(sizeof(int)*comp_length);
	char* chosen = (char*)malloc(sizeof(char)*comp_length);


	int num_constraint_each_pair = int(double(num_constraints) / (double)ER->num_pairs);
	time_t t;
	srand((unsigned)time(&t));
	//printf("Random seed used in this iteration = %lld\n", (long long) time(NULL));
	//printf("Total number of pairs %d. Will selected %d constraints randomly for each pair. \n",ER->num_pairs, num_constraint_each_pair);
	/* Now add the generic constraints.  */
	for (int cur_pair = 0; cur_pair < ER->num_pairs; cur_pair++)
	{
		int i = ER->pairs[0][cur_pair];
		int j = ER->pairs[1][cur_pair];
		//for(int base=0; base<comp_length;base++)
		//{
		//printf("On %3d-th pair=(%d,%d)\n", cur_pair, i,j);
		//for(int k=0;k<comp_length;k++){
		//	chosen[k] = k;
		memset(chosen, 0, sizeof(char)*comp_length);
		int pair_count = 0;
		int maxshift = (PD->num_rvs - 2);
		while (pair_count < num_constraint_each_pair) {
			int randnum = rand()*rand();
			int base = randnum - ((randnum >> maxshift) << maxshift); // take modulo operation
			if (chosen[base] == 1)
				continue;

			chosen[base] = 1;
			//base = chosen[index];
			//memcpy(chosen+index,chosen+index+1, sizeof(int)*(comp_length-index-1-pair_count));
			pair_count++;

			// note: must have i>j
			// set base_shifted as the conditioned random variables
			int base_shifted = ((base >> (i - 1)) << (i + 1));
			base_shifted = base_shifted | (((base&((1 << (i - 1)) - 1)) >> j) << (j + 1));
			base_shifted = base_shifted | (base&((1 << j) - 1));

			positions[0] = base_shifted | (1 << i);
			positions[1] = base_shifted | (1 << j);
			positions[2] = positions[1] | positions[0];
			positions[3] = base_shifted;

			memcpy(doublevalues, initial_values, sizeof(double) * 4);
			int num_validpos = map_compress(positions, doublevalues, 4, ER);

			if (num_validpos != 0) { // adding new constraints
				status = status | GRBaddconstr(lp, num_validpos, positions, doublevalues, GRB_GREATER_EQUAL, rhs, NULL);

				if (status)
					printf("Something wrong in add rows\n");
				count++;

				if (((count >> 20) << 20) == count) {
					printf("%12dth constraints generated.\n", count); // provide some information as the problem is being generated
					//last_count = count;
					fflush(stdout);
					if (count >= (1 << 30)) {
						printf("Too many constraints, early turning off to avoid runnning out of momery\n");
					}
				}
			}
			if (status)
				return(status);
		}
	}
	free(chosen);
	GRBupdatemodel(lp);
	return(0);
}


//*******************************************************************************************
//             Separators: Functions to the primal part are above
//*******************************************************************************************

void print_Gurobiinequalities(GRBmodel* primallp, GRBmodel* duallp, problem_description* PD, extracted_reductions* ER)
{
	int num_cols, num_rows;
	int status;
	status = GRBgetintattr(duallp, "NumVars", &num_cols);
	status = status | GRBgetintattr(duallp, "NumConstrs", &num_rows);
	//int num_combs = ER->num_effetive;
	int num_entropy = (1 << PD->num_rvs);
	double* x = (double*)malloc(sizeof(double)*num_cols);
	status = status | GRBgetdblattrarray(duallp, "X", 0, num_cols, x);

	int nzcnt = 0;
	int matbeg = 0;
	int *matind = (int*)malloc(sizeof(int)*num_rows);
	double *matval = (double*)malloc(sizeof(double)*num_rows);
	//int surplus;
	double rhs;
	int count = 1;
	//int num_effective;

	for (int i = 0; i < num_cols; i++) {
		if (fabs(x[i]) > TOLERANCE_LPGENERATION) {
			//**********************************************************************/
			//*********************specific for Cplex*******************************/
			//**********************************************************************/

			//int status_;
			//int status_ = GRBgetconstrs(primallp, &nzcnt, &matbeg, matind, matval, i, 1);
			GRBgetconstrs(primallp, &nzcnt, &matbeg, matind, matval, i, 1);
			printf("%.3d-th inequality: weight = %6.12f ", count, x[i]);
			count++;
			for (int j = 0; j < nzcnt; j++) {

				if (ER->inverse_mappingtable[matind[j]] >= num_entropy) {// fixed rate terms
					if (fabs(matval[j] - 1) < TOLERANCE_LPGENERATION)
						printf("       %s", PD->list_add_LPvars[ER->inverse_mappingtable[matind[j]] - num_entropy]);
					else if (fabs(matval[j] + 1) < TOLERANCE_LPGENERATION)
						printf("      -%s", PD->list_add_LPvars[ER->inverse_mappingtable[matind[j]] - num_entropy]);
					else
						printf("  %0.1f %s", matval[j], PD->list_add_LPvars[ER->inverse_mappingtable[matind[j]] - num_entropy]);
					continue;
				}

				int k = reduce(ER->inverse_mappingtable[matind[j]], PD);
				if (fabs(matval[j] - 1) < TOLERANCE_LPGENERATION)
					printf("     H(");
				else if (fabs(matval[j] + 1) < TOLERANCE_LPGENERATION)
					printf("    -H(");
				else
					printf("    %0.1fH(", matval[j]);

				//printf("  %0.1fH(",matval[j]);
				int init = 0;

				for (int bit = 0; bit < PD->num_rvs; bit++)
				{
					if (k & 1) {
						if (init) {
							printf(",");
						}
						init = 1;
						printf("%s", PD->list_rvs[bit]);
					}
					k = (k >> 1);
				}
				printf(")");
			}
			//status_ = GRBgetdblattrelement(primallp, "RHS", i, &rhs);
			GRBgetdblattrelement(primallp, "RHS", i, &rhs);
			if (fabs(rhs)>=1e-6) {
				printf("    >=%0.1f*\n", rhs);
				continue;
			}
			else
				printf(">=0\n");
		}
	}
	free(x);
	free(matind);
	free(matval);
}

