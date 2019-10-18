// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "utilities.h"
#include "PDparser.h"

// In the problem description file, there can be 6 groups of inputs.
const char * keywords[] = {
	"random variables",                    // 0: this is the set of random variables
	"additional lp variables",             // 1: optional additional LP variables
	"objective",                           // 2: this is the minimization objective
	"dependency",                          // 3: this is the dependence relation: A,B-->C is written as C: A,B
	"conditional independence",            // 4: conditional independence: A;B|C
	"constant bounds",                     // 5: set certain joint entropy or mutual information to a constant
	"symmetry",                            // 6: symmetry relation: always represented as permutation group
	"bounds to prove",                     // 7: hypothesesed bounds to prove
	"end"                                  // 8: end of the PD file
};

// A few useful function for parsing lines in the problem description file
// Find the next random variable in a string
// Parameters:
//     current_str     :  the string to extract the rvs from
//     num_rvs         :  the total number of rvs found
//     rv_list         :  all rvs found
//     first_find      :  the index of the found rv in rv_list
//     first_find_pos  :  the index in current_str where rv_list[first_find] is
void find_next_rv(char* current_str, int num_rvs, char** rv_list, int* first_find, int* first_find_pos)
{
	// Since our dictionary is small, we'll simply search for each word,
	// pick the longest first match, then eliminate it
	char* find_pt;
	for (int i = 0; i < num_rvs; i++)
		if((find_pt = strstr(current_str, rv_list[i])))
			// find a new word that appears earlier than previously found
			// or find one at the same position: a prefix situation
			if ((find_pt - current_str < *first_find_pos)
					|| ((find_pt - current_str == *first_find_pos) && (strlen(rv_list[i]) > strlen(rv_list[*first_find]))))
			{
				*first_find_pos = (int)(find_pt - current_str);
				*first_find = i;
			}
}

// Extract the random variables in a string, and indicate whether they exist in the var_exist array
void extract_rvs(char* sub_line_part, char* var_exist, int num_rvs, char** rv_list, int label)
{
	char* pos_pt;
	int sub_length;

	int first_find;
	int first_find_pos;

	int first_iteration;

	first_iteration = 1;

	if (!strlen(sub_line_part))
		assert(0);

	pos_pt = sub_line_part;

	// since our dictionary is very small, we'll simply search for each word then eliminate it
	while(!is_blank_line(pos_pt))
	{
		sub_length = (int)strlen(pos_pt);
		first_find = -1; // which word was found
		first_find_pos = sub_length; // position that it is found
		find_next_rv(pos_pt, num_rvs, rv_list, &first_find, &first_find_pos);
		if (first_find >= 0 && ((first_find_pos == 1 && !first_iteration) || (first_find_pos == 0 && first_iteration)))
		{
			if(var_exist[first_find])
			{
				fprintf(stderr,"Found repeated random variables in the expression:\n");
				fprintf(stderr,"%s\n",sub_line_part);
				fprintf(stderr,"Exiting.\n");
				exit(11);
			}

			pos_pt += first_find_pos + strlen(rv_list[first_find]);
			var_exist[first_find] = label;

			first_iteration = 0;
		}

		// cannot find any matching rv expression, but also not empty yet: likely input error
		else
		{
			fprintf(stderr,"Trailing or leading characters or unrecognized random variables in the expression:\n");
			fprintf(stderr,"%s\n", sub_line_part);
			fprintf(stderr,"(Remember that regular RVs and constants are not allowed in dependency lines.)\n");
			fprintf(stderr,"Exiting.\n");
			exit(11);
		}
	}
}

// Function to parse only additional LP variables
int parse_add_LPvar(char* sub_line, int num_LPvars, int num_add_LPvars, char** list_add_LPvars, int pos_tuple[], double value_tuple[])
{
	int sign = 1;
	double coefficient = 1;
	char *pos_pt;
	int first_find_index;
	int first_find_pos;
	int coeff_size;

	// By this point, sub_line should have positive length; this
	// line is just here to check in case we missed something.
	assert(strlen(sub_line));

	// first get the sign and/or coefficient in front
	pos_pt = sub_line;
	if((sign = get_sign(sub_line[0])))
	{
		assert(0);
		pos_pt++;
	}
	else
		sign = 1;
	if(sscanf(pos_pt, "%lf%n", &coefficient, &coeff_size) > 0)
		pos_pt += coeff_size;
	else
		coefficient = 1;

	pos_tuple[0] = -1;

	first_find_index = -1;
	first_find_pos = (int)strlen(pos_pt);
	find_next_rv(pos_pt, num_add_LPvars, list_add_LPvars, &first_find_index, &first_find_pos);

	if(first_find_index == -1 || first_find_pos > 0 || strlen(list_add_LPvars[first_find_index]) != strlen(pos_pt))
	{
		if(first_find_index == -1)
			fprintf(stderr,"Unrecognized LP variable in the following expression:\n");
		else if(first_find_pos > 0)
			fprintf(stderr,"Unrecognized leading characters in the following term:\n");
		else
			fprintf(stderr,"Trailing characters in the following term:\n");
		fprintf(stderr,"%s\n",sub_line);
		fprintf(stderr,"(Remember that regular RVs and constants are not allowed in the objective function or the left-hand side of bounds.)\n");
		fprintf(stderr,"Exiting.\n");
		exit(23);
	}

	pos_tuple[0] = first_find_index + num_LPvars;

	value_tuple[0] = sign * coefficient;

	return 1;
}

// Error message and exit for a bad information term.
void bad_inf_term(char* sub_line, unsigned int exitcode)
{
	fprintf(stderr,"The following part of the objective function should contain 'H(·)', 'H(·|·)', 'I(·;·)', or 'I(·;·|·)':\n");
	fprintf(stderr,"%s\n", sub_line);
	fprintf(stderr,"Exiting.\n");
	exit(exitcode);
}

// Parse mutual information or joint entropy terms
int parse_inf_term(char* sub_line, int num_rvs, char** rv_list, int pos_quadruple[], double value_quadruple[])
{
	int num_values = -1;
	int sign = 1;
	char sub_line_part[MAXLINELEN];
	char* var_exist = (char*)calloc(num_rvs, sizeof(char));
	int* var_pos = (int*)calloc(num_rvs, sizeof(int));
	double coefficient;
	char *pos_pt = NULL;
	int current_pos = 0;

	assert(strlen(sub_line));

	check_valid_inf_term(sub_line);

	// first get the sign and/or coefficient in front
	current_pos = 0;
	if((sign = get_sign(sub_line[0])))
	{
		assert(0);
		current_pos++;
	}
	else
		sign = 1;
	if(sscanf(sub_line + current_pos, "%[^H^I]", sub_line_part) > 0)
	{
		if(!is_only_numeric(sub_line_part))
		{
			fprintf(stderr,"Non-constant coefficient or unexpected characters with the following term:\n");
			fprintf(stderr,"%s\n",sub_line);
			fprintf(stderr,"Exiting.\n");
			exit(8);
		}

		sscanf(sub_line_part, "%lf", &coefficient);
	}
	else
		coefficient = 1;

	// parsing I(): need to process in three steps
	if((pos_pt = strstr(sub_line, "I("))) {
		num_values = 3;
		pos_pt += 2;

		// first parse until ';'
		if(!strstr(pos_pt, ";") || !(sscanf(pos_pt, "%[^;]", sub_line_part) > 0))
			bad_inf_term(sub_line, 10);
		extract_rvs(sub_line_part, var_exist, num_rvs, rv_list, 1);
		pos_pt += strlen(sub_line_part);

		// then parse until '|', if it exists
		if(!strstr(pos_pt, ")") || !(sscanf(pos_pt, "%[^|^)]", sub_line_part) > 0))
			bad_inf_term(sub_line, 10);
		extract_rvs(sub_line_part, var_exist, num_rvs, rv_list, 2);
		pos_pt += strlen(sub_line_part) + 1;

		// finally the other part
		if(!is_blank_line(pos_pt))
		{
			if(!strstr(pos_pt, ")") || !(sscanf(pos_pt, "%[^)]", sub_line_part) > 0))
				bad_inf_term(sub_line, 10);
			extract_rvs(sub_line_part, var_exist, num_rvs, rv_list, 3);
			num_values = 4;
		}

		// now convert it into the position-value form.
		memset(pos_quadruple, 0, 4 * sizeof(int));
		// memset(value_quadruple,0,sizeof(double));
		for (int i = 0; i < num_rvs; i++)
		{
			switch (var_exist[i]) {
				case 1:
					pos_quadruple[0] |= (1 << i);
					pos_quadruple[2] |= (1 << i);
					break;
				case 2:
					pos_quadruple[1] |= (1 << i);
					pos_quadruple[2] |= (1 << i);
					break;
				case 3:
					pos_quadruple[0] |= (1 << i);
					pos_quadruple[1] |= (1 << i);
					pos_quadruple[2] |= (1 << i);
					pos_quadruple[3] |= (1 << i);
					break;
				default:
					break;
			}
		}

		value_quadruple[0] = sign * coefficient;
		value_quadruple[1] = value_quadruple[0];
		value_quadruple[2] = -value_quadruple[0];
		if (num_values == 4)
			value_quadruple[3] = value_quadruple[2];
		else
			value_quadruple[3] = 0;
	}
	// if parsing H(): one or two items
	else if((pos_pt = strstr(sub_line, "H("))){
		num_values = 1;
		pos_pt += 2;

		// parse until '|', if it exists
		if(!strstr(pos_pt, ")") || !(sscanf(pos_pt, "%[^|^)]", sub_line_part) > 0))
			bad_inf_term(sub_line, 10);
		extract_rvs(sub_line_part, var_exist, num_rvs, rv_list, 1);
		pos_pt += strlen(sub_line_part) + 1;

		// finally the other part
		if(!is_blank_line(pos_pt))
		{
			if(!strstr(pos_pt, ")") || !(sscanf(pos_pt, "%[^)]", sub_line_part) > 0))
				bad_inf_term(sub_line, 10);
			extract_rvs(sub_line_part, var_exist, num_rvs, rv_list, 2);
			num_values = 2;
		}

		// now convert it into the position-value form.
		memset(pos_quadruple, 0, 4 * sizeof(int));
		// memset(value_quadruple,0,sizeof(double));
		for (int i = 0; i < num_rvs; i++)
		{
			switch (var_exist[i]) {
				case 1:
					pos_quadruple[0] |= (1 << i);
					break;
				case 2:
					pos_quadruple[0] |= (1 << i);
					pos_quadruple[1] |= (1 << i);
					break;
				default:
					break;
			}
		}
		value_quadruple[0] = sign * coefficient;
		if (num_values == 2)
			value_quadruple[1] = -value_quadruple[0];
		else
			value_quadruple[1] = 0;
	}
	else
	{
		bad_inf_term(sub_line, 9);
		num_values = -1;
	}

	free(var_exist);
	free(var_pos);
	return num_values;
}

// Used to parse the objective line and also to parse bounds
int parse_objective(char* line, int num_rvs, char** rv_list, int num_add_LPvars, char** LPvar_list, int* objective_pos, double* objective_values)
{
	char sub_line[MAXLINELEN];
	int pos_tuple[MAXINDPITEMS];
	double value_tuple[MAXINDPITEMS];

	double *LP_vars = NULL;
	int num_items;
	int line_width;
	int current_pos;
	int sign;


	if(!num_rvs)
		assert(0);

	// Remove whitespace
	squeeze_str(line);

	if(!(line_width = (int)strlen(line)) || (line_width == 1 && line[0]=='\n'))
	{
		fprintf(stderr,"Objective function or bound has length 0 (or contains only a newline) after removing whitespace.\n");
		fprintf(stderr,"Exiting.\n");
		exit(6);
	}

	if(!is_valid_objective_line(line))
	{
		fprintf(stderr,"Unrecognized characters or syntax error in this line:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(6);
	};

	LP_vars = (double*)calloc((1 << num_rvs) + num_add_LPvars, sizeof(double));

	current_pos = 0;

	if((sign = get_sign(*(line + current_pos))))
		current_pos++;
	else
		sign = 1;

	while (current_pos < line_width && sscanf(line + current_pos, "%[^-^+^\n]", sub_line) > 0) {
		current_pos += (int)strlen(sub_line);
		if (strstr(sub_line, "H(") || strstr(sub_line, "I(")) {
			// each sub_line has one information measure term, we'll decompose it into
			// joint entropy terms, i.e., maybe into a maximum of 4 items
			num_items = parse_inf_term(sub_line, num_rvs, rv_list, pos_tuple, value_tuple);

			for (int i = 0; i < num_items; i++)
			{
				value_tuple[i] = sign * value_tuple[i];
				LP_vars[pos_tuple[i]] += value_tuple[i];
			}
		}

		// this should be an additional LP variable
		else if (parse_add_LPvar(sub_line, (1 << num_rvs), num_add_LPvars, LPvar_list, pos_tuple, value_tuple))
			LP_vars[pos_tuple[0]] += value_tuple[0] * sign;

		else
		{
			fprintf(stderr,"Error parsing the objective function line:\n");
			fprintf(stderr,"%s\n",line);
			fprintf(stderr,"Exiting.\n");
			exit(7);
		}

		if (current_pos < line_width) {
			if((sign = get_sign(*(line + current_pos))) || (*(line + current_pos) == '\n'))
				current_pos++;
			else
			{
				fprintf(stderr,"Error parsing the objective function line:\n");
				fprintf(stderr,"%s\n",line);
				fprintf(stderr,"Exiting.\n");
				exit(7);
			}
		}
	}

	if(current_pos < line_width)
	{
		fprintf(stderr,"Finished reading the following objective function line too early:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"You might have two operators in a row or have ended the line with an operator.\n");
		fprintf(stderr,"Exiting.\n");
		exit(6);
	}

	// finally read the LP_var, and use it sparse representation: objective_pos[], objective_values[], and num_items
	int num_items_obj = 0;
	for (int i = 0; i < (1 << num_rvs) + num_add_LPvars; i++) {
		if (fabs(LP_vars[i]) > TOLERANCE_LPGENERATION) {
			objective_pos[num_items_obj] = i;
			objective_values[num_items_obj] = LP_vars[i];
			num_items_obj++;
		}
	}

	free(LP_vars);

	assert(num_items_obj);

	return num_items_obj;
}


/**** Functions to handle different part of problem descriptions ****/


// 3. for dependency relations: one line each call
void parse_dependency(char* line, int num_rvs, char** rv_list, int pos_pair[])
{
	char line_part[MAXLINELEN];
	char* var_exist = (char*)calloc(num_rvs, sizeof(char));
	int* var_pos = (int*)calloc(num_rvs, sizeof(int));
	char *pos_pt = line;

	assert(strlen(line));

	check_valid_dependence_independence(line,1);

	// first parse until ':'
	if (!(sscanf(pos_pt, "%[^:]", line_part) > 0))
	{
		fprintf(stderr,"Problem reading the following line before the ':' character:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(12);
	}
	extract_rvs(line_part, var_exist, num_rvs, rv_list, 1);
	pos_pt += strlen(line_part) + 1;

	// Then parse what that depends on
	if (!(sscanf(pos_pt, "%s", line_part) > 0))
	{
		fprintf(stderr,"Problem reading the following line after the ':' character:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(12);
	}
	extract_rvs(line_part, var_exist, num_rvs, rv_list, 2);

	// now convert it into the position-value form.
	memset(pos_pair, 0, 2 * sizeof(int));
	for (int i = 0; i < num_rvs; i++)
	{
		switch (var_exist[i]) {
			case 1:
				pos_pair[0] = pos_pair[0] | (1 << i);
				break;
			case 2:
				pos_pair[0] = pos_pair[0] | (1 << i);
				pos_pair[1] = pos_pair[1] | (1 << i);
				break;
			default:
				break;
		}
	}

	free(var_exist);
	free(var_pos);
}

// 4. for conditional independence relations: one line each call
int parse_independence(char* line, int num_rvs, char** rv_list, int pos_tuple[], char* type)
{
	int num_values = -1;
	char line_part[MAXLINELEN];
	char* var_exist = (char*)calloc(num_rvs, sizeof(char));
	int* var_pos = (int*)calloc(num_rvs, sizeof(int));
	char *pos_pt = line;
	int label = 1;

	// This means no conditioned term.
	*type = 0;

	squeeze_str(line);

	check_valid_dependence_independence(line,0);

	assert(strlen(line));

	// parse until each ';' or '|'
	while(!is_blank_line(pos_pt))
	{
		if(!(sscanf(pos_pt, "%[^|^;^\n]", line_part) > 0))
		{
			fprintf(stderr,"Error reading the following independence line:\n");
			fprintf(stderr,"%s\n",line);
			fprintf(stderr,"You might have two separators in a row.\n");
			fprintf(stderr,"Exiting.\n");
			exit(13);
		}
		extract_rvs(line_part, var_exist, num_rvs, rv_list, label);
		label++;

		pos_pt += strlen(line_part);
		if (*pos_pt == '|')
			*type = 1; // means has conditional term
		pos_pt++;
	}
	label--;

	if (label - 1 > MAXINDPITEMS) {
		fprintf(stderr,"Too many independent components in the problem definition. Maximum is %d.\n", MAXINDPITEMS);
		fprintf(stderr,"%s\n", line);
		fprintf(stderr,"Exiting.\n");
		exit(13);
	}

	// now convert it into the position-value form.
	memset(pos_tuple, 0, (label + 1) * sizeof(int));
	if (*type)
	{
		for (int i = 0; i < num_rvs; i++)
		{
			if (var_exist[i] > 0)
			{
				if (var_exist[i] == label)
				{
					pos_tuple[var_exist[i] - 1] = pos_tuple[var_exist[i] - 1] | (1 << i);
					for (int j = 0; j < label - 1; j++)
						pos_tuple[j] = pos_tuple[j] | (1 << i);
					pos_tuple[label] = pos_tuple[label] | (1 << i);
				}
				else
				{
					pos_tuple[var_exist[i] - 1] = pos_tuple[var_exist[i] - 1] | (1 << i);
					pos_tuple[label] = pos_tuple[label] | (1 << i);
				}
			}
		}
	}
	else
		for (int i = 0; i < num_rvs; i++)
			if (var_exist[i] > 0)
			{
				pos_tuple[var_exist[i] - 1] = pos_tuple[var_exist[i] - 1] | (1 << i);
				pos_tuple[label] = pos_tuple[label] | (1 << i);
			}
	num_values = label + 1;

	free(var_exist);
	free(var_pos);
	return num_values;
}

// 6. parse symmetry relations: one line each call
void parse_symmetry(char* line, int num_rvs, char** rv_list, int pos_tuple[])
{
	char* pos_pt;
	char* var_exist = (char*)calloc(num_rvs, sizeof(char));
	int current_rv = 0;
	int sub_length;
	int first_find;
	int first_find_pos;

	if(!strlen(line))
		assert(0);

	// This will exit on error.
	check_valid_symmetry(line,num_rvs);

	for (int i = 0; i < num_rvs; i++)
		pos_tuple[i] = -1;

	pos_pt = line;

	// Since our dictionary is very small, we'll simply search for each word then eliminate it.
	while (!is_blank_line(pos_pt))
	{
		sub_length = (int)strlen(pos_pt);

		first_find = -1; // which word was found
		first_find_pos = sub_length; // position that it is found
		find_next_rv(pos_pt, num_rvs, rv_list, &first_find, &first_find_pos);
		if (first_find >= 0) {
			if(var_exist[first_find])
			{
				fprintf(stderr,"Found repeated random variables in the symmetry expression:\n");
				fprintf(stderr,"%s\n",line);
				fprintf(stderr,"Exiting.\n");
				exit(15);
			}

			pos_pt += first_find_pos + strlen(rv_list[first_find]);

			if(*pos_pt != ',' && !is_whitespace(*pos_pt))
			{
				fprintf(stderr,"Trailing Character Error at character '%c' on the following line:\n",*pos_pt);
				fprintf(stderr,"%s\n",line);
				fprintf(stderr,"(Remember that symmetry lines should contain a list of every regular RV, comma-separated, with no written sign or coefficient.)\n");
				fprintf(stderr,"Exiting.\n");
				exit(15);
			}
			pos_pt++;
			pos_tuple[current_rv] = first_find;
			current_rv++;
			var_exist[first_find] = 1;
		}
		else  // cannot find any matching rv expression, but also not empty yet: likely input error
		{
			fprintf(stderr,"Trailing characters or unrecognized random variables in this symmetry line:\n");
			fprintf(stderr,"%s\n", line);
			fprintf(stderr,"Exiting.\n");
			exit(15);
		}
	}

	if(current_rv != num_rvs)
	{
		fprintf(stderr,"The following symmetry line doesn't contain all RVs:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(15);
	}

	free(var_exist);
}

// Check whether the symmetry relation input forms a group.
int check_symmetrygroup(char** permutations, int num_symmtrymaps, int num_rvs)
{
	char sequence[MAXNUMVARS];

	if (num_symmtrymaps == 0)
		return 1;

	// check if the first one is the identity permutation
	for (int j = 0; j < num_rvs; j++)
		if (permutations[0][j] != j)
			return 0;

	for (int i = 1; i < num_symmtrymaps; i++) {// current permutation is i-th row
		for (int j = 0; j < num_rvs; j++)
			if (permutations[i][j] == -1)
				return 0;

		for (int j = 0; j < num_symmtrymaps; j++) {// permute j-th row using i-th row
			for (int k = 0; k < num_rvs; k++)
				sequence[k] = permutations[j][(int)permutations[i][k]];

			// it should become another permutation
			int found = 0;
			for (int jj = 0; jj < num_symmtrymaps; jj++) {
				int k;
				for (k = 0; k < num_rvs; k++)
					if (sequence[k] != permutations[jj][k])
						break;
				if (k == num_rvs) {
					found = 1;
					break;
				}
			}
			if (found != 1)
			{
				fprintf(stderr,"Map %d by %d-th permutation does not produce any valid result.\n", j, i);
				return 0;
			}
		}
	}

	return 1;
}

// Self-explanatory.
void print_symmetryrelations(problem_description* PD, int error)
{
	FILE* stream;

	stream = (error) ? stderr : stdout;

	fprintf(stream,"symmetry permutations:\n");
	for (int i = 0; i < PD->num_symmetrymap; i++) {
		for (int j = 0; j < PD->num_rvs; j++)
			fprintf(stream,"%5d ", PD->list_symmetry[i][j]);
		fprintf(stream,"\n");
	}
}

// Set everything to 0 or NULL.
void initializePD(problem_description* PD)
{
	PD->num_constantbounds = 0;
	PD->num_dependency = 0;
	PD->num_independence = 0;
	PD->num_items_in_objective = 0;
	PD->num_rvs = 0;
	PD->num_symmetrymap = 0;
	PD->num_add_LPvars = 0;
	PD->num_bounds = 0;

	// do not reuse PD structure without releasing existing memory allocation
	PD->list_rvs = NULL;
	PD->list_add_LPvars = NULL;

	PD->list_dependency = NULL;

	PD->list_independence = NULL;
	PD->list_num_items_in_independence = NULL;
	PD->list_type_independence = NULL;

	PD->list_pos_in_objective = NULL;
	PD->list_value_in_objective = NULL;

	PD->list_pos_constantbounds = NULL;
	PD->list_value_constantbounds = NULL;
	PD->list_type_constantbounds = NULL;
	PD->list_num_items_in_constantbounds = NULL;
	PD->list_rhs = NULL;

	PD->list_pos_bounds = NULL;
	PD->list_value_bounds = NULL;
	PD->list_type_bounds = NULL;
	PD->list_num_items_in_bounds = NULL;
	PD->list_rhs_2prove = NULL;

	PD->list_symmetry = NULL;
}

// This prints and exits if there's an error in allocatePD().
void allocate_memory_error()
{
	fprintf(stderr,"Error in allocating memory for problem description. Exiting...\n");
	exit(35);
}

// Make a lot of malloc() calls.
void allocatePD(problem_description* PD)
{
	// initialize random variable list
	if (PD->num_rvs > 0) {
		PD->list_rvs = (char**)malloc(PD->num_rvs * sizeof(char*));
		if (PD->list_rvs == NULL)
			allocate_memory_error();
	}
	if (PD->num_add_LPvars > 0) {
		PD->list_add_LPvars = (char**)malloc(PD->num_add_LPvars * sizeof(char*));
		if (PD->list_add_LPvars == NULL)
			allocate_memory_error();
	}
	// initialize dependence list: this is a 1-D array, where an even-odd pair specifies a dependence relation
	if (PD->num_dependency > 0) {
		PD->list_dependency = (int*)malloc(PD->num_dependency * sizeof(int) * 2);
		if (PD->list_dependency == NULL)
			allocate_memory_error();
	}
	// initialize only list of pointers
	if (PD->num_independence > 0) {
		PD->list_independence = (unsigned int**)malloc(PD->num_independence * sizeof(unsigned int*));
		PD->list_num_items_in_independence = (int*)malloc(PD->num_independence * sizeof(int));
		PD->list_type_independence = (char*)malloc(PD->num_independence * sizeof(char));
		if (PD->list_independence == NULL || PD->list_num_items_in_independence == NULL || PD->list_type_independence == NULL)
			allocate_memory_error();
	}
	if (PD->num_constantbounds > 0) {
		PD->list_pos_constantbounds = (unsigned int**)malloc(PD->num_constantbounds * sizeof(unsigned int*)); // important: this should be set to zero
		PD->list_value_constantbounds = (double**)malloc(PD->num_constantbounds * sizeof(double*));
		PD->list_type_constantbounds = (char*)malloc(PD->num_constantbounds * sizeof(char));
		PD->list_rhs = (double*)malloc(PD->num_constantbounds * sizeof(double));
		PD->list_num_items_in_constantbounds = (int*)malloc(PD->num_constantbounds * sizeof(int));
		if (PD->list_pos_constantbounds == NULL || PD->list_type_constantbounds == NULL || PD->list_rhs == NULL)
			allocate_memory_error();
	}

	if (PD->num_bounds > 0) {
		PD->list_pos_bounds = (unsigned int**)malloc(PD->num_bounds * sizeof(unsigned int*)); // important: this should be set to zero
		PD->list_value_bounds = (double**)malloc(PD->num_bounds * sizeof(double*));
		PD->list_type_bounds = (char*)malloc(PD->num_bounds * sizeof(char));
		PD->list_rhs_2prove = (double*)malloc(PD->num_bounds * sizeof(double));
		PD->list_num_items_in_bounds = (int*)malloc(PD->num_bounds * sizeof(int));
		if (PD->list_pos_bounds == NULL || PD->list_type_bounds == NULL || PD->list_rhs_2prove == NULL)
			allocate_memory_error();
	}

	if (PD->num_symmetrymap > 0) {
		PD->list_symmetry = (char**)malloc(PD->num_symmetrymap * sizeof(char*));
		if (PD->list_symmetry == NULL)
			allocate_memory_error();
	}
}

// Make a lot of free() calls.
void freePD(problem_description* PD)
{
	if (PD->num_rvs > 0) {
		for (int i = 0; i < PD->num_rvs; i++)
			free(PD->list_rvs[i]);
		free(PD->list_rvs);
	}
	if (PD->num_add_LPvars > 0) {
		for (int i = 0; i < PD->num_add_LPvars; i++)
			free(PD->list_add_LPvars[i]);
		free(PD->list_add_LPvars);
	}

	free(PD->list_pos_in_objective);
	free(PD->list_value_in_objective);

	if (PD->num_dependency > 0)
		free(PD->list_dependency);

	if (PD->num_independence > 0) {
		for (int i = 0; i < PD->num_independence; i++)
			free(PD->list_independence[i]);
		free(PD->list_independence);
		free(PD->list_num_items_in_independence);
		free(PD->list_type_independence);
	}

	if (PD->num_symmetrymap > 0) {
		for (int i = 0; i < PD->num_symmetrymap; i++)
			free(PD->list_symmetry[i]);
		free(PD->list_symmetry);
	}

	if (PD->num_constantbounds > 0) {
		for (int i = 0; i < PD->num_constantbounds; i++) {
			free(PD->list_pos_constantbounds[i]);
			free(PD->list_value_constantbounds[i]);
		}
		free(PD->list_pos_constantbounds);
		free(PD->list_value_constantbounds);
		free(PD->list_num_items_in_constantbounds);
		free(PD->list_type_constantbounds);
		free(PD->list_rhs);
	}

	if (PD->num_bounds > 0) {
		for (int i = 0; i < PD->num_bounds; i++) {
			free(PD->list_pos_bounds[i]);
			free(PD->list_value_bounds[i]);
		}
		free(PD->list_pos_bounds);
		free(PD->list_value_bounds);
		free(PD->list_num_items_in_bounds);
		free(PD->list_type_bounds);
		free(PD->list_rhs_2prove);
	}

	PD->list_rvs = NULL;
	PD->list_add_LPvars = NULL;

	PD->list_pos_in_objective = NULL;
	PD->list_value_in_objective = NULL;

	PD->list_dependency = NULL;

	PD->list_type_independence = NULL;
	PD->list_independence = NULL;
	PD->list_num_items_in_independence = NULL;

	PD->list_type_constantbounds = NULL;
	PD->list_pos_constantbounds = NULL;
	PD->list_value_constantbounds = NULL;
	PD->list_rhs = NULL;
	PD->list_num_items_in_constantbounds = NULL;

	PD->list_type_bounds = NULL;
	PD->list_pos_bounds = NULL;
	PD->list_value_bounds = NULL;
	PD->list_rhs_2prove = NULL;
	PD->list_num_items_in_bounds = NULL;

	PD->list_symmetry = NULL;

	PD->num_rvs = 0;
	PD->num_add_LPvars = 0;
	PD->num_items_in_objective = 0;
	PD->num_dependency = 0;
	PD->num_independence = 0;
	PD->num_constantbounds = 0;
	PD->num_symmetrymap = 0;
	PD->num_bounds = 0;
}

// Count random variables.
void count_random_variables(problem_description* PD, int currentkey, char* line, char* line_tmp)
{
	unsigned int current_pos = 0;

	// 0 for regular RVs, 1 for additional RVs.
	if(currentkey != 0 && currentkey != 1)
	{
		fprintf(stderr,"Error: Bad currentkey (%d) passed to count_random_variables().\n",currentkey);
		fprintf(stderr,"Exiting.\n");
		exit(1);
	}

	// Search till next separator and count.
	while (current_pos < strlen(line) && sscanf(line + current_pos, "%[^,^;^\n]", line_tmp) > 0 && !is_blank_line(line_tmp)) {
		current_pos += (int)strlen(line_tmp) + 1;
		if(currentkey == 0)
			PD->num_rvs++;
		else if(currentkey == 1)
			PD->num_add_LPvars++;
	}

	if(current_pos < strlen(line))
	{
		fprintf(stderr,"Finished reading the following line of random variables too early:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"You might have an extra separator.\n");
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}
}

// Function to parse random variables.
// Called for both regular RVs (case 1) and additional LP variables (case 2).
int parse_random_variables(problem_description* PD, int index, char* line, char** rv_arr, int currentkey)
{
	int current_pos;
	int line_width;

	char tmp_rv[MAXLINELEN];

	current_pos = 0;
	line_width = (int)strlen(line);

	// 0 for regular RVs, 1 for additional RVs.
	if(currentkey != 0 && currentkey != 1)
	{
		fprintf(stderr,"Error: Bad currentkey (%d) passed to parse_random_variables().\n",currentkey);
		fprintf(stderr,"Exiting.\n");
		exit(1);
	}

	// Iterate through the line, stopping at each separator.
	while (current_pos < line_width && sscanf(line + current_pos, "%[^;^,^\n]", tmp_rv) > 0) {

		// This line will exit if error
		check_valid_rv(tmp_rv,line);

		strcpy(rv_arr[index],tmp_rv);

		current_pos += (int)strlen(rv_arr[index]) + 1;
		squeeze_str(rv_arr[index]);

		// Regular RVs
		if(currentkey == 0)
		{
			PD->list_rvs[index] = (char*)malloc(sizeof(char)*(strlen(rv_arr[index]) + 1));
			strcpy(PD->list_rvs[index], rv_arr[index]);
		}

		//Additional LP variables
		else
		{
			PD->list_add_LPvars[index] = (char*)malloc(sizeof(char)*(strlen(rv_arr[index]) + 1));
			strcpy(PD->list_add_LPvars[index], rv_arr[index]);
		}

		index++;
	}

	if(current_pos < line_width)
	{
		fprintf(stderr,"Finished reading the following line of random variables too early:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"You might have an extra separator.\n");
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}

	return index;
}

// Read the next line.
// Return whether a line was gotten (1 for true, 0 for false).
int get_line_from_file(char* line, FILE* fp)
{
	unsigned int i;

	if(fgets(line, MAXLINELEN, fp))
	{
		for(i = 0; i < strlen(line); i++)
			if(line[i] == '\r')
				line[i] = ' ';

		// Check whether it got the whole line.
		// Don't force '\n' at end of file....
		if(!strstr(line,"\n") && strlen(line) == MAXLINELEN - 1)
		{
			fprintf(stderr,"The line starting with the following is longer than the maximum of %d characters (including newline):\n",MAXLINELEN);
			fprintf(stderr,"%s\n",line);
			fprintf(stderr,"Exiting.\n");
			exit(1);
		}

		// skip blank lines and comments
		if (is_blank_line(line) || is_comment(line))
			return get_line_from_file(line,fp);

		// will exit if fail
		check_no_carrot(line);

		return 1;
	}
	else
		return 0;
}

// get the keyword index
// return -1 if no keyword
int get_keyword_num(char* line_tmp)
{
	int i;

	for(i=0; i<NUM_KEYWORDS; i++)
		if (strstr(strlower(line_tmp), keywords[i]))
			return i;
	return -1;
}

// The first of two rounds of processing.
// This essentially counts how many of everything there is.
void initial_process_by_keywords(FILE *fp, problem_description* PD)
{
	char line[MAXLINELEN];
	char line_tmp[MAXLINELEN];
	int currentkey = -1;
	int newkey;
	int additional = 0;
	int objectivecount = 0;

	// Read next nonempty, noncomment line from fp and store in variable line.
	while (get_line_from_file(line,fp)) {
		strcpy(line_tmp, line);

		// See if it is a keyword line.
		// If so, we will set the keyword and move to the next line.
		if((newkey = get_keyword_num(line_tmp)) != -1)
		{
			currentkey = newkey;
			if(newkey == 1)
				additional = 1;
			if(newkey != 8)
				continue;
		}

		// Otherwise, it is still in the current group, and we should count accordingly.
		switch (currentkey) {

			case -1:    // No keyword on start.
				fprintf(stderr,"First non-comment line is not a keyword line.\n");
				fprintf(stderr,"Exiting.\n");
				exit(21);

			case 0:     // count random variables
				count_random_variables(PD,currentkey,line,line_tmp);
				break;
			case 1:     // count additional LP variables
				count_random_variables(PD,currentkey,line,line_tmp);
				break;
			case 2:     // objective
				objectivecount++;
				break;
			case 3:     // dependency
				PD->num_dependency++;
				break;
			case 4:     // independence
				PD->num_independence++;
				break;
			case 5:     // constant value bounds
				PD->num_constantbounds++;
				break;
			case 6:     // symmetry
				PD->num_symmetrymap++;
				break;
			case 7:     // bounds to prove
				PD->num_bounds++;
				break;
			case 8:     // end
				if(!PD->num_rvs)
				{
					fprintf(stderr,"You didn't provide any regular random variables.\n");
					fprintf(stderr,"Exiting.\n");
					exit(4);
				}

				if(additional && !PD->num_add_LPvars)
				{
					fprintf(stderr,"You indicated the presence of additional LP random variables but didn't provide any.\n");
					fprintf(stderr,"Exiting.\n");
					exit(4);
				}

				if(objectivecount==0)
				{
					fprintf(stderr,"You don't have an objective function.\n");
					fprintf(stderr,"Exiting.\n");
					exit(4);
				}

				if(objectivecount>1)
				{
					fprintf(stderr,"You have %d objective function lines. You should only have 1.\n",objectivecount);
					fprintf(stderr,"Exiting.\n");
					exit(4);
				}
				return;

			default:
				assert(0);
		}
	}

	fprintf(stderr,"You didn't end the input file with the \"end\" keyword.\n");
	fprintf(stderr,"Exiting.\n");
	exit(5);
}

// Function to parse out the bounds.
// Called for both given bounds (case 5) and bounds to prove (case 7).
int parse_bounds(char* line, int num_rvs, char** rv_list, int num_add_LPvars, char** LPvar_list, int* pos_tuple, double* value_tuple, int* equation_type, double* rhs)
{
	int count;
	char sub_line[MAXLINELEN];
	int num_items;
	int current_pos;

	squeeze_str(line);

	assert(strlen(line));

	check_valid_bounds(line);

	// sub_line must have positive length.
	// sub_line must not be a blank line.
	// There must be a positive number of items found in sub_line by parse_objective.
	if(
			!(sscanf(line, "%[^=^>^<^\n]", sub_line) > 0)
			|| is_blank_line(sub_line)
			|| !(num_items = parse_objective(sub_line, num_rvs, rv_list, num_add_LPvars, LPvar_list, pos_tuple, value_tuple))
	  )
	{
		fprintf(stderr,"Error reading the following line before the comparison sign:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(14);
	}

	current_pos = strlen(sub_line);

	// Find inequality type.
	if (strstr(line + current_pos, ">=") != NULL) {
		*equation_type = 0;
		current_pos += 2;
	}
	else if (strstr(line + current_pos, "<=") != NULL) {
		*equation_type = 1;
		current_pos += 2;
	}
	else if (strstr(line + current_pos, "=") != NULL) {
		*equation_type = 2;
		current_pos++;
	}

	//Must be able to read after comparator.
	if(!(sscanf(line + current_pos, "%lf%n", rhs, &count) > 0))
	{
		fprintf(stderr,"Error reading the following line after the comparison sign:");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"(Remember that the right-hand side must be a constant.)\n");
		fprintf(stderr,"Exiting.\n");
		exit(14);
	}

	current_pos += count;

	// Check for trailing characters.
	if(!is_blank_line(line + current_pos))
	{
		fprintf(stderr,"Trailing character(s) on this bound line (beginning at character '%c'):\n",line[current_pos]);
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(14);
	}

	// If "<=", change it to ">=" for the sake of writing less code.
	if (*equation_type == 1)
	{
		for (int i = 0; i < num_items; i++)
			value_tuple[i] = -value_tuple[i];
		*rhs = -1 * (*rhs);
		*equation_type = 0;
	}

	return num_items;
}

// Second round of processing.
// In the first round we counted how many of everything we had,
// so now we can read everything in.
void process_by_keywords(FILE *fp, problem_description* PD)
{
	char line[MAXLINELEN];                      // local variables
	char line_lwr[MAXLINELEN];
	int currentkey = -1;
	int newkey;
	int num_items;
	char type;

	// indicators to record what elements have been read from the PD file:
	// the first two are mandatory, while the others are optional.
	char PD_elements[NUM_KEYWORDS];

	int pos_tuple[MAXINDPITEMS];
	double value_tuple[MAXINDPITEMS];

	int num_rvs = 0;             // for keyword 0
	int num_items_obj = 0;                           // for keyword 1
	int objective_pos[MAXOBJITEMS * 4];            // to set up objective function: we do not expect it to have a large number of joint entropies
	double objective_values[MAXOBJITEMS * 4];
	int num_dependency = 0;                      // for keyword 2
	int num_independence = 0;                    // for keyword 3
	int num_add_LPvars = 0;       // for keyword 4
	int num_constantbounds = 0, equation_type;   // for keyword 5
	double rhs;
	int num_symmetry = 0;                        // for keyword 6
	int num_bounds = 0;

	// allocate memory to store names of the random variables
	char **rv_list;
	rv_list = (char**)malloc(MAXNUMVARS * sizeof(char*));
	for (int i = 0; i < MAXNUMVARS; i++)
		rv_list[i] = (char*)malloc((MAXVARNAMELENGTH) * sizeof(char));

	char **LPvar_list;
	LPvar_list = (char**)malloc(MAXNUMVARS * sizeof(char*));
	for (int i = 0; i < MAXNUMVARS; i++)
		LPvar_list[i] = (char*)malloc((MAXVARNAMELENGTH) * sizeof(char));

	memset(PD_elements, 0, sizeof(char)*NUM_KEYWORDS);


	// read lines
	while (get_line_from_file(line,fp)) {
		strcpy(line_lwr, line);

		// if this is a new keyword, we need to finish processing for the old keyword
		if ((newkey = get_keyword_num(line_lwr)) != -1) {
			PD_elements[newkey] = 1;    // mark this keyword as 1
			switch (currentkey) {
				case -1: // First stars
					print_stars();
					break;

				case 0: // random variables
					if (num_rvs != PD->num_rvs) {
						fprintf(stderr,"Inconsistency between the two file-processing passes: Number of random variables.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"-The following %d random variables were found:\n", num_rvs);
					for (int i = 0; i < num_rvs; i++) {
						fprintf(stdout,"%s ", rv_list[i]);
					}
					fprintf(stdout,"\n");

					break;

				case 1: // additional LP variables
					if (num_add_LPvars != PD->num_add_LPvars) {
						fprintf(stderr,"Inconsistency between two file-processing passes: Number of additional LP variables.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"---The problem has %d additional LP variables.\n", num_add_LPvars);

					break;
				case 2: // objective functions
					if(!num_items_obj)
					{
						fprintf(stderr,"No objective function.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					PD->num_items_in_objective = num_items_obj;
					PD->list_pos_in_objective = (unsigned  int*)malloc(sizeof(unsigned int)*num_items_obj);
					PD->list_value_in_objective = (double*)malloc(sizeof(double)*num_items_obj);
					memcpy(PD->list_pos_in_objective, objective_pos, sizeof(int)*num_items_obj);
					memcpy(PD->list_value_in_objective, objective_values, sizeof(double)*num_items_obj);

					fprintf(stdout,"---The objective function has %d non-zero terms.\n", num_items_obj);

					break;
				case 3: // dependency
					if (num_dependency != PD->num_dependency) {
						fprintf(stderr,"Inconsistency between two file-processing passes: Number of dependence relations.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"---The problem has %d dependency relations.\n", num_dependency);

					break;
				case 4: // independence
					if (num_independence != PD->num_independence) {
						fprintf(stderr,"Inconsistency between two file-processing passes: Number of independence relations.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"---The problem has %d independence relations.\n", num_independence);

					break;

				case 5: // setting the joint entropy or mutual information to be a constant value
					if (num_constantbounds != PD->num_constantbounds) {
						fprintf(stderr,"Inconsistency between two file-processing passes: Number of constant bounds.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"---The problem has %d constant value bounds.\n", num_constantbounds);

					break;

				case 6: // symmetry
					if (num_symmetry != PD->num_symmetrymap) {
						fprintf(stderr,"Inconsistency between two file-processing passes: Number of symmetry permutations.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"---Permutations in the symmetry relation =  %d.\n", num_symmetry);

					break;

				case 7: // bounds to prove
					if (num_bounds != PD->num_bounds) {
						fprintf(stderr,"Inconsistant between two file processing passes: number of bounds to prove.\n");
						fprintf(stderr,"Exiting.\n");
						exit(20);
					}

					fprintf(stdout,"---Number of bounds to prove =  %d.\n", num_bounds);

					break;

				case 8: // end
					fprintf(stderr,"File contains lines after \"end\".\n");
					fprintf(stderr,"Exiting.\n");
					exit(20);
					break;

				default:
					assert(0);
			}

			// now we set the currentkey to the new keyword and continue
			currentkey = newkey;
			continue;
		}

		// otherwise, it is still in the current group, then we should process the particular line accordingly
		switch (currentkey) {
			case 0:     // parsing random variables
				num_rvs = parse_random_variables(PD, num_rvs, line, rv_list, currentkey);
				break;
			case 1:     // parsing additional LP variables
				num_add_LPvars = parse_random_variables(PD, num_add_LPvars, line, LPvar_list, currentkey);
				break;
			case 2:     // objective function: linear and a vector of entropy, mutual information, and additional LP variables
				squeeze_str(line);
				num_items_obj = parse_objective(line, num_rvs, rv_list, num_add_LPvars, LPvar_list, objective_pos, objective_values);
				break;
			case 3:     // dependency
				squeeze_str(line);
				parse_dependency(line, num_rvs, rv_list, pos_tuple);
				PD->list_dependency[num_dependency * 2] = pos_tuple[0];
				PD->list_dependency[num_dependency * 2 + 1] = pos_tuple[1];
				num_dependency++;
				break;

			case 4:     // independence
				squeeze_str(line);
				num_items = parse_independence(line, num_rvs, rv_list, pos_tuple, &type);
				if (num_items > 1) {
					PD->list_num_items_in_independence[num_independence] = num_items;
					PD->list_independence[num_independence] = (unsigned int*)malloc(sizeof(unsigned int)*num_items);
					memcpy(PD->list_independence[num_independence], pos_tuple, sizeof(int)*num_items);
					PD->list_type_independence[num_independence] = type;
					num_independence++;
				}

				// **** this is only for one line yet TODO: set it into a data structure
				break;

			case 5:     // constant value bounds
				num_items = parse_bounds(line,num_rvs,rv_list,num_add_LPvars,LPvar_list,pos_tuple,value_tuple,&equation_type,&rhs);

				PD->list_type_constantbounds[num_constantbounds] = equation_type;
				PD->list_rhs[num_constantbounds] = rhs;
				PD->list_pos_constantbounds[num_constantbounds] = (unsigned int*)malloc(sizeof(unsigned int)*num_items);
				memcpy(PD->list_pos_constantbounds[num_constantbounds], pos_tuple, num_items * sizeof(int));
				PD->list_value_constantbounds[num_constantbounds] = (double*)malloc(sizeof(double)*num_items);
				memcpy(PD->list_value_constantbounds[num_constantbounds], value_tuple, num_items * sizeof(double));
				PD->list_num_items_in_constantbounds[num_constantbounds] = num_items;
				num_constantbounds++;

				break;

			case 6:     // symmetry
				squeeze_str(line);

				if(!strlen(line))
					assert(0);

				parse_symmetry(line, num_rvs, rv_list, pos_tuple);
				PD->list_symmetry[num_symmetry] = (char*)malloc(sizeof(char)*num_rvs);
				for (int i = 0; i < num_rvs; i++)
					PD->list_symmetry[num_symmetry][i] = pos_tuple[i];
				num_symmetry++;

				break;

			case 7:     // bounds to prove
				num_items = parse_bounds(line,num_rvs,rv_list,num_add_LPvars,LPvar_list,pos_tuple,value_tuple,&equation_type,&rhs);

				PD->list_type_bounds[num_bounds] = equation_type;
				PD->list_rhs_2prove[num_bounds] = rhs;
				PD->list_pos_bounds[num_bounds] = (unsigned int*)malloc(sizeof(unsigned int)*num_items);
				memcpy(PD->list_pos_bounds[num_bounds], pos_tuple, num_items * sizeof(int));
				PD->list_value_bounds[num_bounds] = (double*)malloc(sizeof(double)*num_items);
				memcpy(PD->list_value_bounds[num_bounds], value_tuple, num_items * sizeof(double));
				PD->list_num_items_in_bounds[num_bounds] = num_items;
				num_bounds++;

				break;

			default:
				assert(0);
		}
	}

	// checked for in the first pass
	assert(PD_elements[8]);

	// free memory allocation for the strings
	for (int i = 0; i < MAXNUMVARS; i++)
		free(rv_list[i]);
	free(rv_list);

	for (int i = 0; i < MAXNUMVARS; i++)
		free(LPvar_list[i]);
	free(LPvar_list);

	print_stars();
}

// The function that can be called from drive code
// to actually start the parsing process.
void parsePD(FILE* fp, problem_description* PD)
{
	// The first of two rounds of processing.
	initial_process_by_keywords(fp, PD);

	// Allocate memory. This will exit on error.
	allocatePD(PD);

	// Rewind and do the second round of processing.
	rewind(fp);
	process_by_keywords(fp, PD);

	// Check whether the symmetry relation input forms a group.
	if (!check_symmetrygroup(PD->list_symmetry, PD->num_symmetrymap, PD->num_rvs)) {
		print_symmetryrelations(PD,1);
		fprintf(stderr,"---Permutations appear not to form a valid group.\n");
		exit(36);
	}
}
