# ChessCounter
Estimate the number of legal chess positions

**ChessCounter tries to accurately calculate the number of legal chess positions.**

First some history. Claude Shannon first made an estimate of around 1E+43 chess positions in 1950: [[1]](https://vision.unipv.it/IA1/ProgrammingaComputerforPlayingChess.pdf). Victor Allis makes a mention that an upper bound of 5E+52 has been calculated and assumes the true space-complexity to be close to 1E+50 [[2]](https://www.dphu.org/uploads/attachements/books/books_3721_0.pdf). Will Entriken got 2.4E+49, John Tromp came up with 4.5E+46  [[3]](https://tromp.github.io/chess/chess.html) and possibly 7.7E+45, Will Entriken with 2.4E+49 [[4]](https://groups.google.com/g/rec.games.chess.computer/c/vmvI0ePH2kI) and Shirish Chinchalkar with 1.8E+46. There are some more attempts here: [[5]](https://codegolf.stackexchange.com/questions/19397/smallest-chess-board-compression). 

As we can see there is a huge variety and unfortunately these estimates are mostly inaccurate because they often don’t consider underpromotions or assume that all 32 pieces are present. This program attempts to do it in a more documented and systematic way to come up with a better answer.

First, we place two kings. This can be done in 3,612 different legal ways (so that they are not on neighboring squares). Second, we pick 32-2=30 squares out of the remaining 64-2=62. There are C(62, 30) = 450,883,717,216,034,179 (4.5088E+17) combinations in which way we can do this. There are 11 different possible pieces (including empty square) that can be placed in each of these squares (Q, R, B, N, P, q, r, b, n, p, empty). So there are 11^30=17,449,402,268,886,407,318,558,803,753,801 (1.74495E+31) possible ways to fill these 30 squares. This gives a raw number of 3,612 * 4.5088E+17 * 1.74495E+31 = 2.84178E+52 pseudo-legal positions to consider.

Each of these positions can be white-to-move or black-to-move. Some of them might also have various castling and en passant rights. This program using a Monte Carlo approach - it creates billions of random positions out of these 2.84178E+52 possibilities and applies various rules to check whether they are legal and whether castling or en passant are possible and calculates an estimate of all possible legal positions. In addition, it can estimate the number of "likely" legal positions (e.g. when there are no more than total 6 queens on board and there were no underpromotions).

The program counts just positions with white-to-move and then doubles the result. It counts the number of possible castling moves and en passant moves and weighs each random position as more than 1 when it’s needed. For example, if a position allows white short, white long, and black short castling and there are black pawns on f5 and c5 (with empty squares at f6, f7, and c6, and c7) and white pawns at g5 and b5, it will count at 2^3*3=24 positions.
The following rules for determining if a position is legal are used:

1. No pawns on 1st or 8th rank
2. At most 16 pieces per side

3. At most 8 pawns, 9 queens, 10 bishops, 10 rooks, 10 knights per side

4. The total number of promoted pieces plus the number of pawns per side can’t be more than 8. A piece is counted as promoted if it is a 2nd+ queen, 3rd+ rook, 3rd+ knight, 3rd+ bishop, or a 2nd bishop that is on the same-colored squares as the 1st bishop

5. The number of promoted pieces (counted separately for white and black) can’t be more than the number of captures of pieces. E.g. if there is an extra white queen and an extra black bishop, it means that at least 1 piece of any side was captured.

6. One capture of a pawn allows 3 total promotions (2 on the capturing side, 1 on the captured side). 

7. A bishop that is not on its starting position can’t be at positions unreachable because of pawns still at their starting positions. E.g. if there is a white pawn at b2, there can’t be a bishop at a1 or if there are black pawns at d7 and f7, there can’t be a bishop at e8.

8. If there are still pawns initially blocking a bishop (e.g. white pawns at b2 and d2), this means that there has to a white bishop (at c1) behind them.

9. Various pawn structures are impossible from the starting chess position, especially on the sides. For example, white pawns at a2, b2, a3, white pawns at a2, a3, c2, a4, white pawns at a3, a4, b3, c2, white pawns at b2, c2, d2, c3, etc.

10. Each extra pawn on the same file means the opposite-side piece was captured. E.g. if there are white pawns at d2, d4, and d6, it means that at least two black pieces were captured.

11. If black king is in check when it’s white-to-move then the position is illegal.

12. If white king is in check by three or more attackers, the position is illegal.

13. A position in which white king is checked by two black attackers is legal only under certain circumstances:

     A. There was a double-discovered en passant move. The white king would have to be above or below the source square of the en passant capturing black pawn. It would have to be attacked through the rays crossing that source square and the square where the captured pawn was previously.

     B. One of the checking pieces was blocking another currently checking piece from checking previously. This means that, for example, the checks can’t be from two rooks at once. If a piece that moved to enable the discovery check has an attacking slider piece behind it that would be checking the white king if the square was empty, that position would have been illegal (black-to-move and white king in check).

     C. Black previously made a promotion move. The white king would have to be attacked by the promoted piece and this pawn must have been previously blocking another piece from checking.

14. A position in which white king is checked by one black attacker is illegal if the checking piece has no place to come from because it would have been checking the white king at all possible source locations before it moved (or is just completely blocked off). This rule has to consider that castling could result in a check and that a black piece on the 1st rank could have captured or moved like a pawn (e.g. a rook at d1 could have come from e2). A pawn that hasn’t moved from the starting location can’t be checking (e.g. black pawn at e7).


## Results

Upper bound estimate of possible legal chess position (counts en passant, castling, sides to move, allows underpromotions): **3.6176E+50**

Less than 0.64% of checked positions were determined as legal under the most relaxed parameters. Counting by the number of pieces on the board shows that there were most legal positions with 27 pieces. The number of possible positions with fewer than 16 pieces or with a complete set of 32 pieces is tiny compared to this. In computer chess, perfect-play endgame tablesbases were so far only created up to 7 pieces (Lomonosov, Syzygy). Creating endgame tablebases for 8 pieces might be possible on one of the top supercomputers and each additional piece means a couple magnitudes more of storage and computing power is needed.

Highest estimate of "realistic" possible legal chess position (counts en passant, castling, sides to move) with at most 3 queens per side (6 total) and without underpromotions as calculated by the program: **1.22E+47**

6 queens on the board occurred a few times in real games, so going below this would be too limiting.

If we were to create computer endgame tablebases, we could reduce the number of positions to be considered. For example, for positions in which there are no pawns, there are horizontal, vertical, and diagonal symmetries. 

## Notes

This program uses Stockfish for fast bitboard-based computation (https://github.com/official-stockfish/Stockfish, GPL-3.0, license included) and it is also released under GPL-3.0. It uses PCG random number generator.

The calculations don’t consider the 50-move rule or repetitions. An argument can be made that a position differs from another with an identical-looking board depending on these previous states (when a pawn was moved or a capture made or which positions were already present in the game).

