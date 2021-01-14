#pragma once

#include "position.h"
#include "OpeningLimit.h"

enum class ESampleType { 
	PIECES,					//Most general case, kings in 3612 possible locations, up to 30 pieces picked 
	PIECES_WB,				//White and black pieces picked separately
	WB_RESTRICTED,			//White and black pieces picked separately, slowly calculates restricted case. For validation
	RESTRICTED,				//Restricted case: no underpromotions, at most 3 queens per side
	VERY_RESTRICTED		//Restricted case: no underpromotions, at most 1 queen per side
};

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
													//Only for double-attacks
	Bitboard middleSquares = 0;			//Squares between an attacker and white king (for sliders)
	bool directAttack = false;				//Direct attacks are attacks by pawns or knights or other pieces from the neighboring square
													//7k/8/3rp3/3K4/3qb3/4n3/8/8 w - - 0 2
};


class LegalChecker
{
private:
	std::array<Square, 32> squares;				//Piece locations (including empty)
	std::array<Piece, 32> pieces;					//What pieces are there
	Square wk = SQUARE_NB;							//White king location
	Square bk = SQUARE_NB;							//Black king location
	LegalParams* lp = nullptr;
	std::array<int, PIECE_NB> count;				//Count of each piece type
	std::array<int, PIECE_NB> maxcount;			//Max number of each piece types for restricted case
	Position posWTM;									//White-to-move position for SF
	Position posBTM;									//Black-to-move position for SF
	int threadNum = -1;								//Current thread number
	int nTotal = -1;									//Total number of pieces
	int nWhite = -1;									//Number of white pieces
	int nBlack = -1;									//Number of black pieces
	File wkFile = FILE_NB;							//White king file
	Rank wkRank = RANK_NB;							//White king rank
	uint64_t wkBit = 0;								//Bit where white king is
	int kingInPawnSquares = -1;					//How many kings there are in pawn squares
	int nattacks = -1;								//How many black pieces check white king
	std::array<File, 14> preEnpassantsFrom;	//Black's previous possible en-passants (from)
	std::array<File, 14> preEnpassantsTo;		//Black's previous possible en-passants (to)
	int prevEPCount = -1;							//Count of black's previous possible en-passants
	std::vector<Attacker> attackers;				//List of who checks white king (at most 2)

public:
	[[nodiscard]] int getKingInPawnSquares() const;
	[[nodiscard]] std::pair<Square, Square> getKings() const;
	[[nodiscard]] const std::array<int, PIECE_NB>& getCount() const;
	void init(LegalParams* lpIn, int tnum);
	bool prepareMate();
	bool prepareMateVarious();
	template<ESampleType sampleType>
	bool prepare();
	void setKingInfo();
	void createCounts();
	[[nodiscard]] bool checkBySide() const;
	[[nodiscard]] bool checkPawnRanks() const;
	[[nodiscard]] bool checkAdditionalConditions(bool underpromotions, int maxQueensOneSide, int maxQueensTotal) const;
	[[nodiscard]] bool checkCounts() const;
	[[nodiscard]] bool checkConditions();
	void createTotalCounts();
	void setSFPositions();
	[[nodiscard]] Piece pieceOn(Square sq) const;
	[[nodiscard]] bool checkPieces(const std::vector<std::pair<Square, Piece>>& pieces) const;
	[[nodiscard]] Piece pieceFR(File f, Rank r) const;
	[[nodiscard]] bool checkBishops() const;
	[[nodiscard]] bool pawnStructureOk(const std::vector<Square>& plist, Piece p) const;
	[[nodiscard]] bool checkPawnStructures() const;
	[[nodiscard]] bool checkSameFileAndCounts() const;
	[[nodiscard]] int countEnPassantPossibilities() const;
	[[nodiscard]] int countAttacks() const;
	void listBlackEnPassants();
	[[nodiscard]] Bitboard betweenKingAndAttacker(Square attacker, PieceType pt) const;
	void makeListOfAttackers();
	[[nodiscard]] bool checkDoubleAttacked() const;
	[[nodiscard]] bool blackPossCastled(const Attacker& att) const;
	[[nodiscard]] bool checkSingleAttacked() const;
	[[nodiscard]] bool isDoubleDiscEPPossible() const;
	[[nodiscard]] int countCastling() const;
	[[nodiscard]] int totalPieces() const;
	void fromFen(const std::string& fen);
	[[nodiscard]] std::string fen() const;
	[[nodiscard]] bool isSanityCheck() const;
	[[nodiscard]] bool isSanityCheck2() const;
	[[nodiscard]] bool checkOpening(const OpeningLimit& ol) const;
	[[nodiscard]] std::pair<bool,bool> isMatedOrStalemated() const;
	[[nodiscard]] bool isMated(int inMoves);
	[[nodiscard]] bool isMate(int inMoves, bool noShorter);
};

