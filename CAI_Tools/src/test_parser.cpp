#include "problem_description.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstdio>

using namespace std;
using nlohmann::json;
using namespace CAI;


int main(int argc, char **argv)
{
	string prompt;
	Problem_Description *pd;

	if (argc > 2 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
		fprintf(stderr, "usage: test_app [prompt]\n");
		fprintf(stderr, "\n");
		print_commands(stderr);
		exit(1);
	}

	if (argc == 2) {
		prompt = argv[1];
		prompt += " ";
	}

	pd = new Problem_Description;

	while (1) {
		if (prompt != "")
			printf("%s", prompt.c_str());
		if (!Get_Command(cin,pd))
		{
			pd->Run_Cmd_Line_Modifier_Commands();
			break;
		}
	}

	delete pd;

	return 0;
}
