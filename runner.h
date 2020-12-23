#pragma once

#include "bitboard.h"
#include "endgame.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"
#include "OpeningLimit.h"


class Runner
{
private:
	std::vector<OpeningLimit> openingsToCheck;

public:
	void init();
	void posEstimate(int argc, char* argv[]);
};

