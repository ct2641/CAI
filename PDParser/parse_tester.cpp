// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PDparser.h"

int main(int argc, char** argv)
{
	FILE *fp;
	char* fn;
	char* filename;
	char* dir;

	if (sizeof(int) < 4) {
		fprintf(stderr,"The program is not compatible with legacy 16-bit systems.\n");
		fprintf(stderr,"Exiting.\n");
		return 1;
	}


	// This code block forces the default file to be stresstests/PD.txt
	// or stresstests/[argv[1]] if argv[1] is provided and doesn't include '/'.
	fn = (argc == 1) ? (char*)"PD.txt" : argv[1];
	dir = (char*)"stresstests";
	if(!strstr(fn,"/"))
	{
		filename = (char*) calloc(strlen(fn)+strlen(dir)+2,1);
		strcat(filename,dir);
		strcat(filename,"/");
		strcat(filename,fn);
	}
	else
	{
		filename = (char*) calloc(strlen(fn)+1,1);
		strcat(filename,fn);
	}


	/*
	// Uncomment this code block to force input file name on cmd line.
	if(argc < 2)
	{
		fprintf(stderr,"No input file.\n");
		fprintf(stderr,"Exiting.\n");
		return 1;
	}
	filename = argv[1];
	*/

	fp = fopen(filename, "r");

	if (fp == NULL) {
		fprintf(stderr,"Could not open file %s\n", filename);
		fprintf(stderr,"Exiting.\n");
		return 1;
	}

	problem_description PD;

	// Memory in PD will be allocated if necessary in parsePD().
	initializePD(&PD);
	parsePD(fp, &PD);
	freePD(&PD);

	fclose(fp);
	free(filename);

	return 0;
}
