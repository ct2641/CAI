#include "expression_helpers.hpp"
#include "nlohmann/json.hpp"
#include "problem_description.hpp"
#include <iostream>
#include <cstdio>
#include <string>
#include <cstdlib>

using namespace std;

int main()
{
	string a;
	string b;
	nlohmann::json j;

	a = "4.1A + 0.6B - H(A,B,C|D,E) >= -0.3";

	cout << a << endl;
	cout << endl << endl;

	j = Bound_String_To_Json(a);

	cout << j.dump(4) << endl;
	cout << endl << endl;

	b = Bound_Json_To_String(j);

	cout << b << endl;
	cout << endl << endl;

	return 0;
}
