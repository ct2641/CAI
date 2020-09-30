#include "expression_helpers.hpp"
#include "problem_description.hpp"
#include "nlohmann/json.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

using std::string;
using namespace CAI;

typedef std::runtime_error SRE;

void Remove_Spaces(string& s)
{
	unsigned int index;
	unsigned int i;

	index = 0;
	for(i = 0; i < s.size(); i++)
	{
		if(s[i] != ' ')
		{
			s[index] = s[i];
			index++;
		}
	}

	s.resize(index);
}


string Double_To_String(double d)
{
	string s;
	std::size_t decimal;
	std::size_t i;

	s = std::to_string(d);
	decimal = s.find('.');

	if(decimal == string::npos)
		return s;

	for(i = s.size() - 1; i >= decimal; i--)
	{
		if(s[i] == '0')
			s[i] = ' ';
		else if(s[i] == '.')
		{
			s[i] = ' ';
			break;
		}
		else
			break;
	}

	Remove_Spaces(s);

	return s;
}


void Check_If_Number(const string& s)
{
	unsigned int found_period;
	unsigned int i = 0;

	found_period = 0;

	for(i = 0; i < s.size(); i++)
	{
		if(s[i] == '-')
		{
			if(i)
				throw SRE("Check_If_Number(): Number \"" + s + "\" contains a negative sign not at the beginning.");
		}
		else if(s[i] == '.')
		{
			if(found_period)
				throw SRE("Check_If_Number(): Number \"" + s + "\" contains more than 1 period.");
			found_period = 1;
		}
		else if(s[i] < '0' || s[i] > '9')
		{
			throw SRE("Check_If_Number(): Number \"" + s + "\" contains characters that aren't digits or period.");
		}
	}
}

int Is_Alpha(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int Contains_Alpha(const string& s)
{
	for(unsigned int i = 0; i < s.size(); i++)
		if(Is_Alpha(s[i]))
			return 1;
	return 0;
}

string Get_String_Size_One(char c)
{
	string s;
	s.clear();
	s.push_back(c);
	return s;
}

nlohmann::json Term_String_To_Json(const string& the_param)
{
	nlohmann::json j;
	nlohmann::json j1;
	string s;
	unsigned int i;
	unsigned int k;
	unsigned int seen_pipe;
	unsigned int seen_semicolon;
	std::size_t found;

	s = the_param;
	Remove_Spaces(s);

	if(!s.size())
		throw SRE("Term_String_To_Json(): Term has size zero.");


	if(Is_Alpha(s[0]))
	{
		if(s[0] == 'H' && s.size() > 1 && s[1] == '(')
		{
			// entropy term
			j = nlohmann::json::array();
			j.push_back("H");

			if(s.back() != ')')
				throw SRE("Term_String_To_Json(): Last character of entropy term isn't ')'. Term = \"" + s + "\".");

			j1 = nlohmann::json::array();
			k = 2;
			seen_pipe = 0;
			for(i = 2; i < s.size(); i++)
			{
				if(s[i] == ',')
				{
					j1.push_back(s.substr(k, i - k));
					k = i + 1;
				}
				else if(s[i] == '|')
				{
					if(seen_pipe)
						throw SRE("Term_String_To_Json(): Multiple pipes in \"" + s + "\".");
					seen_pipe = 1;

					j1.push_back(s.substr(k, i - k));
					k = i + 1;

					j.push_back(j1);
					j1 = nlohmann::json::array();
				}
				else if(s[i] == ')')
				{
					j1.push_back(s.substr(k, i - k));
					j.push_back(j1);
				}
			}

			return j;
		}
		else if(s[0] == 'I' && s.size() > 1 && s[1] == '(')
		{
			// information term
			j = nlohmann::json::array();
			j.push_back("I");

			if(s.back() != ')')
				throw SRE("Term_String_To_Json(): Last character of information term isn't ')'. Term = \"" + s + "\".");

			j1 = nlohmann::json::array();
			k = 2;
			seen_semicolon = 0;
			seen_pipe = 0;
			for(i = 2; i < s.size(); i++)
			{
				if(s[i] == ',')
				{
					j1.push_back(s.substr(k, i - k));
					k = i + 1;
				}
				else if(s[i] == ';')
				{
					if(seen_semicolon)
						throw SRE("Term_String_To_Json(): Multiple ; in \"" + s + "\".");
					seen_semicolon = 1;

					j1.push_back(s.substr(k, i - k));
					k = i + 1;

					j.push_back(j1);
					j1 = nlohmann::json::array();
				}
				else if(s[i] == '|')
				{
					if(seen_pipe)
						throw SRE("Term_String_To_Json(): Multiple pipes in \"" + s + "\".");
					seen_pipe = 1;

					j1.push_back(s.substr(k, i - k));
					k = i + 1;

					j.push_back(j1);
					j1 = nlohmann::json::array();
				}
				else if(s[i] == ')')
				{
					j1.push_back(s.substr(k, i - k));
					j.push_back(j1);
				}
			}

			if(!seen_semicolon)
				throw SRE("Term_String_To_Json(): Information term without ; in\"" + s + "\".");

			return j;
		}
		else
		{
			// Var
			j = s;
			return j;
		}
	}
	else
	{
		// Entropy term with coefficient
		if((found = s.find("H(")) != string::npos)
		{
			j = nlohmann::json::array();
			j.push_back(std::stod(s.substr(0, found)));
			j.push_back(Term_String_To_Json(s.substr(found, string::npos)));
			return j;
		}
		// Information term with coefficient
		if((found = s.find("I(")) != string::npos)
		{
			j = nlohmann::json::array();
			j.push_back(std::stod(s.substr(0, found)));
			j.push_back(Term_String_To_Json(s.substr(found, string::npos)));
			return j;
		}
		// Var with coefficient
		else if(Contains_Alpha(s))
		{
			for(i = 0; i < s.size(); i++)
				if(Is_Alpha(s[i]))
					break;

			j = nlohmann::json::array();
			j.push_back(std::stod(s.substr(0,i)));
			j.push_back(Term_String_To_Json(s.substr(i, string::npos)));
			return j;
		}
		// Should be constant
		else
		{
			j = std::stod(s);
			return j;
		}
	}
}

nlohmann::json Expression_String_To_Json(const string& thestring)
{
	nlohmann::json j;
	std::vector<unsigned int> operator_indices;
	unsigned int i;
	string sub;
	string s;

	s = thestring;

	Remove_Spaces(s);

	j = nlohmann::json::array();

	for(i = 1; i < s.size(); i++)
		if(s[i] == '+' || s[i] == '-')
			operator_indices.push_back(i);

	// one term
	if(!operator_indices.size())
	{
		j.push_back(Term_String_To_Json(s));
		return j;
	}
	// more than one term
	else
	{
		sub = s.substr(0, operator_indices[0]);
		j.push_back(Term_String_To_Json(sub));

		for(i = 0; i < operator_indices.size() - 1; i++)
		{
			j.push_back(Get_String_Size_One(s[operator_indices[i]]));

			sub = s.substr(operator_indices[i] + 1, operator_indices[i+1] - operator_indices[i] - 1);
			j.push_back(Term_String_To_Json(sub));
		}

		j.push_back(Get_String_Size_One(s[operator_indices.back()]));

		sub = s.substr(operator_indices.back() + 1, string::npos);
		j.push_back(Term_String_To_Json(sub));

		return j;
	}
}

string Expression_Json_To_String(const nlohmann::json& j)
{
	string s;
	nlohmann::json j1;
	nlohmann::json j2;
	nlohmann::json::const_iterator it;
	nlohmann::json::const_iterator it2;
	double d;
	string type;

	if(!j.is_array())
		throw SRE("Expression_Json_To_String(): j isn't an array.");

	s.clear();

	for(it = j.begin(); it != j.end(); ++it)
	{
		// +, -, V
		if((*it).is_string())
		{
			s += static_cast<std::string const&>(*it);
		}
		// C
		else if((*it).is_number())
		{
			d = *it;
			s += Double_To_String(d);
		}
		// H, I, OR Coefficient != 1
		else if((*it).is_array() && (*it).size())
		{
			if((*it).at(0).is_string())
			{
				j1 = *it;

				if(j1.at(0) != "H" && j1.at(0) != "I")
					throw SRE((string) "Expression_Json_To_String(): Term is an array starting with a string that isn't \"H\" or \"I\". j = \n" + j1.dump(4));

				type = static_cast<std::string const&>(j1.at(0));

				if(type == "H")
				{
					if(j1.size() < 2 || !j1.at(1).is_array() || (j1.size() == 3 && !j1.at(2).is_array()) || j1.size() > 3)
						throw SRE((string) "Expression_Json_To_String(): Term is an entropy term but isn't of the form [\"H\",[],[]]. j = \n" + j1.dump(4));
				}
				else
				{
					if(j1.size() < 3 || !j1.at(1).is_array() || !j1.at(2).is_array() || (j1.size() == 4 && !j1.at(3).is_array()) || j1.size() > 4)
						throw SRE((string) "Expression_Json_To_String(): Term is an information term but isn't of the form [\"I\",[],[],[]]. j = \n" + j1.dump(4));
				}

				s += type;
				s += "(";

				j2 = j1.at(1);
				if(!j2.size())
					throw SRE("Expression_Json_To_String(): 2nd entry in entropy or information term has size 0.");

				for(it2 = j2.begin(); it2 != j2.end(); ++it2)
				{
					if(!(*it2).is_string())
						throw SRE("Expression_Json_To_String(): 2nd entry in entropy or information term contains non-string.");

					if(it2 != j2.begin())
						s += ",";

					s += static_cast<std::string const&>(*it2);
				}

				if(j1.size() >= 3 && j1.at(2).size())
				{
					if(type == "H")
						s += "|";
					else
						s += ";";

					j2 = j1.at(2);

					for(it2 = j2.begin(); it2 != j2.end(); ++it2)
					{
						if(!(*it2).is_string())
							throw SRE("Expression_Json_To_String(): 3rd entry in entropy or information term contains non-string.");

						if(it2 != j2.begin())
							s += ",";

						s += static_cast<std::string const&>(*it2);
					}
				}
				else if(type == "I")
				{
					throw SRE("Expression_Json_To_String(): 3rd entry in information term has size 0.");
				}

				if(j1.size() == 4 && j1.at(3).size())
				{
					s += "|";

					j2 = j1.at(3);

					for(it2 = j2.begin(); it2 != j2.end(); ++it2)
					{
						if(!(*it2).is_string())
							throw SRE("Expression_Json_To_String(): 4th entry in information term contains non-string.");

						if(it2 != j2.begin())
							s += ",";

						s += static_cast<std::string const&>(*it2);
					}
				}

				s += ")";
			}
			else if((*it).at(0).is_number())
			{
				if((*it).size() != 2)
					throw SRE((string) "Expression_Json_To_String(): j = \n" + (*it).dump(4));
				if((*it).at(1).is_string() && ((*it).at(1) == "+" || (*it).at(1) == "-"))
					throw SRE((string) "Expression_Json_To_String(): There's a term which is just a coefficient before a \"+\" or a \"-\". j = \n" + j.dump(4));

				d = (*it).at(0);
				s += Double_To_String(d);

				if((*it).at(1).is_array())
				{
					j1 = (*it).at(1);

					if(j1.at(0) != "H" && j1.at(0) != "I")
						throw SRE((string) "Expression_Json_To_String(): Term is an array starting with a string that isn't \"H\" or \"I\". j = \n" + j1.dump(4));

					type = static_cast<std::string const&>(j1.at(0));

					if(type == "H")
					{
						if(j1.size() < 2 || !j1.at(1).is_array() || (j1.size() == 3 && !j1.at(2).is_array()) || j1.size() > 3)
							throw SRE((string) "Expression_Json_To_String(): Term is an entropy term but isn't of the form [\"H\",[],[]]. j = \n" + j1.dump(4));
					}
					else
					{
						if(j1.size() < 3 || !j1.at(1).is_array() || !j1.at(2).is_array() || (j1.size() == 4 && !j1.at(3).is_array()) || j1.size() > 4)
							throw SRE((string) "Expression_Json_To_String(): Term is an information term but isn't of the form [\"I\",[],[],[]]. j = \n" + j1.dump(4));
					}

					s += type;
					s += "(";

					j2 = j1.at(1);
					if(!j2.size())
						throw SRE("Expression_Json_To_String(): 2nd entry in entropy or information term has size 0.");

					for(it2 = j2.begin(); it2 != j2.end(); ++it2)
					{
						if(!(*it2).is_string())
							throw SRE("Expression_Json_To_String(): 2nd entry in entropy or information term contains non-string.");

						if(it2 != j2.begin())
							s += ",";

						s += static_cast<std::string const&>(*it2);
					}

					if(j1.size() >= 3 && j1.at(2).size())
					{
						if(type == "H")
							s += "|";
						else
							s += ";";

						j2 = j1.at(2);

						for(it2 = j2.begin(); it2 != j2.end(); ++it2)
						{
							if(!(*it2).is_string())
								throw SRE("Expression_Json_To_String(): 3rd entry in entropy or information term contains non-string.");

							if(it2 != j2.begin())
								s += ",";

							s += static_cast<std::string const&>((*it2));
						}
					}
					else if(type == "I")
					{
						throw SRE("Expression_Json_To_String(): 3rd entry in information term has size 0.");
					}

					if(j1.size() == 4 && j1.at(3).size())
					{
						s += "|";

						j2 = j1.at(3);

						for(it2 = j2.begin(); it2 != j2.end(); ++it2)
						{
							if(!(*it2).is_string())
								throw SRE("Expression_Json_To_String(): 4th entry in information term contains non-string.");

							if(it2 != j2.begin())
								s += ",";

							s += static_cast<std::string const&>(*it2);
						}
					}

					s += ")";
				}
				else if((*it).at(1).is_string())
				{
					s += static_cast<std::string const&>((*it).at(1));
				}
				else
					throw SRE((string) "Expression_Json_To_String(): There's a term which is a coefficient followed by something that's not a string or an array. j = \n" + (*it).dump(4));
			}
			else
			{
				throw SRE((string) "Expression_Json_To_String(): Term is an array that doesn't start with a number or with \"H\". j = \n" + (*it).dump(4));
			}
		}
		else
		{
			throw SRE("Expression_Json_To_String(): Term isn't string, number, or array with positive size.");
		}

		s += " ";
	}

	while(s.back() == ' ')
		s.resize(s.size()-1);

	return s;
}

nlohmann::json Bound_String_To_Json(const string& s)
{
	nlohmann::json j;
	nlohmann::json exp_j;
	string type_s;
	string rhs_s;
	string tmp;
	std::size_t equals_index;

	if((equals_index = s.find("=")) == string::npos)
		throw SRE("Bound_String_To_Json(): Bound doesn't contain \"=\".");

	if(equals_index == 0)
		throw SRE("Bound_String_To_Json(): equals_index is 0");


	// Get type
	type_s.clear();
	if(s.at(equals_index - 1) == '<' || s.at(equals_index - 1) == '>')
		type_s.push_back(s.at(equals_index - 1));
	type_s.push_back('=');


	// Get LHS
	exp_j = Expression_String_To_Json(s.substr(0, (type_s.size() == 2) ? equals_index - 1 : equals_index));


	// Get RHS
	rhs_s.clear();
	rhs_s = s.substr(equals_index + 1, string::npos);
	Remove_Spaces(rhs_s);
	Check_If_Number(rhs_s);


	// Put them together
	j["LHS"] = exp_j;
	j["type"] = type_s;
	j["RHS"] = std::stod(rhs_s);

	return j;
}
string Bound_Json_To_String(const nlohmann::json& j)
{
	string s;
	double d;

	if(!j.is_object())
		throw SRE("Bound_Json_To_String(): j isn't an object.");

	if(j.find("LHS") == j.end() || j.find("type") == j.end() || j.find("RHS") == j.end())
		throw SRE("Bound_Json_To_String(): j isn't of the form {\"LHS\" : [] , \"type\" : "" , \"RHS\" : [num]}.");

	s.clear();

	d = j["RHS"];

	s = Expression_Json_To_String(j["LHS"]);
	s += " ";
	s += static_cast<std::string const&>(j["type"]);
	s += " ";
	s += Double_To_String(d);

	return s;
}

static string PD_PP_Array(const nlohmann::json &j,const string &key)
{
	size_t i;
	string s;

	if (j.contains(key)) {
		if (!j[key].is_array()) throw SRE("Pkey_Json_Pretty_Print -- Key key needs to be an array");
		if (j[key].size() <= 1) {
			s += ((string) ",\n  \"" + key + "\": " + j[key].dump());
			return s;
		}
		s += ((string) ",\n  \"" + key + "\": [\n    " + j[key][0].dump());
		for (i = 1; i < j[key].size(); i++) {
			s += ((string) ",\n    " + j[key][i].dump());
		}
		s += " ]";
	}
	return s;
}

string PD_Json_Pretty_Print(const nlohmann::json& j)
{
	string s;

	if (!j.is_object()) throw SRE("PD_Json_Pretty_Print - not given json object.");
	s = "{\n";
	if (j.contains("RV")) {
		s += ((string) "  \"RV\": " + j["RV"].dump());
	} else throw SRE("PD_Json_Pretty_Print -- json needs key RV");

	if (j.contains("AL")) s += ((string) ",\n  \"AL\": " + j["AL"].dump());
	if (j.contains("O")) s += ((string) ",\n  \"O\": " + j["O"].dump());
	s += PD_PP_Array(j, "D");
	s += PD_PP_Array(j, "I");
	s += PD_PP_Array(j, "BC");
	s += PD_PP_Array(j, "BP");
	s += PD_PP_Array(j, "S");
	s += PD_PP_Array(j, "EQ");
	s += PD_PP_Array(j, "QU");
	s += PD_PP_Array(j, "SE");
	s += "\n}\n";
	return s;
}
