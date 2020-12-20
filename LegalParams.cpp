#include "LegalParams.h"
#include "thread.h"
#include "uci.h"
#include <omp.h>
#include <random>

namespace PSQT {
void init();
}


//Inclusive
int LegalParams::intRand(const int& minx, const int& maxx)
{
	const std::uniform_int_distribution<int> distribution(minx, maxx);
	int ret;
	ret = distribution(rgensPCG[omp_get_thread_num()]);
	return ret;
}


//Inclusive
int64_t LegalParams::intRand64(const int64_t& minx, const int64_t& maxx)
{
	const std::uniform_int_distribution<int64_t> distribution(minx, maxx);
	int64_t ret;
	ret = distribution(rgensPCG[omp_get_thread_num()]);
	return ret;
}


void LegalParams::setupKingLocations()
{
	whiteKingLocs.clear();
	blackKingLocs.clear();
	whiteKingLocs.reserve(3612);
	blackKingLocs.reserve(3612);

	int tK = 0;
	for (Square wk = SQ_A1; wk <= SQ_H8; ++wk)
	{
		for (Square bk = SQ_A1; bk <= SQ_H8; ++bk)
		{
			int diffx = abs(file_of(wk) - file_of(bk));
			int diffy = abs(rank_of(wk) - rank_of(bk));
			if (diffx >= 2 || diffy >= 2)
			{
				whiteKingLocs.emplace_back(wk);
				blackKingLocs.emplace_back(bk);
				tK++;
			}
		}
	}
	//[Combinations of 30 chosen from 62] * 2 sides to move * 11^30
	//Combinations of 30 chosen from 62: 450,883,717,216,034,179
	//3612*450,883,717,216,034,179*2 * 11^30
	//3612*450,883,717,216,034,179*2 * 1.7449402e+31 = 5.6835913e+52

}

void LegalParams::setupSF(int argc, char* argv[])
{
	CommandLine::init(argc, argv);
	UCI::init(Options);
	Tune::init();
	PSQT::init();
	Bitboards::init();
	Position::init();
	Bitbases::init();
	Endgames::init();
	Threads.set(size_t(Options["Threads"]));
	Search::clear(); // After threads are up
	Eval::NNUE::init();
}

void LegalParams::setup(int argc, char* argv[], int nthreads)
{
	rgensPCG.resize(omp_get_max_threads());
	int64_t x = 5423;
	for (auto& r : rgensPCG)
	{
		r = pcg64(x);
		x += 3252351;
	}

	states.resize(nthreads);
	states2.resize(nthreads);
	for (auto& s : states)
		s = std::make_unique<std::deque<StateInfo>>(1);
	for (auto& s : states2)
		s = std::make_unique<std::deque<StateInfo>>(1);
	setupSF(argc, argv);
	setupKingLocations();
}

