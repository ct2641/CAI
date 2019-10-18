// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef PDPARSER
#define PDPARSER

#include "problemdes.h"

#define MAXLINELEN 512                     // maximum number of characters per line
#define MAXNUMVARS 64                      // maximum number of random variables in the problem description
#define NUM_KEYWORDS 9                     // number of the keywords we allow in the problem description file:
                                           // see comments for the char strings keywords[]
#define MAXVARNAMELENGTH 32
#define MAXOBJITEMS 32
#define MAXINDPITEMS 32                    // maximum number of random variable groups that are mutually independent


void parsePD(FILE* fp, problem_description* PD);
void initializePD(problem_description* PD);
void freePD(problem_description* PD);

#endif
