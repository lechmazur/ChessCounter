#include "runner.h"
#include "LegalChecker.h"
#include "LegalParams.h"
#include <omp.h>
#include <algorithm>
#include <chrono>

using std::vector;
using std::cout;
using std::endl;


namespace PSQT {
void init();
}


void Runner::init()
{
	cout << engine_info() << endl;
	//rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6
	openingsToCheck.emplace_back(OpeningLimit("Najdorf", { SQ_D2, SQ_E2 }, { SQ_A7, SQ_C7, SQ_D7 }, 1, 1));
	//rnbqkb1r/1p3ppp/p2p1n2/4p3/3NP3/2N1B3/PPP2PPP/R2QKB1R w KQkq - 0 7
	openingsToCheck.emplace_back(OpeningLimit("Najdorf English Attack", { SQ_D2, SQ_E2 }, { SQ_A7, SQ_C7, SQ_D7, SQ_E7 }, 1, 1));
	//rnbqkb1r/pppp1ppp/4pn2/8/2PP4/8/PP2PPPP/RNBQKBNR w KQkq - 0 3
	openingsToCheck.emplace_back(OpeningLimit("Indian Game Normal Variation", { SQ_C2, SQ_D2 }, { SQ_E7 }, 0, 0));
	//rnbqkb1r/p1pp1ppp/1p2pn2/8/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 4
	openingsToCheck.emplace_back(OpeningLimit("Queen's Indian Defense", { SQ_C2, SQ_D2 }, { SQ_E7, SQ_B7 }, 0, 0));
	//rnbqkb1r/ppp2ppp/4pn2/3p4/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 4
	openingsToCheck.emplace_back(OpeningLimit("Indian Gr�nfeld Defense", { SQ_C2, SQ_D2 }, { SQ_D7, SQ_E7 }, 0, 0));
	//rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq - 0 2
	openingsToCheck.emplace_back(OpeningLimit("Queen's Gambit", { SQ_C2, SQ_D2 }, { SQ_D7 }, 0, 0));
	//rnbqkbnr/pp2pppp/2p5/3p4/2PP4/8/PP2PPPP/RNBQKBNR w KQkq - 0 3
	openingsToCheck.emplace_back(OpeningLimit("Slav Defense", { SQ_C2, SQ_D2 }, { SQ_D7, SQ_C7 }, 0, 0));
	//rnbqkb1r/ppp2ppp/5n2/3p4/3P4/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 5
	openingsToCheck.emplace_back(OpeningLimit("Queen's Gambit Declined Exchange Variation", { SQ_C2, SQ_D2 }, { SQ_D7, SQ_E7 }, 1, 1));
	//r1bqk2r/1pppbppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 4 6
	openingsToCheck.emplace_back(OpeningLimit("Ruy Lopez: Closed", { SQ_E2 }, { SQ_A7, SQ_E7 }, 0, 0));
	//r1bq1rk1/2p1bppp/p1np1n2/1p2p3/4P3/1BP2N1P/PP1P1PP1/RNBQR1K1 b - - 0 9
	openingsToCheck.emplace_back(OpeningLimit("Ruy Lopez: Closed Main Line", { SQ_E2, SQ_C2, SQ_H2 }, { SQ_A7, SQ_B7, SQ_D7, SQ_E7 }, 0, 0));
	//rnbq1rk1/ppp2pbp/3p1np1/4p3/2PPP3/2N2N2/PP2BPPP/R1BQK2R w KQ - 0 7
	openingsToCheck.emplace_back(OpeningLimit("King's Indian Defense: Orthodox Variation", { SQ_E2, SQ_D2, SQ_E2, SQ_D3 }, { SQ_D7, SQ_E7}, 0, 0));
	//rnbqkb1r/ppp2ppp/3p4/8/4n3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 5
	openingsToCheck.emplace_back(OpeningLimit("Russian Game", { SQ_E2 }, { SQ_D7, SQ_E7 }, 1, 1));
}


template<typename TEl>
[[nodiscard]] static TEl vectorSum(const std::vector<TEl>& v)
{
	TEl sum = 0;
	for (const auto& el : v)
		sum += el;
	return sum;
}


bool checkFromFen(const std::string& fen, LegalParams& lp, bool restricted)
{
	LegalChecker lc;
	lc.init(&lp, omp_get_thread_num());
	lc.fromFen(fen);
	lc.createCounts();
	lc.createTotalCounts();
	bool isok = lc.checkConditions();
	if (isok && restricted)
		isok = lc.checkAdditionalConditions(false, 3, 6);
	cout << fen << "  result = " << isok << endl;
	return isok;
}


void validate(LegalParams& lp)
{
	vector<std::pair<std::string, bool>> fensR =
	{
		{"r3bK2/pPp2R1p/3Ppq1P/3k4/2r2P2/1p1p1PRQ/1Bn2Pp1/BN2Q2N w - - 0 1", false}
	};

	for (const auto& [fen, val] : fensR)
	{
		assert(checkFromFen(fen, lp, true) == val);
	}

	vector<std::pair<std::string, bool>> fens =
	{
		{"rnbqkb1r/ppppppp1/8/7P/7p/8/PPPPPP1P/RNBQKBNR w KQkq - 0 2", true},
		{"rnbqkbnr/ppppppp1/8/7P/7p/8/PPPPPPP1/RNBQKBNR w KQkq - 0 2", false},
		{"rnbqkbnr/pppppp2/7P/7P/7p/8/PPPPPP2/RNBQKBNR w KQkq - 0 2", false},
		{"rnbqkbnr/ppppp1pp/5P2/5p2/8/8/PPPPP1PP/RNBQKBNR w KQkq - 0 2", false},
		{"rnbqkb1r/ppppp1pp/5P2/5P2/8/5p2/PPPPP2P/RNBQKBNR w KQkq - 0 2", false},
		{"rnbqkb1r/pppppp1p/8/6P1/6p1/8/PPPPP1PP/RNBQKBNR w KQkq - 0 2", true},
		{"8/kqrqN1P1/6Q1/1PP1KB1P/n1p1bQp1/bQn1ppN1/RBP3rp/2qR4 w - - 0 1", false},
		{"6k1/8/4n3/1P6/P4n2/3B3R/4BB2/K7 w - - 0 8", true},
		{"rnbqkbnr/q1q1q1q1/8/Q2Q1QQ1/8/8/1Q1Q1Q1R/RNBQKBNR w KQkq - 0 8", true},
		{"2bqkbnr/2pppppp/8/1p6/8/3Q4/PQQPPPPP/QQ1QKBNR w Kk - 0 8", false},
		{"rnbqkbnr/ppppq1pp/8/8/8/8/PPPPQQPP/RNBQKBNR w KQkq - 0 4", true},
		{"rnbqkbnr/pppppqqp/8/8/8/8/PPPPPQQP/RNBQKBNR w KQkq - 0 2", false},
		{"rn1qkbnr/ppp1pq1p/8/8/3pQ3/8/PPPP1QQP/RNBQKBNR w KQkq - 0 4", true},
		{"rn1qkbnr/ppp1pq1p/8/8/P2pQ3/8/PPPP1QQP/RNBQKBNR w KQkq - 0 4", false},
		{"rnbqkbnr/pppppppp/8/8/8/8/PPNPPPPP/RNBQKBNR w KQkq - 0 4", false},
		{"rnb1kbnr/ppp1pppp/5q2/8/2pQ4/8/PP1PPPPP/RNBQKB1R w KQkq - 0 8", true},
		{"2bqkbnr/p1pppppp/8/1p6/8/8/PQQPPPPP/RNBQKBNR w KQk - 0 8", true},
		{"2bqkbnr/p1pppppp/8/1p6/8/3Q4/PQQPPPPP/RN1QKBNR w KQk - 0 8", false}
	};

	for (const auto& [fen, val] : fens)
	{
		assert(checkFromFen(fen, lp, false) == val);
	}
}

void Runner::posEstimate(int argc, char* argv[])
{
	LegalParams lp;

	const int nthreads = std::max(1, omp_get_max_threads() - 1);
	lp.setup(argc, argv, nthreads);

	const int64_t RUNS = 1'000'000'000'000'000;
	const ESampleType sampleType = ESampleType::PIECES_WB;				//Choose which type of sampling to run
	cout << endl << "Running ";
	if (sampleType == ESampleType::PIECES)
		cout << "PIECES: estimating legal positions from a general case. Slow, used to validate PIECES_WB " << endl;
	else if (sampleType == ESampleType::PIECES_WB)
		cout << "PIECES_WB: estimating legal positions when white and black pieces are chosen separately " << endl;
	else if (sampleType == ESampleType::WB_RESTRICTED)
		cout << "WB_RESTRICTED: estimating legal positions without underpromotions and with at most 3 queens per side from the search space with white and black pieces chosen separately. Slow, used to validate RESTRICTED" << endl;
	else if (sampleType == ESampleType::RESTRICTED)
			cout << "RESTRICTED: estimating legal positions without underpromotions and with at most 3 queens per side" << endl;
		
		const double totalPossibilities = (sampleType == ESampleType::PIECES ? lp.combsSum :
		(sampleType == ESampleType::PIECES_WB || sampleType == ESampleType::WB_RESTRICTED) ? lp.combsSumWB : lp.combsSumRestricted) * 2.0 * lp.KING_COMBINATIONS;	//* 2 because WTM and BTM

	vector<int64_t> legal(nthreads, 0), legalRestricted(nthreads, 0), all(nthreads, 0);
	vector<vector<int64_t>> byCount(nthreads), byCountRestricted(nthreads);
	vector<vector<int64_t>> legalOpeningsCount(openingsToCheck.size());
	for (auto& lo : legalOpeningsCount)
		lo.resize(nthreads, 0);
	for (auto& b : byCount)
		b.resize(33, 0);
	for (auto& b : byCountRestricted)
		b.resize(33, 0);

	vector<vector<int64_t>> kingSquares(64);
	for (auto& k : kingSquares)
		k.resize(nthreads, 0);

	validate(lp);

	vector<vector<vector<int64_t>>> pieceCountsByThread(nthreads);
	for (auto& pc : pieceCountsByThread)
	{
		pc.resize(int(PIECE_NB));
		for (auto& p : pc)
			p.resize(16, 0);
	}

	vector<vector<int64_t>> kingIn(3);
	for (auto& k : kingIn)
		k.resize(nthreads, 0);

	auto start = std::chrono::steady_clock::now();

#pragma omp parallel for num_threads(nthreads)
	for (int64_t x = 0; x < RUNS; x++)
	{
		int tnum = omp_get_thread_num();
		all[tnum]++;
		LegalChecker lc;
		lc.init(&lp, tnum);
		bool cont = lc.prepare<sampleType>();
		if (cont)
		{
			lc.createTotalCounts();
			bool isok = lc.checkConditions();

			if (isok)
			{
				auto [wk, bk] = lc.getKings();
				kingSquares[(int)wk][tnum]++;
				kingSquares[(int)bk][tnum]++;
				kingIn[lc.getKingInPawnSquares()][tnum]++;

				for (Piece p = W_PAWN; p <= B_KING; ++p)
					pieceCountsByThread[tnum][(int)p][lc.getCount()[p]]++;

				int countAs = lc.countCastling();		//If there are castling possibilities, this position counts as multiple (2^castling_possibilties)
				int epPoss = lc.countEnPassantPossibilities();		//En passant possibilities

				countAs *= (1 + epPoss);
				legal[tnum] += countAs;
				byCount[tnum][lc.totalPieces()] += countAs;

				int c = 0;
				for (const auto& open : openingsToCheck)
				{
					auto allowOpening = lc.checkOpening(open);
					if (allowOpening)
						legalOpeningsCount[c][tnum]++;
					c++;
				}

				bool isokRestricted = lc.checkAdditionalConditions(false, 3, 6);
				if (isokRestricted)
				{
					legalRestricted[tnum] += countAs;
					byCountRestricted[tnum][lc.totalPieces()] += countAs;
				}
			}

			if (all[tnum] % (1024 * 2048) == (1024 * 2048 * tnum / nthreads))
			{
#pragma omp critical
				{
					auto totalGood = vectorSum(legal);
					auto totalGoodRestricted = vectorSum(legalRestricted);
					auto totalAny = vectorSum(all);
					std::chrono::duration<double> elapsedSeconds = std::chrono::steady_clock::now() - start;
					cout << "Per second: " << totalAny / elapsedSeconds.count();
					if (sampleType == ESampleType::RESTRICTED || sampleType == ESampleType::WB_RESTRICTED)
						cout << "  legal_restricted: " << totalGoodRestricted << " / all: " << totalAny << "  estimate restricted: "
						<< (double)totalGoodRestricted / totalAny * totalPossibilities << endl;

					if (sampleType == ESampleType::PIECES_WB || sampleType == ESampleType::PIECES)
						cout << "  legal: " << totalGood << " / all: " << totalAny << "  fraction legal: "
						<< (double)totalGood / totalAny << "  estimate all: " << (double)totalGood / totalAny * totalPossibilities << endl;
					if (tnum % 8 == 0)
					{
						cout << "king squares ";
						for (int q = 0; q < 64; q++)
							cout << vectorSum(kingSquares[q]) << " ";
						cout << endl;
						for (int q = 2; q <= 32; q++)
						{
							int64_t s = 0;
							for (int t = 0; t < nthreads; t++)
								s += byCount[t][q];
							int64_t sRestricted = 0;
							for (int t = 0; t < nthreads; t++)
								sRestricted += byCountRestricted[t][q];
							cout << q << " pieces:   ";

							if (sampleType == ESampleType::PIECES_WB || sampleType == ESampleType::PIECES)
								cout << "legal = " << s << endl;
							else
								cout << "legal restricted = " << sRestricted << endl;
						}

						for (Piece p = W_PAWN; p <= B_KING; ++p)
						{
							std::string s = color_of(p) ? "White " : "Black ";
							auto pt = type_of(p);
							if (pt == PAWN || pt == BISHOP || pt == KNIGHT || pt == ROOK || pt == QUEEN)
							{
								if (pt == PAWN)
									s += "pawn";
								else if (pt == BISHOP)
									s += "bishop";
								else if (pt == KNIGHT)
									s += "knight";
								else if (pt == ROOK)
									s += "rook";
								else if (pt == QUEEN)
									s += "queen";
								s += "   ";
								for (int q = 0; q < 16; q++)
								{
									int64_t sum = 0;
									for (int t = 0; t < nthreads; t++)
										sum += pieceCountsByThread[t][int(p)][q];
									s += std::to_string(sum) + " ";
								}
								cout << s << endl;
							}
						}

						int c = 0;
						for (const auto& open : openingsToCheck)
						{
							auto numLegal = vectorSum(legalOpeningsCount[c]);
							cout << open.name << ":  legal = " << numLegal << "  estimate = " << (double)numLegal / totalAny * totalPossibilities << endl;
							c++;
						}

					}
				}
			}
		}
	}
}

