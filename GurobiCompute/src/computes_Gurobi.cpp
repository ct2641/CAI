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
#include "transcribe2Gurobi.h"
#include "ERfromPD.h"
#include "utilities_Gurobi.h"
#include "computes_Gurobi.h"
#include "gurobi_c.h"
#include "expression_helpers.hpp"

using namespace CAI;

int regular_compute(Problem_Description* PD, Problem_Description_C_Style* PDC)
{
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	GRBenv* env;
	GRBmodel* lp;
	char* problemname = (char*)"regular";

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname);
	if (!status) {
		status = status | populate_Gurobirows_generic(lp, PDC, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PDC, &ER);
		GRBsetintparam(GRBgetenv(lp), "OutputFlag", PD->LP_display);
		if (!status) {
			//status = GRBsetintparam(env, "Method", 2); //explicitly set it to barrier method, if one prefers			
			if (!(status = GRBoptimize(lp))) {
				int solstatus;
				status = GRBgetintattr(lp, "Status", &solstatus);
				if (solstatus == GRB_OPTIMAL) {
					double objval;
					status = GRBgetdblattr(lp, "ObjVal", &objval);
					printf("\n******************************************************************\n");
					printf("Optimal value is %s = %12.12lf.\n", Expression_Json_To_String(PD->Objective->Form_For_Printing()->As_Json()).c_str(), objval);

					// processing queries if they exist
					if (PDC->num_queries > 0) {
						int num_x;
						status = GRBgetintattr(lp, "NumVars", &num_x);
						double* x = (double*)malloc(sizeof(double) * num_x);
						status = status | GRBgetdblattrarray(lp, "X", 0, num_x, x);

						if (!status) {
							printf("Queried values:\n");
							for (int i = 0; i < PDC->num_queries; i++)
							{
								double value = 0;
								for (int j = 0; j < PDC->list_num_items_in_queries[i]; j++)
									value += PDC->list_value_queries[i][j] * (x[ER.forward_mappingtable[PDC->list_pos_queries[i][j]]]);
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

	GRBfreemodel(lp);
	GRBfreeenv(env);

	return status;
}


int sensitivity_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC)
{
	int positions[MAXINDPITEMS_C_STYLE];
	double doublevalues[MAXINDPITEMS_C_STYLE];
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	GRBenv* env;
	GRBmodel* lp;
	char* problemname = (char*)"sensitivity";

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname);
	if (!status) {
		status = status | populate_Gurobirows_generic(lp, PDC, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PDC, &ER);
		GRBsetintparam(GRBgetenv(lp), "OutputFlag", PD->LP_display);
		if (!status) {
			//status = GRBsetintparam(env, "Method", 2); //explicitly set it to barrier method, if one prefers
			if (!(status = GRBoptimize(lp))) {
				int solstatus;
				status = GRBgetintattr(lp, "Status", &solstatus);
				if (solstatus == GRB_OPTIMAL) {
					double objval;
					status = GRBgetdblattr(lp, "ObjVal", &objval);
					printf("\n******************************************************************\n");
					printf("Optimal value is %s = %12.12lf.\n", Expression_Json_To_String(PD->Objective->Form_For_Printing()->As_Json()).c_str(), objval);

					// processing queries if they exist
					if (PDC->num_sensitivities > 0) {

						GRBsetintparam(GRBgetenv(lp), "OutputFlag", PD->LP_display);

						objval += 1e-12; // slightly increase the optimal value to avoid numerical problem.
						memcpy(doublevalues, PDC->list_value_in_objective, PDC->num_items_in_objective * sizeof(double));
						memcpy(positions, PDC->list_pos_in_objective, PDC->num_items_in_objective * sizeof(int));
						int num_effective = ER.Map_Compress(positions, doublevalues, PDC->num_items_in_objective);

						if (num_effective > 0)
							//we don't need to rest objective here													
							GRBaddconstr(lp, num_effective, positions, doublevalues, GRB_LESS_EQUAL, objval, NULL);

						double ub = GRB_INFINITY, lb = -GRB_INFINITY;
						//we are now ready to probe the sensitivity of each expression
						printf("Sensitivity results:\n");

						for (int i = 0; i < PDC->num_sensitivities; i++) {
							memcpy(doublevalues, PDC->list_value_sensitivities[i], PDC->list_num_items_in_sensitivities[i] * sizeof(double));
							memcpy(positions, PDC->list_pos_sensitivities[i], PDC->list_num_items_in_sensitivities[i] * sizeof(int));
							int num_effective = ER.Map_Compress(positions, doublevalues, PDC->list_num_items_in_sensitivities[i]);
							if (num_effective > 0) {
								// find lower bound and upper bound
								status = GRBsetobjectiven(lp, 0, 1, 1, 0.0, 0.0, "primary", 0.0, num_effective, positions, doublevalues);

								status = status | GRBsetintattr(lp, "ModelSense", 1); // minimization									
								if (!(status = status | GRBoptimize(lp))) {
									status = GRBgetdblattr(lp, "ObjVal", &lb);
								}
								else
									printf("Cannot optimize for purpopse of sensitivity analysis.\n");

								status = status | GRBsetintattr(lp, "ModelSense", -1); // maximization
								if (!(status = status | GRBoptimize(lp))) {
									status = GRBgetdblattr(lp, "ObjVal", &ub);
								}
								else
									printf("Cannot optimize for the purpopse of sensitivity analysis.\n");

								if (status == 0)
									printf("Sensitivity %-25s= [%0.5f, %0.5f]\n", Expression_Json_To_String(PD->Sensitivities[i]->Form_For_Printing()->As_Json()).c_str(), lb, ub);
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

	GRBfreemodel(lp);
	GRBfreeenv(env);

	return status;
}


int random_selected_constraints_compute(Problem_Description* PD, Problem_Description_C_Style* PDC, int num_constraints, int num_iter)
{
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	if ((1 << (PDC->num_rvs - 2)) * ER.num_pairs <= num_constraints * 2) {
		printf("Convert to full set of constraints.\n");
		regular_compute(PD, PDC);
		return 0;
	}

	int status;
	printf("Completed mapping table with %d LP variables and pair selection %d pairs.\n", ER.num_effective, ER.num_pairs);
	printf("\n******************************************************************\n");

	for (int i = 0; i < num_iter; i++) {
		GRBenv* env;
		GRBmodel* lp;
		char* problemname = (char*)"random_constraints";

		status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname);
		if (!status) {
			status = status | populate_Gurobirows_random(lp, PDC, &ER, num_constraints);
			status = status | add_Gurobirows_problemspecific(lp, PDC, &ER);
			GRBsetintparam(GRBgetenv(lp), "OutputFlag", PD->LP_display);
			if (!status) {
				//status = GRBsetintparam(env, "Method", 2); //explicitly set it to barrier method, if one prefers
				if (!(status = GRBoptimize(lp))) {
					int solstatus;
					status = GRBgetintattr(lp, "Status", &solstatus);
					if (solstatus == 2) {
						double objval;
						status = GRBgetdblattr(lp, "ObjVal", &objval);
						if (fabs(objval) >= 1e-3) {
							printf("Optimal value is %12.12f in %10d-th iteration.\n", objval, i);
							int num_x;
							status = GRBgetintattr(lp, "NumVars", &num_x);
							double* x = (double*)malloc(sizeof(double) * num_x);
							status = status | GRBgetdblattrarray(lp, "X", 0, num_x, x);
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
		GRBfreemodel(lp);
		GRBfreeenv(env);
	}
	return status;
}

int prove_bounds(Problem_Description* PD, Problem_Description_C_Style* PDC)
{
	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	GRBenv* env;
	GRBmodel* lp = NULL;
	char* problemname = (char*)"prove_bounds";

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname);

	if (!status) {
		status = status | populate_Gurobirows_generic(lp, PDC, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PDC, &ER);
		GRBmodel* lp_dual = NULL;
		GRBmodel* mip = NULL;
		if (!make_dual(env, lp, &lp_dual, &mip)) {
			int num_rows;
			GRBgetintattr(lp_dual, "NumConstrs", &num_rows);
			GRBsetintparam(GRBgetenv(lp_dual), "OutputFlag", PD->LP_display);
			GRBsetintparam(GRBgetenv(mip), "OutputFlag", PD->LP_display);
			for (int i = 0; i < PDC->num_bounds; i++) {

				double* rhs = (double*)malloc(sizeof(double) * (PDC->list_num_items_in_bounds[i] + 1));
				memcpy(rhs, PDC->list_value_bounds[i], PDC->list_num_items_in_bounds[i] * sizeof(double));
				int* rhs_pos = (int*)malloc(sizeof(int) * (PDC->list_num_items_in_bounds[i] + 1));
				for (int j = 0; j < PDC->list_num_items_in_bounds[i]; j++)
					//	rhs_pos[j] = ER.forward_mappingtable[PD->list_pos_bounds[i][j]];
					rhs_pos[j] = PDC->list_pos_bounds[i][j];
				int num_effective = ER.Map_Compress(rhs_pos, rhs, PDC->list_num_items_in_bounds[i]);
				for (int j = 0; j < num_effective; j++) {
					//double rhs = PD->list_value_bounds[i][j];
					status = GRBsetdblattrelement(lp_dual, "RHS", rhs_pos[j], rhs[j]);
					status = status | GRBsetdblattrelement(mip, "RHS", rhs_pos[j], rhs[j]);
				}
				status = status | GRBsetdblattrelement(lp_dual, "RHS", num_rows - 1, PDC->list_rhs_2prove[i]);
				status = status | GRBsetdblattrelement(mip, "RHS", num_rows - 1, PDC->list_rhs_2prove[i]);
				free(rhs);
				free(rhs_pos);

				if ((status = (status | GRBoptimize(lp_dual)))) {
					fprintf(stderr, "Failed to optimize LP.\n");
					goto TERMINATE;
				}
				int solstatus;
				status = GRBgetintattr(lp_dual, "Status", &solstatus);
				if (solstatus == GRB_OPTIMAL) {
					printf("******************************************************************\n");
					double objval;
					status = GRBgetdblattr(lp_dual, "ObjVal", &objval);
					printf("LP dual value %f \n", objval);
					printf("Proved %d-th inequality: %s\n", i + 1, Bound_Json_To_String(PD->Bounds_To_Prove[i]->Form_For_Printing()->As_Json()).c_str());
					print_Gurobiinequalities(lp, lp_dual, PDC, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\nSomething is wrong: Solution status = %d.\n", status);

				//solve again to make it integers
				if ((status = GRBoptimize(mip))) {
					fprintf(stderr, "Failed to optimize MIP.\n");
					goto TERMINATE;
				}
				status = GRBgetintattr(mip, "Status", &solstatus);
				if (solstatus == GRB_OPTIMAL) {
					printf("******************************************************************\n");
					double objval;
					status = GRBgetdblattr(mip, "ObjVal", &objval);
					printf("MIP dual value %f \n", objval);
					printf("Proved %d-th inequality using integer values: %s\n", i + 1, Bound_Json_To_String(PD->Bounds_To_Prove[i]->Form_For_Printing()->As_Json()).c_str());
					print_Gurobiinequalities(lp, mip, PDC, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\n Could not prove the inequality using integer values.\n");

				//clean up right hand side, since otherwise the next bound might be messed up.
				for (int j = 0; j < PDC->list_num_items_in_bounds[i]; j++) {
					status = GRBsetdblattrelement(lp_dual, "RHS", ER.forward_mappingtable[PDC->list_pos_bounds[i][j]], 0);
					status = status | GRBsetdblattrelement(mip, "RHS", ER.forward_mappingtable[PDC->list_pos_bounds[i][j]], 0);
				}
				status = status | GRBsetdblattrelement(lp_dual, "RHS", num_rows - 1, 0);
				status = status | GRBsetdblattrelement(mip, "RHS", num_rows - 1, 0);
			}
		}
		if (lp_dual != NULL) {
			GRBfreemodel(lp_dual);
			GRBfreemodel(mip);
		}
	}

TERMINATE:
	if (lp != NULL)
		GRBfreemodel(lp);
	GRBfreeenv(env);
	return status;
}


int hull_compute(CAI::Problem_Description* PD, CAI::Problem_Description_C_Style* PDC)
{
	if (PDC->num_items_in_objective != 2) {
		printf("Hull computation can only be done for a problem with two items in the objective function.\n");
		return 1;
	}
	int num_values = 2;

	Extracted_Reductions ER;
	ER.Make_Mappingtable(PDC);
	ER.Make_Pairs(PDC);

	GRBenv* env;
	GRBmodel* lp = NULL;
	char* problemname = (char*)"hull";

	double* Alist = (double*)malloc(sizeof(double) * 2);
	double* Blist = (double*)malloc(sizeof(double) * 2);

	int status = set_defaultLP(&env, &lp, ER.num_effective + 1, problemname);
	if (!status) {
		status = status | populate_Gurobirows_generic(lp, PDC, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PDC, &ER);
		GRBsetintparam(GRBgetenv(lp), "OutputFlag", PD->LP_display);
		if (!status) {
			//status = GRBsetintparam(env, "Method", 2); //explicitly set it to barrier method, if one prefers
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

			status = GRBsetdblattrelement(lp, "Obj", positions[0], currentweights[0]);
			status = status | GRBsetdblattrelement(lp, "Obj", positions[1], currentweights[1]);
			if (status) {
				fprintf(stderr, "Failure to change objective function, error %d.\n", status);
				goto TERMINATE;
			}
			if ((status = GRBoptimize(lp))) {
				fprintf(stderr, "Failed to optimize LP.\n");
				goto TERMINATE;
			}
			int solstatus;
			status = GRBgetintattr(lp, "Status", &solstatus);
			if (solstatus == GRB_OPTIMAL) {
				status = GRBgetdblattrelement(lp, "X", positions[0], Alist);
				status = GRBgetdblattrelement(lp, "X", positions[1], Blist);
			}
			else {
				fprintf(stderr, "Failure to find finite value solution, error %d.\n", status);
				goto TERMINATE;
			}
			printf("New point (%6.6f, %6.6f).\n", Alist[0], Blist[0]);

			// second extreme point
			currentweights[0] = baseweights[0] * 1e-4;
			currentweights[1] = baseweights[1];

			status = GRBsetdblattrelement(lp, "Obj", positions[0], currentweights[0]);
			status = status | GRBsetdblattrelement(lp, "Obj", positions[1], currentweights[1]);
			if (status) {
				fprintf(stderr, "Failure to change objective function, error %d.\n", status);
				goto TERMINATE;
			}
			if ((status = GRBoptimize(lp))) {
				fprintf(stderr, "Failed to optimize LP.\n");
				goto TERMINATE;
			}
			status = GRBgetintattr(lp, "Status", &solstatus);
			if (solstatus == GRB_OPTIMAL) {
				status = GRBgetdblattrelement(lp, "X", positions[0], Alist + 1);
				status = GRBgetdblattrelement(lp, "X", positions[1], Blist + 1);
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
					currentweights[1] = (-(Alist[i + 1] - Alist[i]) / (Blist[i + 1] - Blist[i])) * baseweights[1];
				last_objval = Alist[i] * currentweights[0] + Blist[i] * currentweights[1];
				status = GRBsetdblattrelement(lp, "Obj", positions[0], currentweights[0]);
				status = status | GRBsetdblattrelement(lp, "Obj", positions[1], currentweights[1]);
				if (status) {
					fprintf(stderr, "Failure to change objective function, error %d.\n", status);
					goto TERMINATE;
				}
				if ((status = GRBoptimize(lp))) {
					fprintf(stderr, "Failed to optimize LP.\n");
					goto TERMINATE;
				}
				double objval;
				status = GRBgetintattr(lp, "Status", &solstatus);
				if (solstatus == GRB_OPTIMAL)
					status = GRBgetdblattr(lp, "ObjVal", &objval);
				else
				{
					printf("\nSomething is wrong: Solution status = %d.\n", solstatus);
					goto TERMINATE;
				}

				if (fabs(Alist[i] * currentweights[0] + Blist[i] * currentweights[1] - objval) > TOL_ALG * fabs(last_objval)) {
					// not terminal: insert the extreme point, and then rewind i
					double* Btemplist = (double*)malloc(sizeof(double) * (num_values + 1));
					memcpy(Btemplist, Blist, sizeof(double) * (i + 1));
					status = GRBgetdblattrelement(lp, "X", positions[1], Btemplist + i + 1);
					memcpy(Btemplist + i + 2, Blist + i + 1, sizeof(double) * (num_values - i - 1));
					free(Blist);
					Blist = Btemplist;

					double* Atemplist = (double*)malloc(sizeof(double) * (num_values + 1));
					memcpy(Atemplist, Alist, sizeof(double) * (i + 1));
					status = GRBgetdblattrelement(lp, "X", positions[0], Atemplist + i + 1);
					memcpy(Atemplist + i + 2, Alist + i + 1, sizeof(double) * (num_values - i - 1));
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
	GRBfreemodel(lp);
	GRBfreeenv(env);

	return status;
}
