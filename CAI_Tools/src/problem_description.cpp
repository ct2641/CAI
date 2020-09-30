#include "problem_description.hpp"
#include "expression_helpers.hpp"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <string>
#include <map>
#include <set>
#include "nlohmann/json.hpp"
#include <iostream>

#pragma warning(disable: 4996)

using namespace std;
using nlohmann::json;
using namespace CAI;

typedef std::runtime_error SRE;

// Need to initialize Coefficient
Term::Term()
{
	Operation = "";
	Coefficient = 1;
	Var = NULL;
	Constant = 0;
	Information_Vars.resize(2);
}
Term::Term(Term* t)
{
	unsigned int i,j;

	Operation = t->Operation;
	Coefficient = t->Coefficient;

	Information_Vars.resize(2);

	for(i = 0; i < t->Entropy_Of.size(); i++)
		Entropy_Of.push_back(t->Entropy_Of[i]);
	for(i = 0; i < t->Given_Vars.size(); i++)
		Given_Vars.push_back(t->Given_Vars[i]);
	for(i = 0; i < t->Information_Vars.size(); i++)
		for(j = 0; j < t->Information_Vars[i].size(); j++)
			Information_Vars[i].push_back(t->Information_Vars[i][j]);

	Var = t->Var;
	Constant = t->Constant;
}

Bound::Bound()
{
	cp = '\0';
	LHS = NULL;
	Type = "";
	RHS = 0;
}

Bound* Bound::Form_For_Printing() const
{
	Bound* b;
	unsigned int i;

	// This will have a memory leak;

	b = new Bound;

	b->cp = cp;
	b->Type = Type;
	b->RHS = RHS;
	b->LHS = new Expression(LHS);

	if(b->LHS->Terms.size() && b->LHS->Terms.front()->Coefficient < 0)
	{
		for(i = 0; i < b->LHS->Terms.size(); i += 2)
			b->LHS->Terms[i]->Coefficient *= -1;

		if(b->Type == "<=")
			b->Type = ">=";
		else if(b->Type ==">=")
			b->Type = "<=";

		if(b->RHS != 0)
			b->RHS *= -1;
	}
	b->LHS = b->LHS->Form_For_Printing();

	return b;
}

Cmd_Line_Modifier::Cmd_Line_Modifier()
{
}

Cmd_Line_Modifier::Cmd_Line_Modifier(std::string k, char a, nlohmann::json jsn)
{
	Key = k;
	Action = a;
	j = jsn;
}

// Keeping this minimal
Problem_Description::Problem_Description()
{
	Clear();
	Objective = NULL;
	LP_display = 0; //false
}

// Utility function to call delete on every element of a vector.
template<typename T>
void Delete_Vector_Contents(vector<T>& vec)
{
	unsigned int i;

	for(i = 0; i < vec.size(); ++i)
		delete vec[i];
}

// Delete all the things.
void Problem_Description::Delete_Everything()
{
	Delete_Vector_Contents(All_Variables_V);
	Delete_Vector_Contents(All_Terms_V);
	Delete_Vector_Contents(All_Expression_V);
	Delete_Vector_Contents(All_Bounds_V);
	Delete_Vector_Contents(Dependencies);
	Delete_Vector_Contents(Independences);
	Delete_Vector_Contents(Symmetries);
	Delete_Vector_Contents(Cmd_Line_Modifiers);

	// These two are already deleted in All_Expression_V
	/* Delete_Vector_Contents(Queries); */
	/* Delete_Vector_Contents(Sensitivities); */
}

// Just call Delete_Everything()
Problem_Description::~Problem_Description()
{
	Delete_Everything();
}

// Delete, then clear
void Problem_Description::Clear()
{
	Delete_Everything();

	All_Variables_V.clear();
	All_Variables_M.clear();
	All_Expression_V.clear();
	All_Bounds_V.clear();
	All_Terms_V.clear();

	RandomV.clear();
	RandomM.clear();
	AdditionalV.clear();
	AdditionalM.clear();
	Dependencies.clear();
	Independences.clear();
	Constant_Bounds.clear();
	Bounds_To_Prove.clear();
	Symmetries.clear();
	Queries.clear();
	Sensitivities.clear();
	Equivalences2d.clear();
	Cmd_Line_Modifiers.clear();
}

// Check whether the symmetry relation input forms a group.

void Problem_Description::Check_Symmetry_Group() const
{
	unsigned int i, j, k;
	char buf[20];
	string key;
	nlohmann::json ej;
	vector <int> sequence;

	if (Symmetries.size())
	{
		sequence.resize(RandomV.size());

		// Current permutation is the ith row.
		for(i = 1; i < Symmetries.size(); i++)
		{
			/* cout << i << endl; */
			// Permute jth row using ith row.
			for(j = 0; j < Symmetries.size(); j++)
			{
				key = "";
				for(k = 0; k < RandomV.size(); k++) {
					sequence[k] = Symmetries[j]->Order[Symmetries[i]->Order[k]->Index]->Index;
					sprintf(buf, "%d ", sequence[k]);
					key += buf;
				}

				if (SymmetryM.find(key) == SymmetryM.end()) {
					ej = json::array();
					for (k = 0; k < sequence.size(); k++) {
						ej.push_back(RandomV[sequence[k]]->Name);
					}
					throw SRE((string) "Bad Symmetry -- missing permutation " + ej.dump());
				}
			}
		}
	}

	fprintf(stdout,"Symmetries have been successfully checked.\n");
}

// Variables must be alphanumeric and can't start with a numeral
void Problem_Description::Check_Valid_Variable_Name(const std::string& s)
{
	if(!s.size())
		throw SRE((string) "Problem_Description::Is_Valid_Variable_Name(): Variable name has size zero.");

	if(s.find_first_not_of(LegalChars_Variables) != std::string::npos)
		throw SRE((string) "Problem_Description::Is_Valid_Variable_Name(): Variable name isn't alphanumeric. s = \"" + s + (string) "\"");

	if(IllegalBeginningChars_Variables.find(s[0]) != std::string::npos)
		throw SRE((string) "Problem_Description::Is_Valid_Variable_Name(): Variable name starts with an illegal starting (but otherwise legal) character. s = \"" + s + (string) "\"");

	if(All_Variables_M.count(s))
		throw SRE((string) "Problem_Description::Is_Valid_Variable_Name(): Variable \"" + s + (string) "\" already exists.");
}


nlohmann::json Variable::As_Json() const
{
	nlohmann::json j;
	j = Name;
	return j;
}

nlohmann::json Term::As_Json() const
{
	nlohmann::json j;
	nlohmann::json j2;
	unsigned int i;

	if(Operation == "+" || Operation == "-")
	{
		j = Operation;
		return j;
	}
	else if(Operation == "C")
	{
		j = Constant;
		return j;
	}
	else if(Operation == "V")
	{
		if(Coefficient == 1)
		{
			j = Var->As_Json();
			return j;
		}
		else
		{
			j = nlohmann::json::array();
			j.push_back(Coefficient);
			j.push_back(Var->As_Json());
			return j;
		}
	}
	else if(Operation == "H")
	{
		j = nlohmann::json::array();

		j.push_back("H");

		j2 = nlohmann::json::array();
		for(i = 0; i < Entropy_Of.size(); i++)
			j2.push_back(Entropy_Of[i]->As_Json());
		j.push_back(j2);

		j2 = nlohmann::json::array();
		for(i = 0; i < Given_Vars.size(); i++)
			j2.push_back(Given_Vars[i]->As_Json());
		j.push_back(j2);

		if(Coefficient == 1)
			return j;
		else
		{
			j2 = nlohmann::json::array();
			j2.push_back(Coefficient);
			j2.push_back(j);
			return j2;
		}
	}
	else if(Operation == "I")
	{
		j = nlohmann::json::array();

		j.push_back("I");

		j2 = nlohmann::json::array();
		for(i = 0; i < Information_Vars.at(0).size(); i++)
			j2.push_back(Information_Vars.at(0).at(i)->As_Json());
		j.push_back(j2);

		j2 = nlohmann::json::array();
		for(i = 0; i < Information_Vars.at(1).size(); i++)
			j2.push_back(Information_Vars.at(1).at(i)->As_Json());
		j.push_back(j2);

		j2 = nlohmann::json::array();
		for(i = 0; i < Given_Vars.size(); i++)
			j2.push_back(Given_Vars.at(i)->As_Json());
		j.push_back(j2);

		if(Coefficient == 1)
			return j;
		else
		{
			j2 = nlohmann::json::array();
			j2.push_back(Coefficient);
			j2.push_back(j);
			return j2;
		}
	}
	else
		throw SRE((string) "Term::As_Json(): Somehow a Term happened to have an operation not in {\"+\",\"-\",\"C\",\"V\",\"H\"}. Operation = " + Operation);
}

Expression::Expression()
{
}

Expression::Expression(Expression* old)
{
	for(unsigned int i = 0; i < old->Terms.size(); i++)
		Terms.push_back(new Term(old->Terms[i]));
}

nlohmann::json Expression::As_Json() const
{
	nlohmann::json j;
	unsigned int i;

	j = nlohmann::json::array();

	for(i = 0; i < Terms.size(); i++)
		j.push_back(Terms[i]->As_Json());

	return j;
}

Expression* Expression::Form_For_Printing()
{
	Expression* exp;
	unsigned int i;

	exp = new Expression;

	// Adding this line removes a memory leak but makes a lot of disgusting changes
	/* All_Expression_V.push_back(exp); */

	for(i = 0; i < Terms.size(); i++)
		exp->Terms.push_back(new Term(Terms[i]));

	for(i = 0; i < exp->Terms.size() - 1; i++)
		if(exp->Terms[i]->Operation == "+" && exp->Terms[i+1]->Coefficient < 0)
		{
			exp->Terms[i]->Operation = "-";
			exp->Terms[i+1]->Coefficient *= -1;
		}

	return exp;
}

nlohmann::json Dependency::As_Json() const
{
	nlohmann::json j;
	unsigned int i;

	j["dependent"] = nlohmann::json::array();
	for(i = 0; i < Dependent_Vars.size(); i++)
		j["dependent"].push_back(Dependent_Vars[i]->As_Json());

	j["given"] = nlohmann::json::array();
	for(i = 0; i < Given_Vars.size(); i++)
		j["given"].push_back(Given_Vars[i]->As_Json());

	return j;
}
nlohmann::json Independence::As_Json() const
{
	nlohmann::json j;
	nlohmann::json j1;
	unsigned int i;
	unsigned int k;

	j["independent"] = nlohmann::json::array();
	for(i = 0; i < Independent_Vars_Vecs.size(); i++)
	{
		if(Independent_Vars_Vecs[i].size() == 1)
			j["independent"].push_back(Independent_Vars_Vecs[i].front()->As_Json());
		else
		{
			j1 = nlohmann::json::array();
			for(k = 0; k < Independent_Vars_Vecs[i].size(); k++)
				j1.push_back(Independent_Vars_Vecs[i][k]->As_Json());
			j["independent"].push_back(j1);
		}
	}

	j["given"] = nlohmann::json::array();
	for(i = 0; i < Given_Vars.size(); i++)
		j["given"].push_back(Given_Vars[i]->As_Json());

	return j;
}
nlohmann::json Bound::As_Json() const
{
	nlohmann::json j;
	nlohmann::json jtmp;
	Bound* b;

	b = Form_For_Printing();

	j["LHS"] = b->LHS->As_Json();

	jtmp = b->Type;
	j["type"] = jtmp;

	jtmp = b->RHS;
	j["RHS"] = jtmp;

	return j;
}
nlohmann::json Symmetry::As_Json() const
{
	nlohmann::json j;
	unsigned int i;

	j = nlohmann::json::array();
	for(i = 0; i < Order.size(); i++)
		j.push_back(Order[i]->As_Json());

	return j;
}
nlohmann::json Problem_Description::As_Json()
{
	nlohmann::json j;
	nlohmann::json j2;
	unsigned int i;
	unsigned int k;

	j["RV"] = nlohmann::json::array();
	for(i = 0; i < RandomV.size(); i++)
		j["RV"].push_back(RandomV[i]->As_Json());

	j["AL"] = nlohmann::json::array();
	for(i = 0; i < AdditionalV.size(); i++)
		j["AL"].push_back(AdditionalV[i]->As_Json());

	if(Objective != NULL)
		j["O"] = Expression_Json_To_String(Objective->Form_For_Printing()->As_Json());

	j["D"] = nlohmann::json::array();
	for(i = 0; i < Dependencies.size(); i++)
		j["D"].push_back(Dependencies[i]->As_Json());

	j["I"] = nlohmann::json::array();
	for(i = 0; i < Independences.size(); i++)
		j["I"].push_back(Independences[i]->As_Json());

	j["S"] = nlohmann::json::array();
	for(i = 0; i < Symmetries.size(); i++)
		j["S"].push_back(Symmetries[i]->As_Json());

	j["EQ"] = nlohmann::json::array();
	for(i = 0; i < Equivalences2d.size(); i++)
	{
		j2 = nlohmann::json::array();
		for(k = 0; k < Equivalences2d[i].size(); ++k)
			j2.push_back(Equivalences2d[i][k]);
		j["EQ"].push_back(j2);
	}

	j["BC"] = nlohmann::json::array();
	for(i = 0; i < Constant_Bounds.size(); i++)
		j["BC"].push_back(Bound_Json_To_String(Constant_Bounds[i]->As_Json()));

	j["BP"] = nlohmann::json::array();
	for(i = 0; i < Bounds_To_Prove.size(); i++)
		j["BP"].push_back(Bound_Json_To_String(Bounds_To_Prove[i]->As_Json()));

	// sensitivity analysis: each can be any linear combinations of the information measures and additional LP variables
	j["SE"] = nlohmann::json::array();
	for (i = 0; i < Sensitivities.size(); i++)
		j["SE"].push_back(Expression_Json_To_String(Sensitivities[i]->As_Json()));

	//Queries: each can be any linear combinations of the information measures and additional LP variables
	j["QU"] = nlohmann::json::array();
	for (i = 0; i < Queries.size(); i++)
		j["QU"].push_back(Expression_Json_To_String(Queries[i]->As_Json()));

	return j;
}


void Variable::From_Json(const nlohmann::json &j)
{
	if(!j.is_string())
		throw SRE((string) "Variable::From_Json(): j isn't a string. j = \n" + j.dump(4));

	Name = static_cast<std::string const&>(j);
}
void Term::From_Json(const nlohmann::json& j, const std::map<std::string,Variable*>& All_Variables_M)
{
	std::string s;
	std::map<std::string, Variable*>::const_iterator mit;
	nlohmann::json jeo;
	nlohmann::json jg;
	nlohmann::json::const_iterator jit;

	if(j.is_string())
	{
		s = static_cast<std::string const&>(j);

		if(s == "+" || s == "-")
			Operation = s;
		else
		{
			if((mit = All_Variables_M.find(s)) == All_Variables_M.end())
				throw SRE((string) "Term::From_Json(): Unrecognized variable json. j = \n" + j.dump(4));

			if(mit->second->RL != 'L')
				throw SRE((string) "Term::From_Json(): Trying to use an RV as a term; only ALs allowed. j = \n" + j.dump(4));

			Operation = "V";
			Var = mit->second;
			Coefficient = 1;
		}
	}
	else if(j.is_number())
	{
		Operation = "C";
		Constant = j;
	}
	else if(j.is_array() && j.size())
	{
		if(j.at(0).is_string())
		{
			if(j.at(0) == "H")
			{
				if(j.size() < 2 || !j.at(1).is_array() || (j.size() == 3 && !j.at(2).is_array()) || j.size() > 3)
					throw SRE((string) "Term::From_Json(): Term is an entropy term but isn't of the form [\"H\",[],[]]. j = \n" + j.dump(4));

				Operation = "H";

				jeo = j.at(1);

				if(!jeo.size())
					throw SRE((string) "Term::From_Json(): Entropy term without anything left of the |. j = \n" + j.dump(4));

				for(jit = jeo.begin(); jit != jeo.end(); ++jit)
				{
					if(!(*jit).is_string())
						throw SRE((string) "Term::From_Json(): There's an entropy entry that's not a string. j = \n" + j.dump(4));
					s = static_cast<std::string const&>(*jit);

					if((mit = All_Variables_M.find(s)) == All_Variables_M.end())
						throw SRE((string) "Term::From_Json(): Unrecognized variable json. j = \n" + j.dump(4));
					if(mit->second->RL != 'R')
						throw SRE((string) "Term::From_Json(): Trying to use an AL in an entropy term; only RVs allowed. j = \n" + j.dump(4));

					Entropy_Of.push_back(mit->second);
				}

				if(j.size() == 3)
				{
					jg = j.at(2);
					for(jit = jg.begin(); jit != jg.end(); ++jit)
					{
						if(!(*jit).is_string())
							throw SRE((string) "Term::From_Json(): There's an entropy entry that's not a string. j = \n" + j.dump(4));
						s = static_cast<std::string const&>(*jit);

						if((mit = All_Variables_M.find(s)) == All_Variables_M.end())
							throw SRE((string) "Term::From_Json(): Unrecognized variable json. j = \n" + j.dump(4));
						if(mit->second->RL != 'R')
							throw SRE((string) "Term::From_Json(): Trying to use an AL in an entropy term; only RVs allowed. j = \n" + j.dump(4));

						Given_Vars.push_back(mit->second);
					}
				}
			}
			else if(j.at(0) == "I")
			{
				if(j.size() < 3 || !j.at(1).is_array() || !j.at(2).is_array() || (j.size() == 4 && !j.at(3).is_array()) || j.size() > 4)
					throw SRE((string) "Expression_Json_To_String(): Term is an information term but isn't of the form [\"I\",[],[],[]]. j = \n" + j.dump(4));

				Operation = "I";

				Information_Vars.resize(2);

				jeo = j.at(1);
				if(!jeo.size())
					throw SRE((string) "Term::From_Json(): Information term with empty 2nd element (ie nothing left of ;). j = \n" + j.dump(4));

				for(jit = jeo.begin(); jit != jeo.end(); ++jit)
				{
					if(!(*jit).is_string())
						throw SRE((string) "Term::From_Json(): There's an information entry that's not a string. j = \n" + j.dump(4));
					s = static_cast<std::string const&>(*jit);

					if((mit = All_Variables_M.find(s)) == All_Variables_M.end())
						throw SRE((string) "Term::From_Json(): Unrecognized variable json. j = \n" + j.dump(4));
					if(mit->second->RL != 'R')
						throw SRE((string) "Term::From_Json(): Trying to use an AL in an information term; only RVs allowed. j = \n" + j.dump(4));

					Information_Vars[0].push_back(mit->second);
				}

				jeo = j.at(2);
				if(!jeo.size())
					throw SRE((string) "Term::From_Json(): Information term with empty 3rd element (ie nothing right of ;). j = \n" + j.dump(4));

				for(jit = jeo.begin(); jit != jeo.end(); ++jit)
				{
					if(!(*jit).is_string())
						throw SRE((string) "Term::From_Json(): There's an information entry that's not a string. j = \n" + j.dump(4));
					s = static_cast<std::string const&>(*jit);

					if((mit = All_Variables_M.find(s)) == All_Variables_M.end())
						throw SRE((string) "Term::From_Json(): Unrecognized variable json. j = \n" + j.dump(4));
					if(mit->second->RL != 'R')
						throw SRE((string) "Term::From_Json(): Trying to use an AL in an information term; only RVs allowed. j = \n" + j.dump(4));

					Information_Vars[1].push_back(mit->second);
				}

				if(j.size() == 4)
				{
					jg = j.at(3);
					for(jit = jg.begin(); jit != jg.end(); ++jit)
					{
						if(!(*jit).is_string())
							throw SRE((string) "Term::From_Json(): There's an entropy entry that's not a string. j = \n" + j.dump(4));
						s = static_cast<std::string const&>(*jit);

						if((mit = All_Variables_M.find(s)) == All_Variables_M.end())
							throw SRE((string) "Term::From_Json(): Unrecognized variable json. j = \n" + j.dump(4));
						if(mit->second->RL != 'R')
							throw SRE((string) "Term::From_Json(): Trying to use an AL in an information term; only RVs allowed. j = \n" + j.dump(4));

						Given_Vars.push_back(mit->second);
					}
				}
			}
			else
			{
				throw SRE((string) "Term::From_Json(): Term is an array starting with a string that isn't \"H\" or \"I\". j = \n" + j.dump(4));
			}
		}
		else if(j.at(0).is_number())
		{
			if(j.size() != 2)
				throw SRE((string) "Term::From_Json(): j = \n" + j.dump(4));
			if(j.at(1).is_string() && (j.at(1) == "+" || j.at(1) == "-"))
				throw SRE((string) "Term::From_Json(): There's a term which is just a coefficient before a \"+\" or a \"-\". j = \n" + j.dump(4));

			if(j.at(1).is_array() || j.at(1).is_string())
			{
				From_Json(j.at(1), All_Variables_M);
				Coefficient = j.at(0);
			}
			else
				throw SRE((string) "Term::From_Json(): There's a term which is a coefficient followed by something that's not a string or an array. j = \n" + j.dump(4));
		}
		else
			throw SRE((string) "Term::From_Json(): Term is an array that doesn't start with a number or with \"H\". j = \n" + j.dump(4));
	}
	else
		throw SRE((string) "Term::From_Json(): Term isn't a string, array with positive size, or number. j = \n" + j.dump(4));
}

void Expression::From_Json(const nlohmann::json& j, std::vector<Term*>& All_Terms_V, const std::map<std::string,Variable*>& All_Variables_M)
{
	nlohmann::json::const_iterator jit;
	Term* t;
	unsigned int i;

	if(!j.is_array())
		throw SRE((string) "Expression::From_Json(): j is not an array. j = \n" + j.dump(4));

	if((j.size() % 2) != 1)
		throw SRE((string) "Expression::From_Json(): j has even size. Size should be odd; every other field should be \"+\" or \"-\". j = \n" + j.dump(4));

	for(jit = j.begin(); jit != j.end(); ++jit)
	{
		t = new Term;
		t->From_Json(*jit, All_Variables_M);
		Terms.push_back(t);

		All_Terms_V.push_back(t);
	}

	for(i = 0; i < Terms.size(); i++)
	{
		if((i % 2 == 0 && (Terms[i]->Operation == "-" || Terms[i]->Operation == "+"))
				|| (i % 2 == 1 && (Terms[i]->Operation != "-" && Terms[i]->Operation != "+")))
			throw SRE((string) "Expression::From_Json(): This Expression doesn't contain \"+\" and \"-\" in (only) every odd-numbered slot (0-indexed). j = \n" + j.dump(4));
		if(Terms[i]->Operation == "-")
		{
			Terms[i+1]->Coefficient *= -1;
			Terms[i]->Operation = "+";
		}
	}
}

void From_Json_Array_To_Variable_Vector(const nlohmann::json& j, std::vector<Variable*>& vec, const std::map<std::string, Variable*>&  VarMap)
{
	nlohmann::json::const_iterator it;
	set<Variable*> s;

	if(!j.is_array())
		throw SRE((string) "From_Json_Array_To_Variable_Vector(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		if(!(*it).is_string())
			throw SRE((string) "From_Json_Array_To_Variable_Vector(): element of j isn't a string. j = \n" + j.dump(4) + (string) "\nelement = \n" + (*it).dump(4));

		if(VarMap.find(*it) == VarMap.end())
			throw SRE((string) "From_Json_Array_To_Variable_Vector(): j contains variable " + (*it).dump(4) + (string) " which isn't found. j = \n" + j.dump(4));

		vec.push_back(VarMap.find(*it)->second);
	}

	// Make sure nothing's repeated.
	for(unsigned int i = 0; i < vec.size(); i++)
		s.insert(vec[i]);
	if(vec.size() != s.size())
		throw SRE((string) "From_Json_Array_To_Variable_Vector(): There's a repeated Variable here. j = \n" + j.dump(4));
}
void From_Json_Array_To_Variable_Vector_Ind(const nlohmann::json& j, std::vector<std::vector<Variable*> >& vec, const std::map<std::string, Variable*>&  VarMap)
{
	nlohmann::json::const_iterator it;
	nlohmann::json::const_iterator it2;

	if(!j.is_array())
		throw SRE((string) "From_Json_Array_To_Variable_Vector(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		vec.resize(vec.size() + 1);

		if((*it).is_string())
		{
			if(VarMap.find(*it) == VarMap.end())
				throw SRE((string) "From_Json_Array_To_Variable_Vector(): j contains variable " + (*it).dump(4) + (string) " which isn't found. j = \n" + j.dump(4));
			vec.back().push_back(VarMap.find(*it)->second);
		}
		else if((*it).is_array())
		{
			for(it2 = (*it).begin(); it2 != (*it).end(); ++it2)
			{
				if(VarMap.find(*it2) == VarMap.end())
					throw SRE((string) "From_Json_Array_To_Variable_Vector(): j contains variable " + (*it2).dump(4) + (string) " which isn't found. j = \n" + j.dump(4));
				vec.back().push_back(VarMap.find(*it2)->second);
			}
		}
		else
		{
			throw SRE((string) "From_Json_Array_To_Variable_Vector(): element of j isn't a string or an array. j = \n" + j.dump(4) + (string) "\nelement = \n" + (*it).dump(4));
		}
	}
}

void Check_All_Vars_RV(const std::vector<Variable*>& vec, std::string Calling_Function_Name, const nlohmann::json& j)
{
	for(unsigned int i = 0; i < vec.size(); i++)
		if(vec[i]->RL != 'R')
			throw SRE(Calling_Function_Name + vec[i]->Name + " isn't an RV. j = \n" + j.dump(4));
}
void Check_All_Vars_RV_Ind(const std::vector<std::vector<Variable*> >& vec, std::string Calling_Function_Name, const nlohmann::json& j)
{
	for(unsigned int i = 0; i < vec.size(); i++)
		for(unsigned int k = 0; j < vec[i].size(); k++)
			if(vec[i][k]->RL != 'R')
				throw SRE(Calling_Function_Name + vec[i][k]->Name + " isn't an RV. j = \n" + j.dump(4));
}

void Dependency::From_Json(const nlohmann::json &j, const std::map<std::string, Variable*>&  All_Variables_M)
{
	if(!j.is_object())
		throw SRE((string) "Dependency::From_Json(): j is not an object. j = \n" + j.dump(4));

	if(j.find("dependent") == j.end() || j.find("given") == j.end())
		throw SRE((string) "Dependency::From_Json(): j isn't of the form {\"dependent\" : [] , \"given\" : []}. j = \n" + j.dump(4));

	From_Json_Array_To_Variable_Vector(j["dependent"], Dependent_Vars, All_Variables_M);
	From_Json_Array_To_Variable_Vector(j["given"], Given_Vars, All_Variables_M);

	if(!Dependent_Vars.size() || !Given_Vars.size())
		throw SRE((string) "Dependency::From_Json(): Dependency has either no dependent or no given. Neither list can have size 0. j = \n" + j.dump(4));

	Check_All_Vars_RV(Dependent_Vars, "Dependency::From_Json(): ", j);
	Check_All_Vars_RV(Given_Vars, "Dependency::From_Json(): ", j);
}
void Independence::From_Json(const nlohmann::json &j, const std::map<std::string, Variable*>& All_Variables_M)
{
	if(!j.is_object())
		throw SRE((string) "Independence::From_Json(): j is not an object. j = \n" + j.dump(4));

	if(j.find("independent") == j.end() || j.find("given") == j.end())
		throw SRE((string) "Independence::From_Json(): j isn't of the form {\"independent\" : [] , \"given\" : []}. j = \n" + j.dump(4));

	From_Json_Array_To_Variable_Vector_Ind(j["independent"], Independent_Vars_Vecs, All_Variables_M);
	From_Json_Array_To_Variable_Vector(j["given"], Given_Vars, All_Variables_M);

	if(Independent_Vars_Vecs.size() < 2)
		throw SRE((string) "Independency::From_Json(): Independence has independent list of size less than 2. j = \n" + j.dump(4));

	Check_All_Vars_RV_Ind(Independent_Vars_Vecs, "Independence::From_Json(): ", j);
	Check_All_Vars_RV(Given_Vars, "Independence::From_Json(): ", j);
}
void Bound::From_Json(const nlohmann::json& j, std::vector<Term*>& All_Terms_V, const std::map<std::string,Variable*>& All_Variables_M)
{
	unsigned int i;

	if(!j.is_object())
		throw SRE((string) "Bound::From_Json(): j is not an object. j = \n" + j.dump(4));

	if(j.find("LHS") == j.end() || j.find("type") == j.end() || j.find("RHS") == j.end())
		throw SRE((string) "Bound::From_Json(): j isn't of the form {\"LHS\" : [] , \"type\" : \"\" , \"RHS\" : number}. j = \n" + j.dump(4));

	LHS = new Expression;
	LHS->From_Json(j["LHS"], All_Terms_V, All_Variables_M);

	if(!j["type"].is_string())
		throw SRE((string) "Bound::From_Json(): j[\"type\"] isn't a string. j = \n" + j.dump(4));

	Type = static_cast<std::string const&>(j["type"]);
	if(Type != "<=" && Type != "=" && Type != ">=")
		throw SRE((string) "Bound::From_Json(): Type must be in {\"<=\",\"=\",\">=\"}. j = \n" + j.dump(4));

	if(!j["RHS"].is_number())
		throw SRE((string) "Bound::From_Json(): RHS isn't a number. j = \n" + j.dump(4));
	RHS = j["RHS"];

	// For compatability with C_Style, change all <= to >=
	if(Type == "<=")
	{
		Type = ">=";

		for(i = 0; i < LHS->Terms.size(); i++)
			if(LHS->Terms[i]->Operation != "+" && LHS->Terms[i]->Operation != "-")
				LHS->Terms[i]->Coefficient *= -1;

		if(RHS != 0)
			RHS *= -1;
	}
}
void Symmetry::From_Json(const nlohmann::json &j, const std::map<std::string, Variable*>& RandomM)
{
	if(!j.is_array())
		throw SRE((string) "Symmetry::From_Json(): j is not an array. j = \n" + j.dump(4));

	From_Json_Array_To_Variable_Vector(j, Order, RandomM);

	if(Order.size() != RandomM.size())
		throw SRE((string) "Symmetry::From_Json(): Symmetry size doesn't equal number or RVs. j = \n" + j.dump(4));
}
void Problem_Description::From_Json(const nlohmann::json &j)
{
	if(!j.is_object())
		throw SRE((string) "Problem_Description::From_Json(): j is not a json::object. j = \n" + j.dump(4));

	// Make sure all keys are valid
	for(nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it)
	{
		if(it.key() != "RV"
				&& it.key() != "AL"
				&& it.key() != "O"
				&& it.key() != "D"
				&& it.key() != "I"
				&& it.key() != "S"
				&& it.key() != "BC"
				&& it.key() != "BP"
				&& it.key() != "EQ"
				&& it.key() != "QU"
				&& it.key() != "SE"
				&& it.key() != "CMD"
				&& it.key() != "OPT"
		  )
			throw SRE("Problem_Description::From_Json(): Bad key: " + static_cast<std::string const&>(it.key()));
	}

	if(j.find("RV") != j.end())
		Key_RV(j["RV"]);

	if(j.find("AL") != j.end())
		Key_AL(j["AL"]);

	if(j.find("O") != j.end())
		Key_O(j["O"]);

	if(j.find("D") != j.end())
		Key_D(j["D"]);

	if(j.find("I") != j.end())
		Key_I(j["I"]);

	if(j.find("S") != j.end())
		Key_S(j["S"]);

	if(j.find("BC") != j.end())
		Key_BC(j["BC"]);

	if(j.find("BP") != j.end())
		Key_BP(j["BP"]);

	if(j.find("EQ") != j.end())
		Key_EQ(j["EQ"]);

	if(j.find("QU") != j.end())
		Key_QU(j["QU"]);

	if(j.find("SE") != j.end())
		Key_SE(j["SE"]);

	if(j.find("CMD") != j.end())
		Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("CMD",'+',j["CMD"]));

	if(j.find("OPT") != j.end())
		Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("CMD",'+',j["OPT"]));

	Check_Equivalence();
}

void Problem_Description::Key_RV(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	Variable* v;

	if(RandomV.size())
		throw SRE("Key_RV(): Only one command allowed per type. Multiple RV commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_RV(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		v = new Variable;
		v->From_Json(*it);

		Check_Valid_Variable_Name(v->Name);

		v->RL = 'R';
		All_Variables_V.push_back(v);
		All_Variables_M[v->Name] = v;

		v->Index = RandomV.size();
		RandomV.push_back(v);
		RandomM[v->Name] = v;
	}
}

void Problem_Description::Key_AL(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	Variable* v;

	if(AdditionalV.size())
		throw SRE("Key_AL(): Only one command allowed per type. Multiple AL commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_AL(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		v = new Variable;
		v->From_Json(*it);

		Check_Valid_Variable_Name(v->Name);

		v->RL = 'L';
		All_Variables_V.push_back(v);
		All_Variables_M[v->Name] = v;

		v->Index = AdditionalV.size();
		AdditionalV.push_back(v);
		AdditionalM[v->Name] = v;
	}
}
void Problem_Description::Key_O(const nlohmann::json& j)
{
	nlohmann::json j1;
	std::string s;

	if(Objective)
		throw SRE("Key_O(): Only one command allowed per type. Multiple O commands found.");

	Objective = new Expression;
	if(j.is_string())
	{
		s = static_cast<std::string const&>(j);
		j1 = Expression_String_To_Json(s);
		Objective->From_Json(j1, All_Terms_V, All_Variables_M);
	}
	else
	{
		Objective->From_Json(j, All_Terms_V, All_Variables_M);
	}
	All_Expression_V.push_back(Objective);
}
void Problem_Description::Key_D(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	Dependency* d;

	if(Dependencies.size())
		throw SRE("Key_D(): Only one command allowed per type. Multiple D commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_D(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		d = new Dependency;
		d->From_Json(*it, All_Variables_M);
		Dependencies.push_back(d);
	}
}
void Problem_Description::Key_I(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	Independence* ind;

	if(Independences.size())
		throw SRE("Key_I(): Only one command allowed per type. Multiple I commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_I(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		ind = new Independence;
		ind->From_Json(*it, All_Variables_M);
		Independences.push_back(ind);
	}
}

void Problem_Description::Check_First_Symmetry_Matches_RV_List()
{
	for(unsigned int i = 0; i < Symmetries[0]->Order.size(); ++i)
		if(Symmetries[0]->Order[i] != RandomV[i])
			throw SRE("Problem_Description::Check_First_Symmetry_Matches_RV_List(): The first symmetry listed isn't a copy of the RV line.");
}

void Problem_Description::Key_S(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	Symmetry* s;
	char buf[20];
	size_t i;

	if(Symmetries.size())
		throw SRE("Key_S(): Only one command allowed per type. Multiple S commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_S(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		s = new Symmetry;
		s->From_Json(*it, RandomM);
		Symmetries.push_back(s);
		for (i = 0; i < s->Order.size(); i++) {
			sprintf(buf, "%d ", s->Order[i]->Index);
			s->Key += buf;
		}

		if (SymmetryM.find(s->Key) != SymmetryM.end())
			throw SRE((string) "Problem_Description::Duplicate Symmetry " + it->dump());
		SymmetryM[s->Key] = s;
	}

	if(Symmetries.size())
	{
		Check_First_Symmetry_Matches_RV_List();
		/* Check_Symmetry_Group(); */
	}
}
void Problem_Description::Key_EQ(const nlohmann::json& j)
{
	if(Equivalences2d.size())
		throw SRE("Key_EQ(): Only one command allowed per type. Multiple EQ commands found.");

	if(!j.is_array())
		throw SRE("Key_EQ(): Needs to be an array of arrays of strings. j = " + j.dump(4));

	for(nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it)
	{
		if(!(*it).is_array())
			throw SRE("Key_EQ(): Needs to be an array of arrays of strings. j = " + j.dump(4));

		Equivalences2d.resize(Equivalences2d.size() + 1);

		for(nlohmann::json::const_iterator it2 = (*it).begin(); it2 != (*it).end(); ++it2)
		{
			if(!(*it2).is_string())
				throw SRE("Key_EQ(): Needs to be an array of arrays of strings. j = " + j.dump(4));
			Equivalences2d.back().push_back(*it2);
		}
	}
}
void Problem_Description::Key_BC(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	nlohmann::json j1;
	std::string s;
	Bound* b;

	if(Constant_Bounds.size())
		throw SRE("Key_BC(): Only one command allowed per type. Multiple BC commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_BC(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		b = new Bound;

		if((*it).is_string())
		{
			s = static_cast<std::string const&>(*it);
			j1 = Bound_String_To_Json(s);
		}
		else
		{
			j1 = *it;
		}
		b->From_Json(j1, All_Terms_V, All_Variables_M);
		All_Bounds_V.push_back(b);
		All_Expression_V.push_back(b->LHS);

		b->cp = 'C';
		Constant_Bounds.push_back(b);
	}
}
void Problem_Description::Key_BP(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	nlohmann::json j1;
	std::string s;
	Bound* b;

	if(Bounds_To_Prove.size())
		throw SRE("Key_BP(): Only one command allowed per type. Multiple BP commands found.");

	if(!j.is_array())
		throw SRE((string) "Problem_Description::Key_BP(): j isn't an array. j = \n" + j.dump(4));

	for(it = j.begin(); it != j.end(); ++it)
	{
		b = new Bound;

		if((*it).is_string())
		{
			s = static_cast<std::string const&>(*it);
			j1 = Bound_String_To_Json(s);
		}
		else
		{
			j1 = *it;
		}
		b->From_Json(j1, All_Terms_V, All_Variables_M);
		All_Bounds_V.push_back(b);
		All_Expression_V.push_back(b->LHS);

		b->cp = 'P';
		Bounds_To_Prove.push_back(b);
	}
}

void Problem_Description::Key_QU(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	nlohmann::json j1;
	std::string s;
	Expression* e;

	if (Queries.size())
		throw SRE("Key_QU(): Only one command allowed per type. Multiple QU commands found.");

	if (!j.is_array())
		throw SRE((string)"Problem_Description::Key_QU(): j isn't an array. j = \n" + j.dump(4));

	for (it = j.begin(); it != j.end(); ++it)
	{
		e = new Expression;

		if ((*it).is_string())
		{
			s = static_cast<std::string const&>(*it);
			j1 = Expression_String_To_Json(s);
		}
		else
		{
			j1 = *it;
		}
		e->From_Json(j1, All_Terms_V, All_Variables_M);
		Queries.push_back(e);
		All_Expression_V.push_back(e);
	}
}
void Problem_Description::Key_SE(const nlohmann::json& j)
{
	nlohmann::json::const_iterator it;
	nlohmann::json j1;
	std::string s;
	Expression* e;

	if (Sensitivities.size())
		throw SRE("Key_SE(): Only one command allowed per type. Multiple SE commands found.");

	if (!j.is_array())
		throw SRE((string)"Problem_Description::Key_SE(): j isn't an array. j = \n" + j.dump(4));

	for (it = j.begin(); it != j.end(); ++it)
	{
		e = new Expression;

		if ((*it).is_string())
		{
			s = static_cast<std::string const&>(*it);
			j1 = Expression_String_To_Json(s);
		}
		else
		{
			j1 = *it;
		}
		e->From_Json(j1, All_Terms_V, All_Variables_M);
		Sensitivities.push_back(e);
		All_Expression_V.push_back(e);
	}
}
void Problem_Description::Get_Cmd_Line_Modifiers(char** argv, int argc, int startindex)
{
	int i;
	json j;

	for(i = startindex; i < argc; i++)
	{
		try
		{
			j = json::parse(argv[i]);
		}
		catch(...)
		{
			throw SRE("Get_Cmd_Line_Modifiers(): Error parsing argv[" + std::to_string(i) + "] to json");
		}

		if(!j.is_object())
			throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "] isn't a json object");


		if(j.find("RV") != j.end())
		{
			if(!j["RV"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s RV's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("RV",'R',j["RV"]));
		}
		if(j.find("+RV") != j.end())
		{
			if(!j["+RV"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +RV's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("RV",'+',j["+RV"]));
		}
		if(j.find("AL") != j.end())
		{
			if(!j["AL"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s AL's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("AL",'R',j["AL"]));
		}
		if(j.find("+AL") != j.end())
		{
			if(!j["+AL"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +AL's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("AL",'+',j["+AL"]));
		}
		if(j.find("O") != j.end())
		{
			if(!j["O"].is_array() && !j["O"].is_string()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s O's val should be an array or a string");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("O",'R',j["O"]));
		}
		if(j.find("+O") != j.end())
		{
			throw SRE("Get_Cmd_Line_Modifiers(): Can't add an objective function, you can only replace it");
		}
		if(j.find("D") != j.end())
		{
			if(!j["D"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s D's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("D",'R',j["D"]));
		}
		if(j.find("+D") != j.end())
		{
			if(!j["+D"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +D's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("D",'+',j["+D"]));
		}
		if(j.find("I") != j.end())
		{
			if(!j["I"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s I's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("I",'R',j["I"]));
		}
		if(j.find("+I") != j.end())
		{
			if(!j["+I"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +I's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("I",'+',j["+I"]));
		}
		if(j.find("S") != j.end())
		{
			if(!j["S"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s S's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("S",'R',j["S"]));
		}
		if(j.find("+S") != j.end())
		{
			if(!j["+S"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +S's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("S",'+',j["+S"]));
		}
		if(j.find("BC") != j.end())
		{
			if(!j["BC"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s BC's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("BC",'R',j["BC"]));
		}
		if(j.find("+BC") != j.end())
		{
			if(!j["+BC"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +BC's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("BC",'+',j["+BC"]));
		}
		if(j.find("BP") != j.end())
		{
			if(!j["BP"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s BP's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("BP",'R',j["BP"]));
		}
		if(j.find("+BP") != j.end())
		{
			if(!j["+BP"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +BP's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("BP",'+',j["+BP"]));
		}
		if(j.find("EQ") != j.end())
		{
			if(!j["EQ"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s EQ's val should be an array of arrays of strings");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("EQ",'R',j["EQ"]));
		}
		if(j.find("+EQ") != j.end())
		{
			throw SRE("Get_Cmd_Line_Modifiers(): Only EQ allowed, no +EQ");
		}
		if(j.find("QU") != j.end())
		{
			if(!j["QU"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s QU's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("QU",'R',j["QU"]));
		}
		if(j.find("+QU") != j.end())
		{
			if(!j["QU"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s QU's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("QU",'+',j["QU"]));
		}
		if(j.find("SE") != j.end())
		{
			if(!j["SE"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s SE's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("SE",'R',j["SE"]));
		}
		if(j.find("+SE") != j.end())
		{
			if(!j["SE"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s SE's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("SE",'+',j["SE"]));
		}
		if(j.find("CMD") != j.end())
		{
			throw SRE("Get_Cmd_Line_Modifiers(): Only +CMD allowed, no CMD");
		}
		if(j.find("+CMD") != j.end())
		{
			if(!j["+CMD"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +CMD's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("CMD",'+',j["+CMD"]));
		}
		if (j.find("OPT") != j.end())
		{
			throw SRE("Get_Cmd_Line_Modifiers(): Only +OPT allowed, no OPT");
		}
		if (j.find("+OPT") != j.end())
		{
			if (!j["+OPT"].is_array()) throw SRE("Get_Cmd_Line_Modifiers(): argv[" + std::to_string(i) + "]'s +OPT's val should be an array");
			Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("OPT", '+', j["+OPT"]));
		}
	}
}

void Problem_Description::Run_Cmd_Line_Modifier_Commands()
{
	unsigned int i;
	string s;
	Cmd_Line_Modifier* clm;
	nlohmann::json::iterator it;

	for(i = 0; i < Cmd_Line_Modifiers.size(); i++)
	{
		clm = Cmd_Line_Modifiers[i];

		if(clm->Key == "CMD")
		{
			for(it = clm->j.begin(); it != clm->j.end(); ++it)
			{
				if(!(*it).is_string())
					throw SRE("Problem_Description::Run_Cmd_Line_Modifier_Commands(): entry in +CMD isn't string");

				s = static_cast<std::string const&>(*it);

				if(s == "SER")
				{
					std::vector<std::string> sv;
					sv.push_back("SER");
					Command_SER(this,sv,0);
				}
				else if(s == "DESTROY")
				{
					throw SRE("Problem_Description::Run_Cmd_Line_Modifier_Commands(): DESTROY not allowed to be added with +CMD on command line");
				}
				else if(s == "PDC")
				{
					Problem_Description_C_Style* pdc = As_Problem_Description_C_Style();
					Print_Problem_Description_C_Style(pdc);
				}
				else if(s == "DESER")
				{
					throw SRE("Problem_Description::Run_Cmd_Line_Modifier_Commands(): DESER not allowed to be added with +CMD on command line");
				}
				else if(s == "CS")
				{
					if(Symmetries.size())
						Check_Symmetry_Group();
				}
				else if (s == "LP_DISP")
				{
					LP_display = 1;
				}
			}
		}
	}
}

void Problem_Description::Modify_Json_From_Cmd_Line_Before_From_Json(nlohmann::json& j)
{
	unsigned int i;
	Cmd_Line_Modifier* clm;
	nlohmann::json::iterator it;

	if(!j.is_object())
		throw SRE("Problem_Description::Modify_Json_From_Cmd_Line_Before_From_Json(): j isn't object. j = \n" + j.dump(4));

	for(i = 0; i < Cmd_Line_Modifiers.size(); i++)
	{
		clm = Cmd_Line_Modifiers[i];

		if(clm->Key != "CMD")
		{
			if(clm->Action == 'R')
				j[clm->Key] = clm->j;
			else if(clm->Action == '+')
			{
				if(j.find(clm->Key) == j.end())
					j[clm->Key] = nlohmann::json::array();
				for(it = clm->j.begin(); it != clm->j.end(); ++it)
					j[clm->Key].push_back(*it);
			}
		}
	}
}

void Get_Expression_Into_Map(std::map<unsigned int, double>& pos_val_m, Expression* e, unsigned int numrvs)
{
	unsigned int i;
	unsigned int j;
	unsigned int entropy_of_set;
	unsigned int entropy_of_set_A;
	unsigned int entropy_of_set_B;
	unsigned int entropy_of_set_C;
	unsigned int given_set;
	std::map<unsigned int, double>::iterator mit;
	std::map<unsigned int, double>::iterator mit2;

	pos_val_m.clear();

	for(i = 0; i < e->Terms.size(); i += 2)
	{
		if(e->Terms[i]->Operation == "H")
		{
			entropy_of_set = 0;
			given_set = 0;

			for(j = 0; j < e->Terms[i]->Entropy_Of.size(); j++)
				entropy_of_set |= (1 << e->Terms[i]->Entropy_Of[j]->Index);
			for(j = 0; j < e->Terms[i]->Given_Vars.size(); j++)
				given_set |= (1 << e->Terms[i]->Given_Vars[j]->Index);

			pos_val_m[entropy_of_set | given_set] += e->Terms[i]->Coefficient;
			if(given_set)
				pos_val_m[given_set] += (-1 * e->Terms[i]->Coefficient);
		}
		else if (e->Terms[i]->Operation == "I")
		{
			// I(A;B)   = H(A)   + H(B)   - H(A,B)
			// I(A;B|C) = H(A,C) + H(B,C) - H(C) - H(A,B,C)
			// A = Information_Vars[0]
			// B = Information_Vars[1]
			// C = Given_Vars

			if(e->Terms[i]->Information_Vars.size() != 2)
				throw SRE("Get_Expression_Into_Map(): Somehow Information_Vars.size() != 2. (This exception should be impossible to hit.)");

			entropy_of_set_A = 0;
			for(j = 0; j < e->Terms[i]->Information_Vars[0].size(); j++)
				entropy_of_set_A |= (1 << e->Terms[i]->Information_Vars[0][j]->Index);

			entropy_of_set_B = 0;
			for(j = 0; j < e->Terms[i]->Information_Vars[1].size(); j++)
				entropy_of_set_B |= (1 << e->Terms[i]->Information_Vars[1][j]->Index);

			entropy_of_set_C = 0;
			for(j = 0; j < e->Terms[i]->Given_Vars.size(); j++)
				entropy_of_set_C |= (1 << e->Terms[i]->Given_Vars[j]->Index);

			// H(A,C)
			pos_val_m[entropy_of_set_A | entropy_of_set_C] += e->Terms[i]->Coefficient;

			// H(B,C)
			pos_val_m[entropy_of_set_B | entropy_of_set_C] += e->Terms[i]->Coefficient;

			// -H(C)
			if(entropy_of_set_C)
				pos_val_m[entropy_of_set_C] += (-1 * e->Terms[i]->Coefficient);

			// -H(A,B,C)
			pos_val_m[entropy_of_set_A | entropy_of_set_B | entropy_of_set_C] += (-1 * e->Terms[i]->Coefficient);
		}
		else if (e->Terms[i]->Operation == "V")
			pos_val_m[(1 << numrvs) + e->Terms[i]->Var->Index] += e->Terms[i]->Coefficient;
		else
			throw SRE("Problem_Description::Get_Expression_Into_Map(): Somehow one of the terms in the Expression has Operation that isn't in {\"H\",\"V\"}.");
	}

	// Erase entries with coefficient 0.
	// This sometimes is necessary when terms are combined.
	for(mit = pos_val_m.begin(); mit != pos_val_m.end(); )
	{
		if(mit->second == 0)
		{
			mit2 = mit;
			++mit;
			pos_val_m.erase(mit2);
		}
		else
			++mit;
	}
}

void Get_Map_Into_C_Style_Expression(const std::map<unsigned int, double>& pos_val_m, unsigned int* list_pos, double* list_value)
{
	unsigned int i = 0;
	std::map<unsigned int, double>::const_iterator mit;

	for(mit = pos_val_m.begin(); mit != pos_val_m.end(); ++mit,++i)
	{
		list_pos[i] = mit->first;
		list_value[i] = mit->second;
	}
}

// All 5 lists are size num_constantbounds
// numitems is as in the objective function
// list_pos is as in the objective function
// list_value is as in the objective function
// list_type is (int)0 for >= and (int)2 for =
//     all <= are changed to >= with negation
// list_rhs is just a constant
void FillBounds(const std::vector<Bound*>& BoundVec, unsigned int** list_pos, double** list_value, char* list_type, double* list_rhs, int* list_num_items, unsigned int numrvs)
{
	unsigned int i;
	Bound* b;
	std::map<unsigned int, double> pos_val_m;

	for(i = 0; i < BoundVec.size(); i++)
	{
		b = BoundVec[i];

		if(b->Type == ">=")
			list_type[i] = 0;
		else if(b->Type == "=")
			list_type[i] = 2;
		else
			throw SRE((string) "FillBounds(): Somehow BoundVec[i]->Type isn't in {\"<=\",\">=\",\"=\"}, or, if \"<=\", it got past the part of the code that should've changed it to \">=\", . Type = " + b->Type);

		list_rhs[i] = b->RHS;

		Get_Expression_Into_Map(pos_val_m, b->LHS, numrvs);

		list_num_items[i] = (int) pos_val_m.size();
		list_pos[i] = (unsigned int*)calloc(list_num_items[i], sizeof(unsigned int));
		list_value[i] = (double*)calloc(list_num_items[i], sizeof(double));

		Get_Map_Into_C_Style_Expression(pos_val_m, list_pos[i], list_value[i]);
	}
}

// All 3 lists are size num_constantbounds (very similar to FillBounds)
// numitems is as in the objective function
// list_pos is as in the objective function
// list_value is as in the objective function
void FillExpressions(const std::vector<Expression*>& ExoressionVec, unsigned int** list_pos, double** list_value, int* list_num_items, unsigned int numrvs)
{
	unsigned int i;
	Expression* b;
	std::map<unsigned int, double> pos_val_m;

	for (i = 0; i < ExoressionVec.size(); i++)
	{
		b = ExoressionVec[i];

		Get_Expression_Into_Map(pos_val_m, b, numrvs);

		list_num_items[i] = (int)pos_val_m.size();
		list_pos[i] = (unsigned int*)calloc(list_num_items[i], sizeof(unsigned int));
		list_value[i] = (double*)calloc(list_num_items[i], sizeof(double));

		Get_Map_Into_C_Style_Expression(pos_val_m, list_pos[i], list_value[i]);
	}
}

void Problem_Description::Check_Valid_For_C_Style()
{
	if(RandomV.size() > MAXNUMVARS_C_STYLE)
		throw SRE((string) "Check_Valid_For_C_Style(): RandomV.size() is " + std::to_string(RandomV.size()) + (string)" but it shouldn't be bigger than " + std::to_string(MAXNUMVARS_C_STYLE));
	if(Independences.size() > MAXINDPITEMS_C_STYLE)
		throw SRE((string) "Check_Valid_For_C_Style(): Independences.size() is " + std::to_string(Independences.size()) + (string)" but it shouldn't be bigger than " + std::to_string(MAXINDPITEMS_C_STYLE));
	if(Objective)
		for(unsigned int i = 0; i < Objective->Terms.size(); i++)
			if(Objective->Terms[i]->Operation == "C")
				throw SRE((string) "Check_Valid_For_C_Style(): The Objective funtion has a Term which is a Constant. That's not allowed when converting to C style.");
}

void Problem_Description::Print_Old_Format(std::ostream& fout)
{
	unsigned int i;
	unsigned int j;
	unsigned int z;
	Dependency* dep;
	Independence* ind;
	Symmetry* sym;

	// RV
	if(RandomV.size())
	{
		fout << "Random variables:" << endl;
		fout << RandomV.front()->Name;
		for(i = 1; i < RandomV.size(); i++)
			fout << ", " << RandomV[i]->Name;
		fout << endl << endl;
	}

	// AL
	if(AdditionalV.size())
	{
		fout << "Additional LP variables:" << endl;
		fout << AdditionalV.front()->Name;
		for(i = 1; i < AdditionalV.size(); i++)
			fout << ", " << AdditionalV[i]->Name;
		fout << endl << endl;
	}

	// O
	if(Objective)
	{
		fout << "Objective:" << endl;
		fout << Expression_Json_To_String(Objective->Form_For_Printing()->As_Json());
		fout << endl << endl;
	}

	// D
	if(Dependencies.size())
	{
		fout << "Dependency:" << endl;
		for(i = 0; i < Dependencies.size(); i++)
		{
			dep = Dependencies[i];

			if(dep->Dependent_Vars.size() && dep->Given_Vars.size())
			{
				fout << dep->Dependent_Vars.front()->Name;
				for(j = 1; j < dep->Dependent_Vars.size(); j++)
					fout << "," << dep->Dependent_Vars[j]->Name;
				fout << ": ";

				fout << dep->Given_Vars.front()->Name;
				for(j = 1; j < dep->Given_Vars.size(); j++)
					fout << ", " << dep->Given_Vars[j]->Name;
				fout << endl;
			}
		}
		fout << endl << endl;
	}

	// I
	if(Independences.size())
	{
		fout << "Conditional independence:" << endl;
		for(i = 0; i < Independences.size(); i++)
		{
			ind = Independences[i];

			if(ind->Independent_Vars_Vecs.size())
			{
				fout << ind->Independent_Vars_Vecs.front().front()->Name;
				for(z = 1; z < ind->Independent_Vars_Vecs.front().size(); z++)
					fout << "," << ind->Independent_Vars_Vecs.front()[z]->Name;

				for(j = 1; j < ind->Independent_Vars_Vecs.size(); j++)
				{
					fout << ";";
					fout << ind->Independent_Vars_Vecs[j].front()->Name;
					for(z = 1; z < ind->Independent_Vars_Vecs[j].size(); z++)
						fout << "," << ind->Independent_Vars_Vecs[j][z]->Name;
				}

				if(ind->Given_Vars.size())
				{
					fout << "|" << ind->Given_Vars.front()->Name;
					for(j = 1; j < ind->Given_Vars.size(); j++)
						fout << "," << ind->Given_Vars[j];
				}

				fout << endl;
			}
		}
		fout << endl << endl;
	}


	// BC
	if(Constant_Bounds.size())
	{
		fout << "Constant bounds:" << endl;
		for(i = 0; i < Constant_Bounds.size(); i++)
			fout << Bound_Json_To_String(Constant_Bounds[i]->Form_For_Printing()->As_Json()) << endl;
		fout << endl << endl;
	}


	// S
	if(Symmetries.size())
	{
		fout << "Symmetry:" << endl;
		for(i = 0; i < Symmetries.size(); i++)
		{
			sym = Symmetries[i];

			if(sym->Order.size())
			{
				fout << sym->Order.front()->Name;
				for(j = 1; j < sym->Order.size(); j++)
					fout << "," << sym->Order[j]->Name;
				fout << endl;
			}
		}
		fout << endl << endl;
	}

	// EQ
	// Commented out because this will break the old tool
	/* if(Equivalence != "")
	   {
	   fout << "Equivalence classes (not part of the old CAI tool)" << endl;
	   fout << Equivalence << endl;
	   fout << endl;
	   } */

	if ( Queries.size())
	{
		fout << "Queries:" << endl;
		for (i = 0; i < Queries.size(); i++)
			fout << Expression_Json_To_String(Queries[i]->Form_For_Printing()->As_Json()) << endl;

		fout << endl << endl;
	}

	if (Sensitivities.size())
	{
		fout << "Sensitivities:" << endl;
		for (i = 0; i < Sensitivities.size(); i++)
			fout << Expression_Json_To_String(Sensitivities[i]->Form_For_Printing()->As_Json()) << endl;

		fout << endl << endl;
	}

	// BP
	if(Bounds_To_Prove.size())
	{
		fout << "Bounds to prove:" << endl;
		for(i = 0; i < Bounds_To_Prove.size(); i++)
			fout << Bound_Json_To_String(Bounds_To_Prove[i]->Form_For_Printing()->As_Json()) << endl;
		fout << endl << endl;
	}

	// end
	fout << "end" << endl;
}

Problem_Description_C_Style* Problem_Description::As_Problem_Description_C_Style()
{
	Problem_Description_C_Style* pdc;
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int num_to_or;
	std::map<unsigned int, double> pos_val_m;

	Check_Valid_For_C_Style();

	pdc = new Problem_Description_C_Style;

	// RVs
	// list_rvs is just a list of the names
	pdc->num_rvs = RandomV.size();
	if(RandomV.size())
	{
		pdc->list_rvs = (char**) calloc(pdc->num_rvs, sizeof(char*));
		for(i = 0; i < RandomV.size(); i++)
		{
			pdc->list_rvs[i] = (char*) calloc(RandomV[i]->Name.size() + 1, sizeof(char));
			strcpy(pdc->list_rvs[i], RandomV[i]->Name.c_str());
		}
	}

	// ALs
	// list_add_LPvars is just a list of the names
	pdc->num_add_LPvars = AdditionalV.size();
	if(AdditionalV.size())
	{
		pdc->list_add_LPvars = (char**) calloc(pdc->num_add_LPvars, sizeof(char*));
		for(i = 0; i < AdditionalV.size(); i++)
		{
			pdc->list_add_LPvars[i] = (char*) calloc(AdditionalV[i]->Name.size() + 1, sizeof(char));
			strcpy(pdc->list_add_LPvars[i], AdditionalV[i]->Name.c_str());
		}
	}

	// Objective
	// list_pos and list_value are both have size num_items.
	// list_pos[i] and list_value[i] correspond.
	// list_value[i] is the coefficient for the term represented by list_pos[i].
	// for all i < (1 << numrvs), list_pos[i] represents H(set represented by the bits of i)
	//     Coeff*H(setA|setB) is represented equivalently as Coeff*H(setA U setB) - Coeff*H(setB)
	// for all i >= (1 << numrvs), i represents AL with index (i - (1 << numrvs))
	// num_items is the number of distince sets and ALs ended up with
	if(Objective)
	{
		Get_Expression_Into_Map(pos_val_m, Objective, RandomV.size());

		pdc->num_items_in_objective = (int) pos_val_m.size();
		pdc->list_pos_in_objective = (unsigned int*)calloc(pdc->num_items_in_objective, sizeof(unsigned int));
		pdc->list_value_in_objective = (double*)calloc(pdc->num_items_in_objective, sizeof(double));

		Get_Map_Into_C_Style_Expression(pos_val_m, pdc->list_pos_in_objective, pdc->list_value_in_objective);
	}

	// Dependencies
	// list_dependency is an array of size 2*num_dependency.
	//     For each pair, [2*i] is a binary list of all vars in Dependent_Vars and Given_Vars
	//     and [2*i+1] is a binary list of all vars in Given_Vars only
	pdc->num_dependency = Dependencies.size();
	if(Dependencies.size())
	{
		pdc->list_dependency = (int*)calloc(2 * pdc->num_dependency, sizeof(int));
		for(i = 0; i < Dependencies.size(); i++)
		{
			for(j = 0; j < Dependencies[i]->Dependent_Vars.size(); j++)
			{
				pdc->list_dependency[i*2] |= (1 << Dependencies[i]->Dependent_Vars[j]->Index);
			}
			for(j = 0; j < Dependencies[i]->Given_Vars.size(); j++)
			{
				pdc->list_dependency[i*2] |= (1 << Dependencies[i]->Given_Vars[j]->Index);
				pdc->list_dependency[i*2+1] |= (1 << Dependencies[i]->Given_Vars[j]->Index);
			}
		}
	}

	// Independences
	// list_independence, list_type_independence, and list_num_items_in_independence are all size num_independence
	// list_independence[i] has size list_num_items_in_independence[i]
	//
	//     if type is 0, size is n+1.
	//         The first n are (1 << index) of corresponding var.
	//         list[n] is all those ORd together.
	//     if type is 1, size is n+2.
	//         The first n are (1 << index) of corresponding var ORd together with list[n].
	//         list[n] is (1 << index) for ALL the "given" list.
	//         list[n+1] is all those ORd together.


	pdc->num_independence = Independences.size();
	if(Independences.size())
	{
		pdc->list_independence = (unsigned int**)calloc(pdc->num_independence, sizeof(unsigned int*));
		pdc->list_num_items_in_independence = (int*)calloc(pdc->num_independence, sizeof(int));
		pdc->list_type_independence = (char*)calloc(pdc->num_independence, sizeof(char));

		for(i = 0; i < Independences.size(); i++)
		{
			if(Independences[i]->Given_Vars.size())
			{
				pdc->list_type_independence[i] = 1;
				pdc->list_num_items_in_independence[i] = Independences[i]->Independent_Vars_Vecs.size() + 2;
			}
			else
			{
				pdc->list_type_independence[i] = 0;
				pdc->list_num_items_in_independence[i] = Independences[i]->Independent_Vars_Vecs.size() + 1;
			}

			pdc->list_independence[i] = (unsigned int*)calloc(pdc->list_num_items_in_independence[i], sizeof(unsigned int));

			// HERE BRENT
			for(j = 0; j < Independences[i]->Independent_Vars_Vecs.size(); j++)
			{
				num_to_or = (1 << Independences[i]->Independent_Vars_Vecs[j].front()->Index);
				for(unsigned int zz = 1; zz < Independences[i]->Independent_Vars_Vecs[j].size(); zz++)
					num_to_or ^= Independences[i]->Independent_Vars_Vecs[j][zz]->Index;
				pdc->list_independence[i][j] |= num_to_or;
				pdc->list_independence[i][pdc->list_num_items_in_independence[i]-1] |= num_to_or;
			}
			for(j = 0; j < Independences[i]->Given_Vars.size(); j++)
				for(k = 0; k < (unsigned int) pdc->list_num_items_in_independence[i]; k++)
					pdc->list_independence[i][k] |= (1 << Independences[i]->Given_Vars[j]->Index);
		}
	}

	// Constant Bounds
	pdc->num_constantbounds = Constant_Bounds.size();
	if(pdc->num_constantbounds)
	{
		pdc->list_pos_constantbounds = (unsigned int**)calloc(pdc->num_constantbounds, sizeof(unsigned int*));
		pdc->list_value_constantbounds = (double**)calloc(pdc->num_constantbounds, sizeof(double*));
		pdc->list_type_constantbounds = (char*)calloc(pdc->num_constantbounds, sizeof(char));
		pdc->list_rhs = (double*)calloc(pdc->num_constantbounds, sizeof(double));
		pdc->list_num_items_in_constantbounds = (int*)calloc(pdc->num_constantbounds, sizeof(int));

		FillBounds(Constant_Bounds, pdc->list_pos_constantbounds, pdc->list_value_constantbounds, pdc->list_type_constantbounds, pdc->list_rhs, pdc->list_num_items_in_constantbounds, RandomV.size());
	}

	// Bounds to Prove
	pdc->num_bounds = Bounds_To_Prove.size();
	if(pdc->num_bounds)
	{
		pdc->list_pos_bounds = (unsigned int**)calloc(pdc->num_bounds, sizeof(unsigned int*));
		pdc->list_value_bounds = (double**)calloc(pdc->num_bounds, sizeof(double*));
		pdc->list_type_bounds = (char*)calloc(pdc->num_bounds, sizeof(char));
		pdc->list_rhs_2prove = (double*)calloc(pdc->num_bounds, sizeof(double));
		pdc->list_num_items_in_bounds = (int*)calloc(pdc->num_bounds, sizeof(int));

		FillBounds(Bounds_To_Prove, pdc->list_pos_bounds, pdc->list_value_bounds, pdc->list_type_bounds, pdc->list_rhs_2prove, pdc->list_num_items_in_bounds, RandomV.size());
	}

	// sensititivies
	pdc->num_sensitivities = Sensitivities.size();
	if (pdc->num_sensitivities)
	{
		pdc->list_pos_sensitivities = (unsigned int**)calloc(pdc->num_sensitivities, sizeof(unsigned int*));
		pdc->list_value_sensitivities = (double**)calloc(pdc->num_sensitivities, sizeof(double*));
		pdc->list_num_items_in_sensitivities = (int*)calloc(pdc->num_sensitivities, sizeof(int));

		FillExpressions(Sensitivities, pdc->list_pos_sensitivities, pdc->list_value_sensitivities, pdc->list_num_items_in_sensitivities, RandomV.size());
	}

	// queries
	pdc->num_queries =Queries.size();
	if (pdc->num_queries)
	{
		pdc->list_pos_queries = (unsigned int**)calloc(pdc->num_queries, sizeof(unsigned int*));
		pdc->list_value_queries = (double**)calloc(pdc->num_queries, sizeof(double*));
		pdc->list_num_items_in_queries = (int*)calloc(pdc->num_queries, sizeof(int));

		FillExpressions(Queries, pdc->list_pos_queries, pdc->list_value_queries, pdc->list_num_items_in_queries, RandomV.size());
	}

	// Symmetries
	// Each row is a permutation of indices
	pdc->num_symmetrymap = Symmetries.size();
	if(Symmetries.size())
	{
		pdc->list_symmetry = (char**)calloc(pdc->num_symmetrymap, sizeof(char*));
		for(i = 0; i < Symmetries.size(); i++)
		{
			pdc->list_symmetry[i] = (char*)calloc(pdc->num_rvs, sizeof(char));
			for(j = 0; j < Symmetries[i]->Order.size(); j++)
				pdc->list_symmetry[i][j] = Symmetries[i]->Order[j]->Index;
		}
	}

	// Equivalence classes
	if(Equivalences2d.size() && Equivalences2d.front().size())
		pdc->equivalences = Equivalences2d;

	return pdc;
}

void CAI::Initialize_Problem_Description_C_Style(Problem_Description_C_Style* PD)
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

	PD->list_pos_queries = NULL;
	PD->list_value_queries = NULL;
	PD->list_num_items_in_queries = NULL;

	PD->list_pos_sensitivities = NULL;
	PD->list_value_sensitivities = NULL;
	PD->list_num_items_in_sensitivities = NULL;

	PD->list_symmetry = NULL;
	PD->equivalences.clear();
}

void CAI::Free_Problem_Description_C_Style(Problem_Description_C_Style* PD)
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

	if (PD->num_sensitivities > 0) {
		for (int i = 0; i < PD->num_sensitivities; i++) {
			free(PD->list_pos_sensitivities[i]);
			free(PD->list_value_sensitivities[i]);
		}
		free(PD->list_pos_sensitivities);
		free(PD->list_value_sensitivities);
		free(PD->list_num_items_in_sensitivities);
	}

	if (PD->num_queries > 0) {
		for (int i = 0; i < PD->num_queries; i++) {
			free(PD->list_pos_queries[i]);
			free(PD->list_value_queries[i]);
		}
		free(PD->list_pos_queries);
		free(PD->list_value_queries);
		free(PD->list_num_items_in_queries);
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

	PD->list_pos_sensitivities = NULL;
	PD->list_value_sensitivities = NULL;
	PD->list_num_items_in_sensitivities = NULL;

	PD->list_pos_queries = NULL;
	PD->list_value_queries = NULL;
	PD->list_num_items_in_queries = NULL;

	PD->equivalences.clear();
}

// All 5 lists are size num_constantbounds
// numitems is as in the objective function
// list_pos is as in the objective function
// list_value is as in the objective function
// list_type is (int)0 for >= and (int)2 for =
//     all <= are changed to >= with negation
// list_rhs is just a constant
void PrintBound(int numbounds, int* list_numitems, unsigned int** list_pos, double** list_value, char* list_type, double* list_rhs)
{
	int i, j;

	cout << "numbounds:" << endl << "\t" << numbounds << endl;

	cout << "list_numitems:" << endl;
	for(i = 0; i < numbounds; i++)
		cout << "\t" << list_numitems[i] << endl;

	cout << "list_pos:" << endl;
	for(i = 0; i < numbounds; i++)
	{
		cout << "\t";
		for(j = 0; j < list_numitems[i]; j++)
			cout << list_pos[i][j] << ", ";
		cout << endl;
	}

	cout << "list_value:" << endl;
	for(i = 0; i < numbounds; i++)
	{
		cout << "\t";
		for(j = 0; j < list_numitems[i]; j++)
			cout << list_value[i][j] << ", ";
		cout << endl;
	}

	cout << "list_type:" << endl;
	for(i = 0; i < numbounds; i++)
		cout << "\t" << (int)list_type[i] << endl;

	cout << "list_rhs:" << endl;
	for(i = 0; i < numbounds; i++)
		cout << "\t" << list_rhs[i] << endl;

}

// All 3 lists are size num_constantbounds
// numitems is as in the objective function
// list_pos is as in the objective function
// list_value is as in the objective function
void PrintExpression(int numexpressions, int* list_numitems, unsigned int** list_pos, double** list_value)
{
	int i, j;

	cout << "numexpressions:" << endl << "\t" << numexpressions << endl;

	cout << "list_numitems:" << endl;
	for (i = 0; i < numexpressions; i++)
		cout << "\t" << list_numitems[i] << endl;

	cout << "list_pos:" << endl;
	for (i = 0; i < numexpressions; i++)
	{
		cout << "\t";
		for (j = 0; j < list_numitems[i]; j++)
			cout << list_pos[i][j] << ", ";
		cout << endl;
	}

	cout << "list_value:" << endl;
	for (i = 0; i < numexpressions; i++)
	{
		cout << "\t";
		for (j = 0; j < list_numitems[i]; j++)
			cout << list_value[i][j] << ", ";
		cout << endl;
	}
}

void CAI::Print_Problem_Description_C_Style(Problem_Description_C_Style* PD)
{
	int i,j;

	// RVs are self-explanatory
	cout << "RVs" << endl;
	cout << "num_rvs:" << endl << "\t" << PD->num_rvs << endl;
	cout << "list_rvs:" << endl << "\t";
	for(int i = 0; i < PD->num_rvs; i++)
		cout << PD->list_rvs[i] << ", ";
	cout << endl;
	cout << endl;

	// ALs are self-explanatory
	cout << "ALs" << endl;
	cout << "num_add_LPvars:" << endl << "\t" << PD->num_add_LPvars << endl;
	cout << "list_add_LPvars:" << endl << "\t";
	for(i = 0; i < PD->num_add_LPvars; i++)
		cout << PD->list_add_LPvars[i] << ", ";
	cout << endl;
	cout << endl;

	// Objective
	// list_pos and list_value are both have size num_items.
	// list_pos[i] and list_value[i] correspond.
	// list_value[i] is the coefficient for the term represented by list_pos[i].
	// for all i < (1 << numrvs), list_pos[i] represents H(set represented by the bits of i)
	//     Coeff*H(setA|setB) is represented equivalently as Coeff*H(setA U setB) - Coeff*H(setB)
	// for all i >= (1 << numrvs), i represents AL with index (i - (1 << numrvs))
	// num_items is the number of distince sets and ALs ended up with
	cout << "Objective" << endl;
	cout << "num_items_in_objective:" << endl << "\t" << PD->num_items_in_objective << endl;
	cout << "list_pos_in_objective:" << endl << "\t";
	for(i = 0; i < PD->num_items_in_objective; i ++)
		cout << PD->list_pos_in_objective[i] << ", ";
	cout << endl;
	cout << "list_value_in_objective:" << endl << "\t";
	for(i = 0; i < PD->num_items_in_objective; i ++)
		cout << PD->list_value_in_objective[i] << ", ";
	cout << endl << endl;


	// Dependencies
	// list_dependency is an array of size 2*num_dependency.
	// For each pair, [2*i] is a binary list of all vars in Dependent_Vars and Given_Vars
	// and [2*i+1] is a binary list of all vars in Given_Vars only
	cout << "Dependencies" << endl;
	cout << "num_dependency:" << endl << "\t" << PD->num_dependency << endl;
	cout << "list_dependency:" << endl;
	for(i = 0; i < PD->num_dependency; i++)
		cout << "\t" << PD->list_dependency[i*2] << " " << PD->list_dependency[i*2+1] << endl;
	cout << endl;

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

	cout << "Independences" << endl;
	cout << "num_independence:" << endl << "\t" << PD->num_independence << endl;
	cout << "list_independence:" << endl;
	for(i = 0; i < PD->num_independence; i++)
	{
		cout << "\t";
		for(j = 0; j < PD->list_num_items_in_independence[i]; j++)
			cout << PD->list_independence[i][j] << ", ";
		cout << endl;
	}
	cout << "list_type_independence:" << endl;
	for(i = 0; i < PD->num_independence; i++)
		cout << "\t" << (int)PD->list_type_independence[i] << endl;
	cout << "list_num_items_in_independence:" << endl;
	for(i = 0; i < PD->num_independence; i++)
		cout << "\t" << PD->list_num_items_in_independence[i] << endl;
	cout << endl;

	cout << "Constant Bounds" << endl;
	PrintBound(PD->num_constantbounds, PD->list_num_items_in_constantbounds, PD->list_pos_constantbounds, PD->list_value_constantbounds, PD->list_type_constantbounds, PD->list_rhs);
	cout << endl << endl;

	cout << "Bounds to Prove" << endl;
	PrintBound(PD->num_bounds, PD->list_num_items_in_bounds, PD->list_pos_bounds, PD->list_value_bounds, PD->list_type_bounds, PD->list_rhs_2prove);
	cout << endl << endl;

	cout << "Sensitivities" << endl;
	PrintExpression(PD->num_sensitivities, PD->list_num_items_in_sensitivities, PD->list_pos_sensitivities, PD->list_value_sensitivities);
	cout << endl << endl;

	cout << "Queries" << endl;
	PrintExpression(PD->num_sensitivities, PD->list_num_items_in_sensitivities, PD->list_pos_sensitivities, PD->list_value_sensitivities);
	cout << endl << endl;

	// Symmetries
	// Each row is a permutation of indices
	cout << "Symmetries" << endl;
	cout << "num_symmetrymap:" << endl << "\t" << PD->num_symmetrymap << endl;
	for(i = 0; i < PD->num_symmetrymap; i++)
	{
		cout << "\t";
		for(j = 0; j < PD->num_rvs; j++)
			cout << (int)PD->list_symmetry[i][j] << ", ";
		cout << endl;
	}

	// Equivalence classes
	if (PD->equivalences.size() != 0) {
		cout << endl;
		cout << "Equivalence classes (not part of the old PD files, BTW)\n";
		for (i = 0; i < (int) PD->equivalences.size(); i++) {
			cout << "\t";
			for (j = 0; j < (int) PD->equivalences.size(); j++) {
				cout << PD->equivalences[i][j] <<  " ";
			}
			cout << endl;
		}
	}
}

// Reading from file to create Problem_Description
int CAI::Get_Command(std::istream& Input_Stream, Problem_Description* pd)
{
	istringstream ss;
	vector <string> sv;
	string s, l;
	json j1, j2;
	/* streampos onelineback; */

	/* onelineback = Input_Stream.tellg(); */
	if (!getline(Input_Stream, l))
		return 0;

	sv.clear();
	ss.clear();
	ss.str(l);
	while (ss >> s) sv.push_back(s);

	// Do nothing for empty lines
	if (sv.size() == 0)
	{
	}
	// Ignore comments
	else if (sv[0][0] == '#')
	{
	}
	// Print commands
	else if (sv[0] == "?")
	{
		print_commands(stdout);
	}
	// Quit
	else if (sv[0] == "Q")
	{
		return 0;
	}

	/* Process lines */
	else if (sv[0] == "PD")
	{
		if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		pd->Modify_Json_From_Cmd_Line_Before_From_Json(j1);
		pd->From_Json(j1);
	}
	else if(sv[0] == "CMD" || sv[0] == "OPT")
	{
		if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid CMD or OPT.");
		if(!j1.is_array()) throw SRE((string) "Get_Command(): CMD or OPT arg must be a json array.");
		pd->Cmd_Line_Modifiers.push_back(new Cmd_Line_Modifier("CMD",'+',j1));
	}
	// This is the same as PD but without the PD
	// Commenting this out on 20200709
	/* else if (sv[0][0] == '{')
	   {
	   if(!read_json(sv, 0, j1, Input_Stream))
	   {
	   Input_Stream.seekg(onelineback);
	   if(!read_json(sv, 1, j1, Input_Stream))
	   throw SRE((string) "Get_Command(): Invalid json on {.");
	   }
	   pd->Modify_Json_From_Cmd_Line_Before_From_Json(j1);
	   pd->From_Json(j1);
	   } */
	   else if (sv[0] == "RV")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_RV(j1);
	   }
	   else if (sv[0] == "AL")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_AL(j1);
	   }
	   else if (sv[0] == "O")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_O(j1);
	   }
	   else if (sv[0] == "D")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_D(j1);
	   }
	   else if (sv[0] == "I")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_I(j1);
	   }
	   else if (sv[0] == "BC")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_BC(j1);
	   }
	   else if (sv[0] == "S")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_S(j1);
	   }
	   else if (sv[0] == "EQ")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_EQ(j1);
	   }
	   else if (sv[0] == "QU")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_QU(j1);
	   }
	   else if (sv[0] == "SE")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_SE(j1);
	   }
	   else if (sv[0] == "BP")
	   {
		   if(!read_json(sv, 1, j1, Input_Stream)) throw SRE((string) "Get_Command(): Invalid json.");
		   pd->Key_BP(j1);
	   }
	   else if (sv[0] == "PDC")
	   {
		   Problem_Description_C_Style* pdc = pd->As_Problem_Description_C_Style();
		   Print_Problem_Description_C_Style(pdc);
	   }
	   else if (sv[0] == "DESTROY")
	   {
		   delete pd;
		   pd = new Problem_Description;
	   }
	   else if (sv[0] == "DESER")
	   {
		   Command_DESER(pd,sv);
	   }
	   else if (sv[0] == "CS")
	   {
		   pd->Check_Symmetry_Group();
	   }
	   else if (sv[0] == "SER")
	   {
		   Command_SER(pd,sv,0);
	   }
	   else if (sv[0] == "POF")
	   {
		   Command_SER(pd,sv,1);
	   }
	   else
	   {
		   throw SRE((string) "Get_Command(): Invalid command \"" + sv[0] + "\". Use '?' to print a list of commands.");
	   }

	   return 1;
}

void CAI::Command_SER(Problem_Description* pd, vector<std::string>& sv, int old_style)
{
	ofstream fout;

	if(sv.size() != 1 && sv.size() != 3)
		throw SRE((string) "Command_SER(): Too many arguments to SER.\nSER [-a|-t file],\nwhere a is append and t is truncate.\n");


	if(sv.size() == 1)
	{
		/* cout << "PD" << endl << pd->As_Json().dump(4) << endl; */
		if(old_style)
			pd->Print_Old_Format(cout);
		else
			cout << "PD" << endl << PD_Json_Pretty_Print(pd->As_Json()) << endl;

	}
	else
	{
		if(sv[1] == "-a")
			fout.open(sv[2].c_str(), ios::out | ios::app);
		else if(sv[1] == "-t")
			fout.open(sv[2].c_str(), ios::out | ios::trunc);
		else
			throw SRE((string) "Command_SER(): Bad argument to SER.\nSER [-a|-t file],\nwhere a is append and t is truncate.\n");

		if(!fout.is_open())
			throw SRE((string) "Command_SER(): Couldn't open file \"" + sv[2].c_str() + (string) "\".");

		/* fout << "PD" << endl << pd->As_Json().dump(4) << endl; */
		if(old_style)
			pd->Print_Old_Format(fout);
		else
			fout << "PD" << endl << PD_Json_Pretty_Print(pd->As_Json()) << endl;

		fout.close();
	}
}

void CAI::Command_DESER(Problem_Description* pd, vector<std::string>& sv)
{
	unsigned int i;
	std::ifstream ifs;

	if(sv.size() <= 1)
		return;

	if(sv[1] == "-")
	{
		while(Get_Command(cin, pd));
	}
	else
	{
		for(i = 1; i < sv.size(); ++i)
		{
			ifs.clear();
			ifs.open(sv[i],std::ifstream::in);
			if(!ifs.is_open())
				throw SRE((string) "Command_DESER(): Couldn't open file " + sv[i]);

			while(Get_Command(ifs, pd));

			ifs.close();
		}
	}
}

void CAI::Read_Problem_Description_From_File(Problem_Description* pd, std::string filename)
{
	std::vector<std::string> v;
	nlohmann::json j;

	if(filename == "-")
	{
		while(Get_Command(cin, pd));
	}
	else
	{
		v.resize(2);

		v[1] = filename;

		Command_DESER(pd, v);
	}

	pd->Run_Cmd_Line_Modifier_Commands();
}

void Problem_Description::Check_Equivalence() const
{
	for(unsigned int i = 0; i < Equivalences2d.size(); ++i)
		for(unsigned int k = 0; k < Equivalences2d[i].size(); ++k)
			if (Equivalences2d[i][k].size() != RandomV.size())
				throw SRE("Problem_Description::Check_Equivalence(): Error in problem description file -- equivalence class specification string's size must equal |RV|. Equivalences[" + std::to_string(i) + "] doesn't.");

	//printf("Check_Equivalence() succeeded\n");
}

// Reads json from vector or continues
// reading json from Input_Stream.
bool CAI::read_json(const std::vector <std::string> &sv, size_t starting_field, nlohmann::json &rv, std::istream& Input_Stream)
{
	string s;
	size_t i;

	rv.clear();
	if (starting_field < sv.size()) {
		s = sv[starting_field];
		for (i = starting_field+1; i < sv.size(); i++) {
			s += " ";
			s += sv[i];
		}
		try {
			rv = json::parse(s);
		} catch (...) {
			return false;
		}
		return true;
	}

	try {
		Input_Stream >> rv;
		getline(Input_Stream, s);
		return true;
	} catch (...) {
		return false;
	}
}

void CAI::print_commands(FILE *f)
{
	fprintf(f, "For commands that take a single json, either put it all on the line with the command,\n");
	fprintf(f, "or it can be multiple lines, starting on the line after the command.\n");
	fprintf(f, "\n");
	fprintf(f, "PD  json                     - Give entire problem description in one json\n");
	fprintf(f, "\n");
	fprintf(f, "RV  json_variable_array      - Add random variable(s)\n");
	fprintf(f, "AL  json_variable_array      - Add additional linear variable(s) \n");
	fprintf(f, "O   json_expression_array    - Add objective function\n");
	fprintf(f, "D   json_array               - Add dependency\n");
	fprintf(f, "I   json_array               - Add independence\n");
	fprintf(f, "BC  json_array               - Add constant bounds\n");
	fprintf(f, "S   json_array               - Add symmetry\n");
	fprintf(f, "EQ  string                   - Add equivalence classes\n");
	fprintf(f, "QU  string                   - Add query expressions\n");
	fprintf(f, "SE  string                   - Add sensitivity analysis\n");
	fprintf(f, "BP  json_array               - Add bounds to prove\n");
	fprintf(f, "\n");
	fprintf(f, "PDC                - Print the problem description after initial processing.\n");
	fprintf(f, "LP_DISP            - Print intermediate output in LP optimization.\n");
	fprintf(f, "\n");
	fprintf(f, "DESTROY            - Call the destructor, then create a new one.\n");
	fprintf(f, "DESER files        - Deserialize.\n");
	fprintf(f, "SER [-a|-t file]   - Serialize.\n");
	fprintf(f, "\n");
	fprintf(f, "Q                  - Quit.\n");
	fprintf(f, "?                  - Print commands.\n");
}
