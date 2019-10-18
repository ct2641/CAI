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
#include "transcribe2Gurobi.h"
#include "../ERfromPD.h"
#include "utilities_Gurobi.h"
#include "computes_Gurobi.h"
#include "gurobi_c.h"

int regular_compute(problem_description* PD)
{
	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	GRBenv* env;
	GRBmodel* lp;
	char* problemname = (char*)"regular";

	int status = set_defaultLP(&env, &lp, ER.num_effetive + 1, problemname);
	if (!status) {
		status = status | populate_Gurobirows_generic(lp, PD, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PD, &ER);
		if (!status) {
			if (!(status = GRBoptimize(lp))) {
				int solstatus;
				status = GRBgetintattr(lp, "Status" ,&solstatus);
				if (solstatus == GRB_OPTIMAL) {
					double objval;
					status=  GRBgetdblattr(lp,"ObjVal", &objval);
					printf("\n******************************************************************\n");
					printf("Optimal value is %6.6f.\n", objval);
					int num_x;
					status = GRBgetintattr(lp, "NumVars", &num_x);
					double* x = (double*)malloc(sizeof(double)*num_x);
					status = status | GRBgetdblattrarray(lp, "X", 0, num_x, x);
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

	GRBfreemodel(lp);
	GRBfreeenv(env);
	freeER(&ER);

	return status;
}

int random_selected_constraints_compute(problem_description* PD, int num_constraints, int num_iter)
{
	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	if((1<<(PD->num_rvs-2))*ER.num_pairs<=num_constraints*2){
		printf("Convert to full set of constraints.\n");
		freeER(&ER);
		regular_compute(PD);
		return 0;
	}

	int status;
	printf("Completed mapping table with %d LP variables and pair selection %d pairs.\n", ER.num_effetive,ER.num_pairs);
	printf("\n******************************************************************\n");

	for (int i=0; i<num_iter; i++){
		GRBenv* env;
		GRBmodel* lp;
		char* problemname=(char*)"random_constraints";

		status = set_defaultLP(&env, &lp, ER.num_effetive+1, problemname);
		if(!status){
			status = status|populate_Gurobirows_random(lp, PD, &ER, num_constraints);
			status = status|add_Gurobirows_problemspecific(lp, PD, &ER);
			if(!status){
				if(!(status=GRBoptimize(lp))){
					int solstatus;
					status = GRBgetintattr(lp, "Status", &solstatus);
					if(solstatus==2){
						double objval;
						status = GRBgetdblattr(lp, "ObjVal", &objval);
						if(fabs(objval)>=1e-3){
							printf("Optimal value is %6.6f in %10d-th iteration.\n",objval,i);
							int num_x;
							status = GRBgetintattr(lp, "NumVars", &num_x);
							double* x = (double*)malloc(sizeof(double)*num_x);
							status = status | GRBgetdblattrarray(lp, "X", 0, num_x, x);
								printf("Values achieving the optimal solution:\n ");
							for (int i = 0; i < PD->num_items_in_objective; i++)
								printf("%f   ", x[ER.forward_mappingtable[PD->list_pos_in_objective[i]]]);
							printf("\n******************************************************************\n");
							free(x);
						}
						else
							printf("%10d-th iteration: optimal solution is all 0.\r",i);
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
	freeER(&ER);
    return status;
}

int prove_bounds(problem_description* PD)
{
	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	GRBenv* env;
	GRBmodel* lp=NULL;
	char* problemname=(char*)"prove_bounds";

	int status = set_defaultLP(&env, &lp, ER.num_effetive + 1, problemname);
	if (!status) {
		status = status | populate_Gurobirows_generic(lp, PD, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PD, &ER);
		GRBmodel* lp_dual = NULL;
		GRBmodel* mip = NULL;
		if (!make_dual(env, lp, &lp_dual, &mip)) {
			int num_rows;
			GRBgetintattr(lp_dual, "NumConstrs", &num_rows);

			for (int i = 0; i < PD->num_bounds; i++) {

				double* rhs = (double*)malloc(sizeof(double)*(PD->list_num_items_in_bounds[i] + 1));
				memcpy(rhs, PD->list_value_bounds[i], PD->list_num_items_in_bounds[i] * sizeof(double));
				int* rhs_pos = (int*)malloc(sizeof(int)*(PD->list_num_items_in_bounds[i] + 1));
				for (int j = 0; j < PD->list_num_items_in_bounds[i]; j++)
					//	rhs_pos[j] = ER.forward_mappingtable[PD->list_pos_bounds[i][j]];
					rhs_pos[j] = PD->list_pos_bounds[i][j];
				int num_effective = map_compress(rhs_pos, rhs, PD->list_num_items_in_bounds[i], &ER);
				for (int j = 0; j < num_effective; j++){
					//double rhs = PD->list_value_bounds[i][j];
					status = GRBsetdblattrelement(lp_dual, "RHS", rhs_pos[j], rhs[j]);
					status = status|GRBsetdblattrelement(mip, "RHS", rhs_pos[j], rhs[j]);
				}
				status = status|GRBsetdblattrelement(lp_dual, "RHS", num_rows-1, PD->list_rhs_2prove[i]);
				status = status|GRBsetdblattrelement(mip, "RHS", num_rows-1, PD->list_rhs_2prove[i]);
				free(rhs);
				free(rhs_pos);

				if((status=(status|GRBoptimize(lp_dual)))){
					fprintf(stderr, "Failed to optimize LP.\n");
					goto TERMINATE;
				}
				int solstatus;
				status = GRBgetintattr(lp_dual, "Status", &solstatus);
				if(solstatus == GRB_OPTIMAL){
					printf("******************************************************************\n");
					double objval;
					status = GRBgetdblattr(lp_dual, "ObjVal", &objval);
					printf("LP dual value %f \n", objval);
					printf("Proved %d-th inequality.\n", i);
					print_Gurobiinequalities(lp, lp_dual, PD, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\nSomething is wrong: Solution status = %d.\n", status);

				//solve again to make it integers
				if((status=GRBoptimize(mip))){
					fprintf(stderr, "Failed to optimize MIP.\n");
					goto TERMINATE;
				}
				status = GRBgetintattr(mip, "Status", &solstatus);
				if(solstatus == GRB_OPTIMAL){
					printf("******************************************************************\n");
					double objval;
					status = GRBgetdblattr(mip, "ObjVal", &objval);
					printf("MIP dual value %f \n", objval);
					printf("Proved %d-th inequality using integer values.\n", i);
					print_Gurobiinequalities(lp,mip, PD, &ER);
					printf("******************************************************************\n");
				}
				else
					printf("\n Could not prove the inequality using integer values.\n");

				//clean up right hand side, since otherwise the next bound might be messed up.
				for (int j = 0; j < PD->list_num_items_in_bounds[i]; j++) {
					status = GRBsetdblattrelement(lp_dual, "RHS", ER.forward_mappingtable[PD->list_pos_bounds[i][j]], 0);
					status = status | GRBsetdblattrelement(mip, "RHS", ER.forward_mappingtable[PD->list_pos_bounds[i][j]], 0);
				}
				status = status | GRBsetdblattrelement(lp_dual, "RHS", num_rows - 1, 0);
				status = status | GRBsetdblattrelement(mip, "RHS", num_rows - 1, 0);
			}
		}
		if(lp_dual!=NULL){
			GRBfreemodel(lp_dual);
			GRBfreemodel(mip);
		}
	}

TERMINATE:
	if(lp!=NULL)
		GRBfreemodel(lp);
	GRBfreeenv(env);
	freeER(&ER);
	return status;
}


int hull_compute(problem_description* PD)
{
	if(PD->num_items_in_objective!=2){
		printf("Hull computation can only be done for a problem with two items in the objective function.\n");
		return 1;
	}
	int num_values = 2;

	extracted_reductions ER;
	initializeER(&ER);
	make_mappingtable(PD, &ER);
	makepairs(PD, &ER);

	GRBenv* env;
	GRBmodel* lp = NULL;
	char* problemname=(char*)"hull";

	double* Alist = (double*)malloc(sizeof(double)*2);
	double* Blist = (double*)malloc(sizeof(double)*2);

	int status = set_defaultLP(&env, &lp, ER.num_effetive+1, problemname);
	if(!status){
		status = status | populate_Gurobirows_generic(lp, PD, &ER);
		status = status | add_Gurobirows_problemspecific(lp, PD, &ER);
		if(!status){
			double baseweights[2];
			int positions[2];
			baseweights[0] = (PD->list_value_in_objective[0]>0)?1.0:-1.0;
			baseweights[1] = (PD->list_value_in_objective[1]>0)?1.0:-1.0;
			positions[0] = ER.forward_mappingtable[PD->list_pos_in_objective[0]];
			positions[1] = ER.forward_mappingtable[PD->list_pos_in_objective[1]];
			double currentweights[2];
			// first extreme point
			currentweights[0] = baseweights[0];
			currentweights[1] = baseweights[1]*1e-4;

			status = GRBsetdblattrelement(lp, "Obj", positions[0], currentweights[0]);
			status = status|GRBsetdblattrelement(lp, "Obj", positions[1], currentweights[1]);
			if(status){
				fprintf(stderr, "Failure to change objective function, error %d.\n", status);
				goto TERMINATE;
			}
			if((status = GRBoptimize(lp))){
				fprintf(stderr, "Failed to optimize LP.\n");
				goto TERMINATE;
			}
			int solstatus;
			status = GRBgetintattr(lp, "Status", &solstatus);
			if (solstatus == GRB_OPTIMAL) {
				status = GRBgetdblattrelement(lp, "X", positions[0], Alist);
				status = GRBgetdblattrelement(lp, "X", positions[1], Blist);
			}
			else{
				fprintf(stderr, "Failure to find finite value solution, error %d.\n", status);
				goto TERMINATE;
			}
			printf("New point (%6.6f, %6.6f).\n",Alist[0],Blist[0]);

			// second extreme point
			currentweights[0] = baseweights[0]*1e-4;
			currentweights[1] = baseweights[1];

			status = GRBsetdblattrelement(lp, "Obj", positions[0], currentweights[0]);
			status = status | GRBsetdblattrelement(lp, "Obj", positions[1], currentweights[1]);
			if(status){
				fprintf(stderr, "Failure to change objective function, error %d.\n", status);
				goto TERMINATE;
			}
			if((status = GRBoptimize(lp))){
				fprintf(stderr, "Failed to optimize LP.\n");
				goto TERMINATE;
			}
			status = GRBgetintattr(lp, "Status", &solstatus);
			if (solstatus == GRB_OPTIMAL) {
				status = GRBgetdblattrelement(lp, "X", positions[0], Alist+1);
				status = GRBgetdblattrelement(lp, "X", positions[1], Blist+1);
			}
			else{
				fprintf(stderr, "Failure to find finite value solution, error %d.\n", status);
				goto TERMINATE;
			}
			printf("New point (%6.6f, %6.6f).\n",Alist[1],Blist[1]);

			double last_objval;
			// find intermediate points
			for(int i=0;i<num_values-1;i++){
				currentweights[0] = baseweights[0];
				if(fabs(Blist[i+1]-Blist[i])>1e-6)
					currentweights[1] = (-(Alist[i+1]-Alist[i])/(Blist[i+1]-Blist[i]))*baseweights[1];
				last_objval = Alist[i]*currentweights[0]+Blist[i]*currentweights[1];
				status = GRBsetdblattrelement(lp, "Obj", positions[0], currentweights[0]);
				status = status | GRBsetdblattrelement(lp, "Obj", positions[1], currentweights[1]);
				if(status){
					fprintf(stderr, "Failure to change objective function, error %d.\n", status);
					goto TERMINATE;
				}
				if((status = GRBoptimize(lp))){
					fprintf(stderr, "Failed to optimize LP.\n");
					goto TERMINATE;
				}
				double objval;
				status = GRBgetintattr(lp, "Status", &solstatus);
				if (solstatus == GRB_OPTIMAL)
					status = GRBgetdblattr(lp, "ObjVal", &objval);

				if(fabs(Alist[i]*currentweights[0]+Blist[i]*currentweights[1]-objval)>TOL_ALG*fabs(last_objval)){
					// not terminal: insert the extreme point, and then rewind i
					double* Btemplist = (double*)malloc(sizeof(double)*(num_values+1));
					memcpy(Btemplist, Blist, sizeof(double)*(i+1));
					status = GRBgetdblattrelement(lp, "X", positions[1], Btemplist + i+1);
					memcpy(Btemplist+i+2,Blist+i+1,sizeof(double)*(num_values-i-1));
					free(Blist);
					Blist = Btemplist;

					double* Atemplist = (double*)malloc(sizeof(double)*(num_values+1));
					memcpy(Atemplist, Alist, sizeof(double)*(i+1));
					status = GRBgetdblattrelement(lp, "X", positions[0], Atemplist + i + 1);
					memcpy(Atemplist+i+2,Alist+i+1,sizeof(double)*(num_values-i-1));
					free(Alist);
					Alist = Atemplist;
					printf("New point (%6.6f, %6.6f).\n",Alist[i+1],Blist[i+1]);
					num_values++;
					i--;
					last_objval = objval;
				}
			}
			printf("\n\nList of found points on the hull:\n");
			for(int i=0; i<num_values; i++)
				printf("(%6.6f, %6.6f).\n",Alist[i],Blist[i]);
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
	freeER(&ER);

    return status;
}
