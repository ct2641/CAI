// An Open-Source Toolbox for Computer-Aided Investigation 
// on the Fundamental Limits of Information Systems, Version 0.1
// 
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
// 
#ifndef UTILITY
#define UTILITY

#include <string.h>

#define TOLERANCE_LPGENERATION (1e-14)

// A simple function to compute the factorial of integer n
unsigned int factorial(unsigned int n);

// A simple function to compute the integer power of n^k.
int intPower(int n, int k);

// code for timing: copy and paste from elsewhere
// it allows for cross-platform timing function to a good precision
// Different definitions depending on Windows vs Posix
double get_cpu_time();
double get_wall_time();

// Returns 1 if line contains "//", else 0
int is_comment(char* line);

// Returns 1 if line is empty or only tabs and spaces and a newline, else 0
int is_blank_line(char* line);

// Returns 1 if c is ',', ';', or '|', else 0
int is_rv_separator(char c);

// Returns 1 if c is '\t' or ' ', else 0
int is_tab_or_space(char c);

// Returns 1 if c is '\t', ' ', '\n', or '\0', else 0
int is_whitespace(char c);

// Returns 1 if only contains characters from a valid objective line, else 0
int is_valid_objective_line(char* line);

// Returns 1 if all characters are 0-9 until '\0', else 0.
int is_only_numeric(char* line);

// Take out the blank spaces in a string in-place
void squeeze_str(char* str);

// Exit if invalid inf term
void check_valid_inf_term(char* term);

// Exit if invalid dependence or independence
void check_valid_dependence_independence(char* line, int dependence);

// Exit if invalid bound line
void check_valid_bounds(char* line);

// Exit if invalid symmetry line
void check_valid_symmetry(char* line,int num_rvs);


// Exit if rv isn't a valid rv string
void check_valid_rv(char* rv, char* line);

// Exit if the line contains a '^'.
void check_no_carrot(char* line);

// A function to free and null a pointer
void free_and_null(char **ptr);

// Print the string of asterisks
void print_stars();

// Return 1 if '+', -1 if '-', and 0 else
int get_sign(char c);

// Return the amount of times c appears in line
int count_char(char* line, char c);

//Cross-platform compatible function equivalent to Windows strlwr()
char* strlower(char* str);

#endif
