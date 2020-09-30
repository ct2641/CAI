#include "problem_description.hpp"
#include "nlohmann/json.hpp"
#include <string>

using namespace CAI;


nlohmann::json Expression_String_To_Json(const std::string& s);
std::string Expression_Json_To_String(const nlohmann::json& j);

nlohmann::json Bound_String_To_Json(const std::string& s);
std::string Bound_Json_To_String(const nlohmann::json& j);

std::string PD_Json_Pretty_Print(const nlohmann::json& j);
