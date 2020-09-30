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
#include "problem_description.hpp"
#include "ERfromPD.h"
#include "transcribe2Cplex.h"
#include "utilities_Cplex.h"
#include "computes_Cplex.h"
#include "expression_helpers.hpp"
#pragma warning(disable: 4996)

int random_selected_constraints_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC, int num_constraints, int num_iter)
{
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	if ((1 << (PDC->num_rvs - 2))*ER.num_pairs <= num_constraints * 2) {
		printf("Convert to full set of constraints.\n");
		regular_compute(PD, PDC);
		return 0;
	}

	int status;
	printf("Completed mapping table with %d LP variables and pair selection %d pairs.\n", ER.num_effective, ER.num_pairs);
	printf("\n******************************************************************\n");

	for (int i = 0; i < num_iter; i++) {
		CPXENVptr env;
		CPXLPptr lp;
		char* problemname = (char*)"random_constraints";

		status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname, PD->LP_display? CPX_ON: CPX_OFF, CPX_ON);
		if (!status) {
			status = status | populate_Cplexrows_random(env, lp, PDC, &ER, num_constraints);
			status = status | add_Cplexrows_problemspecific(env, lp, PDC, &ER);
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
							double* x = (double*)malloc(sizeof(double)*(ER.num_effective + 1));
							CPXgetx(env, lp, x, 0, ER.num_effective);
							//for(int i=0; i<(1<<PD.num_rvs); i++)
							//	printf("%f   \n", x[ER.forward_mappingtable[i]]);
							printf("Values achieving the optimal solution:\n ");
							for (int i = 0; i < PDC->num_items_in_objective; i++)
								printf("%f   ", x[ER.forward_mappingtable[PDC->list_pos_in_objective[i]]]);
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

	return status;
}

int prove_bounds(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC)
{
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	CPXENVptr env;
	CPXLPptr lp = NULL;
	char* problemname = (char*)"prove_bounds"; 

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname, PD->LP_display ? CPX_ON : CPX_OFF, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PDC, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PDC, &ER);
		CPXLPptr lp_dual = NULL;
		CPXLPptr mip = NULL;
		if (!make_dual(env, lp, &lp_dual, &mip, (char*)"dual LP")) {

			// no need for the primal now
			freeLP(env, lp);
			lp = NULL;

			for (int i = 0; i < PDC->num_bounds; i++) {

				double* rhs = (double*)malloc(sizeof(double)*(PDC->list_num_items_in_bounds[i] + 1));
				memcpy(rhs, PDC->list_value_bounds[i], PDC->list_num_items_in_bounds[i] * sizeof(double));
				int* rhs_pos = (int*)malloc(sizeof(int)*(PDC->list_num_items_in_bounds[i] + 1));
				for (int j = 0; j < PDC->list_num_items_in_bounds[i]; j++)
				//	rhs_pos[j] = ER.forward_mappingtable[PD->list_pos_bounds[i][j]];
					rhs_pos[j] = PDC->list_pos_bounds[i][j];
				int num_effective = ER.Map_Compress(rhs_pos, rhs, PDC->list_num_items_in_bounds[i]);
				rhs[num_effective] = PDC->list_rhs_2prove[i];
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
					printf("Proved %d-th inequality: %s.\n", i+1, Bound_Json_To_String(PD->Bounds_To_Prove[i]->Form_For_Printing()->As_Json()).c_str());
					print_Cplexinequalities(env, lp_dual, PDC, &ER);
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
					printf("Proved %d-th inequality using integer values: %s.\n", i+1, Bound_Json_To_String(PD->Bounds_To_Prove[i]->Form_For_Printing()->As_Json()).c_str());
					print_Cplexinequalities(env, mip, PDC, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\n Could not prove the inequality using integer values.\n");

				//clean up right hand side, since otherwise the next bound might be messed up.
				for (int j = 0; j < PDC->list_num_items_in_bounds[i]; j++)
					rhs[j] = 0;
				rhs[PDC->list_num_items_in_bounds[i]] = PDC->list_rhs_2prove[i];
				status = CPXchgrhs(env, lp_dual, PDC->list_num_items_in_bounds[i] + 1, rhs_pos, rhs);
				status = CPXchgrhs(env, mip, PDC->list_num_items_in_bounds[i] + 1, rhs_pos, rhs);

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
	return status;
}

int sensitivity_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC)
{
	int positions[MAXINDPITEMS_C_STYLE];
	double doublevalues[MAXINDPITEMS_C_STYLE];
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	CPXENVptr env;
	CPXLPptr lp;
	char* problemname = (char*)"sensitivity";

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname, PD->LP_display ? CPX_ON : CPX_OFF, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PDC, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PDC, &ER);		
		
		if (!status) {
			if (!(status = CPXbaropt(env, lp))) { // barrier is preferred if the problem is large or use //if(!(status=CPXlpopt(env, lp))){
				status = CPXgetstat(env, lp);
				if (status == 1 || status == 6) {

					double objval;
					CPXgetobjval(env, lp, &objval);
					printf("\n******************************************************************\n");
					printf("Optimal value for %s = %0.6f.\n", Expression_Json_To_String(PD->Objective->Form_For_Printing()->As_Json()).c_str(), objval);

					if (PDC->num_sensitivities > 0){ 
						// start to perform sensitivity analysis: this is different from Cplex function because cplex provides the range that the optimal basis does not change
						// which is not our interest in this context. 						
						// strategy: we will bound the objective function <= objval, then minimize and maximize the sensitivity terms

						char* sense = (char*)"GL";
						int rmatbeg = 0;
						objval += 1e-12; // slightly increase the optimal value to avoid numerical problem.
						memcpy(doublevalues, PDC->list_value_in_objective, PDC->num_items_in_objective* sizeof(double));
						memcpy(positions, PDC->list_pos_in_objective, PDC->num_items_in_objective * sizeof(int));
						int num_effective = ER.Map_Compress(positions, doublevalues, PDC->num_items_in_objective);
						if (num_effective > 0) {
							CPXaddrows(env, lp, 0, 1, PDC->num_items_in_objective, &objval, sense + 1, &rmatbeg, positions, doublevalues, NULL, NULL);
							//we'll also reset the objective function here
							for (int i = 0; i < num_effective; i++)
								doublevalues[i] = 0;
							status = CPXchgobj(env, lp, num_effective, positions, doublevalues);
							if (status)
								printf("Cannot reset objective function.\n");
						}

						double ub= CPX_INFBOUND, lb=-CPX_INFBOUND;
						//we are now ready to probe the sensitivity of each expression
						printf("Sensitivity results:\n");
						for (int i = 0; i < PDC->num_sensitivities; i++) {							
							memcpy(doublevalues, PDC->list_value_sensitivities[i], PDC->list_num_items_in_sensitivities[i] * sizeof(double));
							memcpy(positions, PDC->list_pos_sensitivities[i], PDC->list_num_items_in_sensitivities[i] * sizeof(int));
							int num_effective = ER.Map_Compress(positions, doublevalues, PDC->list_num_items_in_sensitivities[i]);
							if (num_effective > 0) {
								// find lower bound and upper bound
								status = CPXchgobj(env, lp, num_effective, positions, doublevalues);
								status = status|CPXchgobjsen(env, lp, CPX_MIN);
								if (!(status = status|CPXbaropt(env, lp))) {
									CPXgetobjval(env, lp, &lb);
								}
								else
									printf("Cannot optimize for purpopse of sensitivity analysis.\n");
								status = status | CPXchgobjsen(env, lp, CPX_MAX);
								if (!(status = status | CPXbaropt(env, lp))) {
									CPXgetobjval(env, lp, &ub);
								}
								else
									printf("Cannot optimize for the purpopse of sensitivity analysis.\n");
								if(status==0)
									printf("Sensitivity %-25s= [%0.5f, %0.5f]\n", Expression_Json_To_String(PD->Sensitivities[i]->Form_For_Printing()->As_Json()).c_str(),lb,ub);
								// again reset the objective function for next round
								for (int j = 0; j < num_effective; j++)
									doublevalues[j] = 0;
								status = CPXchgobj(env, lp, num_effective, positions, doublevalues);
								if (status)
									printf("Cannot reset objective function.\n");
							}
						}												
						printf("******************************************************************\n");
					}				
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

	return status;
}

int regular_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC)
{
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	CPXENVptr env;
	CPXLPptr lp;
	char* problemname = (char*)"regular";

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname, PD->LP_display ? CPX_ON : CPX_OFF, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PDC, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PDC, &ER);
		if (!status) {
			if (!(status = CPXbaropt(env, lp))) { // barrier is preferred if the problem is large or use //if(!(status=CPXlpopt(env, lp))){
				status = CPXgetstat(env, lp);
				if (status == 1 || status == 6) {
					double objval;
					status = CPXgetobjval(env, lp, &objval);
					printf("\n******************************************************************\n");
					printf("Optimal value for %s = %0.6f.\n", Expression_Json_To_String(PD->Objective->Form_For_Printing()->As_Json()).c_str(), objval);
					// processing queries if they exist
					if (PDC->num_queries > 0) {
						double* x = (double*)malloc(sizeof(double) * (ER.num_effective + 1));
						status = status|CPXgetx(env, lp, x, 0, ER.num_effective);
						if (!status) {
							printf("Queried values:\n");
							for (int i = 0; i < PDC->num_queries; i++)
							{
								double value = 0;
								for(int j=0; j<PDC->list_num_items_in_queries[i]; j++)
									value += PDC->list_value_queries[i][j]*(x[ER.forward_mappingtable[PDC->list_pos_queries[i][j]]]);
								printf("%-25s = %0.5f\n", Expression_Json_To_String(PD->Queries[i]->Form_For_Printing()->As_Json()).c_str(), value);
							}							
						}
						else
							printf("Fail to retrieve optimal solution.\n");
						free(x);
					}
					printf("******************************************************************\n");
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

	return status;
}

int hull_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC)
{
	if (PDC->num_items_in_objective != 2) {
		printf("Hull computation can only be done for a problem with two items in the objective function.\n");
		return 1;
	}
	setbuf(stdout, NULL);

	int num_values = 2;

	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	CPXENVptr env;
	CPXLPptr lp;
	char* problemname = (char*)"hull";

	double* Alist = (double*)malloc(sizeof(double) * 2);
	double* Blist = (double*)malloc(sizeof(double) * 2);

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname, PD->LP_display? CPX_ON:CPX_OFF, CPX_ON);
	if (!status) {
		status = status | populate_Cplexrows_generic(env, lp, PDC, &ER);
		status = status | add_Cplexrows_problemspecific(env, lp, PDC, &ER);
		if (!status) {
			double baseweights[2];
			int positions[2];
			baseweights[0] = (PDC->list_value_in_objective[0] > 0) ? 1.0 : -1.0;
			baseweights[1] = (PDC->list_value_in_objective[1] > 0) ? 1.0 : -1.0;
			positions[0] = ER.forward_mappingtable[PDC->list_pos_in_objective[0]];
			positions[1] = ER.forward_mappingtable[PDC->list_pos_in_objective[1]];
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

	return status;
}
