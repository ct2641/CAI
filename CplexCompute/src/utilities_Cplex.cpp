// An Open-Source Toolbox for Computer-Aided Investigation
// on the Fundamental Limits of Information Systems, Version 0.1
//
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
//
#include <ilcplex/cplex.h>
#include <math.h>
#include "problem_description.hpp"

// utility function to manipulate CPLEX environment and problems

// set up a default LP environment and a default LP problem, such that we can add constraint and objective functions
// the last two parameters are optional which are for display purpose during the computation: recommend to keep the default
int set_defaultLP(CPXENVptr* p_env, CPXLPptr* p_lp, int num_columns, char* problem_name, CPXINT SCRIND = CPX_ON, CPXINT DATACHECK = CPX_ON)
{
	int status;
	*p_env = CPXopenCPLEX(&status);
	if (*p_env == NULL) {
		char  errmsg[CPXMESSAGEBUFSIZE];
		fprintf(stderr, "Could not open CPLEX environment.\n");
		CPXgeterrorstring(*p_env, status, errmsg);
		fprintf(stderr, "%s", errmsg);
		return(status);
	}
	status = CPXsetintparam(*p_env, CPX_PARAM_DATACHECK, DATACHECK);
	if (status) {
		fprintf(stderr, "Failure to turn on data checking, error %d.\n", status);
		return(status);
	}
	status = CPXsetintparam(*p_env, CPX_PARAM_SCRIND, SCRIND);
	if (status) {
		fprintf(stderr, "Failure to turn on screen indicator, error %d.\n", status);
		return(status);
	}

	/*********************************************************/
	/* A generic LP problem setup--usually no need to change */
	/*********************************************************/
	*p_lp = CPXcreateprob(*p_env, &status, problem_name);
	if (*p_lp == NULL) {
		fprintf(stderr, "Failed to create LP.\n");
		return(status);
	}
	status = CPXnewcols(*p_env, *p_lp, num_columns, NULL, NULL, NULL, NULL, NULL);
	if (status)
		return(status);
	CPXchgobjsen(*p_env, *p_lp, CPX_MIN);  /* Problem is minimization */
	return(status);
	/**********************************************************/
}

int freeLP(CPXENVptr env, CPXLPptr lp)
{
	int status;
	if (lp != NULL) {
		status = CPXfreeprob(env, &lp);
		if (status) {
			fprintf(stderr, "CPXfreeprob failed, error code %d.\n", status);
		}
	}
	return(status);
}

int free_Lp_env(CPXENVptr env)
{
	int status;
	/* Free up the CPLEX environment, if necessary */
	if (env != NULL) {
		status = CPXcloseCPLEX(&env);
		if (status) {
			char errmsg[CPXMESSAGEBUFSIZE];
			fprintf(stderr, "Could not close CPLEX environment.\n");
			CPXgeterrorstring(env, status, errmsg);
			fprintf(stderr, "%s", errmsg);
		}
	}
	return(status);
}

int make_dual(CPXENVptr env, CPXLPptr primalLP, CPXLPptr* p_dualLP, CPXLPptr* p_MIP, char* problemName)
{
	int numrows = CPXgetnumrows(env, primalLP);
	int numcols = CPXgetnumcols(env, primalLP);
	int status;

	*p_dualLP = CPXcreateprob(env, &status, problemName);
	*p_MIP = CPXcreateprob(env, &status, problemName);
	CPXchgobjsen(env, *p_dualLP, CPX_MIN);  /* Problem is minimization */
	CPXchgobjsen(env, *p_MIP, CPX_MIN);  /* Problem is minimization */
	//status = CPXnewrows(env, *p_dualLP, numCols, NULL, NULL, NULL, NULL);

	if (CPXnewrows(env, *p_dualLP, numcols, NULL, NULL, NULL, NULL) | CPXnewrows(env, *p_MIP, numcols, NULL, NULL, NULL, NULL)) {
		printf("Cannnot create dual problem\n");
		return(status);
	}

	int nzcnt = CPXgetnumnz(env, primalLP);
	int* rmatbeg = (int*)malloc(sizeof(int)*numrows);
	int* rmatind = (int*)malloc(sizeof(int)*nzcnt);
	double* rmatval = (double*)malloc(sizeof(double)*nzcnt);
	double* obj = (double*)malloc(numrows * sizeof(double));
	double* rhs = (double*)malloc(numrows * sizeof(double));
	double* compressed_rhs = (double*)malloc(numrows * sizeof(double));
	int* compressed_rhs_pos = (int*)malloc(numrows * sizeof(int));
	int surplus;
	for (int i = 0; i < numrows; i++)
		obj[i] = 1.0;

	status = CPXgetrows(env, primalLP, &nzcnt, rmatbeg, rmatind, rmatval, nzcnt, &surplus, 0, numrows - 1);
	status = status | CPXaddcols(env, *p_dualLP, numrows, nzcnt, obj, rmatbeg, rmatind, rmatval, NULL, NULL, NULL);
	status = status | CPXaddcols(env, *p_MIP, numrows, nzcnt, obj, rmatbeg, rmatind, rmatval, NULL, NULL, NULL);

	// add one more row for the rhs
	status = status | CPXgetrhs(env, primalLP, rhs, 0, numrows - 1);
	nzcnt = 0;
	for (int i = 0; i < numrows; i++) {
		if (fabs(rhs[i]) >= TOLERANCE_LPGENERATION) {
			compressed_rhs[nzcnt] = rhs[i];
			compressed_rhs_pos[nzcnt] = i;
			nzcnt++;
		}
	}
	char sense = 'G';
	rmatbeg[0] = 0;
	status = status | CPXaddrows(env, *p_dualLP, 0, 1, nzcnt, NULL, &sense, rmatbeg, compressed_rhs_pos, compressed_rhs, NULL, NULL);
	status = status | CPXaddrows(env, *p_MIP, 0, 1, nzcnt, NULL, &sense, rmatbeg, compressed_rhs_pos, compressed_rhs, NULL, NULL);

	char* ctype = NULL;
	int num_col = CPXgetnumcols(env, *p_MIP);
	ctype = (char*)malloc(sizeof(char)*num_col);
	memset(ctype, 'I', num_col);
	status = CPXcopyctype(env, *p_MIP, ctype);
	free(ctype);

	if (status) {
		printf("Can not populate columns.\n");
		goto TERMINATE;
	}

TERMINATE:
	free(rmatbeg);
	free(rmatind);
	free(rmatval);
	free(obj);
	free(rhs);
	free(compressed_rhs);
	free(compressed_rhs_pos);
	return(status);
}
