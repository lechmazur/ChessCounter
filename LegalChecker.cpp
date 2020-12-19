#include "LegalChecker.h"
#include "LegalParams.h"
#include "position.h"
#include "thread.h"
#include <omp.h>
#include <vector>


template <typename Tarr>
void resetArr(Tarr& v)
{
	typename Tarr::value_type val(0);
	std::fill(v.begin(), v.end(), val);
}


template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}


//Create random board
void LegalChecker::prepare(LegalParams* lpIn, int tnum)
{
	lp = lpIn;
	threadNum = tnum;
	//Kings are put into random locations where they are on neighboring squares
	int l1 = lp->intRand(0, int(lp->whiteKingLocs.size() - 1));
	wk = lp->whiteKingLocs[l1];
	wkFile = file_of(wk);
	wkRank = rank_of(wk);
	wkBit = (uint64_t)1 << wk;
	bk = lp->blackKingLocs[l1];
	Bitboard bbAll = 0;
	bbAll |= (uint64_t)1 << wk;
	bbAll |= (uint64_t)1 << bk;

	squares[0] = wk;
	squares[1] = bk;
	pieces[0] = W_KING;
	pieces[1] = B_KING;

	//Choose a random piece for each square for the 30 chosen random locations (can be empty)
	for (int n = 0; n < 30; n++)
	{
		int rLoc = -1;
		do
		{
			rLoc = lp->intRand(0, 63);
		} while ((bbAll & ((uint64_t)1 << rLoc)) != 0);
		squares[n + 2] = (Square)rLoc;
		bbAll |= (uint64_t)1 << rLoc;
		pieces[n + 2] = lp->chooseFrom[lp->intRand(0, 10)];
	}
}


//Counts of pieces by type
void LegalChecker::createCounts()
{
	resetArr(count);

	for (auto& p : pieces)
		count[p]++;

	nWhite = 1 + count[W_PAWN] + count[W_QUEEN] + count[W_ROOK] + count[W_BISHOP] + count[W_KNIGHT];
	nBlack = 1 + count[B_PAWN] + count[B_QUEEN] + count[B_ROOK] + count[B_BISHOP] + count[B_KNIGHT];
}


bool LegalChecker::checkBasics() const
{
	for (int n = 2; n < 32; n++)
	{
		//Can't have pawns on RANK_1 or RANK_8
		if ((pieces[n] == W_PAWN || pieces[n] == B_PAWN) && (rank_of(squares[n]) == RANK_1 || rank_of(squares[n]) == RANK_8))
			return false;
	}

	if (nWhite > 16 || nBlack > 16)	//Can't have more than 16 pieces per side
		return false;

	return true;
}


bool LegalChecker::checkAdditionalConditions() const
{
	if (lp->maxQueensOneSide != -1)
	{
		if (count[W_QUEEN] > lp->maxQueensOneSide || count[B_QUEEN] > lp->maxQueensOneSide)
			return false;
	}

	if (lp->maxQueensTotal != -1)
	{
		if (count[W_QUEEN] + count[B_QUEEN] > lp->maxQueensTotal)
			return false;
	}

	if (!lp->underpromotions)
	{
		if (count[W_BISHOP] > 2 || count[W_KNIGHT] > 2 || count[W_ROOK] > 2
			|| count[B_BISHOP] > 2 || count[B_KNIGHT] > 2 || count[B_ROOK] > 2)
			return false;

		auto checkBishops = [&](Piece p)
		{
			if (count[p] == 2)
			{
				int found = 0;
				std::array<int, 2> checkerboard = { -1, -1 };	//black or white checkboard square
				for (int n = 0; n < 32; n++)
				{
					if (pieces[n] == p)
						checkerboard[found++] = (int(rank_of(squares[n])) + int(file_of(squares[n]))) % 2;
				}
				assert(found == 2);
				if (checkerboard[0] == checkerboard[1])	//can't get two same color square bishops without underpromotions (to minor pieces)
					return false;
			}
			return true;
		};

		auto okWhiteB = checkBishops(W_BISHOP);
		auto okBlackB = checkBishops(B_BISHOP);

		if (!okWhiteB || !okBlackB)
			return false;
	}
	return true;
}


bool LegalChecker::checkCounts() const
{
	if (count[W_PAWN] > 8 || count[B_PAWN] > 8
		|| count[W_QUEEN] > 9 || count[B_QUEEN] > 9
		|| count[W_BISHOP] > 10 || count[B_BISHOP] > 10
		|| count[W_ROOK] > 10 || count[B_ROOK] > 10
		|| count[W_KNIGHT] > 10 || count[B_KNIGHT] > 10)
		return false;

	std::array<int, 2> checkerboardWhite = { 0, 0 };	//black or white checkboard square
	std::array<int, 2> checkerboardBlack = { 0, 0 };	//black or white checkboard square
	for (int n = 0; n < 32; n++)
	{
		if (pieces[n] == W_BISHOP)
			checkerboardWhite[(int(rank_of(squares[n])) + int(file_of(squares[n]))) % 2]++;
		if (pieces[n] == B_BISHOP)
			checkerboardBlack[(int(rank_of(squares[n])) + int(file_of(squares[n]))) % 2]++;
	}

	int extrasWP = 0;
	extrasWP += std::max(count[W_BISHOP] - 2, 0);
	if (count[W_BISHOP] == 2 && (checkerboardWhite[0] == 2 || checkerboardWhite[1] == 2))
		extrasWP += 1;		//If there are two bishops but on the same color squares, that means one of them was promoted
	extrasWP += std::max(count[W_KNIGHT] - 2, 0);
	extrasWP += std::max(count[W_ROOK] - 2, 0);
	extrasWP += std::max(count[W_QUEEN] - 1, 0);

	if (count[W_PAWN] + extrasWP > 8)		//Can't have promotions without losing pawns
		return false;

	int extrasBP = 0;
	extrasBP += std::max(count[B_BISHOP] - 2, 0);
	if (count[B_BISHOP] == 2 && (checkerboardBlack[0] == 2 || checkerboardBlack[1] == 2))
		extrasBP += 1;
	extrasBP += std::max(count[B_KNIGHT] - 2, 0);
	extrasBP += std::max(count[B_ROOK] - 2, 0);
	extrasBP += std::max(count[B_QUEEN] - 1, 0);

	if (count[B_PAWN] + extrasBP > 8)
		return false;

	//1 capture allows 1 white and 1 black promotion
	int maxPromo = std::max(extrasWP, extrasBP);
	if (maxPromo + nWhite + nBlack > 32)
		return false;

	if (extrasWP >= 9 || extrasBP >= 9)
		return false;


	return true;
}


void LegalChecker::setSFPositions()
{
	std::memset(&posWTM, 0, sizeof(Position));
	std::memset(&lp->states[threadNum]->back(), 0, sizeof(StateInfo));

	posWTM.setMine(pieces, squares, SQ_NONE, &lp->states[threadNum]->back(), Threads.main(), WHITE);

	std::memset(&posBTM, 0, sizeof(Position));
	std::memset(&lp->states2[threadNum]->back(), 0, sizeof(StateInfo));

	posBTM.setMine(pieces, squares, SQ_NONE, &lp->states2[threadNum]->back(), Threads.main(), BLACK);
}


Piece LegalChecker::pieceOn(Square sq) const
{
	return posWTM.piece_on(sq);
}


Piece LegalChecker::pieceFR(File f, Rank r) const
{
	return pieceOn(make_square(f, r));
}


bool LegalChecker::checkBishops() const
{
	auto isBishopCornered = [&](Piece pawn, Square sqPawn, Square sqBishop)
	{
		//Can't get a bishop into the corner if a blocking pawn is in its starting location
		return (pieceOn(sqPawn) == pawn && (pieceOn(sqBishop) == W_BISHOP || pieceOn(sqBishop) == B_BISHOP));
	};

	if (isBishopCornered(W_PAWN, SQ_B2, SQ_A1))
		return false;

	if (isBishopCornered(W_PAWN, SQ_G2, SQ_H1))
		return false;

	if (isBishopCornered(B_PAWN, SQ_G7, SQ_H8))
		return false;

	if (isBishopCornered(B_PAWN, SQ_B7, SQ_A8))
		return false;

	//Bishop can't get into a square that's blocked by two pawns in their starting locations, except for bishops' starting locations
	//4b2k/3p1p2/8/8/8/8/P1P5/1B4K1 w - - 0 2
	auto isBishopBehindTwoPawns = [&](Piece pawn, File c, Rank rankPawn, Rank rankBishop)
	{
		return pieceFR(File(c - 1), rankPawn) == pawn
			&& pieceFR(File(c + 1), rankPawn) == pawn
			&& (pieceFR(File(c), rankBishop) == W_BISHOP || pieceFR(File(c), rankBishop) == B_BISHOP);
	};

	for (File c : {FILE_B, FILE_D, FILE_E, FILE_G})
	{
		if (isBishopBehindTwoPawns(W_PAWN, c, RANK_2, RANK_1))
			return false;
		if (isBishopBehindTwoPawns(B_PAWN, c, RANK_7, RANK_8))
			return false;
	}

	//But also white bishop can't be at the starting position of the black bishop and vice-versa, if blocked by pawns
	//2B4k/1p1p4/8/7K/8/8/4P1P1/5b2 w - - 0 2
	auto isBishopBehindTwoPawnsWrongColor = [&](Piece pawn, Piece bishop, File c, Rank rankPawn, Rank rankBishop)
	{
		return pieceFR(File(c - 1), rankPawn) == pawn
			&& pieceFR(File(c + 1), rankPawn) == pawn
			&& pieceFR(File(c), rankBishop) == bishop;
	};

	for (File c : {FILE_C, FILE_F})
	{
		if (isBishopBehindTwoPawnsWrongColor(W_PAWN, B_BISHOP, c, RANK_2, RANK_1))
		{
			return false;
		}
		if (isBishopBehindTwoPawnsWrongColor(B_PAWN, W_BISHOP, c, RANK_7, RANK_8))
		{
			return false;
		}
	}

	//If the pawns were not moved, bishops need to be at their original locations
	// .p.p..
	// ..B...
	//7k/8/8/8/8/8/1P1P4/2B1K3 w - - 0 2
	if (pieceOn(SQ_B2) == W_PAWN && pieceOn(SQ_D2) == W_PAWN && pieceOn(SQ_C1) != W_BISHOP)
		return false;
	if (pieceOn(SQ_E2) == W_PAWN && pieceOn(SQ_G2) == W_PAWN && pieceOn(SQ_F1) != W_BISHOP)
		return false;
	if (pieceOn(SQ_B7) == B_PAWN && pieceOn(SQ_D7) == B_PAWN && pieceOn(SQ_C1) != B_BISHOP)
		return false;
	if (pieceOn(SQ_E7) == B_PAWN && pieceOn(SQ_G7) == B_PAWN && pieceOn(SQ_F1) != B_BISHOP)
		return false;

	return true;
}


bool LegalChecker::checkPawnStructures() const
{
	auto pawnStructureOk = [&](const std::vector<Square>& plist, Piece p)
	{
		//4 total flips
		bool all = true;
		for (Square s : plist)
			if (pieceOn(s) != p)
				all = false;
		if (all)
			return false;

		all = true;
		for (Square s : plist)
			if (pieceOn(flip_file(s)) != p)
				all = false;
		if (all)
			return false;


		all = true;
		for (Square s : plist)
			if (pieceOn(flip_rank(s)) != ~p)
				all = false;
		if (all)
			return false;

		all = true;
		for (Square s : plist)
			if (pieceOn(flip_file(flip_rank(s))) != ~p)
				all = false;
		if (all)
			return false;
		return true;
	};

	//Impossible pawn structures on the sides
	//...
	//P..
	//PP.
	//...
	//7k/8/8/8/8/7P/6PP/4K3 w - - 0 
	if (!pawnStructureOk({ SQ_A2, SQ_B2, SQ_A3 }, W_PAWN))
		return false;

	//...
	//P..
	//...
	//PPP
	//...
	//5k2/8/8/8/P7/8/PPP3K1/8 w - - 0 2
	if (!pawnStructureOk({ SQ_A2, SQ_B2, SQ_C2, SQ_A4 }, W_PAWN))
		return false;

	//...
	//P..
	//.P.
	//P.P
	//...
	//6k1/8/8/8/P7/1P6/P1P5/6K1 w - - 0 2
	if (!pawnStructureOk({ SQ_A2, SQ_B3, SQ_C2, SQ_A4 }, W_PAWN))
		return false;

	//...
	//P..
	//P..
	//P.P
	//...
	//6k1/8/8/8/P7/P7/P1P5/6K1 w - - 0 2
	if (!pawnStructureOk({ SQ_A2, SQ_A3, SQ_C2, SQ_A4 }, W_PAWN))
		return false;

	//...
	//...
	//PP.
	//P.P
	//...
	//6k1/8/8/8/8/PP6/P1P5/6K1 w - - 0 4
	if (!pawnStructureOk({ SQ_A2, SQ_A3, SQ_B3, SQ_C2 }, W_PAWN))
		return false;

	//...
	//...
	//PP.
	//.PP
	//...
	//6k1/8/8/8/8/PP6/1PP5/6K1 w - - 0 8
	if (!pawnStructureOk({ SQ_A3, SQ_B2, SQ_B3, SQ_C2 }, W_PAWN))
		return false;

	//...
	//P..
	//.P.
	//.PP
	//...
	//6k1/8/8/8/P7/1P6/1PP5/6K1 w - - 0 16
	if (!pawnStructureOk({ SQ_A4, SQ_B2, SQ_B3, SQ_C2 }, W_PAWN))
		return false;

	//...
	//P..
	//PP.
	//..P
	//...
	//6k1/8/8/8/P7/PP6/2P5/6K1 w - - 0 8
	if (!pawnStructureOk({ SQ_A4, SQ_A3, SQ_B3, SQ_C2 }, W_PAWN))
		return false;


	for (File c = FILE_B; c <= FILE_G; ++c)
	{
		//Detect these impossible pawn structures:
		//  P      
		// PPP   
		// ...
		//7k/8/8/8/8/2P5/1PPP4/4K3 w - - 0 
		if (pieceFR(c, RANK_2) == W_PAWN
			&& pieceFR(File(c - 1), RANK_2) == W_PAWN
			&& pieceFR(File(c + 1), RANK_2) == W_PAWN
			&& pieceFR(c, RANK_3) == W_PAWN)
			return false;

		if (pieceFR(c, RANK_7) == B_PAWN
			&& pieceFR(File(c - 1), RANK_7) == B_PAWN
			&& pieceFR(File(c + 1), RANK_7) == B_PAWN
			&& pieceFR(c, RANK_6) == B_PAWN)
			return false;
	}

	return true;
}


bool LegalChecker::checkSameFileAndCounts() const
{
	//Each extra additional pawn on the same file means the opposite piece has been captured
	//7k/8/8/2P3P1/2P5/6P1/6P1/4K3 w - - 0 2
	auto countSameFilePawns = [&](Piece pieceCounted)
	{
		int sameCol = 0;
		for (File c = FILE_A; c <= FILE_H; ++c)
		{
			int pawns = 0;
			for (Rank y = RANK_1; y <= RANK_8; ++y)
			{
				auto p = pieceFR(c, y);
				if (p == pieceCounted)
					pawns++;
			}
			sameCol += std::max(0, pawns - 1);
		}
		return sameCol;
	};

	int sameColW = countSameFilePawns(W_PAWN);
	int sameColB = countSameFilePawns(B_PAWN);

	if (nBlack + sameColW > 16 || nWhite + sameColB > 16)
		return false;

	return true;
}


int LegalChecker::countEnPassantPossibilities() const
{
	//Count how many ways could en-passant occur
	//xxxx
	//x.xx
	//x.xx
	//xpPx

	//xxxx
	//x.xx
	//x.xx
	//Ppxx

	//6k1/8/8/3pP1Pp/8/8/8/6K1 w - - 0 16
	int epPoss = 0;
	for (File c = FILE_A; c <= FILE_H; ++c)
	{
		if (pieceFR(c, RANK_5) == B_PAWN
			&& pieceFR(c, RANK_6) == NO_PIECE
			&& pieceFR(c, RANK_7) == NO_PIECE
			&& ((c >= FILE_B && pieceFR(File(c - 1), RANK_5) == W_PAWN)
				|| (c <= FILE_G && pieceFR(File(c + 1), RANK_5) == W_PAWN)))
			epPoss++;
	}
	return epPoss;
}

int bitCount(uint64_t n) 
{
	int counter = 0;
	while (n) 
	{
		counter += n % 2;
		n >>= 1;
	}
	return counter;
}


int LegalChecker::countAttacks() const
{
	return bitCount(posWTM.checkers());		//Get the number of pieces checking the white king from the bitboard
}


//Did black play en-passant on the previous move? Save all possibilities
void LegalChecker::listBlackEnPassants()
{
	prevEPCount = 0;
	for (File c = FILE_A; c <= FILE_H; ++c)
	{
		if (nWhite < 16 && count[W_PAWN] < 8 //En-passant is a capture, so only possible if fewer than 16 white pieces and fewer than 8 white pawns
			&& pieceFR(c, RANK_3) == B_PAWN
			&& pieceFR(c, RANK_2) == NO_PIECE
			&& pieceFR(c, RANK_4) == NO_PIECE)
		{
			if (c < FILE_H && pieceFR(File(c + 1), RANK_4) == NO_PIECE)
			{
				preEnpassantsFrom[prevEPCount] = File(c + 1);
				preEnpassantsTo[prevEPCount++] = c;
			}
			if (c > FILE_A && pieceFR(File(c - 1), RANK_4) == NO_PIECE)
			{
				preEnpassantsFrom[prevEPCount] = File(c - 1);
				preEnpassantsTo[prevEPCount++] = c;
			}
		}
	}
	assert(prevEPCount <= 14);
}


bool LegalChecker::betweenKingAndAttacker(Square attacker, PieceType pt) const
{
	return attacks_bb(pt, attacker, posBTM.pieces())
		& attacks_bb(pt, wk, posBTM.pieces()) & (~posBTM.pieces());
}

//Those that check white king
void LegalChecker::makeListOfAttackers()
{
	auto myat = posWTM.checkers();
	attackers.resize(nattacks);
	for (Attacker& a : attackers)
	{
		assert(myat);
		a.pos = pop_lsb(&myat);
		a.abit = uint64_t(1) << a.pos;
		a.piece = pieceOn(a.pos);
		a.file = file_of(a.pos);
		a.rank = rank_of(a.pos);
		a.pt = type_of(a.piece);

		if (a.pt == QUEEN)		//Find out if queens attack as rooks or bishops
			a.pt = (PseudoAttacks[ROOK][a.pos] & wkBit) ? ROOK : BISHOP;

		if (a.pt == PAWN)
		{
			a.comeFrom = (shift<NORTH_WEST>(a.abit) | shift<NORTH_EAST>(a.abit)) & ~posBTM.pieces();
			//No need to worry about moves such as b7-b5 or b7-b6 because a non-capture pawn move can't result in discovery check without promotions
		}
		else
			a.comeFrom = a.piece == B_QUEEN ? 0 : attacks_bb(a.pt, a.pos, posBTM.pieces()) & ~posBTM.pieces();	//Queen can't be a blocker

		if (a.rank == RANK_1)
		{
			//Promotion possible
			//White king's location couldn't have been attacked by that black pawn before promotion or it'd be an illegal position
			a.comeFrom |= (shift<NORTH>(a.abit) | shift<NORTH_WEST>(a.abit) | shift<NORTH_EAST>(a.abit)) & ~posBTM.pieces() & ~PawnAttacks[WHITE][wk];
		}

		a.middleSquares = a.pt == PAWN ? 0 : betweenKingAndAttacker(a.pos, a.pt);

		if (a.piece == B_KNIGHT || a.piece == B_PAWN || (PseudoAttacks[KING][wk] & a.abit) != 0)
			a.directAttack = true;
	}
}


bool LegalChecker::isDoubleDiscEPPossible() const
{
	//If white is checked by two black pieces, it has to be a result of a discovered check
	//It can't be from castling
	Square asBishopLoc = SQ_NONE;	//Diagonal check
	Square asRookLoc = SQ_NONE;	//Horizontal or vertical check

	for (auto& a : attackers)
	{
		if (a.pt == BISHOP)
			asBishopLoc = a.pos;
		if (a.pt == ROOK)
			asRookLoc = a.pos;
	}

	//En-passant capture by black means that two neighboring squares on the 4th rank are emptied
	//This can result in two discovered checks attacking white king but only when the king is above or below the source square of 
	//the ep capturing pawn
	//These attacks need to be through the squares with captured pawns, so one has to be diagonal and one vertical
	//7k/3q4/8/1q6/2Pp4/3K4/8/8 b - - 0 2
	//7k/8/8/3K4/2Pp4/8/q2q4/8 b - - 0 4
	bool doubleDiscEPPossible = false;
	if (asBishopLoc != SQ_NONE && asRookLoc != SQ_NONE)
	{
		for (int g = 0; g < prevEPCount; g++)
		{
			if (preEnpassantsFrom[g] == wkFile)
			{
				Bitboard middleSquaresRook = betweenKingAndAttacker(asRookLoc, ROOK);
				Bitboard middleSquaresBishop = betweenKingAndAttacker(asBishopLoc, BISHOP);

				//emptied squares are [preEnpassantsTo[g], RANK_4], [wkFile, RANK_4]
				//if the attack rays go through emptied squares then this discovery EP double check is legal
				//E.g. 1qNqbR2/3Rb2q/Q2B1brk/rb2q1r1/8/Q4pK1/r1BQQ3/2RnRNQN w - - 0 1
				if ((middleSquaresRook & (uint64_t(1) << make_square(wkFile, RANK_4)))
					&& (middleSquaresBishop & (uint64_t(1) << make_square(preEnpassantsTo[g], RANK_4))))
					doubleDiscEPPossible = true;
			}
		}
	}
	return doubleDiscEPPossible;
}


bool LegalChecker::checkDoubleAttacked() const
{
	bool doubleDiscEPPossible = isDoubleDiscEPPossible();

	if (!doubleDiscEPPossible)
	{
		//Possible checks from 2 pieces after promotion
		//One of the checking pieces has to be on RANK_1
		//5k2/8/8/8/8/8/1r2K3/4r3 w - - 0 2
		//5k2/8/8/8/3r4/8/8/3Kr3 w - - 0 2

		bool blocked = false;
		//One of the checking pieces must have been blocking another from checking
		for (int q = 0; q < 2; q++)
		{
			auto& attacks = attackers[1 - q];
			auto possBlockers = (attackers[q].middleSquares & attacks.comeFrom);
			bool attackedFromBehind = false;

			//If a piece that moved to enable discovery check has an attacking slider piece behind it 
			//that would be checking the white king if the square was empty,
			//it means that the white king was attacked on that ray earlier, which would've been illegal before black move
			//7k/6q1/8/4b3/3K2r1/8/8/8 w - - 0 2
			if (possBlockers != 0 && attacks.pt != KNIGHT)
			{
				//Just do it without bitboards
				int signFile = sgn(attacks.file - wkFile);
				int signRank = sgn(attacks.rank - wkRank);
				File pFile = File(attacks.file + signFile);
				Rank pRank = Rank(attacks.rank + signRank);
				while (pFile >= 0 && pFile <= 7 && pRank >= 0 && pRank <= 7)
				{
					auto onSq = pieceFR(pFile, pRank);
					if ((attacks.pt == BISHOP && (onSq == B_BISHOP || onSq == B_QUEEN))
						|| (attacks.pt == ROOK && (onSq == B_ROOK || onSq == B_QUEEN)))
						attackedFromBehind = true;

					if (onSq != NO_PIECE)
						break;
					pFile = File(pFile + signFile);
					pRank = Rank(pRank + signRank);
				}

			}

			if (possBlockers != 0 && !attackedFromBehind)
				blocked = true;
		}

		if (!blocked)
			return false;
	}

	return true;
}

bool LegalChecker::blackPossCastled(const Attacker& att) const
{
	bool blackPossCastled = false;
	if (att.piece == B_ROOK)
	{
		if (att.pos == SQ_F8 && pieceOn(SQ_G8) == B_KING && pieceOn(SQ_E8) == NO_PIECE && pieceOn(SQ_H8) == NO_PIECE)
			blackPossCastled = true;
		if (att.pos == SQ_D8 && pieceOn(SQ_C8) == B_KING && pieceOn(SQ_E8) == NO_PIECE && pieceOn(SQ_A8) == NO_PIECE && pieceOn(SQ_B8) == NO_PIECE)
			blackPossCastled = true;
	}
	return blackPossCastled;
}


bool LegalChecker::checkSingleAttacked() const
{
	const auto& att = attackers[0];
	//Queen has no place to come from:
	//1rr2b1r/1n4nR/P1N1P1Kq/qb3Bp1/2k1n2P/5BNR/6r1/nQbQ4 w - - 0 1
	//This doesn't work for discovered checks, so we only do it for direct attackers
	//Also not for castling, e.g. 2K2rk1/4pppp/8/8/8/8/8/8 w - - 0 2

	bool didBlackPossCastled = blackPossCastled(att);

	if (att.directAttack && (att.piece == B_QUEEN || att.piece == B_BISHOP || att.piece == B_ROOK || att.piece == B_KNIGHT) && !didBlackPossCastled)
	{
		Bitboard bb = attacks_bb(type_of(att.piece), att.pos, posWTM.pieces()) & ~posWTM.pieces();
		bool anyOk = false;
		while (bb)
		{
			Square sqFrom = pop_lsb(&bb);
			if ((attacks_bb(type_of(att.piece), sqFrom, posWTM.pieces() & ~att.abit) & wkBit) == 0)
			{
				anyOk = true;
				break;
			}
		}
		if (!anyOk && att.rank == RANK_1)
		{
			//Possibly promoted pawn has no place to come from
			//2NNr2N/2k3B1/1bP1qpn1/N1q1RPp1/1n1p1R2/1NQ2N1q/1p1nKpr1/5bR1 w - - 0 1
			Bitboard bb2 = (shift<NORTH>(att.abit) | shift<NORTH_WEST>(att.abit) | shift<NORTH_EAST>(att.abit)) & ~posBTM.pieces();
			while (bb2)
			{
				Square sqFrom = pop_lsb(&bb2);
				if ((PawnAttacks[BLACK][sqFrom] & wkBit) == 0)
				{
					anyOk = true;
					break;
				}
			}
		}

		if (!anyOk)
			return false;
	}

	if (att.piece == B_PAWN)
	{
		if (att.rank == RANK_7)	//A pawn that hasn't moved from the starting location can't be checking
			return false;
		Bitboard bb = (shift<NORTH>(att.abit) | shift<NORTH_WEST>(att.abit) | shift<NORTH_EAST>(att.abit)) & ~posBTM.pieces();

		bool anyOk = bb != 0;
		if (!anyOk)
			return false;
	}

	return true;
}


int LegalChecker::countCastling() const
{
	int includingCastling = 1;

	if (wk == SQ_E1)
	{
		if (pieceOn(SQ_A1) == W_ROOK)
			includingCastling++;
		if (pieceOn(SQ_H1) == W_ROOK)
			includingCastling++;
	}

	if (bk == SQ_E8)
	{
		if (pieceOn(SQ_A8) == B_ROOK)
			includingCastling++;
		if (pieceOn(SQ_H8) == B_ROOK)
			includingCastling++;
	}
	return 1 << includingCastling;
}


int LegalChecker::totalPieces() const
{
	return nWhite + nBlack;
}


bool LegalChecker::checkConditions()
{
	bool isok = checkBasics();
	if (!isok)
		return false;

	isok = checkAdditionalConditions();
	if (!isok)
		return false;

	isok = checkCounts();
	if (!isok)
		return false;

	setSFPositions();
	//print(posWTM.fen());

	if (posBTM.checkers())	//Black king in check = illegal position
		return false;

	isok = checkBishops();
	if (!isok)
		return false;

	isok = checkPawnStructures();
	if (!isok)
		return false;

	isok = checkSameFileAndCounts();
	if (!isok)
		return false;

	nattacks = countAttacks();

	if (nattacks >= 3)  //More than 3 pieces checking at once are illegal (at most 2 can result from a discovered check)
		return false;

	listBlackEnPassants();
	makeListOfAttackers();

	if (nattacks == 2)
		isok = checkDoubleAttacked();
	else if (nattacks == 1)
		isok = checkSingleAttacked();
	if (!isok)
		return false;

	return true;
}

