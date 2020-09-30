// An Open-Source Toolbox for Computer-Aided Investigation
// on the Fundamental Limits of Information Systems, Version 0.1
//
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
//
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <math.h>
#include "problem_description.hpp"
#include "ERfromPD.h"
#include "utilities_Gurobi.h"
#include "computes_Gurobi.h"


//#include <crtdbg.h>
//#define _CRTDBG_MAP_ALLOC

using namespace CAI;
typedef std::runtime_error SRE;

int main(int argc, char *argv[])
{
	std::string filename;
	int StartingJsonPos;
	int num_samples;
	int num_runs;


	if (argc < 2) {
		printf("Error: Usage when using %s\n: %s inputfile.pd [regular|hull|random num_samples num_runs|prove|sensitivity] [Optional Modifying JSONS].\n", argv[0], argv[0]);
		return 0;
	}

	if (sizeof(int) < 4) {
		printf("The program is not compatible with legacy 16-bit system.\n");
		return 1;
	}

	int compute_type = 0;
	if (argc > 2) {
		if (!strcmp(argv[2], "regular"))
			compute_type = 0;
		else if (!strcmp(argv[2], "hull"))
			compute_type = 1;
		else if (!strcmp(argv[2], "random"))
			compute_type = 2;
		else if (!strcmp(argv[2], "prove"))
			compute_type = 3;
		else if (!strcmp(argv[2], "sensitivity"))
			compute_type = 4;
		// else assume it is "regular"
	}

	filename = argv[1];

	if(compute_type == 0)
		StartingJsonPos = 2;
	else if (compute_type == 2)
		StartingJsonPos = 5;
	else
		StartingJsonPos = 3;

	try {
		Problem_Description* thePD = new Problem_Description;
		thePD->Get_Cmd_Line_Modifiers(argv,argc,StartingJsonPos);
		Read_Problem_Description_From_File(thePD, filename);
		Problem_Description_C_Style* PD = thePD->As_Problem_Description_C_Style();

		switch (compute_type) {
			case 0:
				regular_compute(thePD, PD);
				break;
			case 1:
				hull_compute(thePD,PD);
				break;
			case 2:
				if(argc < 5)
				{
					printf("Error: Usage when using \"random\":\n%s inputfile.pd random num_samples num_runs [Optional Modifying JSONS]\n",argv[0]);
					exit(2);
				}

				num_samples = atoi(argv[3]);
				if(num_samples >= 1000000000 || num_samples < 10)
					printf("Number of inequalities in the run out of range. Set to default value = %d.\n", num_samples = 50000);

				num_runs = atoi(argv[4]);
				if(num_runs > 500 || num_runs <= 0)
					printf("Number of runs out of range. Set to default value = %d.\n",num_runs = 10);

				random_selected_constraints_compute(thePD, PD, num_samples, num_runs);
				break;
			case 3:
				prove_bounds(thePD,PD);
				break;
			case 4:
				sensitivity_compute(thePD, PD);
				break;
			default:
				break;
		}
	} catch (std::runtime_error &e) {
		printf("Caught a runtime error:\n\n%s\n", e.what());
		printf("\n");
		CAI::print_commands(stdout);
		exit(1);
	}

	return 0;
}
