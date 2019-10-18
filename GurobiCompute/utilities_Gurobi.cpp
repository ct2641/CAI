// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <math.h>
#include <stdlib.h>
#include "gurobi_c.h"
#include "../PDParser/utilities.h"
// utility function to manipulate CPLEX environment and problems

// set up a default LP environment and a default LP problem, such that we can add constraint and objective functions
// the last two parameters are optional which are for display purpose during the computation: recommend to keep the default
int set_defaultLP(GRBenv** p_env, GRBmodel** p_lp, int num_columns, char* problem_name)
{
	int status;
	status = GRBloadenv(p_env, "gurobisolver_log.txt");
	status = status|GRBstartenv(*p_env);
	if(status){
		fprintf(stderr, "Could not open Gurobi environment.\n");
		fprintf(stderr, "%s", GRBgeterrormsg(*p_env));
		return(status);
	}

	/*********************************************************/
	/* A generic LP problem setup--usually no need to change */
	/*********************************************************/
	if(GRBnewmodel(*p_env, p_lp, problem_name, num_columns, NULL, NULL, NULL, NULL,NULL)){
		fprintf(stderr, "Failed to create LP.\n");
		return(status);
	}
	if (GRBsetintattr(*p_lp, "ModelSense", 1)) {
		fprintf(stderr, "Failed to set LP objective sense.\n");
		return(status);
	}

	return(status);

	/**********************************************************/
}

int make_dual(GRBenv* env, GRBmodel* primallp, GRBmodel** p_duallp, GRBmodel** p_mip)
{
	int status;
	int numrows, numcols, nzcnt;
	int num_col_mip;
	status = GRBgetintattr(primallp, "NumConstrs", &numrows);
	status = status|GRBgetintattr(primallp, "NumVars", &numcols);

	if (GRBnewmodel(env, p_duallp, "dual_lp", 0, NULL, NULL, NULL, NULL, NULL)) {
		fprintf(stderr, "Failed to create LP.\n");
		return(status);
	}
	if (GRBsetintattr(*p_duallp, "ModelSense", 1)) {
		fprintf(stderr, "Failed to set LP objective sense.\n");
		return(status);
	}

	if (GRBnewmodel(env, p_mip, "mip" , 0, NULL, NULL, NULL, NULL, NULL)) {
		fprintf(stderr, "Failed to create LP.\n");
		return(status);
	}
	if (GRBsetintattr(*p_mip, "ModelSense", 1)) {
		fprintf(stderr, "Failed to set LP objective sense.\n");
		return(status);
	}

	char* sense = (char*)malloc(sizeof(char)*numcols);
	memset(sense, GRB_EQUAL, sizeof(char)*numcols);

	// add new empty rows: not sure if this works.
	status = GRBaddconstrs(*p_duallp, numcols, 0, NULL, NULL, NULL, sense, NULL, NULL) | GRBaddconstrs(*p_mip, numcols, 0, NULL, NULL, NULL, sense, NULL, NULL);
	if(status){
		printf("Cannnot create dual model.\n");
		free(sense);
		return(status);
	}
	free(sense);

	GRBgetintattr(primallp, "NumNZs", &nzcnt);
	int* rmatbeg=(int*)malloc(sizeof(int)*numrows);
	int* rmatind=(int*)malloc(sizeof(int)*nzcnt);
	double* rmatval=(double*)malloc(sizeof(double)*nzcnt);
	double* obj=(double*)malloc(numrows*sizeof(double));
	double* rhs=(double*)malloc(numrows*sizeof(double));
	double* compressed_rhs=(double*)malloc(numrows*sizeof(double));
	int* compressed_rhs_pos=(int*)malloc(numrows*sizeof(int));
	//int surplus;
	for(int i=0;i<numrows;i++)
		obj[i] = 1.0;

	status = GRBgetintattr(*p_duallp, "NumVars", &num_col_mip);
	status = GRBgetintattr(*p_mip, "NumVars", &num_col_mip);

	status = GRBgetconstrs(primallp, &nzcnt, rmatbeg, rmatind, rmatval, 0, numrows);
	status = status|GRBaddvars(*p_duallp, numrows, nzcnt, rmatbeg, rmatind, rmatval, obj, NULL, NULL, NULL,NULL);
	char* ctype = NULL;
	num_col_mip = numrows;
	ctype = (char*)malloc(sizeof(char)*num_col_mip);
	memset(ctype, 'I', num_col_mip);
	//status = GRBsetcharattrarray(*p_mip, "VType", 0, num_col_mip, ctype);
	status = status|GRBaddvars(*p_mip, numrows, nzcnt, rmatbeg, rmatind, rmatval, obj, NULL, NULL, ctype, NULL);

	// add one more row for the rhs
	status = status | GRBgetdblattrarray(primallp, "RHS", 0, numrows, rhs);
	nzcnt = 0;
	for (int i = 0; i < numrows; i++) {
		if (fabs(rhs[i]) >= TOLERANCE_LPGENERATION) {
			compressed_rhs[nzcnt] = rhs[i];
			compressed_rhs_pos[nzcnt] = i;
			nzcnt++;
		}
	}

	status = status | GRBaddconstr(*p_duallp, nzcnt, compressed_rhs_pos, compressed_rhs, GRB_GREATER_EQUAL, 0, NULL);
	status = status | GRBaddconstr(*p_mip, nzcnt, compressed_rhs_pos, compressed_rhs, GRB_GREATER_EQUAL, 0, NULL);

	if (status)
		printf("Can not populate dual columns.\n");

	GRBupdatemodel(*p_duallp);
	GRBupdatemodel(*p_mip);

	//status = GRBgetintattr(*p_duallp, "NumVars", &num_col_mip);
	//status = GRBgetintattr(*p_mip, "NumVars", &num_col_mip);

	free(ctype);
	free(rmatbeg);
	free(rmatind);
	free(rmatval);
	free(obj);
	free(rhs);
	free(compressed_rhs);
	free(compressed_rhs_pos);
	return(status);
}
