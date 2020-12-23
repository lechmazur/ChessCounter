#pragma once

#include <string>
#include <vector>
#include "position.h"

struct OpeningLimit
{
	std::string name;
	std::vector<Square> noPawnsLocationsWhite;
	std::vector<Square> noPawnsLocationsBlack;
	int missingWhite = -1;
	int missingBlack = -1;

	OpeningLimit() = delete;
	~OpeningLimit() = default;
	OpeningLimit(const std::string& nameIn, const std::vector<Square>& noPawnsLocationsWhiteIn, const std::vector<Square>& noPawnsLocationsBlackIn, int missingWhiteIn, int missingBlackIn) 
		: name(nameIn), noPawnsLocationsWhite(noPawnsLocationsWhiteIn), noPawnsLocationsBlack(noPawnsLocationsBlackIn), missingWhite(missingWhiteIn), missingBlack(missingBlackIn) {}
};

