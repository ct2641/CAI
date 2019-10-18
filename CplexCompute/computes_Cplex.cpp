// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../PDParser/PDparser.h"
#include "transcribe2Cplex.h"
#include "../ERfromPD.h"
#include "utilities_Cplex.h"
#include "computes_Cplex.h"

int random_selected_constraints_compute(problem_description* PD, int num_constraints, int num_iter, int whetherdisplay)
{
	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	if ((1 << (PD->num_rvs - 2))*ER.num_pairs <= num_constraints * 2) {
		printf("Convert to full set of constraints.\n");
		freeER(&ER);
		regular_compute(PD, CPX_ON);
		return 0;
	}

	int status;
	printf("Completed mapping table with %d LP variables and pair selection %d pairs.\n", ER.num_effetive, ER.num_pairs);
	printf("\n******************************************************************\n");

	for (int i = 0; i < num_iter; i++) {
		CPXENVptr env;
		CPXLPptr lp;
		char* problemname = (char*)"random_constraints";

		status = set_defaultLP(&env, &lp, ER.num_effetive + 1, problemname, whetherdisplay, CPX_ON);
		if (!status) {
			status = status | populate_Cplexrows_random(env, lp, PD, &ER, num_constraints);
			status = status | add_Cplexrows_problemspecific(env, lp, PD, &ER);
			//printf("Completed populating by rows.\n");
			//printf("%10d-th iteration: optimal solution is all 0.\r",i);
			if (!status) {
				if (!(status = CPXbaropt(env, lp))) { // barrier is preferred if the problem is large or use //if(!(status=CPXlpopt(env, lp))){
					status = CPXgetstat(env, lp);
					if (status == 1 || status == 6) {
						double objval;
						CPXgetobjval(env, lp, &objval);
						if (fabs(objval) >= 1e-3) {
							printf("Optimal value is %6.6f in %10d-th iteration.\n", objval, i);
							double* x = (double*)malloc(sizeof(double)*(ER.num_effetive + 1));
							CPXgetx(env, lp, x, 0, ER.num_effetive);
							//for(int i=0; i<(1<<PD.num_rvs); i++)
							//	printf("%f   \n", x[ER.forward_mappingtable[i]]);
							printf("Values achieving the optimal solution:\n ");
							for (int i = 0; i < PD->num_items_in_objective; i++)
								printf("%f   ", x[ER.forward_mappingtable[PD->list_pos_in_objective[i]]]);
							printf("\n******************************************************************\n");
							free(x);
						}
						else
							printf("%10d-th iteration: optimal solution is all 0.\r", i);
					}
					else
						printf("Fail to find optimal value.\n");
				}
				else
					printf("Cplex failed to solve the problem.\n");
			}
		}
		freeLP(env, lp);
		free_Lp_env(env);
	}

	freeER(&ER);
	return status;
}

int prove_bounds(problem_description* PD, int whetherdisplay)
{
	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	CPXENVptr env;
	CPXLPptr lp = NULL;
	char* problemname = (char*)"prove_bounds";

	int status = set_defaultLP(&env, &lp, ER.num_effetive + 1, problemname, whetherdisplay, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PD, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PD, &ER);
		CPXLPptr lp_dual = NULL;
		CPXLPptr mip = NULL;
		if (!make_dual(env, lp, &lp_dual, &mip, (char*)"dual LP")) {

			// no need for the primal now
			freeLP(env, lp);
			lp = NULL;

			for (int i = 0; i < PD->num_bounds; i++) {

				double* rhs = (double*)malloc(sizeof(double)*(PD->list_num_items_in_bounds[i] + 1));
				memcpy(rhs, PD->list_value_bounds[i], PD->list_num_items_in_bounds[i] * sizeof(double));
				int* rhs_pos = (int*)malloc(sizeof(int)*(PD->list_num_items_in_bounds[i] + 1));
				for (int j = 0; j < PD->list_num_items_in_bounds[i]; j++)
				//	rhs_pos[j] = ER.forward_mappingtable[PD->list_pos_bounds[i][j]];
					rhs_pos[j] = PD->list_pos_bounds[i][j];
				int num_effective = map_compress(rhs_pos, rhs, PD->list_num_items_in_bounds[i], &ER);
				rhs[num_effective] = PD->list_rhs_2prove[i];
				rhs_pos[num_effective] = CPXgetnumrows(env, lp_dual) - 1;
				status = CPXchgrhs(env, lp_dual, num_effective+1, rhs_pos, rhs);
				status = CPXchgrhs(env, mip, num_effective+1, rhs_pos, rhs);

				if ((status = CPXlpopt(env, lp_dual))) {
					//if(status=CPXdualopt (env, lp_dual)){
						//if(status=CPXprimopt (env, lp_dual)){
					fprintf(stderr, "Failed to optimize LP.\n");
					goto TERMINATE;
				}
				status = CPXgetstat(env, lp_dual);
				if (status == 1) {
					printf("******************************************************************\n");
					double objval;
					CPXgetobjval(env, lp_dual, &objval);
					printf("LP dual value %f \n", objval);
					printf("Proved %d-th inequality.\n", i);
					print_Cplexinequalities(env, lp_dual, PD, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\nSomething is wrong: Solution status = %d.\n", status);


				//solve again to make it integers

				if ((status = CPXmipopt(env, mip))) {
					//if(status=CPXdualopt (env, lp_dual)){
					fprintf(stderr, "Failed to optimize MIP.\n");
					goto TERMINATE;
				}
				status = CPXgetstat(env, mip);
				if (status == CPXMIP_OPTIMAL) {
					printf("******************************************************************\n");
					double objval;
					CPXgetobjval(env, mip, &objval);
					printf("MIP dual value %f \n", objval);
					printf("Proved %d-th inequality using integer values.\n", i);
					print_Cplexinequalities(env, mip, PD, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\n Could not prove the inequality using integer values.\n");

				//clean up right hand side, since otherwise the next bound might be messed up.
				for (int j = 0; j < PD->list_num_items_in_bounds[i]; j++)
					rhs[j] = 0;
				rhs[PD->list_num_items_in_bounds[i]] = PD->list_rhs_2prove[i];
				status = CPXchgrhs(env, lp_dual, PD->list_num_items_in_bounds[i] + 1, rhs_pos, rhs);
				status = CPXchgrhs(env, mip, PD->list_num_items_in_bounds[i] + 1, rhs_pos, rhs);

				free(rhs);
				free(rhs_pos);

			}
		}
		if (lp_dual != NULL) {
			freeLP(env, lp_dual);
			freeLP(env, mip);
		}
	}

TERMINATE:
	if (lp != NULL)
		freeLP(env, lp);
	free_Lp_env(env);
	freeER(&ER);
	return status;
}

int regular_compute(problem_description* PD, int whetherdisplay)
{
	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	CPXENVptr env;
	CPXLPptr lp;
	char* problemname = (char*)"regular";

	int status = set_defaultLP(&env, &lp, ER.num_effetive + 1, problemname, whetherdisplay, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PD, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PD, &ER);
		if (!status) {
			if (!(status = CPXbaropt(env, lp))) { // barrier is preferred if the problem is large or use //if(!(status=CPXlpopt(env, lp))){
				status = CPXgetstat(env, lp);
				if (status == 1 || status == 6) {
					double objval;
					CPXgetobjval(env, lp, &objval);
					printf("\n******************************************************************\n");
					printf("Optimal value is %6.6f.\n", objval);
					double* x = (double*)malloc(sizeof(double)*(ER.num_effetive + 1));
					CPXgetx(env, lp, x, 0, ER.num_effetive);
					//for(int i=0; i<(1<<PD.num_rvs); i++)
					//	printf("%f   \n", x[ER.forward_mappingtable[i]]);
					printf("Values achieving the optimal solution:\n ");
					for (int i = 0; i < PD->num_items_in_objective; i++)
						printf("%f   ", x[ER.forward_mappingtable[PD->list_pos_in_objective[i]]]);
					printf("\n******************************************************************\n");
					free(x);
				}
				else
					printf("Fail to find optimal value.\n");
			}
			else
				printf("Cplex failed to solve the problem.\n");
		}
	}

	freeLP(env, lp);
	free_Lp_env(env);
	freeER(&ER);

	return status;
}

int hull_compute(problem_description* PD, int whetherdisplay)
{
	if (PD->num_items_in_objective != 2) {
		printf("Hull computation can only be done for a problem with two items in the objective function.\n");
		return 1;
	}
	setbuf(stdout, NULL);

	int num_values = 2;

	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	CPXENVptr env;
	CPXLPptr lp;
	char* problemname = (char*)"hull";

	double* Alist = (double*)malloc(sizeof(double) * 2);
	double* Blist = (double*)malloc(sizeof(double) * 2);

	int status = set_defaultLP(&env, &lp, ER.num_effetive + 1, problemname, whetherdisplay, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PD, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PD, &ER);
		if (!status) {
			double baseweights[2];
			int positions[2];
			baseweights[0] = (PD->list_value_in_objective[0] > 0) ? 1.0 : -1.0;
			baseweights[1] = (PD->list_value_in_objective[1] > 0) ? 1.0 : -1.0;
			positions[0] = ER.forward_mappingtable[PD->list_pos_in_objective[0]];
			positions[1] = ER.forward_mappingtable[PD->list_pos_in_objective[1]];
			double currentweights[2];
			// first extreme point
			currentweights[0] = baseweights[0];
			currentweights[1] = baseweights[1] * 1e-4;
			status = CPXchgobj(env, lp, 2, positions, currentweights);
			if (status) {
				fprintf(stderr, "Failure to change objective function, error %d.\n", status);
				goto TERMINATE;
			}
			if ((status = CPXbaropt(env, lp))) {
				fprintf(stderr, "Failed to optimize LP.\n");
				goto TERMINATE;
			}
			status = CPXgetstat(env, lp);
			if (status == 1 || status == 6) {
				double temp;
				status = CPXgetx(env, lp, &temp, positions[0], positions[0]);
				Alist[0] = temp;
				status = CPXgetx(env, lp, &temp, positions[1], positions[1]);
				Blist[0] = temp;
			}
			else {
				fprintf(stderr, "Failure to find finite value solution, error %d.\n", status);
				goto TERMINATE;
			}
			printf("New point (%6.6f, %6.6f).\n", Alist[0], Blist[0]);

			// second extreme point
			currentweights[0] = baseweights[0] * 1e-4;
			currentweights[1] = baseweights[1];
			status = CPXchgobj(env, lp, 2, positions, currentweights);
			if (status) {
				fprintf(stderr, "Failure to change objective function, error %d.\n", status);
				goto TERMINATE;
			}
			if ((status = CPXbaropt(env, lp))) {
				fprintf(stderr, "Failed to optimize LP.\n");
				goto TERMINATE;
			}
			status = CPXgetstat(env, lp);
			if (status == 1 || status == 6) {
				double temp;
				status = CPXgetx(env, lp, &temp, positions[0], positions[0]);
				Alist[1] = temp;
				status = CPXgetx(env, lp, &temp, positions[1], positions[1]);
				Blist[1] = temp;
			}
			else {
				fprintf(stderr, "Failure to find finite value solution, error %d.\n", status);
				goto TERMINATE;
			}
			printf("New point (%6.6f, %6.6f).\n", Alist[1], Blist[1]);

			double last_objval;
			// find intermediate points
			for (int i = 0; i < num_values - 1; i++) {
				currentweights[0] = baseweights[0];
				if (fabs(Blist[i + 1] - Blist[i]) > 1e-6)
					currentweights[1] = (-(Alist[i + 1] - Alist[i]) / (Blist[i + 1] - Blist[i]))*baseweights[1];
				last_objval = Alist[i] * currentweights[0] + Blist[i] * currentweights[1];
				status = CPXchgobj(env, lp, 2, positions, currentweights);
				if (status) {
					fprintf(stderr, "Failure to change objective function, error %d.\n", status);
					goto TERMINATE;
				}
				if ((status = CPXbaropt(env, lp))) {
					fprintf(stderr, "Failed to optimize LP.\n");
					goto TERMINATE;
				}

				double objval;
				CPXgetobjval(env, lp, &objval);
				if (fabs(Alist[i] * currentweights[0] + Blist[i] * currentweights[1] - objval) > TOL_ALG*fabs(last_objval)) {
					double temp;
					// not terminal: insert the extreme point, and then rewind i
					double* Btemplist = (double*)malloc(sizeof(double)*(num_values + 1));
					memcpy(Btemplist, Blist, sizeof(double)*(i + 1));
					CPXgetx(env, lp, &temp, positions[1], positions[1]);
					Btemplist[i + 1] = temp;
					memcpy(Btemplist + i + 2, Blist + i + 1, sizeof(double)*(num_values - i - 1));
					free(Blist);
					Blist = Btemplist;

					double* Atemplist = (double*)malloc(sizeof(double)*(num_values + 1));
					memcpy(Atemplist, Alist, sizeof(double)*(i + 1));
					CPXgetx(env, lp, &temp, positions[0], positions[0]);
					Atemplist[i + 1] = temp;
					memcpy(Atemplist + i + 2, Alist + i + 1, sizeof(double)*(num_values - i - 1));
					free(Alist);
					Alist = Atemplist;
					printf("New point (%6.6f, %6.6f).\n", Alist[i + 1], Blist[i + 1]);
					num_values++;
					i--;
					last_objval = objval;
				}
			}
			printf("\n\nList of found points on the hull:\n");
			for (int i = 0; i < num_values; i++)
				printf("(%6.6f, %6.6f).\n", Alist[i], Blist[i]);
			printf("End of list of found points.\n");
		}
		else
			printf("Failed to set up the problem.\n\n");
	}

TERMINATE:

	free(Alist);
	free(Blist);
	freeLP(env, lp);
	free_Lp_env(env);
	freeER(&ER);

	return status;
}
