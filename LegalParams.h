#pragma once

#include "position.h"
#include "types.h"
#include "pcg-cpp/include/pcg_random.hpp"

struct LegalParams
{
	bool underpromotions = false;	//Is promotion to minor pieces allowed
	int maxQueensOneSide = -1;		//-1 == no max
	int maxQueensTotal = -1;		//-1 == no max
	static const inline std::vector<Piece> chooseFrom = 
		{ W_PAWN, B_PAWN, W_BISHOP, B_BISHOP, W_ROOK, B_ROOK, W_KNIGHT, B_KNIGHT, W_QUEEN, B_QUEEN, NO_PIECE };

	std::vector<Square> whiteKingLocs, blackKingLocs;	//King locations
	std::vector<StateListPtr> states, states2;			//SF states
	std::vector<pcg64> rgensPCG;								//Random number generators, one per thread


	void setupKingLocations();
	void setupSF(int argc, char* argv[]);
	void setup(int argc, char* argv[], int nthreads);
	[[nodiscard]] int intRand(const int& minx, const int& maxx);
	[[nodiscard]] int64_t intRand64(const int64_t& minx, const int64_t& maxx);
};

