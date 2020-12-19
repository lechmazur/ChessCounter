#pragma once

#include "position.h"

struct LegalParams;


struct Attacker
{
	Bitboard abit = 0;						//Bit where the attacker is					
	Square pos = SQUARE_NB;
	Piece piece = PIECE_NB;
	File file = FILE_NB;
	Rank rank = RANK_NB;
	PieceType pt = PIECE_TYPE_NB;			//attack as this piece type (queen -> bishop or rook)
	Bitboard comeFrom = 0;					//Where could an attacker come from? Only from where it can normally move currently but it must be empty locations
													//and white could not be in check then (or it'd be an illegal position) but this only matters for queens
													//Also it could have come from RANK_2 in case of promotions
	Bitboard middleSquares = 0;			//Squares between an attacker and white king (for sliders)
	bool directAttack = false;				//Direct attacks are attacks by pawns or knights or other pieces from the neighboring square
													//7k/8/3rp3/3K4/3qb3/4n3/8/8 w - - 0 2
};


class LegalChecker
{
private:
	std::array<Square, 32> squares;	//Piece locations (including empty)
	std::array<Piece, 32> pieces;		//What pieces are there
	Square wk = SQUARE_NB;				//White king location
	Square bk = SQUARE_NB;				//Black king location
	LegalParams* lp = nullptr;
	std::array<int, PIECE_NB> count;	//Count of each piece type
	Position posWTM;						//White-to-move position for SF
	Position posBTM;						//Black-to-move position for SF
	int threadNum = -1;					//Current thread number
	int nWhite = -1;						//Number of white pieces
	int nBlack = -1;						//Number of black pieces
	File wkFile = FILE_NB;				//White king file
	Rank wkRank = RANK_NB;				//White king rank
	uint64_t wkBit = 0;					//Bit where white king is
	int nattacks = -1;						//How many black pieces check white king
	std::array<File, 14> preEnpassantsFrom;	//Black's previous possible en-passants (from)
	std::array<File, 14> preEnpassantsTo;		//Black's previous possible en-passants (to)
	int prevEPCount = -1;							//Count of black's previous possible en-passants
	std::vector<Attacker> attackers;				//List of who checks white king (at most 2)

public:
	void prepare(LegalParams* lpIn, int tnum);
	[[nodiscard]] bool checkBasics() const;
	[[nodiscard]] bool checkAdditionalConditions() const;
	[[nodiscard]] bool checkCounts() const;
	[[nodiscard]] bool checkConditions();
	void createCounts();
	void setSFPositions();
	[[nodiscard]] Piece pieceOn(Square sq) const;
	[[nodiscard]] Piece pieceFR(File f, Rank r) const;
	[[nodiscard]] bool checkBishops() const;
	[[nodiscard]] bool checkPawnStructures() const;
	[[nodiscard]] bool checkSameFileAndCounts() const;
	[[nodiscard]] int countEnPassantPossibilities() const;
	[[nodiscard]] int countAttacks() const;
	void listBlackEnPassants();
	[[nodiscard]] bool betweenKingAndAttacker(Square attacker, PieceType pt) const;
	void makeListOfAttackers();
	[[nodiscard]] bool checkDoubleAttacked() const;
	[[nodiscard]] bool blackPossCastled(const Attacker& att) const;
	[[nodiscard]] bool checkSingleAttacked() const;
	[[nodiscard]] bool isDoubleDiscEPPossible() const;
	[[nodiscard]] int countCastling() const;
	[[nodiscard]] int totalPieces() const;

};
