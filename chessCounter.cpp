#include <iostream>
#include <omp.h>
#include "bitboard.h"
#include "endgame.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"
#include "runner.h"


int main(int argc, char* argv[])
{
	omp_set_nested(2);
	Runner runner;
	runner.init();
	//runner.generateFens(argc, argv);
	runner.posEstimate<ESampleType::RESTRICTED>(argc, argv);

	Threads.set(0);
}

