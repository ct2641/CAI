

// This header file is divided into 5 "PART"s.


// Note that these three lines are all that's necessary
// to read in a PD and get the C-style struct:
//
// Problem_Description* thePD = new Problem_Description;
// Read_Problem_Description_From_File(thePD, filename);
// Problem_Description_C_Style* PD = thePD->As_Problem_Description_C_Style();
//
// Now PD can be sent to the Gurobi or Cplex code.


#pragma once
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include "nlohmann/json.hpp"

#ifndef PROBLEM_DESCRIPTION
#define PROBLEM_DESCRIPTION

/************ START PART 1 ************/

// These #defines are for compatibility
// with the ERfromPD code and with the
// C-style gurobi and cplex linking
// code.

#define MAXNUMVARS_C_STYLE 31
#define MAXINDPITEMS_C_STYLE 32
#define TOLERANCE_LPGENERATION (1e-14)

/************* END PART 1 *************/


namespace CAI
{

/************ START PART 2 ************/

// This PART is the struct from the
// old C-style PD (now renamed to
// "Problem_Description_C_Style")
// and the associated functions to
// initialize, free, and print it.
// Note that this is all within the
// CAI namespace.


// Problem_Description_C_Style
typedef struct
{
	// RVs are self-explanatory
	int num_rvs;
	char** list_rvs;

	// ALs are self-explanatory
	int num_add_LPvars;
	char** list_add_LPvars;

	// Objective
	// list_pos and list_value are both have size num_items.
	// list_pos[i] and list_value[i] correspond.
	// list_value[i] is the coefficient for the term represented by list_pos[i].
	// for all i < (1 << numrvs), list_pos[i] represents H(set represented by the bits of i)
	//     Coeff*H(setA|setB) is represented equivalently as Coeff*H(setA U setB) - Coeff*H(setB)
	// for all i >= (1 << numrvs), i represents AL with index (i - (1 << numrvs))
	// num_items is the number of distince sets and ALs ended up with
	int num_items_in_objective;
	unsigned int* list_pos_in_objective;
	double* list_value_in_objective;

	// Dependencies
	// list_dependency is an array of size 2*num_dependency.
	// For each pair, [2*i] is a bit list of all vars in Dependent_Vars and Given_Vars
	// and [2*i+1] is a bit list of all vars in Given_Vars only
	int num_dependency;
	int* list_dependency;

	// Independences
	// list_independence, list_type_independence, and list_num_items_in_independence are all size num_independence
	// list_independence[i] has size list_num_items_in_independence[i]
	//
	//     if type is (int)0, size is n+1.
	//         The first n are (1 << index) of corresponding var.
	//         list[n] is all those ORd together.
	//     if type is (int)1, size is n+2.
	//         The first n are (1 << index) of corresponding var ORd together with list[n].
	//         list[n] is (1 << index) for ALL the "given" list.
	//         list[n+1] is all those ORd together.
	int num_independence;
	unsigned int** list_independence;
	char* list_type_independence;
	int* list_num_items_in_independence;

	// Constant Bounds
	// All 5 lists are size num_constantbounds
	// numitems is as in the objective function
	// list_pos is as in the objective function
	// list_value is as in the objective function
	// list_type is (int)0 for >= and (int)2 for =
	//     all <= are changed to >= with negation
	// list_rhs is just a constant
	int num_constantbounds;
	int* list_num_items_in_constantbounds;
	unsigned int** list_pos_constantbounds;
	double** list_value_constantbounds;
	char* list_type_constantbounds;
	double* list_rhs;

	// Bounds to Prove
	// All 5 lists are size num_constantbounds
	// numitems is as in the objective function
	// list_pos is as in the objective function
	// list_value is as in the objective function
	// list_type is (int)0 for >= and (int)2 for =
	//     all <= are changed to >= with negation
	// list_rhs is just a constant
	int num_bounds;
	int* list_num_items_in_bounds;
	unsigned int** list_pos_bounds;
	double** list_value_bounds;
	char* list_type_bounds;
	double* list_rhs_2prove;

	// Symmetries
	// Each row is a permutation of indices
	int num_symmetrymap;
	char** list_symmetry;

	// Sensitivities to analyze:
	// All 3 lists are size num_sensitivity
	// numitems is as in the objective function
	// list_pos is as in the objective function
	// list_value is as in the objective function
	int num_sensitivities;
	int* list_num_items_in_sensitivities;
	unsigned int** list_pos_sensitivities;
	double** list_value_sensitivities;

	// Expressiones to query the values in the optimal solutions:
	// The lists is size num__LPvariables_To_Query
	// list_pos is as in the objective function
	int num_queries;
	int* list_num_items_in_queries;
	unsigned int** list_pos_queries;
	double** list_value_queries;

	// Equivalence classes
	std:: vector < std::vector <std::string> > equivalences;

} Problem_Description_C_Style;

// Sets everything to either 0 or NULL.
void Initialize_Problem_Description_C_Style(Problem_Description_C_Style* PD);

// Frees everything and sets it all back to either 0 or NULL.
void Free_Problem_Description_C_Style(Problem_Description_C_Style* PD);

// This function only exists to print out all variables
// in the Problem_Description_C_Style struct. It's
// meaningless except to check the current state of the
// struct and to compare two different instances.
void Print_Problem_Description_C_Style(Problem_Description_C_Style*);


/************* END PART 2 *************/




/************ START PART 3 ************/

// The classes in this part are
// all the classes used within
// the Problem_Description class.


class Variable {
	public:
		std::string Name;

		// 'R' for Random, 'L' for Additional LP
		char RL;

		// Used to check that the permutations form a group.
		unsigned int Index;

		void From_Json(const nlohmann::json &j);
		nlohmann::json As_Json() const;
};

class Term
{
	public:
		// Operation is in {"+", "-", "H", "I", "C", "V"}.
		// "+" for a + sign,
		// "-" for a - sign,
		// "H" for an entropy term,
		// "I" for a mutual information term,
		// "C" for a constant term,
		// "V" for a variable term.
		std::string Operation;

		//used if Operation is "H", "I", or "V"
		double Coefficient;

		//used if Operation is "H"
		std::vector<Variable*> Entropy_Of;

		//used if Operation is "H" or "I"
		std::vector<Variable*> Given_Vars;

		//used if Operation is "I"
		//Information_Vars[0] is left of the ;
		//Information_Vars[1] is right of the ;
		std::vector<std::vector<Variable*> > Information_Vars;

		//used if Operation is "V"
		Variable* Var;

		//used if Operation is "C"
		double Constant;


		Term();
		Term(Term* t);

		void From_Json(const nlohmann::json& j, const std::map<std::string,Variable*>& All_Variables_M);
		nlohmann::json As_Json() const;
};

class Expression {
	public:
		std::vector<Term*> Terms;

		Expression();
		Expression(Expression*);

		void From_Json(const nlohmann::json& j, std::vector<Term*>& All_Terms_V, const std::map<std::string,Variable*>& All_Variables_M);
		nlohmann::json As_Json() const;
		Expression* Form_For_Printing();
};

class Dependency {
	public:
		std::vector <Variable *> Dependent_Vars;
		std::vector <Variable *> Given_Vars;

		void From_Json(const nlohmann::json &j, const std::map<std::string, Variable*>&  All_Variables_M);
		nlohmann::json As_Json() const;
};

class Independence {
	public:
		std::vector <std::vector<Variable *> > Independent_Vars_Vecs;
		std::vector <Variable *> Given_Vars;

		void From_Json(const nlohmann::json &j, const std::map<std::string, Variable*>& All_Variables_M);
		nlohmann::json As_Json() const;
};

class Bound {
	public:
		// 'C' for constant bound. 'P' for "to prove". 'S' for sensitivity
		char cp;

		// "=", "<=", or ">=".
		std::string Type;

		Expression *LHS;
		double RHS;

		Bound();

		void From_Json(const nlohmann::json& j, std::vector<Term*>& All_Terms_V, const std::map<std::string,Variable*>& All_Variables_M);
		nlohmann::json As_Json() const;
		Bound* Form_For_Printing() const;
};

class Symmetry {
	public:
		std::vector <Variable *> Order;
		std::string Key;

		void From_Json(const nlohmann::json &j, const std::map<std::string, Variable*>& RandomM);
		nlohmann::json As_Json() const;
};

class Cmd_Line_Modifier
{
	public:
		// Action is '+' or 'R' (replace)
		std::string Key;
		char Action;
		nlohmann::json j;

		Cmd_Line_Modifier();
		Cmd_Line_Modifier(std::string k, char a, nlohmann::json jsn);
};


/************* END PART 3 *************/



/************ START PART 4 ************/



class Problem_Description {
	private:
		const std::string IllegalBeginningChars_Variables = "0123456789";
		const std::string LegalChars_Variables = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

		void Delete_Everything();

	public:

		Problem_Description();
		~Problem_Description();
		Problem_Description(const Problem_Description &) = delete;
		Problem_Description& operator= (const Problem_Description &d) = delete;

		void Clear();

		std::vector <Variable *>            All_Variables_V;
		std::map <std::string, Variable *>  All_Variables_M;
		std::vector <Expression *>          All_Expression_V;
		std::vector <Bound *>               All_Bounds_V;
		std::vector <Term *>                All_Terms_V;

		std::vector <Variable *>            RandomV;
		std::map <std::string, Variable *>  RandomM;
		std::vector <Variable *>            AdditionalV;
		std::map <std::string, Variable *>  AdditionalM;
		Expression*                         Objective;
		std::vector <Dependency *>          Dependencies;
		std::vector <Independence *>        Independences;
		std::vector <Bound *>               Constant_Bounds;
		std::vector <Bound *>               Bounds_To_Prove;
		std::vector <Symmetry *>            Symmetries;
		std::map <std::string, Symmetry *>  SymmetryM;
		std::vector <std::vector <std::string> >    Equivalences2d;
		std::vector <Expression*>			Sensitivities;            // new function added in Ver1
		std::vector <Expression*>			Queries;                  // new functions added in Ver1

		int                                 LP_display;

		std::vector <Cmd_Line_Modifier*>    Cmd_Line_Modifiers;

		// Error checking functions. These functions will
		// throw exceptions if they encounter errors.
		void Check_Valid_Variable_Name(const std::string& s);
		void Check_Valid_Expression(Expression* exp);
		void Check_Equivalence() const;
		void Check_Symmetry_Group() const;
		void Check_First_Symmetry_Matches_RV_List();

		// Json
		void From_Json(const nlohmann::json &j);
		nlohmann::json As_Json();

		// While reading keys
		void Key_RV(const nlohmann::json& j);
		void Key_AL(const nlohmann::json& j);
		void Key_O(const nlohmann::json& j);
		void Key_D(const nlohmann::json& j);
		void Key_I(const nlohmann::json& j);
		void Key_S(const nlohmann::json& j);
		void Key_BC(const nlohmann::json& j);
		void Key_BP(const nlohmann::json& j);
		void Key_EQ(const nlohmann::json& j);
		void Key_QU(const nlohmann::json& j);
		void Key_SE(const nlohmann::json& j);

		void Get_Cmd_Line_Modifiers(char** argv, int argc, int startindex);
		void Modify_Json_From_Cmd_Line_Before_From_Json(nlohmann::json& j);
		void Run_Cmd_Line_Modifier_Commands();

		// C-Style
		Problem_Description_C_Style* As_Problem_Description_C_Style();
		void Check_Valid_For_C_Style();
		void Print_Old_Format(std::ostream& fout);
};



/************* END PART 4 *************/



/************ START PART 5 ************/

// These functions are the original
// reading in of Problem_Description.

int Get_Command(std::istream& Input_Stream, Problem_Description* pd);
void Command_SER(Problem_Description* pd, std::vector<std::string>& sv, int old_style);
void Command_DESER(Problem_Description* pd, std::vector<std::string>& sv);
bool read_json(const std::vector <std::string> &sv, size_t starting_field, nlohmann::json &rv, std::istream& Input_Stream);
void print_commands(FILE *f);
void Read_Problem_Description_From_File(Problem_Description* pd, std::string filename);


/************* END PART 5 *************/

};
// end namespace CAI


#endif
