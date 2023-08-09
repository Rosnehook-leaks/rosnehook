#pragma once
#include <string>
#include "settings/String.hpp"

class CSteamID;

extern std::string name_forced;
std::string GetNamestealName(CSteamID steam_id);