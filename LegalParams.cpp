#include "LegalParams.h"
#include "thread.h"
#include "uci.h"
#include <numeric>
#include <fstream>

using std::vector;
using std::cout;
using std::endl;

namespace PSQT {
void init();
}


//Inclusive
template<typename Treal>
Treal LegalParams::realRand(Treal minv, Treal maxv) const
{
	std::uniform_real_distribution<Treal> distribution(minv, maxv);
	Treal ret = distribution(rgensPCG[omp_get_thread_num()]);
	return ret;
}

template double LegalParams::realRand(double minv, double maxv) const;

void LegalParams::setupKingLocations()
{
	whiteKingLocs.clear();
	blackKingLocs.clear();
	whiteKingLocs.reserve(KING_COMBINATIONS);
	blackKingLocs.reserve(KING_COMBINATIONS);

	bothInPawnsField = 0;
	oneInPawnsField = 0;
	noneInPawnsField = 0;

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
				if (wk >= SQ_A2 && wk <= SQ_H7 && bk >= SQ_A2 && bk <= SQ_H7)
					bothInPawnsField++;
				else if ((wk < SQ_A2 || wk > SQ_H7) && (bk < SQ_A2 || bk > SQ_H7))
					noneInPawnsField++;
				else
					oneInPawnsField++;
			}
		}
	}
	cout << "Kings location in pawn squares:  None: " << noneInPawnsField << "  One: " << oneInPawnsField << "  Both: " << bothInPawnsField << endl;
	assert(whiteKingLocs.size() == KING_COMBINATIONS);
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


Piece LegalParams::pickPiece(int tnum) const
{
	return chooseFrom[intRand(0, 9, tnum)];
}

Piece LegalParams::pickWPiece(int tnum) const
{
	return chooseFrom[intRand(0, 4, tnum)];
}

Piece LegalParams::pickBPiece(int tnum) const
{
	return chooseFrom[intRand(5, 9, tnum)];
}

Piece LegalParams::pickWPieceNoPawn(int tnum) const
{
	return chooseFrom[intRand(1, 4, tnum)];
}

Piece LegalParams::pickBPieceNotPawn(int tnum) const
{
	return chooseFrom[intRand(6, 9, tnum)];
}


//Pick one of the indexes based on probabilities
int LegalParams::pickRandomKnownSum(const vector<OneComb>& probs, double sum, const vector<double>& partialSums) const
{
	assert(probs.size() > 0);
	assert(sum > 0);
	auto rv = realRand(0.0, sum);
	double tot = 0;


	auto lower = std::lower_bound(partialSums.begin(), partialSums.end(), rv);
	assert(lower != partialSums.end());
	auto q = std::max(0, int(std::distance(partialSums.begin(), lower) - 1));
	return probs[q].idx;
}


int LegalParams::drawNumOfPieces() const
{
	return pickRandomKnownSum(combs, combsSum, combsPartialSum);
}


std::pair<int,int> LegalParams::drawNumOfWBPieces() const
{
	int v = pickRandomKnownSum(combsWB, combsSumWB, combsWBPartialSum);
	return std::make_pair(v / 16, v % 16);
}

//Returns [wp, bp, wn, bn, wb, bb, wr, br, wq, bq]
std::tuple<int, int, int, int, int, int, int, int, int, int> LegalParams::drawNumRestricted(int kingsInPawnSquares) const
{
	//Not very efficient but it doesn't matter because random number generation takes up 80% of time
	int v = pickRandomKnownSum(combsRestricted, combsSumRestricted, combsRestrictedPartialSum);
	int wp = v / (4 * 4 * 3 * 3 * 3 * 3 * 3 * 3 * 9);
	int bp = v / (4 * 4 * 3 * 3 * 3 * 3 * 3 * 3) % 9;
	int wn = v / (4 * 4 * 3 * 3 * 3 * 3 * 3) % 3;
	int bn = v / (4 * 4 * 3 * 3 * 3 * 3) % 3;
	int wb = v / (4 * 4 * 3 * 3 * 3) % 3;
	int bb = v / (4 * 4 * 3 * 3) % 3;
	int wr = v / (4 * 4 * 3) % 3;
	int br = v / (4 * 4) % 3;
	int wq = v / 4  % 4;
	int bq = v % 4;
	return std::make_tuple(wp, bp, wn, bn, wb, bb, wr, br, wq, bq);
}

void LegalParams::setup(int argc, char* argv[], int nthreads)
{
	vector<int> slist(omp_get_max_threads());
	vector<uint32_t> seeds(slist.size());
	std::seed_seq seq(slist.begin(), slist.end());
	seq.generate(seeds.begin(), seeds.end());
	rgensPCG.resize(omp_get_max_threads());
	for (int x = 0; x < rgensPCG.size(); x++)
		rgensPCG[x] = randGen(seeds[x]);

	states.resize(nthreads);
	states2.resize(nthreads);
	for (auto& s : states)
		s = std::make_unique<std::deque<StateInfo>>(1);
	for (auto& s : states2)
		s = std::make_unique<std::deque<StateInfo>>(1);
	setupSF(argc, argv);
	setupKingLocations();

	kingLocDistribution = std::uniform_int_distribution<int>(0, int(whiteKingLocs.size() - 1));


   /*
from math import comb

f = open('u:/ChessCounter/restricted.txt','w')
c = 0
sum  = 0
xmax = 0

for wp in range(0, 9):
	 for bp in range(0, 9):
		 for wn in range(0, 3):
			 for bn in range(0, 3):
				 for wb in range(0, 3):
					 for bb in range(0, 3):
						 for wr in range(0, 3):
							 for br in range(0, 3):
								 for wq in range(0, min(4,16-wp-wn-wb-wr)):
									 for bq in range(0, min(4,16-bp-bn-bb-br)):
										 c += 1
										 x = comb(62,wp) * comb(62-wp,bp) * comb(62-wp-bp,wn) * comb(62-wp-bp-wn,bn) * comb(62-wp-bp-wn-bn,wb) * comb(62-wp-bp-wn-bn-wb,bb) * comb(62-wp-bp-wn-bn-wb-bb,wr) * comb(62-wp-bp-wn-bn-wb-bb-wr,br) * comb(62-wp-bp-wn-bn-wb-bb-wr-br,wq) * comb(62-wp-bp-wn-bn-wb-bb-wr-br-wq,bq)
										 sum += x
										 if (x>xmax):
											 xmax = x
											 print(xmax, wp,bp,wn,bn,wb,bb,wr,br,wq,bq)

										 f.write(str(c)+'\n')
										 f.write(str(float(x))+'\n')
print(3612*2*sum, c)
print(float(3612*2*sum))
f.close()

   */

	auto fsum = [](double sum, const OneComb& c2)
	{
		return sum + c2.val;
	};
	
	auto readNumbers = [&](std::string fname, int sz)
	{
		vector<OneComb> res(sz);
		std::fstream myfile(fname, std::ios_base::in);
		int idx = -1;
		int count = 0;
		while (myfile >> idx)
		{
			double v;
			myfile >> v;
			res[idx].idx = idx;
			res[idx].val = v;
			count++;
		}

		sort(res.begin(), res.end(), [](const OneComb& c1, const OneComb& c2) { return c1.val < c2.val; });

		vector<double> partialSum(sz + 1);
		double sum = 0;
		for (int q = 0; q < sz; q++)
		{
			partialSum[q] = sum;
			sum += res[q].val;
		}
		partialSum[sz] = sum;

		return make_pair(res, partialSum);
	};

	std::tie(combs, combsPartialSum) = readNumbers("combs.txt", 31);
	std::tie(combsWB, combsWBPartialSum) = readNumbers("combsWB.txt", 16*16);
	std::tie(combsRestricted, combsRestrictedPartialSum) = readNumbers("restricted.txt", 944774);

	combsSum = std::accumulate(combs.begin(), combs.end(), 0.0, fsum);
	cout << "Sample out of " << combsSum * 2.0 * KING_COMBINATIONS << " unique possible legal positions" << endl;

	combsSumWB = std::accumulate(combsWB.begin(), combsWB.end(), 0.0, fsum);
	cout << "Sample out of " << combsSumWB * 2.0 * KING_COMBINATIONS << " unique possible legal positions" << endl;

	combsSumRestricted = std::accumulate(combsRestricted.begin(), combsRestricted.end(), 0.0, fsum);
	cout << "Sample out of " << combsSumRestricted * 2.0 * KING_COMBINATIONS << " unique possible legal positions" << endl;
}

