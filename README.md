# ChessCounter
Estimate the number of legal chess positions

**ChessCounter tries to accurately calculate the number of legal chess positions.**

Claude Shannon first made an estimate of around 1E+43 chess positions in 1950: [[1]](https://vision.unipv.it/IA1/ProgrammingaComputerforPlayingChess.pdf). Victor Allis makes a mention that an upper bound of 5E+52 has been calculated and assumes the true space-complexity to be close to 1E+50 [[2]](https://www.dphu.org/uploads/attachements/books/books_3721_0.pdf). John Tromp came up with 4.5E+46  [[3]](https://tromp.github.io/chess/chess.html) and possibly 7.7E+45, Will Entriken with 2.4E+49 [[4]](https://groups.google.com/g/rec.games.chess.computer/c/vmvI0ePH2kI) and Shirish Chinchalkar with 1.8E+46 [[5]](https://content.iospress.com/articles/icga-journal/icg19-3-05). There are some more attempts here: [[6]](https://codegolf.stackexchange.com/questions/19397/smallest-chess-board-compression). 

As we can see there is a large variety of estimates. This program attempts to do it in a more documented and systematic way to come up with a better answer.

## General case search space
First, we place two kings. This can be done in 3,612 different legal ways (so that they are not on neighboring squares). Now we pick squares where we will place the pieces. There can be from 2-2=0 to 32-2=30 squares chosen out of the remaining 64-2=62 squares. There are 10 different non-king pieces that can be placed in each of these squares (Q, R, B, N, P, q, r, b, n, p). The number of possible combinations for m non-king pieces is 10^m. Each of them can be white-to-move or black-to-move. The total sum of pseudo-legal unique positions to consider is therefore 2 * 3612 * Sum(Comb(62, m) * 10^m, m=[0, 30]) = 3580731277913911451307082034763461066056798885824504 (3.58E+51).

100s of billions of samples from 3.5807312779E+51 positions is enough to get 100s of thousands of hits and get a decent estimate of the number of legal positions. However, we we'd like to reduce the sampling space further. One way to do it is to first select w white pieces and then b black pieces. This will mean that we won't have to sample from impossible positions like with 16 white non-king pieces and 14 black non-king pieces. We get 2 * 3612 * Sum(Sum(Comb(62, w) * 5^w * Comb(62-w,b) * 5^b, w=[0,15]), b=[0,15]) = 568159542837347293680664103149567870390294979574504 = 5.68+50 combinations and our search spaces has been reduced to less than 1/6th of what it was. 

## Restricted case search space
Disallowing underpromotions and limiting queens to three per side reduces the search space greatly. There are now at most 3 queens, 2 knights, 2 bishops, 2 rooks, and 8 pawns per side. 2 * 3,612 * Sum(Sum(Sum(Sum(Sum(Sum(Sum(Sum(Sum(Sum(Comb(62,wp) * Comb(62-wp,bp) * Comb(62-wp-bp,wn) * Comb(62-wp-bp-wn,bn) * Comb(62-wp-bp-wn-bn,wb) * Comb(62-wp-bp-wn-bn-wb,bb) * Comb(62-wp-bp-wn-bn-wb-bb,wr) * Comb(62-wp-bp-wn-bn-wb-bb-wr,br) * Comb(62-wp-bp-wn-bn-wb-bb-wr-br,wq) * Comb(62-wp-bp-wn-bn-wb-bb-wr-br-wq,bq),wp=[0,8]),bp=[0,8]),wn=[0,2]),bn=[0,2]),wb=[0,2]),bb=[0,2]),wr=[0,2]),br=[0,2]),wq=[0,min(min(4,15-wp-wn-wb-wr),30-wp-wn-wb-wr-bp-bn-bb-br)],bq=[0,min(min(4,15-bp-bn-bb-br),30-wp-wn-wb-wr-bp-bn-bb-br-wq)]) = 10969175368880644381707380652409200270873861528 (1.10E+46) combinations.

## Monte Carlo search 
Some of the positions might have castling and en passant rights, increasing the number of possibilities. This program uses a Monte Carlo approach - it samples billions of random positions out of these possibilities and applies various rules to check whether each sample position is legal and whether castling or en passant are possible. When estimating the number of "likely" legal positions (restricted use case above), we need to make sure to sample proportionally to the number of positions for each combination of the number of various pieces. There are 933,156 such combinations to choose from. 

The program only looks at positions with white-to-move. It counts the number of possible castling moves and en passant moves and weighs each random sample position as more than 1 when it’s needed. For example, if a position allows white short, white long, and black short castling and there are black pawns on f5 and c5 (with empty squares at f6, f7, and c6, and c7) and white pawns at g5 and b5, it will be counted at 2^3 * 3 = 24 positions. Since we might be looking at trillion+ positions, we're using a 64-bit PRNG with at least 128-bit of state (PCG or JSF work well).

## Rules
The following rules for determining if a position is legal are used:

1. No pawns on 1st or 8th rank

2. At most 16 pieces per side

3. At most 8 pawns, 9 queens, 10 bishops, 10 rooks, 10 knights per side

4. The total number of promoted pieces plus the number of pawns per side can’t be more than 8. A piece is counted as promoted if it is a 2nd+ queen, 3rd+ rook, 3rd+ knight, 3rd+ bishop, or a 2nd bishop that is on the same-colored squares as the 1st bishop

5. The number of promoted pieces (counted separately for white and black) can’t be more than the number of captures of pieces. E.g. if there is an extra white queen and an extra black bishop, it means that at least 1 piece of any side was captured.

6. One capture of a pawn allows 3 total promotions (2 on the capturing side, 1 on the captured side). 

7. A bishop that is not in its starting position can’t be at positions unreachable because of pawns still at their starting positions. E.g. if there is a white pawn at b2, there can’t be a bishop at a1 or if there are black pawns at d7 and f7, there can’t be a bishop at e8.

8. If there are still pawns initially blocking a bishop (e.g. white pawns at b2 and d2), this means that there has to a white bishop (at c1) behind them.

9. Various pawn structures are impossible from the starting chess position, especially on the sides. For example, white pawns at a2, b2, a3, white pawns at a2, a3, c2, a4, white pawns at a3, a4, b3, c2, white pawns at b2, c2, d2, c3, etc.

10. Each extra pawn on the same file means the opposite-side piece was captured. E.g. if there are white pawns at d2, d4, and d6, it means that at least two black pieces were captured.

11. If the black king is in check when it’s white-to-move then the position is illegal.

12. If the white king is in check by three or more attackers, the position is illegal.

13. A position in which the white king is checked by two black attackers is legal only under certain circumstances:

     A. There was a double-discovered en passant move. The white king would have to be above or below the source square of the en passant capturing black pawn. It would have to be attacked through the rays crossing that source square and the square where the captured pawn was previously.

     B. One of the checking pieces was blocking another currently checking piece from checking previously. This means that, for example, the checks can’t be from two rooks at once. If a piece that moved to enable the discovery check has an attacking slider piece behind it that would be checking the white king if the square was empty, that position would have been illegal (black-to-move and white king in check).

     C. Black previously made a promotion move. The white king would have to be attacked by the promoted piece and this pawn must have been previously blocking another piece from checking.

14. A position in which the white king is checked by one black attacker is illegal if the checking piece has no place to come from because it would have been checking the white king at all possible source locations before it moved (or is just completely blocked off). This rule has to consider that castling could result in a check and that a black piece on the 1st rank could have captured or moved like a pawn (e.g. a rook at d1 could have come from e2). We only look at checks when the piece is checking "directly" (when it's a knight or a pawn or when there are no squares between the white king and the black pieces). A pawn that hasn’t moved from the starting location can’t be checking (e.g. black pawn at e7).


## Results

Upper bound estimate of possible legal chess position (counts en passant, castling, sides to move, allows underpromotions): **8.59+45**

Counting by the number of pieces on the board shows that there were most legal positions with 29 pieces. The number of possible positions with 22 or fewer pieces or with a complete set of 32 pieces is negligible compared to this. In computer chess, perfect-play endgame tablesbases were so far only created up to 7 pieces (Lomonosov, Syzygy). Creating endgame tablebases for 8 pieces might be possible on one of the top supercomputers but each additional piece means a couple of magnitudes more of storage and computing power is needed.

The highest estimate of "realistic" possible legal chess position (counts en passant, castling, sides to move) with at most 3 queens per side (6 total) and without underpromotions as calculated by the program: **1.01E+42**

6 queens on the board has occurred a few times in real games, so going below this would be too limiting.

If we were to create computer endgame tablebases, we could reduce the number of positions to be considered. For example, for positions in which there are no pawns, there are horizontal, vertical, and diagonal symmetries. 

## Notes

This program uses Stockfish for fast bitboard-based computation (https://github.com/official-stockfish/Stockfish, GPL-3.0, license included) and it is also released under GPL-3.0. It's written in C++20 with OpenMP for multithreading and uses Python to generate tables with the number of combinations.

The calculations don’t consider the 50-move rule or repetitions. An argument can be made that a position differs from another with an identical-looking board depending on these previous states (when a pawn was moved or a capture made or which positions were already present in the game).
