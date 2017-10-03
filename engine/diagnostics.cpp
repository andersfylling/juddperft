/*

MIT License

Copyright(c) 2016-2017 Judd Niemann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "juddperft/diagnostics.h"
#include "juddperft/search.h"
#include "juddperft/movegen.h"
#include "juddperft/fen.h"
#include "juddperft/raiitimer.h"

#include <stdio.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <sstream>

////////////////////////////////////////////////////////
//
// Diagnostic Functions:
//
////////////////////////////////////////////////////////

#ifdef INCLUDE_DIAGNOSTICS

namespace juddperft {

void dumpPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, uint64_t correctAnswer)
{
	RaiiTimer timer;
	ChessPosition P;
	readFen(&P, pzFENstring);
	dumpChessPosition(P);

	int64_t n=0;
	perftFastMT(P, depth, n);
	printf("Perft %d: %lld (Correct Answer= %lld)\n", depth, n, correctAnswer);

	if (n != correctAnswer)
		printf("-== FAIL !!! ==-\n");

	printf("\n\n");
}

int perftValidateWithExternal(const std::string& validatorPath, const std::string& fenString, int depth, int64_t value)
{
	std::string command = validatorPath + " \"" + fenString + "\" " + std::to_string(depth) + " " + std::to_string(value);

	/* // mock
	std::cout << command << std::endl;
	return PERFTVALIDATE_TRUE;
	*/

	return (system(command.c_str()) == EXIT_SUCCESS) ? PERFTVALIDATE_TRUE : PERFTVALIDATE_FALSE;
}

void findPerftBug(const std::string& validatorPath, const ChessPosition* pP, int depth)
{
	// allocate fen buffer;
	char fenBuffer[1024];
	std::string fenString;

	// generate Move List
	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(*pP, MoveList);

	PerftInfo T;

	if (depth <= 1) { // terminal node

		// generate FEN string
		memset(fenBuffer, 0, 1024);
		writeFen(fenBuffer, pP);
		fenString.assign(fenBuffer);

		std::cout << "Reached Single Position!\n";

		dumpChessPosition(*pP);
		dumpMoveList(MoveList);
		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = 0LL;
		perftMT(*pP, 1, 1, &T);
		int nResult = perftValidateWithExternal(validatorPath, fenString, 1, T.nMoves);
		if (nResult == PERFTVALIDATE_FALSE) {
			std::cout << "Engines disagree on Number of moves from this position" << std::endl;
		} else {
			std::cout << "ok" << std::endl;
		}

		std::cout << "Hit enter to continue" << std::endl;
		getchar();

		return;
	}

	ChessMove* pM = MoveList;
	ChessPosition Q;

	while (pM->NoMoreMoves == 0)
	{
		// Set up position
		Q = *pP;
		Q.performMove(*pM);
		Q.switchSides();

		// generate FEN string
		memset(fenBuffer, 0, 1024);
		writeFen(fenBuffer, &Q);
		fenString.assign(fenBuffer);

		std::cout << "After Move: ";
		if (pP->BlackToMove)
			std::cout << "... ";
		dumpMove(*pM);
		std::cout << "Position: " << fenString << std::endl;

		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = 0LL;
		perftMT(Q, depth - 1, 1, &T);

		std::cout << "\nValidating depth: " << depth - 1 << " perft: " << T.nMoves << std::endl;
		int nResult = perftValidateWithExternal(validatorPath, fenString, depth - 1, T.nMoves);
		if (nResult == PERFTVALIDATE_FALSE)
		{
			// Investigate further ...
			std::cout << "WRONG !! Taking a closer look ..." << std::endl;
			findPerftBug(validatorPath, &Q, depth - 1);
		}
		else
		{
			std::cout << "ok";
		}
		std::cout << "\n" << std::endl;

		pM++;
	}
	std::cout << "findPerftBug() finished\n\n";
}

// https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
std::array<std::string, 256> getEGNMovesFromEngine(const std::string cmd)
{
	std::array<char, 1028> buffer;
	std::string result;
	std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
			if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
					result += buffer.data();
	}

	// split into array
	std::array<std::string, 256> egns{};
	unsigned int i = 0;
	std::stringstream sstr{result};
  while (sstr.good() && i < 256){
      sstr >> egns[i++];
  }

	return egns;
}

// findPerftDifference
// This only checks for perft(1) for any given FEN string
// if needed to check for perft(2) then this should be used in a iteratively.
// Every FEN for every move at perft(1) needs to be generated and given.
void findPerftDifference (const std::string& validatorPath, const ChessPosition* pP)
{
	ChessMove MoveList[MOVELIST_SIZE];
	ChessMove* pM;

#ifdef _USE_HASH
// Hash not used as I haven't looked into it.
#endif

	// generate FEN string
	// allocate fen buffer;
	char fenBuffer[1024];
	std::string fenString;
	memset(fenBuffer, 0, 1024);
	writeFen(fenBuffer, pP);
	fenString.assign(fenBuffer);

	// get egns from juddperft movegen
	generateMoves(*pP, MoveList);

	// get all moves generated by engine
	std::array<std::string, 256> egns = getEGNMovesFromEngine(validatorPath + " juddperft-egn " + fenString);
	unsigned int length = 0;
	for (; length < 256 && !egns[length].empty(); length++); // find the nr of egns

	// Make sure that every EGN given by the engine is legal
	for (unsigned int i = 0; i < length && !egns[i].empty(); i++) {
		const std::string egn = egns[i];
		unsigned char from = (7 - (egn.at(0) - 'a')) + 8 * ((egn.at(1) == ' ' ? 0 : egn.at(1) - '0') - 1);
		unsigned char to   = (7 - (egn.at(2) - 'a')) + 8 * ((egn.at(3) == ' ' ? 0 : egn.at(3) - '0') - 1);

		bool matched = false;
		for (unsigned int j = 0; j < MOVELIST_SIZE; j++) {

			if (MoveList[j].FromSquare == from && MoveList[j].ToSquare == to) {
				matched = true;
				break;
			}
		}

		if (!matched) {
			std::cout << egn << ": illegal\n";
		}
	}

	// make sure the engine has every move that the juddperft has generated
  const std::array<char, 8> indexes = {'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a'};
  const std::array<char, 8> ints = {'1', '2', '3', '4', '5', '6', '7', '8'};
	for (unsigned int j = 0; j < MOVELIST_SIZE; j++) {
		// if the piece hasnt moved, then this isnt a legal move now is it.
		if (MoveList[j].FromSquare == MoveList[j].ToSquare) {
			continue;
		}
		std::string missingEGN = "----";

	  // from
	  missingEGN.at(0) = indexes[MoveList[j].FromSquare % 8];
	  missingEGN.at(1) = ints[MoveList[j].FromSquare / 8];

	  // to
	  missingEGN.at(2) = indexes[MoveList[j].ToSquare % 8];
	  missingEGN.at(3) = ints[MoveList[j].ToSquare / 8];

		bool matched = false;
		for (unsigned int i = 0; i < length && !egns[i].empty(); i++) {
			const std::string egn = egns[i];
			unsigned char from = (7 - (egn.at(0) - 'a')) + 8 * ((egn.at(1) == ' ' ? 0 : egn.at(1) - '0') - 1);
			unsigned char to   = (7 - (egn.at(2) - 'a')) + 8 * ((egn.at(3) == ' ' ? 0 : egn.at(3) - '0') - 1);

			if (MoveList[j].FromSquare == from && MoveList[j].ToSquare == to) {
				matched = true;
				break;
			}
		}

		if (!matched) {
			std::cout << missingEGN << ": missing\n";
		}
	}

	std::cout << "DONE\n";
}

void runTestSuite()
{
	// see https://chessprogramming.wikispaces.com/perft+Results
	printf("Running Test Suite\n\n");
	dumpPerftScoreFfromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860);				// Position 1: Initial Position
	dumpPerftScoreFfromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25",5,193690690);	// Position 2: 'Kiwipete' position
	dumpPerftScoreFfromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0",7, 178633661);								// Position 3
	dumpPerftScoreFfromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",6, 706045033);		// Position 4
	dumpPerftScoreFfromFEN("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033);		// Position 4 Mirrored (should be same score as previous)
	dumpPerftScoreFfromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194);				// Position 5
	dumpPerftScoreFfromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 7, 287188994746); // Position 6 28/1/2016: Correct (took 8454195 ms)
}

} // namespace juddperft
#endif // INCLUDE_DIAGNOSTICS
