#include "runner.h"
#include "LegalChecker.h"
#include "LegalParams.h"
#include <omp.h>
#include <algorithm>

using std::vector;

namespace PSQT {
void init();
}


void Runner::init()
{
	std::cout << engine_info() << std::endl;
}


template<typename TEl>
[[nodiscard]] static TEl vectorSumNU(const std::vector<TEl>& v)
{
	TEl sum = 0;
	for (const auto& el : v)
		sum += el;
	return sum;
}


void Runner::posEstimate(int argc, char* argv[])
{
	LegalParams lp;
	//Choose parameters
	lp.underpromotions = false;
	lp.maxQueensOneSide = 3;
	lp.maxQueensTotal = 6;
	const int nthreads = std::max(1, omp_get_max_threads() - 1);
	lp.setup(argc, argv, nthreads);

	const int64_t RUNS = 30'000'000'000;

	vector<int64_t> legal(nthreads, 0);
	vector<int64_t> all(nthreads, 0);

	vector<vector<int64_t>> byCount(nthreads);
	for (auto& b : byCount)
		b.resize(33, 0);


#pragma omp parallel for num_threads(nthreads)
	for (int64_t x = 0; x<RUNS; x++)
	{
		int tnum = omp_get_thread_num();
		all[tnum]++;
		LegalChecker lc;
		lc.prepare(&lp, tnum);
		lc.createCounts();
		bool isok = lc.checkConditions();

		if (isok)
		{
			int countAs = lc.countCastling();		//If there are castling possibilities, this position counts as multiple (2^castling_possibilties)
			int epPoss = lc.countEnPassantPossibilities();		//En-passant possibilities

			countAs *= (1 + epPoss);
			legal[tnum] += countAs;
			byCount[tnum][lc.totalPieces()] += countAs;
		}

		if (all[tnum] % (2048*2048) == 1)
		{
#pragma omp critical
			{
				auto totalGood = vectorSumNU(legal);
				auto totalAny = vectorSumNU(all);
				std::cout << "legal: " << totalGood << " / all: " << totalAny << "  fraction: " << (double)totalGood / totalAny 
					<< "  estimate of all legal positions " << (double)totalGood / totalAny * 5.6835913e+52 << std::endl;
			}
		}

	}
	auto totalGood = vectorSumNU(legal);
	std::cout << totalGood << " " << (double)totalGood / RUNS << " " << (double)totalGood / RUNS * 5.6835913e+52 << std::endl;
	for (int q=2; q<=32; q++)
	{
		int64_t s = 0;
		for (int t = 0; t < nthreads; t++)
			s += byCount[t][q];
		std::cout << q << "pieces: " << s << std::endl;
	}
}

