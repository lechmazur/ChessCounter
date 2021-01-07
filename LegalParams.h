#pragma once

#include <random>
#include "position.h"
#include "types.h"
#include "pcg-cpp/include/pcg_random.hpp"
#include "random/jsf.hpp"
#include "random/gjrand.hpp"
#include "random/xoroshiro.hpp"
#include "random/jsf.hpp"
#include <omp.h>

struct OneComb
{
	int idx = -1;
	double val = -1;
};


using randGen = pcg64;		
//using randGen = jsf64;		

struct LegalParams
{
	const int KING_COMBINATIONS = 3612;
	static const inline std::vector<Piece> chooseFrom = 
		{ W_PAWN, W_BISHOP, W_KNIGHT, W_ROOK, W_QUEEN, B_PAWN, B_BISHOP, B_KNIGHT, B_ROOK, B_QUEEN };

	std::vector<Square> whiteKingLocs, blackKingLocs;				//King locations
	std::vector<StateListPtr> states, states2;						//Stockfish states
	mutable std::vector<randGen> rgensPCG;								//Random number generators, one per thread
	std::vector<OneComb> combs;
	std::vector<double> combsPartialSum;
	double combsSum = -1;
	std::vector<OneComb> combsWB;
	std::vector<double> combsWBPartialSum;
	double combsSumWB = -1;
	int bothInPawnsField = -1;
	int oneInPawnsField = -1;
	int noneInPawnsField = -1;
	std::vector<OneComb> combsRestricted, combsVeryRestricted;
	std::vector<double> combsRestrictedPartialSum, combsVeryRestrictedPartialSum;
	double combsSumRestricted = -1, combsSumVeryRestricted = -1;
	std::uniform_int_distribution<int> kingLocDistribution;
	const double kingsMult = 1.0 / KING_COMBINATIONS;

	template<typename Treal> 
	[[nodiscard]] Treal realRand(Treal minv, Treal maxv) const;
	void setupKingLocations();
	void setupSF(int argc, char* argv[]);
	[[nodiscard]] Piece pickPiece(int tnum) const;
	[[nodiscard]] Piece pickWPiece(int tnum) const;
	[[nodiscard]] Piece pickBPiece(int tnum) const;
	[[nodiscard]] Piece pickWPieceNoPawn(int tnum) const;
	[[nodiscard]] Piece pickBPieceNotPawn(int tnum) const;
	[[nodiscard]] int pickRandomKnownSum(const std::vector<OneComb>& probs, double sum, const std::vector<double>& partialSums) const;
	void setup(int argc, char* argv[], int nthreads);
	//Inclusive
	[[nodiscard]] inline int intRand(const int& minx, const int& maxx, int tnum) const
	{
		const std::uniform_int_distribution<int> distribution(minx, maxx);
		return intRand(tnum, distribution);
	}
	//Inclusive
	[[nodiscard]] inline int intRand(const int& minx, const int& maxx) const
	{
		return intRand(minx, maxx, omp_get_thread_num());
	}

	//Inclusive
	[[nodiscard]] inline int intRand(int tnum, const std::uniform_int_distribution<int>& distribution) const
	{
		return distribution(rgensPCG[tnum]);
	}

	[[nodiscard]] int drawNumOfPieces() const;	//draw the number of non-king pieces given probabilities
	[[nodiscard]] std::pair<int, int> drawNumOfWBPieces() const;
	[[nodiscard]] std::tuple<int, int, int, int, int, int, int, int, int, int> drawNumRestricted(int kingsInPawnSquares) const;
	[[nodiscard]] std::tuple<int, int, int, int, int, int, int, int, int, int> drawNumVeryRestricted(int kingsInPawnSquares) const;
};

