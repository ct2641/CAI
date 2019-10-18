// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <stdio.h>
#include <string.h>
#include "../PDParser/PDparser.h"
#include "transcribe2Cplex.h"
#include "../ERfromPD.h"
#include "computes_Cplex.h"


int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Need input configuration file.\n");
		return 0;
	}

	FILE *fp;
	int compute_type = 0;
	if (sizeof(int) < 4) {
		printf("The program is not compatible with legacy 16-bit system.\n");
		return 1;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		printf("Could not open file %s", argv[1]);
		return 1;
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "hull"))
			compute_type = 1;
		else if (!strcmp(argv[2], "random"))
			compute_type = 2;
		else if (!strcmp(argv[2], "prove"))
			compute_type = 3;
		else {
			printf("Unknown computation type. \n");
			fclose(fp);
			return 1;
		}
	}

	problem_description PD;
	initializePD(&PD);
	parsePD(fp, &PD); // memory in PD will be allocated if necessary

	int num_samples = 50000;
	int num_runs = 10;

	switch (compute_type) {
	case 0:
		regular_compute(&PD, CPX_ON);
		break;
	case 1:
		hull_compute(&PD, CPX_OFF);
		break;
	case 2:
		if (argc >= 4) {
			num_samples = atoi(argv[3]);
			if (num_samples >= 1000000000 || num_samples < 10) {
				printf("Number of inequalities in the run out of range.\n");
				num_samples = 50000;
			}
			if (argc >= 5) {
				num_runs = atoi(argv[4]);
				if (num_runs > 500 || num_runs <= 0) {
					printf("Number of runs out of range. Set to default value = %d.\n", num_runs = 10);
				}
			}
		}
		random_selected_constraints_compute(&PD, num_samples, num_runs, CPX_ON);
		break;
	case 3:
		prove_bounds(&PD, CPX_ON);
		break;
	default:
		break;
	}

	freePD(&PD);
	fclose(fp);

	return 0;
}
