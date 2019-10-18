// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include "utilities.h"
#include "PDparser.h"

// A simple function to compute the factorial of integer n.
unsigned int factorial(unsigned int n)
{
	unsigned int ret = 1;
	for (unsigned int i = 2; i <= n; ++i)
		ret *= i;
	return ret;
}

// A simple function to compute the integer power of n^k.
int intPower(int n, int k)
{
	int ret = 1;
	for (int i = 0; i < k; ++i)
		ret *= n;
	return ret;
}

// Code for timing: copy and paste from elsewhere.
// This allows for cross-platform timing function to a good level of precision.

#ifdef _WIN32
#include <Windows.h>
double get_wall_time() {
	LARGE_INTEGER time, freq;
	if (!QueryPerformanceFrequency(&freq)) {
		// TODO - Handle error
		return 0;
	}
	if (!QueryPerformanceCounter(&time)) {
		// TODO - Handle error
		return 0;
	}
	return (double)time.QuadPart / freq.QuadPart;
}
double get_cpu_time() {
	FILETIME a, b, c, d;
	if (GetProcessTimes(GetCurrentProcess(), &a, &b, &c, &d) != 0) {
		//  Returns total user time.
		//  Can be tweaked to include kernel times as well.
		return
			(double)(d.dwLowDateTime |
					((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
	}
	else {
		// TODO - Handle error
		return 0;
	}
}
#define inline __inline

//  Posix/Linux
#else
#include <sys/time.h>
double get_wall_time() {
	struct timeval time;
	if (gettimeofday(&time, NULL)) {
		// TODO - Handle error
		return 0;
	}
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time() {
	return (double)clock() / CLOCKS_PER_SEC;
}
#endif



// The following are a set of utilities for string manipulation.

// Returns 1 if line contains "//", else 0
int is_comment(char* line)
{
	if (strstr(line, "//"))
		return 1;
	else
		return 0;
}

// Returns 1 if c is ',', ';', or '|', else 0.
int is_rv_separator(char c)
{
	return (c == ',' || c == ';' || c == '|');
}

// Returns 1 if c is '\t' or ' ', else 0.
int is_tab_or_space(char c)
{
	return (c == '\t' || c == ' ');
}

// Returns 1 if c is '\t', ' ', '\n', '\r', or '\0', else 0.
int is_whitespace(char c)
{
	return (c == '\t' || c == ' ' || c == '\n' || c == '\r' || c == '\0');
}

// Returns 1 if s only contains whitespace characters.
int is_only_whitespace(char* s)
{
	for(unsigned int i = 0; i < strlen(s); i++)
		if(!is_whitespace(s[i]))
			return 0;
	return 1;
}

// Returns 1 if line is empty or only tabs and spaces and a newline, else 0.
// The '\r' is for compatability with text files made in Windows.
int is_blank_line(char* line)
{
	int i = 0;
	while (line[i] != '\n' && line[i] != '\0' && line[i] != '\r') {
		if (!is_tab_or_space(line[i]))
			return 0;
		i++;
	}
	return 1;
}

// Returns 1 if c is a letter, else 0.
int is_alpha(char c)
{
	return ((c>='a' && c<='z') || (c>='A' && c<='Z'));
}

// Returns 1 if c is a digit, else 0.
int is_numeric(char c)
{
	return (c>='0' && c<='9');
}

// Returns 1 if all characters are 0-9 until '\0', else 0.
int is_only_numeric(char* line)
{
	for(unsigned int i=0; i<strlen(line); i++)
		if(!is_numeric(line[i]))
			return 0;
	return 1;
}

// Returns 1 if s only contains whitespace characters, letters, and digits, else 0.
int is_alphanumeric_whitespace(char* s)
{
	for(unsigned int i = 0; i < strlen(s); i++)
		if(!(is_whitespace(s[i]) || (s[i]>='0' && s[i]<='9') || (s[i]>='a' && s[i]<='z') || (s[i]>='A' && s[i]<='Z')))
			return 0;
	return 1;
}

// Returns 0 if there is a whitespace character between the first
// and the last nonwhitespace character, else 1.
// Example:
//     is_whitespace_only_on_ends("\t\t hello \0") returns 1.
//     is_whitespace_only_on_ends("\t\t hello world \0") returns 0.
int is_whitespace_only_on_ends(char* s)
{
	unsigned int i;

	// Iterate until the first nonwhitespace character.
	for(i=0; i<strlen(s); i++)
		if(!(is_whitespace(s[i])))
			break;

	// Iterate until the next whitespace character.
	for(i++; i<strlen(s); i++)
		if(is_whitespace(s[i]))
			break;

	// Iterate to the end of the string.
	// If there's a nonwhitespace character, then that means there's
	// whitespace in the middle of the string.
	for(i++; i<strlen(s); i++)
		if(!(is_whitespace(s[i])))
			return 0;

	return 1;
}

// Take out the blank spaces in a string in-place.
void squeeze_str(char* str)
{
	char *pt_new;
	pt_new = str;
	for (unsigned int i = 0; i < strlen(str); i++) {
		if (!is_tab_or_space(*(str+i))) {
			*pt_new = *(str + i);
			pt_new++;
		}
	}
	*pt_new = '\0';
}

// A function to free and null a pointer.
void free_and_null(char **ptr)
{
	if (*ptr != NULL) {
		free(*ptr);
		*ptr = NULL;
	}
}

// Exit if the line contains a '^'.
// There should never be a '^' because of the file specifications,
// and not checking specifically for this was messing with some
// of the sscanf() calls for some reason.
void check_no_carrot(char* line)
{
	if(strstr(line,"^"))
	{
		fprintf(stderr,"The following line contains a '^', which isn't allowed:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}
}

// Exit if rv isn't a valid rv string
void check_valid_rv(char* rv, char* line)
{
	// RVs have to have size in [1,MAXVARNAMELENGTH].
	if(!strlen(rv) || (strlen(rv) >= MAXVARNAMELENGTH))
	{
		fprintf(stderr,"Random variable \"%s\" has length %lu from the following line:\n",rv,(unsigned long)strlen(rv));
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Random variable length must be in [1,%u].\n",MAXVARNAMELENGTH);
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}

	// RVs have to have some nonwhitespace characters.
	if(is_only_whitespace(rv))
	{
		fprintf(stderr,"Random variable \"%s\" is only whitespace on the following line:\n",rv);
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}

	// RVs have to be alphanumeric.
	if(!(is_alphanumeric_whitespace(rv)))
	{
		fprintf(stderr,"Random variable \"%s\" is not alphanumeric on the following line. Valid names are [A-Za-z0-9]+\n",rv);
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}

	// RVs shouldn't contain whitespace.
	if(!(is_whitespace_only_on_ends(rv)))
	{
		fprintf(stderr,"Random variable \"%s\" contains whitespace within its name on the following line:\n",rv);
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(3);
	}
}

// Cross-platform compatible function "equivalent" to Windows strlwr().
// Variable str remains unchanged.
// Returns a string equivalent to str but with all capital letters
// made lowercase; all other characters are left as they are.
char* strlower(char* str)
{
	unsigned int i;
	unsigned int len;
	char* rv;

	len = strlen(str) + 1;
	rv = (char*) malloc(len);

	for(i=0; i<len; i++)
		rv[i] = (str[i]>='A' && str[i]<='Z') ? str[i] + ('a' - 'A') : str[i];

	return rv;
}

// Print the string of asterisks.
void print_stars()
{
	fprintf(stdout,"******************************************************************\n");
}

// Return 1 if '+', -1 if '-', and 0 else.
int get_sign(char c)
{
	if(c == '-')
		return -1;
	if(c == '+')
		return 1;
	return 0;
}

// Return the amount of times c appears in line.
int count_char(char* line, char c)
{
	unsigned int count = 0;
	unsigned int i;

	for(i = 0; i < strlen(line); i++)
		if(line[i] == c)
			count++;
	return count;
}

// Return 1 if ',', ';', ':', or '|', else 0.
int is_separator(char c)
{
	return (c==',' || c==';' || c==':' || c=='|');
}

// Returns 1 if only contains characters from a valid objective line, else 0.
int is_valid_objective_line(char* s)
{
	unsigned int i;
	unsigned int len = strlen(s);
	int last_sign = -1;

	for(i = 0; i < len; i++)
		if(!(is_whitespace(s[i]) || is_separator(s[i]) || is_numeric(s[i]) || s[i]=='.' || is_alpha(s[i]) || get_sign(s[i]) || s[i]=='(' || s[i]==')'))
			return 0;
		else if(get_sign(s[i]))
			last_sign = i;

	// Make sure this line doesn't end on a '+' or '-'.
	if(last_sign != -1 && is_blank_line(s + last_sign + 1))
		return 0;

	return 1;
}

// Error message for bad H(·) or I(·) terms.
void print_wrong_character_inf_term_and_exit(char* term, char badchar)
{
	fprintf(stderr,"Error at character '%c' in the following term:\n",badchar);
	fprintf(stderr,"%s\n",term);
	fprintf(stderr,"Where Z is 1 or more comma-separated RVs and A is 2 or more semicolon-separated RVs,\n");
	fprintf(stderr,"    H(·) terms should be H(Z) or H(Z|Z)\n");
	fprintf(stderr,"    I(·) terms should be I(A) or I(A|Z)\n");
	fprintf(stderr,"Exiting.\n");
	exit(8);
}

// Exit if invalid inf term.
void check_valid_inf_term(char* term)
{
	char* ptr;
	char* it;
	char type;
	int seen_pipe;
	int seen_semicolon;

	// There should be exactly 1 '(' and exactly 1 ')'.
	if(count_char(term,'(') != 1 || count_char(term,')') != 1)
	{
		fprintf(stderr,"The following term doesn't contain only a single '(' and a single ')':\n");
		fprintf(stderr,"%s\n",term);
		fprintf(stderr,"This could be caused by a '+' or '-' in the middle of the term.\n");
		fprintf(stderr,"Exiting.\n");
		exit(8);
	}

	// "()" isn't a valid substring because there's nothing inside it....
	if(strstr(term,"()"))
	{
		fprintf(stderr,"Empty parameter list in the following term:\n");
		fprintf(stderr,"%s\n",term);
		fprintf(stderr,"Exiting.\n");
		exit(8);
	}

	// Parse type. The code shouldn't ever reach the assert(0)
	// statement, but it's there in case we missed something.
	if((ptr = strstr(term,"I(")))
		type = 'I';
	else if((ptr = strstr(term,"H(")))
		type = 'H';
	else
		assert(0);

	// Iterate from the start of the term till the 'H' or 'I'.
	// This will check the coefficient.
	for(it = term; *it != type; ++it)
		if(!is_numeric(*it) && (*it) != '.')
		{
			fprintf(stderr,"The following term has a nonnumeric coefficient:\n");
			fprintf(stderr,"%s\n",term);
			fprintf(stderr,"Exiting.\n");
			exit(8);
		}

	// *it should now be 'I' or 'H', so it[1] should be '('.
	if(it[1] != '(')
	{
		fprintf(stderr,"'%c' without '(':\n",type);
		fprintf(stderr,"%s\n",term);
		fprintf(stderr,"Exiting.\n");
		exit(8);
	}


	seen_pipe = 0;
	seen_semicolon = 0;
	// Set the iterator to inside the parentheses.
	for(it += 2; *it != ')'; ++it)
	{
		// H(·) terms shouldn't have ';'.
		// I(·) terms shouldn't have ';' after '|'.
		if(*it == ';' && (type == 'H' || seen_pipe))
			print_wrong_character_inf_term_and_exit(term,*it);

		// This is an I(·) term before a '|'.
		else if(*it == ';')
			seen_semicolon = 1;

		// There shouldn't ever be 2 '|'.
		else if(*it == '|' && seen_pipe)
			print_wrong_character_inf_term_and_exit(term,*it);

		// This is the first time seeing the '|'.
		else if(*it == '|')
			seen_pipe = 1;

		// I(·) terms can only have ',' after '|'.
		else if(*it == ',' && type == 'I' && !seen_pipe)
			print_wrong_character_inf_term_and_exit(term,*it);

		// If it gets to this point, it's not ';' or '|'.
		// It must be numeric, alpha, ',', or ';'.
		else if(!is_numeric(*it) && !is_alpha(*it) && !(*it == ',') && !(*it == ';'))
			print_wrong_character_inf_term_and_exit(term,*it);
	}

	// I(·) terms require ';'.
	if(type == 'I' && !seen_semicolon)
		print_wrong_character_inf_term_and_exit(term,' ');

	// *it should be ')', and the next character should be '\0'.
	if(*(++it))
	{
		fprintf(stderr,"Character '%c' after the following term's closing parenthesis:\n",*it);
		fprintf(stderr,"%s\n",term);
		fprintf(stderr,"Exiting.\n");
		exit(8);
	}
}

// Error message for bad dependence and independence lines.
void print_wrong_character_dep_ind_and_exit(char* line,char badchar)
{
	fprintf(stderr,"Error at character '%c' in the following line:\n",badchar);
	fprintf(stderr,"%s\n",line);
	fprintf(stderr,"Where Z is 1 or more comma-separated RVs and A is 2 or more semicolon-separated RVs,\n");
	fprintf(stderr,"    Dependence lines should be Z:Z\n");
	fprintf(stderr,"    Independence lines should be A or A|Z\n");
	fprintf(stderr,"Exiting.\n");
	exit(25);
}

// Exit if invalid dependence or independence.
void check_valid_dependence_independence(char* line, int dependence)
{
	unsigned int i;
	char c;
	int seen_char;
	int count;

	// If dependence, the separator is ':'.
	// If independence, the separator is '|'.
	c = (dependence) ? ':' : '|';

	// Dependence lines need exactly 1 ':'.
	// Independence lines need exactly 0 or 1 '|'.
	if((count = count_char(line,c)) != 1 && (dependence || count))
	{
		if(dependence)
			fprintf(stderr,"There's not exactly 1 '%c' in the following dependence expression:\n",c);
		else
			fprintf(stderr,"There's not exactly 0 or 1 '%c' in the following independence expression:\n",c);
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(24);
	}

	// Can't begin or end dependence or independence lines with the separator.
	if(line[0] == c || line[strlen(line) - 1] == c || (line[strlen(line) - 1] == '\n' && line[strlen(line) - 2] == c))
	{
		fprintf(stderr,"Can't begin or end a%sdependence line with %c.\n",(dependence) ? " " : "n in",c);
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(24);
	}

	seen_char = 0;
	for(i=0; i<strlen(line); i++)
		if(line[i] == c)
			seen_char = 1;
		else if(line[i] == ',' && !(dependence || seen_char))
			print_wrong_character_dep_ind_and_exit(line,line[i]);
		else if(line[i] == ';' && (dependence || seen_char))
			print_wrong_character_dep_ind_and_exit(line,line[i]);
		else if(!is_alpha(line[i]) && !is_numeric(line[i]) && !is_whitespace(line[i]) && line[i] != ',' && line[i] != ';')
			print_wrong_character_dep_ind_and_exit(line,line[i]);
}

// Exit if invalid bound line.
void check_valid_bounds(char* line)
{
	int count_compares;
	unsigned int i;

	// There should be either "<=", ">=", or "=".
	// There should be exactly 1 "=".
	// The combined number of '<' and '>' should be 0 or 1.
	// If there is a '<' or a '>', there should be a '=' directly following it.
	if(
			(!strstr(line,"<=") && !strstr(line,">=") && !strstr(line,"="))
			|| count_char(line,'=') != 1
			|| (count_compares = count_char(line,'<') + count_char(line,'>')) > 1
			|| (count_compares == 1 && !strstr(line,"<=") && !strstr(line,">="))
	  )
	{
		fprintf(stderr,"Bound doesn't contain exactly 1 of \"<=\" \">=\" or \"=\" on the following line:\n");
		fprintf(stderr,"%s\n",line);
		fprintf(stderr,"Exiting.\n");
		exit(26);
	}

	// See if-statement to see what's allowed.
	for(i=0; i<strlen(line); i++)
		if(!is_alpha(line[i]) && !is_numeric(line[i]) && line[i] != '.' && !is_whitespace(line[i]) && line[i] != '<' && line[i] != '>' && line[i] != '=' && line[i] != '-' && line[i] != '+' && line[i] != '(' && line[i] != ')' && line[i] != ',' && line[i] != '|' && line[i] != ';')
		{
			fprintf(stderr,"Error at character '%c' on the following bound line:\n",line[i]);
			fprintf(stderr,"%s\n",line);
			fprintf(stderr,"Exiting.\n");
			exit(26);
		}
}

// Error message for bad symmetry lines.
void print_symmetry_error_and_exit(char* line,char badchar)
{
	if(badchar == 1)
		fprintf(stderr,"You don't have exactly (num_rvs - 1) commas.\n");
	else
		fprintf(stderr,"Error at character '%c' in the following symmetry line:\n",badchar);
	fprintf(stderr,"%s\n",line);
	fprintf(stderr,"(Remember that symmetry lines should contain a list of every regular RV, comma-separated, with no written sign or coefficient.)\n");
	fprintf(stderr,"Exiting.\n");
	exit(27);
}

// Exit if invalid symmetry line.
void check_valid_symmetry(char* line,int num_rvs)
{
	unsigned int i;

	// Symmetry lines should contain a comma-separated list of every rv.
	if(count_char(line,',') != num_rvs - 1)
		print_symmetry_error_and_exit(line,1);

	for(i=0; i<strlen(line); i++)
	{
		// Must be valid rv character or ','.
		if(!is_alpha(line[i]) && !is_numeric(line[i]) && line[i] != ',' && line[i] != '\n' && line[i] != '\r')
			print_symmetry_error_and_exit(line,line[i]);
		// RVs can't start with a number.
		else if(is_numeric(line[i]) && (i == 0 || line[i-1] == ','))
			print_symmetry_error_and_exit(line,line[i]);
	}
}
